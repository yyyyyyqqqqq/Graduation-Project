#include "RiskEvidenceController.h"
#include "ComponentRepository.h"
#include "CveIdentity.h"
#include "AppLogger.h"
#include <QtConcurrentRun>
#include <limits>

RiskEvidenceController::RiskEvidenceController(QString root,AppLogger& logger,QObject* parent,QNetworkAccessManager* transport)
    :QObject(parent),m_cacheRoot(std::move(root)),m_logger(logger),m_epss(this,transport),m_kev(this,transport)
{
    m_assessmentTimer.setObjectName("assessmentFreshnessTimer");
    m_assessmentTimer.setParent(this);
    m_assessmentTimer.setSingleShot(true);
    m_assessmentTimer.setTimerType(Qt::PreciseTimer);
    m_deadline.setSingleShot(true);m_deadline.setInterval(180000);
    connect(&m_deadline,&QTimer::timeout,this,[this]{stop(EvidenceOperationState::Failed,QueryError::Timeout);});
    connect(&m_epss,&EpssClient::finished,this,[this](const EpssChunkResult& result){
        if(m_stopped || m_operation!=m_generation){releaseStopped();return;}
        for(const auto& cve:result.requested)m_batch->epssErrors.insert(cve,result.error);
        if(result.error==QueryError::None)for(const auto& s:result.snapshots)m_batch->newEpss.insert(s.cve,s);
        nextChunk();
    });
    connect(&m_kev,&KevClient::finished,this,[this](const KevFetchResult& result){
        if(m_stopped || m_operation!=m_generation){releaseStopped();return;}
        m_batch->kev=result;m_kevDone=true;finalize();
    });
}
RiskEvidenceController::~RiskEvidenceController()
{
    stopAssessmentTimer();
    if(m_cancelled)m_cancelled->store(true);
    // Child clients abort replies; value workers never reference this controller.
}
void RiskEvidenceController::setRequest(std::optional<RiskEvidenceRequest> request)
{
    if(request && m_request && request->key()==m_request->key())return;
    invalidate();m_request=std::move(request);emit changed();
}
void RiskEvidenceController::invalidate()
{
    ++m_generation;m_cacheWarning=QueryError::None;m_request.reset();clearPublished();
    stop(EvidenceOperationState::NotStarted);
}
void RiskEvidenceController::stop(EvidenceOperationState state,QueryError error)
{
    m_error=error;m_state=state;m_stopped=true;m_awaiting=false;m_deadline.stop();
    if(m_cancelled)m_cancelled->store(true);
    m_epss.cancel();m_kev.cancel();releaseStopped();emit changed();
}
void RiskEvidenceController::releaseStopped()
{
    if(!m_worker && !m_epss.busy() && !m_kev.busy()) {m_busy=false;m_batch.reset();}
    emit changed();
}
void RiskEvidenceController::cancel(){if(m_busy)stop(EvidenceOperationState::Cancelled);}
void RiskEvidenceController::load(EvidenceLoadMode mode)
{
    if(busy() || !m_request)return;
    ++m_generation;m_error=m_cacheWarning=QueryError::None;m_operation=m_generation;m_stopped=false;m_awaiting=false;
    m_busy=m_worker=true;m_epssDone=m_kevDone=true;m_state=m_profile?EvidenceOperationState::Refreshing:EvidenceOperationState::Loading;
    m_cancelled=std::make_shared<std::atomic_bool>(false);m_deadline.start();
    const auto generation=m_generation;
    auto* watcher=new QFutureWatcher<Batch>(this);
    connect(watcher,&QFutureWatcher<Batch>::finished,this,[this,watcher,generation,mode]{
        auto result=watcher->result();watcher->deleteLater();m_worker=false;
        if(m_stopped || generation!=m_generation){releaseStopped();return;}
        localReady(std::move(result),generation,mode);
    });
    watcher->setFuture(QtConcurrent::run([request=*m_request,root=m_cacheRoot,stop=m_cancelled]{
        Batch batch;batch.profile.key=request.key();
        batch.profile.severity=RiskEvidence::severity(request);
        DependencySnapshot snapshot;
        const auto read=ComponentRepository::readSnapshotInFile(request.databasePath(),request.projectId(),snapshot);
        if(!read.ok()){batch.localError=QueryError::Database;return batch;}
        const auto context=RiskEvidence::dependency(std::move(snapshot),request.key().componentId);
        if(!context){batch.stale=true;return batch;}
        batch.profile.dependency=*context;
        batch.cves=sortedCveIds(request.finding().cveAliases);
        if(batch.cves.size()>RiskEvidence::MaxAliases){batch.localError=QueryError::ResponseLimitExceeded;return batch;}
        for(const auto& cve:batch.cves) {
            if(stop->load())return batch;
            batch.oldEpss.insert(cve,EpssCache(root).read(cve));
        }
        if(!batch.cves.isEmpty() && !stop->load())batch.oldKev=KevCache(root).read();
        return batch;
    }));
    emit changed();
}
void RiskEvidenceController::localReady(Batch batch,quint64 generation,EvidenceLoadMode mode)
{
    if(generation!=m_generation)return;
    if(batch.stale){clearPublished();stop(EvidenceOperationState::Stale);return;}
    if(batch.localError!=QueryError::None){stop(EvidenceOperationState::Failed,batch.localError);return;}
    const auto now=QDateTime::currentDateTimeUtc();
    if(mode!=EvidenceLoadMode::CacheOnly) {
        for(const auto& cve:batch.cves) {
            const auto old=batch.oldEpss.value(cve);
            if(mode==EvidenceLoadMode::Refresh || !old.snapshot ||
                RiskEvidence::freshness(old.snapshot->fetchedAt,now)==EvidenceFreshness::Stale)batch.refreshCves.append(cve);
        }
        batch.refreshKev=!batch.cves.isEmpty() && (mode==EvidenceLoadMode::Refresh || !batch.oldKev.snapshot ||
            RiskEvidence::freshness(batch.oldKev.snapshot->fetchedAt,now)==EvidenceFreshness::Stale);
    }
    m_batch=std::move(batch);
    m_chunks=RiskEvidence::epssChunks(m_batch->refreshCves);m_next=0;
    if(!m_batch->refreshCves.isEmpty() && m_chunks.isEmpty()) {
        for(const auto& c:m_batch->refreshCves)m_batch->epssErrors.insert(c,QueryError::ResponseLimitExceeded);
        m_batch->refreshCves.clear(); // No unsafe/oversized request sent.
    }
    if(!m_chunks.isEmpty()) {
        m_awaiting=true;m_state=EvidenceOperationState::AwaitingConsent;
        emit changed();emit consentRequested(m_batch->refreshCves);
    } else online();
}
void RiskEvidenceController::consent(bool accepted)
{
    if(!m_busy || !m_awaiting)return;
    m_awaiting=false;
    if(!accepted){cancel();return;}
    online();
}
void RiskEvidenceController::online()
{
    if(!m_busy || m_stopped || !m_batch)return;
    m_state=EvidenceOperationState::Refreshing;m_epssDone=m_chunks.isEmpty();m_kevDone=!m_batch->refreshKev;
    if(!m_kevDone && !m_kev.fetch()){m_batch->kev.error=QueryError::RequestRejected;m_kevDone=true;}
    nextChunk();emit changed();
}
void RiskEvidenceController::nextChunk()
{
    if(m_stopped || !m_batch)return;
    if(m_next<m_chunks.size()) {
        const auto chunk=m_chunks[m_next++];
        if(!m_epss.fetch(chunk)) {
            for(const auto& cve:chunk)m_batch->epssErrors.insert(cve,QueryError::RequestRejected);
            QMetaObject::invokeMethod(this,[this]{nextChunk();},Qt::QueuedConnection);
        }
        return;
    }
    m_epssDone=true;finalize();
}
void RiskEvidenceController::finalize()
{
    if(m_stopped || m_worker || !m_batch || !m_epssDone || !m_kevDone)return;
    m_worker=true;const auto generation=m_generation;
    auto* watcher=new QFutureWatcher<Batch>(this);
    connect(watcher,&QFutureWatcher<Batch>::finished,this,[this,watcher,generation]{
        auto batch=watcher->result();watcher->deleteLater();m_worker=false;
        if(m_stopped || generation!=m_generation){releaseStopped();return;}
        if(batch.stale){clearPublished();stop(EvidenceOperationState::Stale);return;}
        if(batch.localError!=QueryError::None){stop(EvidenceOperationState::Failed,batch.localError);return;}
        if(!m_request || batch.profile.key!=m_request->key()){clearPublished();stop(EvidenceOperationState::Stale);return;}
        stopAssessmentTimer();
        m_profile=std::move(batch.profile);m_display=std::move(batch.text);
        // No signal/event-loop reentry between installing the complete profile and its assessment.
        evaluatePublished();
        m_busy=false;m_batch.reset();m_deadline.stop();m_state=EvidenceOperationState::Complete;
        m_logger.write(AppLogger::Level::Info,QStringLiteral("Risk evidence operation completed; EPSS rows=%1, KEV rows=%2")
            .arg(m_profile->epss.size()).arg(m_profile->kev.size()));
        persistCompleted(batch,generation);
        emit changed();
    });
    watcher->setFuture(QtConcurrent::run([batch=*m_batch,request=*m_request,stop=m_cancelled]() mutable {
        // Reread current state before publication too: a successful replacement changes row UUIDs.
        DependencySnapshot current;
        const auto read=ComponentRepository::readSnapshotInFile(request.databasePath(),request.projectId(),current);
        if(!read.ok()){batch.localError=QueryError::Database;return batch;}
        bool present=false;for(const auto& c:current.components)if(c.id==request.key().componentId)present=true;
        if(!present){batch.stale=true;return batch;}
        const auto now=QDateTime::currentDateTimeUtc();
        if(batch.cves.isEmpty()) {
            EpssEvidence e;e.status=EpssStatus::NotQueryable;batch.profile.epss.append(e);
            KevEvidence k;k.status=KevStatus::NotQueryable;batch.profile.kev.append(k);
        }
        for(const auto& cve:batch.cves) {
            if(stop->load())return batch;
            const auto old=batch.oldEpss.value(cve);EpssEvidence e;e.cve=cve;
            const auto error=batch.epssErrors.value(cve,QueryError::None);
            if(batch.newEpss.contains(cve)) {
                const auto& snapshot=batch.newEpss[cve];
                e=RiskEvidence::epssEvidence(snapshot,EvidenceAcquisition::Live,now);
            } else if(old.snapshot) {
                e=RiskEvidence::epssEvidence(*old.snapshot,error==QueryError::None?EvidenceAcquisition::Cache:EvidenceAcquisition::StaleFallback,now);
            } else {
                e.status=error==QueryError::ResponseInvalid?EpssStatus::InvalidResponse:EpssStatus::Failed;
                e.cacheError=old.error;
            }
            e.error=error;batch.profile.epss.append(e);
        }
        for(const auto& cve:batch.cves) {
            KevEvidence e;e.cve=cve;
            if(batch.kev.snapshot) {
                e=RiskEvidence::kevEvidence(*batch.kev.snapshot,cve,EvidenceAcquisition::Live,now);
            } else if(batch.oldKev.snapshot) {
                e=RiskEvidence::kevEvidence(*batch.oldKev.snapshot,cve,batch.kev.error==QueryError::None?EvidenceAcquisition::Cache:EvidenceAcquisition::StaleFallback,now);
            } else e.cacheError=batch.oldKev.error;
            e.error=batch.kev.error;batch.profile.kev.append(e);
        }
        batch.profile.generatedAt=now;batch.text=RiskEvidence::profileText(batch.profile);return batch;
    }));
}

