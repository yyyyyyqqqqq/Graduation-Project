#include "AppDatabase.h"
#include "ComponentRepository.h"
#include "AppLogger.h"
#include "MainWindow.h"
#include "ProjectPage.h"
#include "ProjectRepository.h"

#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QScopeGuard>
#include <QScrollBar>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>
#include <QTextBrowser>
#include <QUuid>

namespace
{
struct Context
{
    QTemporaryDir temporary;
    AppLogger logger;
    AppDatabase database;
    ProjectRepository repository{database, logger};
    ComponentRepository components{database};
    QString error;

    QString file() const { return temporary.filePath(QStringLiteral("projects.db")); }
    bool prepare() { return temporary.isValid() && logger.open(temporary.filePath(QStringLiteral("test.log"))); }
    bool open() { return prepare() && database.open(file(), error); }
};

// Each fixture/inspection connection is removed after all queries and handles are gone.
bool executeSql(const QString& file, const QStringList& statements)
{
    const QString name = QUuid::createUuid().toString();
    const auto cleanup = qScopeGuard([&] { QSqlDatabase::removeDatabase(name); });
    auto db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
    db.setDatabaseName(file);
    if (!db.open()) return false;
    QSqlQuery query(db);
    for (const auto& statement : statements) {
        if (!query.exec(statement)) return false;
    }
    return true;
}

QVariant inspect(const QString& file, const QString& sql)
{
    const QString name = QUuid::createUuid().toString();
    const auto cleanup = qScopeGuard([&] { QSqlDatabase::removeDatabase(name); });
    auto db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
    db.setDatabaseName(file);
    db.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
    if (!db.open()) return {};
    QSqlQuery query(db);
    if (!query.exec(sql) || !query.next()) return {};
    return query.value(0);
}

bool createV1(const QString& file)
{
    return executeSql(file, {
        QStringLiteral("CREATE TABLE app_meta (key TEXT PRIMARY KEY NOT NULL, value TEXT NOT NULL)"),
        QStringLiteral("INSERT INTO app_meta VALUES ('schema_version', '1'), ('preserved', '原有元数据')")});
}

QString version(const QString& file)
{
    return inspect(file, QStringLiteral("SELECT value FROM app_meta WHERE key='schema_version'")).toString();
}

qint64 totalChanges(AppDatabase& db)
{
    QSqlQuery query(db.connection());
    return query.exec(QStringLiteral("SELECT total_changes()")) && query.next() ? query.value(0).toLongLong() : -1;
}

QString screenshotPath(const QString& name)
{
    return QDir(QCoreApplication::applicationDirPath()).filePath(name);
}
}

class Phase02Test final : public QObject
{
    Q_OBJECT
private slots:
    void freshDatabase();
    void migrateV1();
    void migrationRollback();
    void migrationConflict();
    void createAndFind();
    void validation();
    void stableOrder();
    void persistenceAndDelete();
    void databaseErrors();
    void projectUi();
    void uiValidation();
    void uiErrors();
};

void Phase02Test::freshDatabase()
{
    Context context;
    QVERIFY2(context.open(), qPrintable(context.error));
    QCOMPARE(version(context.file()), QStringLiteral("3"));
    auto tables = context.database.connection().tables();
    tables.sort();
    QCOMPARE(tables, (QStringList{QStringLiteral("app_meta"), QStringLiteral("components"), QStringLiteral("projects")}));
    QSqlQuery query(context.database.connection());
    QVERIFY(query.exec(QStringLiteral("PRAGMA table_info(projects)")));
    QStringList columns;
    while (query.next()) columns.append(query.value(1).toString());
    QCOMPARE(columns, (QStringList{QStringLiteral("id"), QStringLiteral("name"),
                                  QStringLiteral("description"), QStringLiteral("created_at")}));
    QList<Project> projects;
    QVERIFY(context.repository.list(projects).ok());
    QVERIFY(projects.isEmpty());
}

