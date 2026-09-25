#pragma once

#include <QWidget>

class ProjectRepository;
class ComponentRepository;
class ProjectComponentsModel;
class DependencyPage;
class VulnerabilityPage;
class QTabWidget;
class AppLogger;
class QLabel;
class QListWidget;
class QPushButton;
class QTextBrowser;

// Owns its widgets/dialogs; borrowed repository and logger must outlive the page.
class ProjectPage final : public QWidget
{
    Q_OBJECT

public:
    explicit ProjectPage(ProjectRepository& repository, ComponentRepository& components, AppLogger& logger, const QString& cacheDirectory, QWidget* parent = nullptr);

private:
    QString selectedId() const;
    void reload(const QString& preferredId = {});
    void showSelection();
    void showCreateDialog();
    void confirmRemoval();
    void showSbomImport();

    ProjectRepository& m_repository;
    ComponentRepository& m_components;
    AppLogger& m_logger;
    QListWidget* m_list;
    QLabel* m_empty;
    QLabel* m_error;
    QLabel* m_selectionHint;
    QTextBrowser* m_details;
    QPushButton* m_remove;
    QPushButton* m_import;
    QTabWidget* m_tabs;
    ProjectComponentsModel* m_componentModel;
    QLabel* m_componentSummary;
    DependencyPage* m_dependencies;
    VulnerabilityPage* m_vulnerabilities;
};
