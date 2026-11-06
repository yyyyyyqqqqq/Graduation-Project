#pragma once
#include <QWidget>

class ProjectRepository;
class ComponentRepository;
class ProjectPage;
class QLabel;
class QGridLayout;
class QBoxLayout;
class QResizeEvent;

// A read-only projection of ProjectPage selection; retains no project ID authority.
class OverviewPage final : public QWidget
{
    Q_OBJECT
public:
    OverviewPage(ProjectRepository&, ComponentRepository&, QWidget* parent = nullptr);
    void follow(ProjectPage&);
signals:
    void navigationRequested(const QString& pageId);
protected:
    void resizeEvent(QResizeEvent*) override;
private:
    void refresh(const QString& projectId);
    ProjectRepository& m_projects;
    ComponentRepository& m_components;
    QLabel* m_project;
    QLabel* m_supplyChain;
    QLabel* m_projectState;
    QLabel* m_supplyHint;
    QWidget* m_projectCard;
    QWidget* m_supplyCard;
    QGridLayout* m_summaryLayout;
    QBoxLayout* m_actionsLayout;
    bool m_wideLayout = false;
};