void Phase02Test::migrateV1()
{
    Context context;
    QVERIFY(context.prepare());
    QVERIFY(createV1(context.file()));
    QVERIFY2(context.database.open(context.file(), context.error), qPrintable(context.error));
    QCOMPARE(version(context.file()), QStringLiteral("3"));
    QCOMPARE(inspect(context.file(), QStringLiteral("SELECT value FROM app_meta WHERE key='preserved'")).toString(),
             QStringLiteral("原有元数据"));
    Project project;
    QVERIFY(context.repository.create(QStringLiteral("迁移后创建"), {}, project).ok());
    context.database.close();
    QVERIFY(!QSqlDatabase::contains(context.database.connectionName()));
    QVERIFY(context.database.open(context.file(), context.error));
    Project found;
    QVERIFY(context.repository.findById(project.id, found).ok());
    QCOMPARE(found.name, project.name);
    QCOMPARE(version(context.file()), QStringLiteral("3"));
}

void Phase02Test::migrationRollback()
{
    Context context;
    QVERIFY(context.prepare());
    QVERIFY(createV1(context.file()));
    // Fail AFTER CREATE TABLE, proving DDL and metadata are rolled back together.
    QVERIFY(executeSql(context.file(), {QStringLiteral(
        "CREATE TRIGGER reject_version BEFORE UPDATE ON app_meta "
        "WHEN OLD.key='schema_version' BEGIN SELECT RAISE(ABORT, 'migration test failure'); END")}));
    QVERIFY(!context.database.open(context.file(), context.error));
    QVERIFY(context.error.contains(QStringLiteral("Migration 1 -> 2")));
    QVERIFY(!QSqlDatabase::contains(context.database.connectionName()));
    QCOMPARE(version(context.file()), QStringLiteral("1"));
    QCOMPARE(inspect(context.file(), QStringLiteral("SELECT count(*) FROM sqlite_master WHERE name='projects'")).toInt(), 0);
    QCOMPARE(inspect(context.file(), QStringLiteral("SELECT value FROM app_meta WHERE key='preserved'")).toString(),
             QStringLiteral("原有元数据"));
    QVERIFY(executeSql(context.file(), {QStringLiteral("DROP TRIGGER reject_version")}));
    QVERIFY(context.database.open(context.file(), context.error));
    QCOMPARE(version(context.file()), QStringLiteral("3"));
}

void Phase02Test::migrationConflict()
{
    Context context;
    QVERIFY(context.prepare());
    QVERIFY(createV1(context.file()));
    QVERIFY(executeSql(context.file(), {QStringLiteral("CREATE TABLE projects (private_data TEXT)"),
                                       QStringLiteral("INSERT INTO projects VALUES ('keep me')")}));
    QVERIFY(!context.database.open(context.file(), context.error));
    QCOMPARE(version(context.file()), QStringLiteral("1"));
    QCOMPARE(inspect(context.file(), QStringLiteral("SELECT private_data FROM projects")).toString(), QStringLiteral("keep me"));
    // Claiming v2 with incompatible tables also fails without rewriting user data.
    QVERIFY(executeSql(context.file(), {QStringLiteral("UPDATE app_meta SET value='2' WHERE key='schema_version'")}));
    QVERIFY(!context.database.open(context.file(), context.error));
    QVERIFY(context.error.contains(QStringLiteral("Invalid projects schema")));
    QCOMPARE(inspect(context.file(), QStringLiteral("SELECT private_data FROM projects")).toString(), QStringLiteral("keep me"));
}

void Phase02Test::createAndFind()
{
    Context context;
    QVERIFY(context.open());
    Project project;
    const auto before = QDateTime::currentMSecsSinceEpoch();
    const QString name = QStringLiteral(" 测试 O'Reilly <b>项目</b> ");
    const QString description = QStringLiteral(" 私有描述正文\n第二行 ");
    QVERIFY(context.repository.create(name, description, project).ok());
    QVERIFY(!QUuid(project.id).isNull());
    QVERIFY(!project.id.contains(u'{'));
    QCOMPARE(project.name, name.trimmed());
    QCOMPARE(project.description, description.trimmed());
    QVERIFY(project.createdAt >= before && project.createdAt <= QDateTime::currentMSecsSinceEpoch());
    Project found;
    QVERIFY(context.repository.findById(project.id, found).ok());
    QCOMPARE(found.id, project.id);
    QCOMPARE(found.name, project.name);
    QCOMPARE(found.description, project.description);
    QCOMPARE(found.createdAt, project.createdAt);
    QCOMPARE(inspect(context.file(), QStringLiteral("SELECT typeof(created_at) FROM projects")).toString(), QStringLiteral("integer"));
    Project duplicate;
    QVERIFY(context.repository.create(name, {}, duplicate).ok());
    QVERIFY(duplicate.id != project.id);
    QCOMPARE(duplicate.name, project.name);
    QCOMPARE(context.repository.findById(QStringLiteral("' OR 1=1 --"), found).error, ProjectError::NotFound);
    QVERIFY(found.id.isEmpty());
    QFile log(context.temporary.filePath(QStringLiteral("test.log")));
    QVERIFY(log.open(QIODevice::ReadOnly));
    const auto content = log.readAll();
    QVERIFY(content.contains("Project created"));
    QVERIFY(!content.contains(description.trimmed().toUtf8()));
    QVERIFY(!content.contains(name.trimmed().toUtf8()));
}

