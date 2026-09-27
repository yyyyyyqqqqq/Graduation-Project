#include "EvidenceClients.h"
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QtConcurrentRun>

EvidenceGet::EvidenceGet(QObject* parent,QNetworkAccessManager* transport)
    :QObject(parent),m_manager(transport?transport:new QNetworkAccessManager(this))
{
    m_timeout.setSingleShot(true);
    connect(&m_timeout,&QTimer::timeout,this,[this]{stop(QueryError::Timeout);});
}
EvidenceGet::~EvidenceGet()
{
    if(m_reply) {m_reply->disconnect(this);m_reply->abort();m_reply->deleteLater();}
}
bool EvidenceGet::start(const QUrl& url,qsizetype max,int timeout)
{
    if(busy())return false;
    // Both consumers use fixed HTTPS hosts. Reject accidental protocol expansion.
    if(url.scheme()!="https" || !(url.host()=="api.first.org" || url.host()=="raw.githubusercontent.com"))return false;
    m_bytes.clear();m_limit=max;m_error=QueryError::None;
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::ManualRedirectPolicy);
    request.setAttribute(QNetworkRequest::CookieLoadControlAttribute,QNetworkRequest::Manual);
    request.setAttribute(QNetworkRequest::CookieSaveControlAttribute,QNetworkRequest::Manual);
    request.setTransferTimeout(timeout);
    m_reply=m_manager->get(request);m_reply->setReadBufferSize(65536);
    connect(m_reply,&QNetworkReply::readyRead,this,&EvidenceGet::drain);
    connect(m_reply,&QNetworkReply::finished,this,&EvidenceGet::finish);
    connect(m_reply,&QNetworkReply::sslErrors,this,[this]{stop(QueryError::TlsFailure);});
    m_timeout.start(timeout);return true;
}
void EvidenceGet::drain()
{
    if(!m_reply || m_error!=QueryError::None)return;
    m_bytes+=m_reply->read(m_limit-m_bytes.size()+1);
    if(m_bytes.size()>m_limit)stop(QueryError::ResponseLimitExceeded);
}
void EvidenceGet::stop(QueryError error)
{
    if(!m_reply || m_error!=QueryError::None)return;
    m_error=error;m_reply->abort();
}
void EvidenceGet::cancel(){stop(QueryError::Cancelled);}
void EvidenceGet::finish()
{
    if(!m_reply)return;
    drain();if(!m_reply)return;
    auto* reply=m_reply.data();m_reply=nullptr;m_timeout.stop();
    auto error=m_error;const int status=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if(error==QueryError::None) {
        if(status>=300 && status<400)error=QueryError::UnexpectedRedirect;
        else if(reply->error()==QNetworkReply::TimeoutError)error=QueryError::Timeout;
        else if(reply->error()==QNetworkReply::SslHandshakeFailedError)error=QueryError::TlsFailure;
        else if(status==429)error=QueryError::RateLimited;
        else if(status>=500)error=QueryError::ServiceUnavailable;
        else if(reply->error()!=QNetworkReply::NoError)error=QueryError::ConnectionFailure;
        else if(status!=200)error=QueryError::RequestRejected;
        else {
            bool ok=false;const auto length=reply->header(QNetworkRequest::ContentLengthHeader).toLongLong(&ok);
            if(reply->rawHeader("Content-Encoding").isEmpty() && ok && length!=m_bytes.size())error=QueryError::ResponseInvalid;
        }
    }
    reply->deleteLater();
    const auto bytes=error==QueryError::None?std::move(m_bytes):QByteArray{};
    m_bytes.clear();emit finished(error,bytes);
}
QUrl EpssClient::url(const QStringList& cves)
{
    QUrl url(RiskEvidence::EpssEndpoint);QUrlQuery query;
    query.addQueryItem("cve",cves.join(','));query.addQueryItem("offset","0");
    query.addQueryItem("limit",QString::number(cves.size()));url.setQuery(query);return url;
}
EpssClient::EpssClient(QObject* parent,QNetworkAccessManager* transport):QObject(parent),m_get(this,transport)
{
    connect(&m_get,&EvidenceGet::finished,this,[this](QueryError error,const QByteArray& bytes){
        if(error!=QueryError::None){complete({m_requested,error,{}});return;}
        m_parser.setFuture(QtConcurrent::run([bytes,requested=m_requested]{
            const auto result=RiskEvidence::parseEpss(bytes,requested,QDateTime::currentDateTimeUtc());
            return result?EpssChunkResult{requested,QueryError::None,*result}
                         :EpssChunkResult{requested,QueryError::ResponseInvalid,{}};
        }));
    });
    connect(&m_parser,&QFutureWatcher<EpssChunkResult>::finished,this,[this]{complete(m_parser.result());});
}
bool EpssClient::fetch(const QStringList& cves)
{
    if(m_busy)return false;
    const auto chunks=RiskEvidence::epssChunks(cves);
    if(chunks.size()!=1 || chunks.first()!=cves)return false;
    m_busy=true;m_cancelled=false;m_requested=cves;
    if(m_get.start(url(cves),RiskEvidence::MaxEpssBytes))return true;
    m_busy=false;return false;
}
void EpssClient::cancel(){if(m_busy){m_cancelled=true;m_get.cancel();}}
void EpssClient::complete(EpssChunkResult result)
{
    if(m_cancelled)result={m_requested,QueryError::Cancelled,{}};
    m_busy=false;emit finished(result);
}
KevClient::KevClient(QObject* parent,QNetworkAccessManager* transport):QObject(parent),m_get(this,transport)
{
    connect(&m_get,&EvidenceGet::finished,this,[this](QueryError error,const QByteArray& bytes){
        if(error!=QueryError::None){complete({error,{}});return;}
        m_parser.setFuture(QtConcurrent::run([bytes]{
            const auto result=RiskEvidence::parseKev(bytes,QDateTime::currentDateTimeUtc());
            return result?KevFetchResult{QueryError::None,result}:KevFetchResult{QueryError::ResponseInvalid,{}};
        }));
    });
    connect(&m_parser,&QFutureWatcher<KevFetchResult>::finished,this,[this]{complete(m_parser.result());});
}
bool KevClient::fetch()
{
    if(m_busy)return false;m_busy=true;m_cancelled=false;
    if(m_get.start(QUrl(RiskEvidence::KevEndpoint),RiskEvidence::MaxKevBytes))return true;
    m_busy=false;return false;
}
void KevClient::cancel(){if(m_busy){m_cancelled=true;m_get.cancel();}}
void KevClient::complete(KevFetchResult result)
{
    if(m_cancelled)result={QueryError::Cancelled,{}};
    m_busy=false;emit finished(result);
}
