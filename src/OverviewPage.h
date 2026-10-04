#pragma once
#include <QWidget>

class ProjectRepository;
class ComponentRepository;
class ProjectPage;
class QLabel;

// A read-only projection of ProjectPage selection; retains no project ID authority.
class OverviewPage final : public QWidget
{
    Q_OBJECT
public:
    OverviewPage(ProjectRepository&, ComponentRepository&, QWidget* parent = nullptr);
    void follow(ProjectPage&);
signals:
    void navigationRequested(const QString& pageId);
private:
    void refresh(const QString& projectId);
    ProjectRepository& m_projects;
    ComponentRepository& m_components;
    QLabel* m_project;
    QLabel* m_supplyChain;
};
