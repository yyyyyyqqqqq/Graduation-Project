#pragma once

#include "ComponentRepository.h"
#include "DependencyAnalyzer.h"
#include <QFutureWatcher>
#include <QWidget>
#include <memory>

class DependencyTableModel;
class QTableView;
class QComboBox;
class QLabel;
class QPushButton;
class QTextBrowser;

// Project-scoped, invalidatable derived state. Workers own their values/SQL
// connections; only this GUI-thread object installs models and query results.
class DependencyPage final : public QWidget
{
    Q_OBJECT
public:
    explicit DependencyPage(QString databasePath, QWidget* parent = nullptr);
    void setProject(const QString& projectId); // Also invalidates same-project Apply/Reload.
    void reload();
signals:
    void analysisFinished(bool success);
    void relationshipFinished();
protected:
    void showEvent(QShowEvent* event) override;
private:
    struct AnalysisResult {
        ComponentResult result;
        std::shared_ptr<const DependencyGraph> graph;
    };
    void invalidate();
    void startAnalysis();
    void installGraph(std::shared_ptr<const DependencyGraph> graph);
    void selectEntry(int row);
    void requestRelationship();
    void startRelationship();

    const QString m_databasePath;
    QString m_projectId;
    quint64 m_generation = 0, m_activeGeneration = 0;
    QString m_activeProject;
    bool m_loading = false, m_pendingAnalysis = false;
    QFutureWatcher<AnalysisResult> m_analysis;
    std::shared_ptr<const DependencyGraph> m_graph;
    quint64 m_queryGeneration = 0, m_activeQuery = 0;
    bool m_querying = false, m_pendingQuery = false;
    QFutureWatcher<QList<DependencyReach>> m_query;
    QTableView *m_entries, *m_targets, *m_components, *m_relationships;
    DependencyTableModel *m_entryModel, *m_targetModel, *m_componentModel, *m_relationshipModel;
    QComboBox* m_direction;
    QLabel *m_status, *m_relationshipStatus;
    QTextBrowser* m_summary;
    QPushButton* m_reload;
};
