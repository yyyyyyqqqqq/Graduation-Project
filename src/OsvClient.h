#pragma once
#include "OsvResponseParser.h"
#include <QObject>
#include <QPointer>
#include <QFutureWatcher>
#include <QSet>
#include <QTimer>

class QNetworkAccessManager;
class QNetworkReply;
class OsvClient final : public QObject {
    Q_OBJECT
public:
    // An injected transport is borrowed on this object's thread and must outlive it.
    explicit OsvClient(QObject* parent = nullptr, QNetworkAccessManager* transport = nullptr);
    ~OsvClient() override;
    bool busy() const { return m_busy; }
    bool start(const QueryIdentity&);
    void cancel();
    QDateTime retryNotBefore() const { return m_retryNotBefore; }
signals:
    void finished(QueryError error, const OsvSnapshot& snapshot);
private:
    struct Parsed { OsvPageResult page; qsizetype received = 0; };
    void send(const QString& token);
    void drain();
    void replyFinished();
    void stop(QueryError);
    void finish(QueryError);
    QNetworkAccessManager* m_manager;
    QPointer<QNetworkReply> m_reply;
    QFutureWatcher<Parsed> m_parser;
    QTimer m_deadline;
    bool m_busy = false;
    bool m_parsing = false;
    QueryError m_stopped = QueryError::None;
    QueryIdentity m_identity;
    QByteArray m_bytes;
    qsizetype m_totalBytes = 0, m_totalRecords = 0;
    int m_pages = 0;
    QSet<QString> m_tokens;
    QList<VulnerabilityCandidate> m_candidates;
    QDateTime m_retryNotBefore;
};