void Phase02Test::validation()
{
    Context context;
    QVERIFY(context.open());
    Project project;
    for (const auto& name : {QString(), QStringLiteral(" \t\n\u3000 ")}) {
        QCOMPARE(context.repository.create(name, {}, project).error, ProjectError::InvalidName);
        QVERIFY(project.id.isEmpty());
    }
    QVERIFY(context.repository.create(QStringLiteral("A"), {}, project).ok());
    QVERIFY(context.repository.create(QString(100, u'中'), QString(500, u'文'), project).ok());
    QCOMPARE(context.repository.create(QString(101, u'中'), {}, project).error, ProjectError::InvalidName);
    QCOMPARE(context.repository.create(QStringLiteral("A"), QString(501, u'文'), project).error, ProjectError::InvalidDescription);
    QVERIFY(context.repository.create(QStringLiteral("  A  "), QStringLiteral(" \t "), project).ok());
    QVERIFY(project.description.isEmpty());
    const QString emoji = QString::fromUcs4(U"🙂");
    QVERIFY(context.repository.create(emoji.repeated(100), emoji.repeated(500), project).ok());
    QCOMPARE(context.repository.create(emoji.repeated(101), {}, project).error, ProjectError::InvalidName);
    QList<Project> projects;
    QVERIFY(context.repository.list(projects).ok());
    QCOMPARE(projects.size(), 4); // Invalid input never reaches persistence.
}

void Phase02Test::stableOrder()
{
    Context context;
    QVERIFY(context.open());
    // Fixed timestamps exercise tie ordering deterministically, without clock sleeps.
    QVERIFY(executeSql(context.file(), {QStringLiteral(
        "INSERT INTO projects VALUES ('b', '同名', '', 100), ('a', '同名', '', 100), ('c', '旧项目', '', 90)")}));
    QList<Project> projects;
    QVERIFY(context.repository.list(projects).ok());
    QCOMPARE(projects.size(), 3);
    QCOMPARE(projects[0].id, QStringLiteral("b"));
    QCOMPARE(projects[1].id, QStringLiteral("a"));
    QCOMPARE(projects[2].id, QStringLiteral("c"));
}

void Phase02Test::persistenceAndDelete()
{
    Context context;
    QVERIFY(context.open());
    Project project;
    Project other;
    QVERIFY(context.repository.create(QStringLiteral("同名项目"), QStringLiteral("保留内容"), project).ok());
    QVERIFY(context.repository.create(project.name, {}, other).ok());
    context.database.close();
    QVERIFY(context.database.open(context.file(), context.error));
    Project found;
    QVERIFY(context.repository.findById(project.id, found).ok());
    QCOMPARE(found.description, project.description);
    QCOMPARE(found.createdAt, project.createdAt);
    QVERIFY(context.repository.remove(project.id).ok());
    QCOMPARE(context.repository.remove(project.id).error, ProjectError::NotFound);
    QCOMPARE(context.repository.remove(QStringLiteral("' OR 1=1 --")).error, ProjectError::NotFound);
    context.database.close();
    QVERIFY(context.database.open(context.file(), context.error));
    QCOMPARE(context.repository.findById(project.id, found).error, ProjectError::NotFound);
    QVERIFY(context.repository.findById(other.id, found).ok());
    QVERIFY(context.repository.remove(other.id).ok());
    context.database.close();
    QVERIFY(context.database.open(context.file(), context.error));
    QList<Project> projects;
    QVERIFY(context.repository.list(projects).ok());
    QVERIFY(projects.isEmpty());
}

