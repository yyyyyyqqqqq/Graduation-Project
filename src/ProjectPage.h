#pragma once

#include <QWidget>

class ProjectRepository;
class QLabel;
class QListWidget;
class QPushButton;
class QTextBrowser;

// Owns its widgets/dialogs, borrows a repository that outlives the page.
class ProjectPage final : public QWidget
{
    Q_OBJECT

public:
    explicit ProjectPage(ProjectRepository& repository, QWidget* parent = nullptr);

private:
    QString selectedId() const;
    void reload(const QString& preferredId = {});
    void showSelection();
    void showCreateDialog();
    void confirmRemoval();

    ProjectRepository& m_repository;
    QListWidget* m_list;
    QLabel* m_empty;
    QLabel* m_error;
    QLabel* m_selectionHint;
    QTextBrowser* m_details;
    QPushButton* m_remove;
};
