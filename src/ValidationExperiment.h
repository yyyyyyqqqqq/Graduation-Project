#pragma once
#include "ValidationDataset.h"

namespace Validation
{
struct SyntheticCase
{
    QString name;
    RiskEvidenceProfile profile;
    QDateTime time;
    double threshold;
    PriorityClass priority;
    DecisionEvidenceSupport support;
    PriorityDriverKind driver;
    QString driverCve;
    QStringList explanationFacts;
};
// Expectations are declared independently before any call to the production evaluator.
QList<SyntheticCase> syntheticCases();
QJsonObject syntheticConformance();
QJsonObject assessmentJson(const RiskPriorityAssessment &);
struct Experiment
{
    QList<RiskPriorityAssessment> baseline, lower, upper;
    QJsonObject result; // Deterministic payload; export adds only runTimestampUtc.
};
Experiment runExperiment(const Dataset &);
bool exportResult(const Experiment &, const QString &path, QString &error);
QString datasetDisclaimer();
QString experimentSummary(const Dataset &, const Experiment &);
} // namespace Validation