void Phase02Test::databaseErrors()
{
    Context context;
    QVERIFY(context.open());
    Project project;
    QVERIFY(context.repository.create(QStringLiteral("保留项目"), {}, project).ok());
    {
        QSqlQuery query(context.database.connection());
        QVERIFY(query.exec(QStringLiteral("PRAGMA query_only=ON")));
    }
    Project failed;
    auto result = context.repository.create(QStringLiteral("不应成功"), {}, failed);
    QCOMPARE(result.error, ProjectError::Database);
    QVERIFY(!result.diagnostic.isEmpty());
    QVERIFY(!result.userMessage().contains(result.diagnostic));
    QVERIFY(failed.id.isEmpty());
    QCOMPARE(context.repository.remove(project.id).error, ProjectError::Database);
    QVERIFY(context.repository.findById(project.id, failed).ok());
    context.database.close();
    QList<Project> projects{project};
    QCOMPARE(context.repository.list(projects).error, ProjectError::Database);
    QVERIFY(projects.isEmpty());
    QCOMPARE(context.repository.findById(project.id, failed).error, ProjectError::Database);
    QCOMPARE(context.repository.remove(project.id).error, ProjectError::Database);
    QCOMPARE(context.repository.create(QStringLiteral("A"), {}, failed).error, ProjectError::Database);
    QVERIFY(!QSqlDatabase::contains()); // No accidental default connection.
}

void Phase02Test::projectUi()
{
    Context context;
    QVERIFY(context.open());
    MainWindow window(new ProjectPage(context.repository, context.components, context.logger), nullptr, QStringLiteral("projects"));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto* page = window.findChild<ProjectPage*>();
    QVERIFY(page);
    auto* list = page->findChild<QListWidget*>(QStringLiteral("projectList"));
    auto* empty = page->findChild<QLabel*>(QStringLiteral("projectsEmpty"));
    auto* create = page->findChild<QPushButton*>(QStringLiteral("createProject"));
    auto* remove = page->findChild<QPushButton*>(QStringLiteral("removeProject"));
    auto* details = page->findChild<QTextBrowser*>(QStringLiteral("projectDetails"));
    QVERIFY(list && empty && create && remove && details);
    QVERIFY(empty->isVisible());
    QVERIFY(!remove->isEnabled());
    QVERIFY(window.grab().save(screenshotPath(QStringLiteral("phase02-empty.png"))));

    for (const QString& name : {QStringLiteral("测试项目 A"), QStringLiteral("测试项目 B")}) {
        QTest::mouseClick(create, Qt::LeftButton);
        QPointer<QDialog> dialog = page->findChild<QDialog*>(QStringLiteral("createProjectDialog"));
        QVERIFY(dialog);
        QVERIFY(dialog->isVisible());
        auto* nameInput = dialog->findChild<QLineEdit*>(QStringLiteral("newProjectName"));
        auto* description = dialog->findChild<QPlainTextEdit*>(QStringLiteral("newProjectDescription"));
        auto* buttons = dialog->findChild<QDialogButtonBox*>();
        QVERIFY(nameInput && description && buttons);
        QTest::mouseClick(buttons->button(QDialogButtonBox::Ok), Qt::LeftButton);
        QVERIFY(dialog->isVisible());
        QVERIFY(!dialog->findChild<QLabel*>(QStringLiteral("createProjectError"))->text().isEmpty());
        nameInput->setText(name);
        description->setPlainText(QStringLiteral("Phase 02 项目管理人工验收"));
        QVERIFY(dialog->grab().save(screenshotPath(QStringLiteral("phase02-create.png"))));
        QTest::mouseClick(buttons->button(QDialogButtonBox::Ok), Qt::LeftButton);
        QTRY_VERIFY(dialog.isNull());
        QVERIFY(!empty->isVisible());
        QVERIFY(list->currentItem());
        QVERIFY(details->toPlainText().contains(name));
        QVERIFY(details->toPlainText().contains(QStringLiteral("Phase 02 项目管理人工验收")));
        Project selected;
        QVERIFY(context.repository.findById(list->currentItem()->data(Qt::UserRole).toString(), selected).ok());
        QVERIFY(details->toPlainText().contains(QDateTime::fromMSecsSinceEpoch(selected.createdAt)
                                                   .toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))));
    }
    QCOMPARE(list->count(), 2);
    QList<Project> projects;
    QVERIFY(context.repository.list(projects).ok());
    QCOMPARE(list->item(0)->data(Qt::UserRole).toString(), projects[0].id);
    QCOMPARE(list->item(1)->data(Qt::UserRole).toString(), projects[1].id);
    list->setCurrentRow(1);
    QVERIFY(details->toPlainText().contains(list->item(1)->text()));
    list->setCurrentRow(0);
    QVERIFY(details->toPlainText().contains(list->item(0)->text()));
    QVERIFY(window.grab().save(screenshotPath(QStringLiteral("phase02-projects.png"))));
    window.resize(680, 420);
    QCoreApplication::processEvents();
    QVERIFY(window.grab().save(screenshotPath(QStringLiteral("phase02-small.png"))));

    const QString selectedId = list->currentItem()->data(Qt::UserRole).toString();
    const auto changesBeforeCancel = totalChanges(context.database);
    QTest::mouseClick(remove, Qt::LeftButton);
    QPointer<QMessageBox> confirmation = page->findChild<QMessageBox*>(QStringLiteral("removeProjectDialog"));
    QVERIFY(confirmation);
    QVERIFY(confirmation->isVisible());
    QCOMPARE(confirmation->defaultButton(), confirmation->button(QMessageBox::Cancel));
    QTest::mouseClick(confirmation->button(QMessageBox::Cancel), Qt::LeftButton);
    QTRY_VERIFY(confirmation.isNull());
    QCOMPARE(totalChanges(context.database), changesBeforeCancel);
    QCOMPARE(list->count(), 2);
    Project found;
    QVERIFY(context.repository.findById(selectedId, found).ok());

    for (int remaining = 1; remaining >= 0; --remaining) {
        list->setCurrentRow(0);
        const QString id = list->currentItem()->data(Qt::UserRole).toString();
        QTest::mouseClick(remove, Qt::LeftButton);
        confirmation = page->findChild<QMessageBox*>(QStringLiteral("removeProjectDialog"));
        QVERIFY(confirmation);
        QTest::mouseClick(confirmation->button(QMessageBox::Yes), Qt::LeftButton);
        QTRY_VERIFY(confirmation.isNull());
        QCOMPARE(list->count(), remaining);
        QCOMPARE(context.repository.findById(id, found).error, ProjectError::NotFound);
        QVERIFY(details->toPlainText().isEmpty());
        QVERIFY(!remove->isEnabled());
    }
    QVERIFY(empty->isVisible());
    for (const QString& id : {QStringLiteral("overview"), QStringLiteral("settings"), QStringLiteral("projects")}) {
        auto* button = window.findChild<QPushButton*>(QStringLiteral("nav_%1").arg(id));
        QVERIFY(button);
        QTest::mouseClick(button, Qt::LeftButton);
        QCOMPARE(window.currentPageId(), id);
        QVERIFY(button->isChecked());
    }
    QVERIFY(window.close());
}

