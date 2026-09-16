#pragma once

#include <QMainWindow>

class QButtonGroup;
class QStackedWidget;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    // Takes ownership of the supplied project page through QStackedWidget.
    explicit MainWindow(QWidget* projectPage, QWidget* parent = nullptr,
                        const QString& initialPage = QStringLiteral("overview"));
    QString currentPageId() const;
    bool selectPage(const QString& pageId);

signals:
    void navigationChanged(const QString& pageId);

private:
    QButtonGroup* m_navigation = nullptr; // Owned by this window.
    QStackedWidget* m_pages = nullptr;   // Owned by the central widget.
};
