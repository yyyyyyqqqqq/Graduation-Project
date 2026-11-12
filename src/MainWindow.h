#pragma once

#include <QMainWindow>

class QButtonGroup;
class QStackedWidget;
class QScrollArea;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    // Takes ownership of the supplied project page through QStackedWidget.
    explicit MainWindow(QWidget* projectPage, QWidget* parent = nullptr,
                        const QString& initialPage = QStringLiteral("overview"), QWidget* validationPage = nullptr,
                        QWidget* overviewPage = nullptr);
    QString currentPageId() const;
    bool selectPage(const QString& pageId);
    QString currentRouteId() const { return m_routeId; }
    bool selectRoute(const QString& routeId);

signals:
    void navigationChanged(const QString& pageId);
    void workspaceRequested(const QString& routeId);
    void routeChanged(const QString& routeId);

private:
    void updateNavigation();
    QString m_routeId = QStringLiteral("overview"); // GUI route only; never a project or persisted preference.
    QScrollArea* m_navigationScroll = nullptr;
    QButtonGroup* m_navigation = nullptr; // Owned by this window.
    QStackedWidget* m_pages = nullptr;   // Owned by the central widget.
};