void Phase02Test::uiValidation()
{
    Context context;
    QVERIFY(context.open());
    MainWindow window(new ProjectPage(context.repository, context.components, context.logger), nullptr, QStringLiteral("projects"));
    window.resize(680, 420);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto* page = window.findChild<ProjectPage*>();
    auto* create = page->findChild<QPushButton*>(QStringLiteral("createProject"));
    const auto changesBefore = totalChanges(context.database);
    QTest::mouseClick(create, Qt::LeftButton);
    QPointer<QDialog> dialog = page->findChild<QDialog*>(QStringLiteral("createProjectDialog"));
    QVERIFY(dialog);
    auto* name = dialog->findChild<QLineEdit*>();
    auto* description = dialog->findChild<QPlainTextEdit*>();
    auto* buttons = dialog->findChild<QDialogButtonBox*>();
    auto* error = dialog->findChild<QLabel*>(QStringLiteral("createProjectError"));
    name->setText(QString(101, u'A'));
    QTest::mouseClick(buttons->button(QDialogButtonBox::Ok), Qt::LeftButton);
    QVERIFY(error->text().contains(QStringLiteral("1—100")));
    name->setText(QString(100, u'A'));
    description->setPlainText(QString(501, u'B'));
    QTest::mouseClick(buttons->button(QDialogButtonBox::Ok), Qt::LeftButton);
    QVERIFY(error->text().contains(QStringLiteral("500")));
    QCOMPARE(totalChanges(context.database), changesBefore);
    description->setPlainText(QString(500, u'B'));
    QTest::mouseClick(buttons->button(QDialogButtonBox::Ok), Qt::LeftButton);
    QTRY_VERIFY(dialog.isNull());
    auto* details = page->findChild<QTextBrowser*>(QStringLiteral("projectDetails"));
    QVERIFY(details->toPlainText().contains(QString(100, u'A')));
    QVERIFY(details->toPlainText().contains(QString(500, u'B')));
    QCoreApplication::processEvents();
    QCOMPARE(window.size(), QSize(680, 420));
    QCOMPARE(details->horizontalScrollBar()->maximum(), 0);
    QVERIFY(window.grab().save(screenshotPath(QStringLiteral("phase02-long-text.png"))));

    const auto changesBeforeCancel = totalChanges(context.database);
    QTest::mouseClick(create, Qt::LeftButton);
    dialog = page->findChild<QDialog*>(QStringLiteral("createProjectDialog"));
    QVERIFY(dialog);
    dialog->findChild<QLineEdit*>()->setText(QStringLiteral("取消创建"));
    buttons = dialog->findChild<QDialogButtonBox*>();
    QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel), Qt::LeftButton);
    QTRY_VERIFY(dialog.isNull());
    QCOMPARE(totalChanges(context.database), changesBeforeCancel);
    QCOMPARE(page->findChild<QListWidget*>()->count(), 1);
}