void RiskEvidenceController::stopAssessmentTimer()
{
    ++m_assessmentTimerGeneration;
    m_assessmentTimer.stop();
    disconnect(m_assessmentTimerConnection);
    m_assessmentTimerConnection = {};
}

void RiskEvidenceController::clearPublished()
{
    stopAssessmentTimer();
    m_profile.reset();
    m_assessment.reset();
    m_display.clear();
    m_assessmentText.clear();
}

void RiskEvidenceController::evaluatePublished()
{
    if (!m_profile) return;
    // QTimer is only a trigger. Late delivery/resume uses actual UTC, never the planned expiry.
    auto assessment = RiskPriorityEvaluator::evaluate(*m_profile, QDateTime::currentDateTimeUtc());
    auto text = priorityExplanation(assessment);
    m_assessment = std::move(assessment);
    m_assessmentText = std::move(text);
    scheduleAssessmentExpiry();
}

void RiskEvidenceController::scheduleAssessmentExpiry()
{
    stopAssessmentTimer();
    if (!m_profile || !m_assessment || !m_assessment->nextFreshnessExpiryUtc) return;
    const auto timerGeneration = m_assessmentTimerGeneration;
    const auto key = m_profile->key;
    const auto generatedAt = m_profile->generatedAt;
    m_assessmentTimerConnection = connect(&m_assessmentTimer, &QTimer::timeout, this,
        [this, timerGeneration, key, generatedAt] {
            if (timerGeneration != m_assessmentTimerGeneration || !m_profile || !m_request
                || m_profile->key != key || m_request->key() != key || m_profile->generatedAt != generatedAt) return;
            // Pure local computation only: no provider, consent, SQL, graph or cache work.
            evaluatePublished();
            emit changed();
        }, Qt::QueuedConnection);
    const auto remaining = QDateTime::currentDateTimeUtc().msecsTo(*m_assessment->nextFreshnessExpiryUtc);
    m_assessmentTimer.start(int(qBound(qint64(1), remaining, qint64(std::numeric_limits<int>::max()))));
}

void RiskEvidenceController::persistCompleted(const Batch& batch,quint64 generation)
{
    if(batch.newEpss.isEmpty() && !batch.kev.snapshot)return;
    // The complete profile is installed on the GUI thread before persistence starts.
    // Cancelled/stale operations never enter this stage, so they cannot replace raw caches.
    // This is a non-cancellable save of an already completed operation; new loads wait
    // for it to finish. Cache write warnings do not mutate the immutable profile.
    m_persisting=true;
    auto* watcher=new QFutureWatcher<QueryError>(this);
    connect(watcher,&QFutureWatcher<QueryError>::finished,this,[this,watcher,generation]{
        const auto error=watcher->result();watcher->deleteLater();m_persisting=false;
        if(generation==m_generation)m_cacheWarning=error;
        emit changed();
    });
    watcher->setFuture(QtConcurrent::run([epss=batch.newEpss,kev=batch.kev.snapshot,root=m_cacheRoot]{
        QueryError warning=QueryError::None;
        for(const auto& snapshot:epss){const auto error=EpssCache(root).write(snapshot);if(error!=QueryError::None)warning=error;}
        if(kev){const auto error=KevCache(root).write(*kev);if(error!=QueryError::None)warning=error;}
        return warning;
    }));
}
