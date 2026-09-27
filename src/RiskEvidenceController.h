#pragma once
#include "EvidenceClients.h"
#include "RiskEvidenceCache.h"
#include <atomic>
#include <memory>
class AppLogger;

// Borrows no Finding owner or SQL handle. Only accepts a validated immutable request.
class RiskEvidenceController final : public QObject {
    Q_OBJECT
public:
    explicit RiskEvidenceController(QString cacheRoot,AppLogger&,QObject* parent=nullptr,
                                    QNetworkAccessManager* transport=nullptr);
    ~RiskEvidenceController() override;
    void setRequest(std::optional<RiskEvidenceRequest>);
    void invalidate();
    void load(EvidenceLoadMode);
    void consent(bool);
    void cancel();
    bool busy() const { return m_busy || m_persisting; }
    bool cancellable() const { return m_busy && !m_stopped; }
    QueryError cacheWarning() const { return m_cacheWarning; }
    QueryError error() const { return m_error; }
    bool hasRequest() const { return m_request.has_value(); }
    quint64 generation() const { return m_generation; }
    EvidenceOperationState state() const { return m_state; }
    const std::optional<RiskEvidenceProfile>& profile() const { return m_profile; }
    const QString& displayText() const { return m_display; }
    QString cacheRoot() const { return m_cacheRoot; }
signals:
    void changed();
    void consentRequested(const QStringList& cves);
private:
    struct Batch {
        RiskEvidenceProfile profile;
        QStringList cves, refreshCves;
        QHash<QString,EpssCacheRead> oldEpss;
        KevCacheRead oldKev;
        QHash<QString,EpssSnapshot> newEpss;
        QHash<QString,QueryError> epssErrors;
        KevFetchResult kev;
        bool refreshKev=false, stale=false;
        QueryError localError=QueryError::None;
        QString text;
    };
    void localReady(Batch,quint64,EvidenceLoadMode);
    void online();
    void nextChunk();
    void finalize();
    void stop(EvidenceOperationState, QueryError error=QueryError::None);
    void persistCompleted(const Batch&, quint64 generation);
    void releaseStopped();
    QString m_cacheRoot;
    AppLogger& m_logger;
    EpssClient m_epss;
    KevClient m_kev;
    QTimer m_deadline;
    std::optional<RiskEvidenceRequest> m_request;
    std::optional<RiskEvidenceProfile> m_profile;
    std::optional<Batch> m_batch;
    QString m_display;
    QList<QStringList> m_chunks;
    qsizetype m_next=0;
    quint64 m_generation=0,m_operation=0;
    QueryError m_error=QueryError::None,m_cacheWarning=QueryError::None;
    bool m_persisting=false;
    bool m_busy=false,m_worker=false,m_stopped=false,m_awaiting=false,m_epssDone=true,m_kevDone=true;
    EvidenceOperationState m_state=EvidenceOperationState::NotStarted;
    std::shared_ptr<std::atomic_bool> m_cancelled;
};