void Phase02Test::uiErrors()
{
    Context context;
    QVERIFY(context.open());
    Project project;
    QVERIFY(context.repository.create(QStringLiteral("保留项目"), {}, project).ok());
    MainWindow window(new ProjectPage(context.repository, context.components, context.logger), nullptr, QStringLiteral("projects"));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto* page = window.findChild<ProjectPage*>();
    auto* list = page->findChild<QListWidget*>(QStringLiteral("projectList"));
    QCOMPARE(list->count(), 1);
    QVERIFY(page->findChild<QLabel*>(QStringLiteral("projectSelectionHint"))->isVisible());
    list->setCurrentRow(0);
    {
        QSqlQuery query(context.database.connection());
        QVERIFY(query.exec(QStringLiteral("PRAGMA query_only=ON")));
    }
    QTest::mouseClick(page->findChild<QPushButton*>(QStringLiteral("createProject")), Qt::LeftButton);
    QPointer<QDialog> dialog = page->findChild<QDialog*>(QStringLiteral("createProjectDialog"));
    QVERIFY(dialog);
    dialog->findChild<QLineEdit*>()->setText(QStringLiteral("创建失败"));
    auto* buttons = dialog->findChild<QDialogButtonBox*>();
    QTest::mouseClick(buttons->button(QDialogButtonBox::Ok), Qt::LeftButton);
    QVERIFY(dialog->isVisible());
    QVERIFY(dialog->findChild<QLabel*>(QStringLiteral("createProjectError"))->text().contains(QStringLiteral("无法读写")));
    QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel), Qt::LeftButton);
    QTRY_VERIFY(dialog.isNull());
    QCOMPARE(list->count(), 1);
    QTest::mouseClick(page->findChild<QPushButton*>(QStringLiteral("removeProject")), Qt::LeftButton);
    QPointer<QMessageBox> confirmation = page->findChild<QMessageBox*>();
    QVERIFY(confirmation);
    QTest::mouseClick(confirmation->button(QMessageBox::Yes), Qt::LeftButton);
    QTRY_VERIFY(confirmation.isNull());
    QCOMPARE(list->count(), 1);
    QVERIFY(page->findChild<QLabel*>(QStringLiteral("projectError"))->isVisible());
    context.database.close();
    QTest::mouseClick(page->findChild<QPushButton*>(QStringLiteral("refreshProjects")), Qt::LeftButton);
    QCOMPARE(list->count(), 0);
    QVERIFY(!page->findChild<QLabel*>(QStringLiteral("projectsEmpty"))->isVisible());
    QVERIFY(page->findChild<QLabel*>(QStringLiteral("projectError"))->isVisible());
    QVERIFY(!page->findChild<QPushButton*>(QStringLiteral("removeProject"))->isEnabled());
}

QTEST_MAIN(Phase02Test)
#include "Phase02Test.moc"
