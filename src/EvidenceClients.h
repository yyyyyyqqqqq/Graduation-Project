#pragma once
#include "RiskEvidence.h"
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QFutureWatcher>
#include <QNetworkAccessManager>
#include <QNetworkReply>

// Shared bounded GET transport only; no provider discovery, registration or strategy framework.
class EvidenceGet final : public QObject {
    Q_OBJECT
public:
    explicit EvidenceGet(QObject* parent=nullptr,QNetworkAccessManager* transport=nullptr);
    ~EvidenceGet() override;
    bool start(const QUrl&,qsizetype maxBytes,int timeoutMs=30000);
    void cancel();
    bool busy() const { return !m_reply.isNull(); }
signals:
    void finished(QueryError,const QByteArray&);
private:
    void drain();
    void finish();
    void stop(QueryError);
    QNetworkAccessManager* m_manager;
    QPointer<QNetworkReply> m_reply;
    QTimer m_timeout;
    QByteArray m_bytes;
    qsizetype m_limit=0;
    QueryError m_error=QueryError::None;
};
struct EpssChunkResult {
    QStringList requested;
    QueryError error=QueryError::None;
    QList<EpssSnapshot> snapshots;
};
class EpssClient final : public QObject {
    Q_OBJECT
public:
    explicit EpssClient(QObject* parent=nullptr,QNetworkAccessManager* transport=nullptr);
    bool fetch(const QStringList&);
    void cancel();
    bool busy() const { return m_busy; }
    static QUrl url(const QStringList&);
signals:
    void finished(const EpssChunkResult&);
private:
    void complete(EpssChunkResult);
    EvidenceGet m_get;
    QFutureWatcher<EpssChunkResult> m_parser;
    QStringList m_requested;
    bool m_busy=false,m_cancelled=false;
};
struct KevFetchResult {
    QueryError error=QueryError::None;
    std::optional<KevSnapshot> snapshot;
};
class KevClient final : public QObject {
    Q_OBJECT
public:
    explicit KevClient(QObject* parent=nullptr,QNetworkAccessManager* transport=nullptr);
    bool fetch();
    void cancel();
    bool busy() const { return m_busy; }
signals:
    void finished(const KevFetchResult&);
private:
    void complete(KevFetchResult);
    EvidenceGet m_get;
    QFutureWatcher<KevFetchResult> m_parser;
    bool m_busy=false,m_cancelled=false;
};
