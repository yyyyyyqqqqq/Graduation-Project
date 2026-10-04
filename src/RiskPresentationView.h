#pragma once
#include "RiskPriority.h"
#include <QTextBrowser>

// Shared widget composition, not an alternative evaluator or persisted result.
class RiskPresentationView final : public QTextBrowser
{
    Q_OBJECT
public:
    explicit RiskPresentationView(QWidget* parent = nullptr);
    void showResult(const RiskEvidenceProfile&, const RiskPriorityAssessment&);
};
