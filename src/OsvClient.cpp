#include "OsvClient.h"
#include "OsvCache.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QtConcurrentRun>

OsvClient::OsvClient(QObject* parent, QNetworkAccessManager* transport)
    : QObject(parent), m_manager(transport ? transport : new QNetworkAccessManager(this)) {
    m_deadline.setSingleShot(true);
    m_deadline.setInterval(120000);
    connect(&m_deadline,&QTimer::timeout,this,[this]{ stop(QueryError::Timeout); });
    connect(&m_parser,&QFutureWatcher<Parsed>::finished,this,[this] {
        m_parsing = false;
        if (m_stopped != QueryError::None) { finish(m_stopped); return; }
        auto parsed = m_parser.result();
        m_totalRecords += parsed.received;
        if (!parsed.page.ok()) { finish(parsed.page.error); return; }
        if (m_totalRecords > OsvResponseParser::MaxRecords) { finish(QueryError::ResponseLimitExceeded); return; }
        m_candidates = std::move(parsed.page.candidates);
        const auto token = parsed.page.nextToken;
        if (token.isEmpty()) { finish(QueryError::None); return; }
        if (m_pages >= OsvResponseParser::MaxPages || m_tokens.contains(token)) {
            finish(QueryError::ResponseLimitExceeded); return;
        }
        m_tokens.insert(token);
        send(token);
    });
}
OsvClient::~OsvClient() {
    if (m_reply) { m_reply->disconnect(this); m_reply->abort(); m_reply->deleteLater(); }
    // Workers capture values only. Watcher destruction does not wait for the worker.
}
bool OsvClient::start(const QueryIdentity& i) {
    if (m_busy) return false;
    m_busy = true; m_stopped = QueryError::None; m_identity = i;
    m_totalBytes = m_totalRecords = 0; m_pages = 0; m_tokens.clear(); m_candidates.clear();
    if (m_retryNotBefore > QDateTime::currentDateTimeUtc()) { finish(QueryError::RateLimited); return true; }
    m_deadline.start(); send({}); return true;
}
void OsvClient::send(const QString& token) {
    QNetworkRequest request{QUrl(OsvEndpoint)};
    request.setHeader(QNetworkRequest::ContentTypeHeader,QStringLiteral("application/json"));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::ManualRedirectPolicy);
    request.setAttribute(QNetworkRequest::CookieLoadControlAttribute,QNetworkRequest::Manual);
    request.setAttribute(QNetworkRequest::CookieSaveControlAttribute,QNetworkRequest::Manual);
    request.setTransferTimeout(30000);
    QJsonObject body{{"package",QJsonObject{{"ecosystem",m_identity.ecosystem},{"name",m_identity.name}}},{"version",m_identity.version}};
    if (!token.isEmpty()) body.insert("page_token",token);
    ++m_pages; m_bytes.clear();
    m_reply = m_manager->post(request,QJsonDocument(body).toJson(QJsonDocument::Compact));
    m_reply->setReadBufferSize(64*1024);
    connect(m_reply,&QNetworkReply::readyRead,this,&OsvClient::drain);
    connect(m_reply,&QNetworkReply::sslErrors,this,[this]{ stop(QueryError::TlsFailure); });
    connect(m_reply,&QNetworkReply::finished,this,&OsvClient::replyFinished);
}
void OsvClient::drain() {
    if (!m_reply || m_stopped != QueryError::None) return;
    const auto remaining = OsvResponseParser::MaxResponseBytes - m_totalBytes;
    const auto bytes = m_reply->read(remaining + 1);
    m_totalBytes += bytes.size();
    if (m_totalBytes > OsvResponseParser::MaxResponseBytes) { stop(QueryError::ResponseLimitExceeded); return; }
    m_bytes.append(bytes);
}
void OsvClient::replyFinished() {
    if (!m_reply) return;
    drain();
    if (!m_reply) return; // abort may synchronously emit finished in a transport.
    auto* reply = m_reply.data(); m_reply = nullptr;
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const auto networkError = reply->error();
    auto error = m_stopped;
    if (error == QueryError::None) {
        if (status >= 300 && status < 400) error = QueryError::UnexpectedRedirect;
        else if (status == 400) error = QueryError::RequestRejected;
        else if (status == 401 || status == 403) error = QueryError::AccessDenied;
        else if (status == 404) error = QueryError::EndpointNotFound;
        else if (status == 429) {
            error = QueryError::RateLimited;
            const auto header = reply->rawHeader("Retry-After");
            bool ok = false; const auto seconds = header.toLongLong(&ok);
            if (ok && seconds >= 0 && seconds <= 31536000) m_retryNotBefore = QDateTime::currentDateTimeUtc().addSecs(seconds);
            else m_retryNotBefore = QDateTime::fromString(QString::fromLatin1(header),Qt::RFC2822Date);
        }
        else if (status >= 500) error = QueryError::ServiceUnavailable;
        else if (networkError == QNetworkReply::TimeoutError) error = QueryError::Timeout;
        else if (networkError == QNetworkReply::SslHandshakeFailedError) error = QueryError::TlsFailure;
        else if (networkError != QNetworkReply::NoError) error = QueryError::ConnectionFailure;
        else if (status != 200) error = QueryError::RequestRejected;
    }
    reply->deleteLater();
    if (error != QueryError::None) { finish(error); return; }
    m_parsing = true;
    m_parser.setFuture(QtConcurrent::run([bytes=std::move(m_bytes), existing=m_candidates]() mutable {
        auto page = OsvResponseParser::parse(bytes);
        const auto received = page.candidates.size();
        if (page.ok()) {
            page.error = OsvResponseParser::merge(existing,page.candidates);
            page.candidates = std::move(existing);
        }
        return Parsed{std::move(page),received};
    }));
}
void OsvClient::cancel() { stop(QueryError::Cancelled); }
void OsvClient::stop(QueryError error) {
    if (!m_busy || m_stopped != QueryError::None) return;
    m_stopped = error;
    if (m_reply) m_reply->abort();
    else if (!m_parsing) finish(error);
}
void OsvClient::finish(QueryError error) {
    m_deadline.stop(); m_busy = false;
    const OsvSnapshot snapshot = error == QueryError::None
        ? OsvSnapshot{m_identity,QDateTime::currentDateTimeUtc(),std::move(m_candidates)} : OsvSnapshot{};
    m_candidates.clear(); m_bytes.clear();
    emit finished(error,snapshot);
}
