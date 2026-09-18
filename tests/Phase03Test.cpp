#include "AppDatabase.h"
#include "ComponentRepository.h"
#include "AppLogger.h"
#include "CycloneDxParser.h"
#include "MainWindow.h"
#include "ProjectPage.h"
#include "ProjectRepository.h"
#include "SbomImportDialog.h"

#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QListWidget>
#include <QPointer>
#include <QPushButton>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QSqlQuery>
#include <QTableView>
#include <QTemporaryDir>
#include <QTest>
#include <QTextBrowser>
#include <QThreadPool>

namespace
{
QJsonObject minimalObject()
{
    return {{QStringLiteral("bomFormat"), QStringLiteral("CycloneDX")},
            {QStringLiteral("specVersion"), QStringLiteral("1.6")}};
}

QByteArray encode(const QJsonObject& object) { return QJsonDocument(object).toJson(QJsonDocument::Compact); }

// Entirely synthetic, shared by parser/UI tests and the ignored manual-acceptance file.
QByteArray demoJson()
{
    return R"({
        "bomFormat":"CycloneDX", "specVersion":"1.6", "version":7,
        "serialNumber":"urn:uuid:00000000-0000-4000-8000-000000000001",
        "metadata":{"timestamp":"2026-01-01T00:00:00Z",
                    "component":{"bom-ref":"demo-app","type":"application","name":"demo-app","version":"2.0"}},
        "components":[
            {"bom-ref":"demo-lib-a","type":"library","name":"demo-lib-a","version":"1.0",
             "purl":"pkg:generic/demo-lib-a@1.0",
             "components":[{"bom-ref":"demo-lib-b","type":"library","name":"demo-lib-b","version":"3.0"}]},
            {"type":"library","name":"demo-lib-c"}
        ],
        "dependencies":[{"ref":"demo-app","dependsOn":["demo-lib-a","demo-lib-c"]},
                        {"ref":"demo-lib-a","dependsOn":["demo-lib-b"]},
                        {"ref":"demo-lib-b","dependsOn":[]}]
    })";
}

bool writeFile(const QString& path, const QByteArray& data)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
}

struct Context
{
    QTemporaryDir temporary;
    AppLogger logger;
    AppDatabase database;
    ProjectRepository repository{database, logger};
    ComponentRepository components{database};
    bool open()
    {
        QString error;
        return temporary.isValid() && logger.open(temporary.filePath(QStringLiteral("test.log")))
            && database.open(temporary.filePath(QStringLiteral("test.db")), error);
    }
};

QVariant scalar(AppDatabase& database, const QString& sql)
{
    QSqlQuery query(database.connection());
    return query.exec(sql) && query.next() ? query.value(0) : QVariant();
}
}

class Phase03Test final : public QObject
{
    Q_OBJECT
private slots:
    void minimal();
    void supportedVersions();
    void components();
    void metadata();
    void dependencies();
    void invalidJson();
    void invalidHeaders();
    void invalidStructures();
    void utf8();
    void limits();
    void fileRead();
    void previewAtomicity();
    void projectIntegration();
    void largePreview();
    void closingDuringImport();
};

void Phase03Test::minimal()
{
    auto object = minimalObject();
    const auto result = CycloneDxParser::parse(encode(object));
    QVERIFY(result.ok());
    QCOMPARE(result.document.bomFormat, QStringLiteral("CycloneDX"));
    QVERIFY(!result.document.bomVersion.has_value());
    QVERIFY(!result.document.metadata.component.has_value());
    QVERIFY(result.document.components.isEmpty());
    QVERIFY(result.document.dependencies.isEmpty());
    object.insert(QStringLiteral("components"), QJsonArray{});
    object.insert(QStringLiteral("dependencies"), QJsonArray{});
    QVERIFY(CycloneDxParser::parse(encode(object)).ok());
}

void Phase03Test::supportedVersions()
{
    for (const auto& version : {QStringLiteral("1.4"), QStringLiteral("1.5"), QStringLiteral("1.6")}) {
        auto object = QJsonDocument::fromJson(demoJson()).object();
        object.insert(QStringLiteral("specVersion"), version);
        const auto result = CycloneDxParser::parse(encode(object));
        QVERIFY(result.ok());
        QCOMPARE(result.document.specVersion, version);
        QCOMPARE(result.document.components.size(), 3);
        QCOMPARE(result.document.dependencies.size(), 3);
    }
}

void Phase03Test::components()
{
    auto result = CycloneDxParser::parse(demoJson());
    QVERIFY(result.ok());
    const auto& first = result.document.components.at(0);
    QCOMPARE(first.bomRef, QStringLiteral("demo-lib-a"));
    QCOMPARE(first.type, QStringLiteral("library"));
    QCOMPARE(first.name, QStringLiteral("demo-lib-a"));
    QCOMPARE(first.version, QStringLiteral("1.0"));
    QCOMPARE(first.purl, QStringLiteral("pkg:generic/demo-lib-a@1.0"));
    QCOMPARE(result.document.components.at(1).name, QStringLiteral("demo-lib-b"));
    QVERIFY(result.document.components.at(2).version.isEmpty());
    QVERIFY(result.document.components.at(2).purl.isEmpty());
    QVERIFY(result.document.components.at(2).bomRef.isEmpty());
    auto object = minimalObject();
    object.insert(QStringLiteral("components"), QJsonArray{QJsonObject{}, QJsonObject{{"purl", " not normalized "}}});
    result = CycloneDxParser::parse(encode(object));
    QVERIFY(result.ok());
    QCOMPARE(result.document.components.size(), 2);
    QCOMPARE(result.document.components.at(1).purl, QStringLiteral(" not normalized "));
    QVERIFY(result.document.dependencies.isEmpty()); // No implicit dependency from containment.
}

void Phase03Test::metadata()
{
    const auto result = CycloneDxParser::parse(demoJson());
    QVERIFY(result.ok());
    QCOMPARE(*result.document.bomVersion, 7);
    QCOMPARE(result.document.serialNumber, QStringLiteral("urn:uuid:00000000-0000-4000-8000-000000000001"));
    QCOMPARE(result.document.metadata.timestamp, QStringLiteral("2026-01-01T00:00:00Z"));
    QVERIFY(result.document.metadata.component.has_value());
    QCOMPARE(result.document.metadata.component->name, QStringLiteral("demo-app"));
    QCOMPARE(result.document.metadata.component->version, QStringLiteral("2.0"));
    auto object = minimalObject();
    const QJsonObject rootComponent{{"name", "root"},
        {"components", QJsonArray{QJsonObject{{"name", "nested"}}}}};
    object.insert(QStringLiteral("metadata"), QJsonObject{{"component", rootComponent}});
    const auto nested = CycloneDxParser::parse(encode(object));
    QVERIFY(nested.ok());
    QCOMPARE(nested.document.components.size(), 1);
    QCOMPARE(nested.document.components[0].name, QStringLiteral("nested"));
    QVERIFY(nested.document.dependencies.isEmpty());
}

void Phase03Test::dependencies()
{
    const auto result = CycloneDxParser::parse(demoJson());
    QVERIFY(result.ok());
    QCOMPARE(result.document.dependencies[0].ref, QStringLiteral("demo-app"));
    QCOMPARE(result.document.dependencies[0].dependsOn, (QStringList{QStringLiteral("demo-lib-a"), QStringLiteral("demo-lib-c")}));
    QVERIFY(result.document.dependencies[2].dependsOn.isEmpty());
    auto object = minimalObject();
    object.insert(QStringLiteral("dependencies"), QJsonArray{QJsonObject{},
        QJsonObject{{"ref", "unknown"}, {"dependsOn", QJsonArray{"missing", "missing"}}}});
    const auto unresolved = CycloneDxParser::parse(encode(object));
    QVERIFY(unresolved.ok()); // Reference quality, missing fields and duplicates belong to Phase 04.
    QVERIFY(unresolved.document.dependencies[0].ref.isEmpty());
    QCOMPARE(unresolved.document.dependencies[1].dependsOn.size(), 2);
}

void Phase03Test::invalidJson()
{
    for (const QByteArray& json : {QByteArray(), QByteArray("{"), QByteArray("{\"x\":1,}"), QByteArray("not-json")}) {
        const auto result = CycloneDxParser::parse(json);
        QCOMPARE(result.error, SbomError::InvalidJson);
        QVERIFY(!result.userMessage().isEmpty());
        QVERIFY(result.document.components.isEmpty());
    }
    QCOMPARE(CycloneDxParser::parse("[]").error, SbomError::InvalidStructure);
}

void Phase03Test::invalidHeaders()
{
    QCOMPARE(CycloneDxParser::parse("{}").error, SbomError::NotCycloneDx);
    auto object = minimalObject();
    object["bomFormat"] = "SPDX";
    QCOMPARE(CycloneDxParser::parse(encode(object)).error, SbomError::NotCycloneDx);
    object = minimalObject();
    object.remove(QStringLiteral("specVersion"));
    QCOMPARE(CycloneDxParser::parse(encode(object)).error, SbomError::MissingSpecVersion);
    for (const auto& version : {QString(), QStringLiteral("1.3"), QStringLiteral("1.7"), QStringLiteral("2.0")}) {
        object["specVersion"] = version;
        QCOMPARE(CycloneDxParser::parse(encode(object)).error, SbomError::UnsupportedSpecVersion);
    }
    object["specVersion"] = 1.6;
    QCOMPARE(CycloneDxParser::parse(encode(object)).error, SbomError::InvalidStructure);
}

void Phase03Test::invalidStructures()
{
    const QList<QPair<QString, QJsonValue>> invalid{
        {"components", QJsonObject{}}, {"components", QJsonValue::Null},
        {"components", QJsonArray{1}}, {"components", QJsonArray{QJsonObject{{"name", 7}}}},
        {"components", QJsonArray{QJsonObject{{"version", false}}}},
        {"components", QJsonArray{QJsonObject{{"purl", QJsonArray{}}}}},
        {"components", QJsonArray{QJsonObject{{"bom-ref", 3}}}},
        {"components", QJsonArray{QJsonObject{{"type", QJsonValue::Null}}}},
        {"components", QJsonArray{QJsonObject{{"components", QJsonObject{}}}}},
        {"dependencies", QJsonObject{}}, {"dependencies", QJsonArray{false}},
        {"dependencies", QJsonArray{QJsonObject{{"ref", 8}}}},
        {"dependencies", QJsonArray{QJsonObject{{"dependsOn", "x"}}}},
        {"dependencies", QJsonArray{QJsonObject{{"dependsOn", QJsonArray{5}}}}},
        {"metadata", QJsonArray{}}, {"metadata", QJsonObject{{"timestamp", 1}}},
        {"metadata", QJsonObject{{"component", false}}},
        {"serialNumber", 1}, {"version", "1"}, {"version", 1.5}, {"version", -1}, {"version", 0},
        {"version", 9007199254740992.0}};
    for (const auto& [key, value] : invalid) {
        auto object = minimalObject();
        object.insert(key, value);
        const auto result = CycloneDxParser::parse(encode(object));
        QCOMPARE(result.error, SbomError::InvalidStructure);
        QVERIFY(result.document.bomFormat.isEmpty());
    }
    auto object = minimalObject();
    object["components"] = QJsonArray{QJsonObject{{"name", "parsed-before-error"}}, 42};
    const auto partial = CycloneDxParser::parse(encode(object));
    QVERIFY(!partial.ok());
    QVERIFY(partial.document.components.isEmpty());
}

void Phase03Test::utf8()
{
    auto object = minimalObject();
    const QString text = QString::fromUtf8("虚构组件🙂 <b>demo</b>");
    object["components"] = QJsonArray{QJsonObject{{"name", text}}};
    auto bytes = encode(object);
    const auto result = CycloneDxParser::parse(bytes);
    QVERIFY(result.ok());
    QCOMPARE(result.document.components[0].name, text);
    bytes = "{\"bomFormat\":\"CycloneDX\",\"specVersion\":\"1.6\",\"components\":[{\"name\":\"";
    bytes.append(char(0xff));
    bytes.append("\"}]}");
    QCOMPARE(CycloneDxParser::parse(bytes).error, SbomError::InvalidJson);
}

void Phase03Test::limits()
{
    QByteArray padded = encode(minimalObject());
    padded.append(CycloneDxParser::MaxFileBytes - padded.size(), ' ');
    QVERIFY(CycloneDxParser::parse(padded).ok());
    padded.append(' ');
    QCOMPARE(CycloneDxParser::parse(padded).error, SbomError::FileTooLarge);
    auto object = minimalObject();
    QJsonObject nested{{"name", "demo"}};
    for (int i = 1; i < CycloneDxParser::MaxComponentDepth; ++i)
        nested = QJsonObject{{"components", QJsonArray{nested}}};
    object["components"] = QJsonArray{nested};
    QVERIFY(CycloneDxParser::parse(encode(object)).ok());
    object["components"] = QJsonArray{QJsonObject{{"components", QJsonArray{nested}}}};
    QCOMPARE(CycloneDxParser::parse(encode(object)).error, SbomError::StructureLimitExceeded);
    QJsonArray many;
    for (qsizetype i = 0; i < CycloneDxParser::MaxEntries; ++i) many.append(QJsonObject{});
    object["components"] = many;
    QVERIFY(CycloneDxParser::parse(encode(object)).ok());
    many.append(QJsonObject{});
    object["components"] = many;
    QCOMPARE(CycloneDxParser::parse(encode(object)).error, SbomError::StructureLimitExceeded);
    object.remove(QStringLiteral("components"));
    object["dependencies"] = many;
    QCOMPARE(CycloneDxParser::parse(encode(object)).error, SbomError::StructureLimitExceeded);
    QJsonArray targets;
    for (qsizetype i = 0; i <= CycloneDxParser::MaxEntries; ++i) targets.append("demo");
    object["dependencies"] = QJsonArray{QJsonObject{{"dependsOn", targets}}};
    QCOMPARE(CycloneDxParser::parse(encode(object)).error, SbomError::StructureLimitExceeded);
}

void Phase03Test::fileRead()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const auto file = temporary.filePath(QStringLiteral("虚构 SBOM.json"));
    QCOMPARE(CycloneDxParser::parseFile(file).error, SbomError::FileOpenError);
    QCOMPARE(CycloneDxParser::parseFile(temporary.path()).error, SbomError::FileOpenError);
    QVERIFY(writeFile(file, demoJson()));
    QVERIFY(CycloneDxParser::parseFile(file).ok());
    QFile oversized(temporary.filePath(QStringLiteral("oversized.json")));
    QVERIFY(oversized.open(QIODevice::WriteOnly));
    QVERIFY(oversized.resize(CycloneDxParser::MaxFileBytes + 1));
    oversized.close();
    QCOMPARE(CycloneDxParser::parseFile(oversized.fileName()).error, SbomError::FileTooLarge);
}

void Phase03Test::previewAtomicity()
{
    Context context;
    QVERIFY(context.open());
    const auto file = context.temporary.filePath(QStringLiteral("demo-%5-sbom.json"));
    const auto bad = context.temporary.filePath(QStringLiteral("invalid.json"));
    QVERIFY(writeFile(file, demoJson()));
    QVERIFY(writeFile(bad, "{not-json"));
    SbomImportDialog dialog(QStringLiteral("synthetic-project-id"), QStringLiteral("demo-project"), context.database.filePath(), context.logger);
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    QSignalSpy finished(&dialog, &SbomImportDialog::importFinished);
    auto* summary = dialog.findChild<QTextBrowser*>(QStringLiteral("sbomSummary"));
    auto* table = dialog.findChild<QTableView*>(QStringLiteral("sbomComponents"));
    auto* choose = dialog.findChild<QPushButton*>(QStringLiteral("chooseSbomFile"));
    QVERIFY(summary && table && choose);
    dialog.importFile(file);
    QVERIFY(!choose->isEnabled());
    dialog.importFile(bad); // An in-flight result cannot be replaced by another request.
    QTRY_COMPARE(finished.count(), 1);
    QCOMPARE(finished.at(0).at(0).toBool(), true);
    QVERIFY(choose->isEnabled());
    QCOMPARE(table->model()->rowCount(), 3);
    QCOMPARE(table->model()->data(table->model()->index(0, 0)).toString(), QStringLiteral("demo-lib-a"));
    QCOMPARE(table->editTriggers(), QAbstractItemView::NoEditTriggers);
    QVERIFY(!(table->model()->flags(table->model()->index(0, 0)) & Qt::ItemIsEditable));
    const auto previous = summary->toPlainText();
    QVERIFY(previous.contains(QStringLiteral("demo-%5-sbom.json"))); // Input must not become a QString::arg placeholder.
    QVERIFY(previous.contains(QStringLiteral("BOM version：7")));
    QVERIFY(previous.contains(QStringLiteral("组件数量：3")));
    QVERIFY(previous.contains(QStringLiteral("依赖条目数量：3")));
    QVERIFY(!previous.contains(context.temporary.path()));
    QVERIFY(dialog.grab().save(QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("phase03-preview.png"))));
    dialog.importFile(bad);
    QTRY_COMPARE(finished.count(), 2);
    QCOMPARE(finished.at(1).at(0).toBool(), false);
    QCOMPARE(summary->toPlainText(), previous);
    QCOMPARE(table->model()->rowCount(), 3);
    QVERIFY(dialog.findChild<QLabel*>(QStringLiteral("sbomStatus"))->text().contains(QStringLiteral("有效的 UTF-8 JSON")));
    dialog.importFile(QString()); // Cancel is a no-op.
    QCOMPARE(summary->toPlainText(), previous);
    auto object = minimalObject();
    const QString literal = QStringLiteral("<b>虚构组件</b>");
    object["components"] = QJsonArray{QJsonObject{{"name", literal}}};
    QVERIFY(writeFile(file, encode(object)));
    dialog.importFile(file);
    QTRY_COMPARE(finished.count(), 3);
    QCOMPARE(table->model()->rowCount(), 1);
    QCOMPARE(table->model()->data(table->model()->index(0, 0)).toString(), literal);
    QVERIFY(!table->model()->data(table->model()->index(0, 0), Qt::ToolTipRole).isValid());
    dialog.resize(560, 400);
    QCoreApplication::processEvents();
    QCOMPARE(dialog.size(), QSize(560, 400));
    QVERIFY(dialog.grab().save(QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("phase03-small.png"))));
    QVERIFY(writeFile(file, encode(minimalObject())));
    dialog.importFile(file);
    QTRY_COMPARE(finished.count(), 4);
    QCOMPARE(table->model()->rowCount(), 0);
    QVERIFY(summary->toPlainText().contains(QStringLiteral("组件数量：0")));
    QFile log(context.temporary.filePath(QStringLiteral("test.log")));
    QVERIFY(log.open(QIODevice::ReadOnly));
    const auto bytes = log.readAll();
    QVERIFY(bytes.contains("components=3, dependencies=3"));
    QVERIFY(bytes.contains("SBOM parse failed: InvalidJson"));
    for (const auto& forbidden : {file.toUtf8(), context.temporary.path().toUtf8(), QByteArray("demo-lib-a"),
                                  QByteArray("pkg:generic"), QByteArray("bomFormat"), QByteArray("not-json")})
        QVERIFY(!bytes.contains(forbidden));
    QVERIFY(dialog.close());
}

void Phase03Test::projectIntegration()
{
    // Only this test uses Qt's widget picker: immediate synthetic close of the
    // Windows native dialog races its platform thread. Production keeps native UI.
    const bool previousNativeSetting = QCoreApplication::testAttribute(Qt::AA_DontUseNativeDialogs);
    QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    const auto restoreNativeSetting = qScopeGuard([previousNativeSetting] {
        QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs, previousNativeSetting);
    });
    Context context;
    QVERIFY(context.open());
    MainWindow window(new ProjectPage(context.repository, context.components, context.logger), nullptr, QStringLiteral("projects"));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto* page = window.findChild<ProjectPage*>();
    auto* button = page->findChild<QPushButton*>(QStringLiteral("importSbom"));
    auto* list = page->findChild<QListWidget*>(QStringLiteral("projectList"));
    QVERIFY(button && list);
    QVERIFY(!button->isEnabled());
    Project project;
    QVERIFY(context.repository.create(QStringLiteral("demo-project"), QStringLiteral("synthetic"), project).ok());
    QTest::mouseClick(page->findChild<QPushButton*>(QStringLiteral("refreshProjects")), Qt::LeftButton);
    QVERIFY(!button->isEnabled());
    list->setCurrentRow(0);
    QVERIFY(button->isEnabled());
    const auto changes = scalar(context.database, QStringLiteral("SELECT total_changes()"));
    QTest::mouseClick(button, Qt::LeftButton);
    QPointer<SbomImportDialog> dialog = page->findChild<SbomImportDialog*>();
    QVERIFY(dialog);
    QCOMPARE(dialog->windowModality(), Qt::WindowModal);
    QCOMPARE(dialog->findChild<QLabel*>(QStringLiteral("sbomProject"))->text(), QStringLiteral("当前项目：demo-project"));
    auto* picker = dialog->findChild<QFileDialog*>();
    QVERIFY(picker);
    QCOMPARE(picker->fileMode(), QFileDialog::ExistingFile);
    QVERIFY(picker->nameFilters().join(QString()).contains(QStringLiteral("*.json")));
    const auto file = context.temporary.filePath(QStringLiteral("demo.json"));
    QVERIFY(writeFile(file, demoJson()));
    QSignalSpy finished(dialog, &SbomImportDialog::importFinished);
    picker->selectFile(file);
    QVERIFY(QMetaObject::invokeMethod(picker, "accept", Qt::DirectConnection));
    QTRY_COMPARE(finished.count(), 1);
    QVERIFY(finished.at(0).at(0).toBool());
    QCOMPARE(scalar(context.database, QStringLiteral("SELECT value FROM app_meta WHERE key='schema_version'")).toString(), QStringLiteral("4"));
    auto tables = context.database.connection().tables();
    tables.sort();
    QCOMPARE(tables, (QStringList{QStringLiteral("app_meta"), QStringLiteral("components"), QStringLiteral("dependency_capture"), QStringLiteral("dependency_entries"), QStringLiteral("dependency_targets"), QStringLiteral("projects")}));
    QCOMPARE(scalar(context.database, QStringLiteral("SELECT total_changes()")), changes);
    Project found;
    QVERIFY(context.repository.findById(project.id, found).ok());
    QCOMPARE(found.name, project.name);
    QCOMPARE(found.description, project.description);
    QCOMPARE(found.createdAt, project.createdAt);
    dialog->reject();
    QTRY_VERIFY(dialog.isNull());
    // Reopening starts an empty session; no per-project cache or hidden history.
    QTest::mouseClick(button, Qt::LeftButton);
    dialog = page->findChild<SbomImportDialog*>();
    QVERIFY(dialog);
    dialog->findChild<QFileDialog*>()->reject();
    QCOMPARE(dialog->findChild<QTableView*>(QStringLiteral("sbomComponents"))->model()->rowCount(), 0);
    dialog->reject();
    QTRY_VERIFY(dialog.isNull());
    QVERIFY(context.repository.remove(project.id).ok());
    QTest::mouseClick(button, Qt::LeftButton); // Stale selection must revalidate the project.
    QVERIFY(!page->findChild<SbomImportDialog*>());
    QVERIFY(!button->isEnabled());
    for (const auto& id : {QStringLiteral("overview"), QStringLiteral("settings"), QStringLiteral("projects")})
        QVERIFY(window.selectPage(id));
    QVERIFY(window.close());
    // Only synthetic acceptance inputs; CMake outputs live in ignored build directories.
    const QDir output(QCoreApplication::applicationDirPath());
    QVERIFY(writeFile(output.filePath(QStringLiteral("phase03-demo-sbom.json")), demoJson()));
    QVERIFY(writeFile(output.filePath(QStringLiteral("phase03-not-cyclonedx.json")), "{\"demo\":true}"));
    QVERIFY(writeFile(output.filePath(QStringLiteral("phase03-invalid-json.json")), "{not-json"));
}

void Phase03Test::largePreview()
{
    Context context;
    QVERIFY(context.open());
    auto object = minimalObject();
    QJsonArray components;
    for (int i = 0; i < 10000; ++i)
        components.append(QJsonObject{{"name", QStringLiteral("demo-%1").arg(i)}, {"version", "1.0"}});
    const QString longText(100000, u'文');
    components[0] = QJsonObject{{"name", longText}};
    object["components"] = components;
    object["metadata"] = QJsonObject{{"timestamp", longText}, {"component", QJsonObject{{"name", longText}}}};
    const auto parsed = CycloneDxParser::parse(encode(object));
    QVERIFY(parsed.ok());
    QCOMPARE(parsed.document.components[0].name, longText); // Only the UI truncates; raw values survive parsing.
    const auto file = context.temporary.filePath(QStringLiteral("large.json"));
    QVERIFY(writeFile(file, encode(object)));
    SbomImportDialog dialog(QStringLiteral("synthetic-project-id"), QStringLiteral("demo"), context.database.filePath(), context.logger);
    dialog.show();
    QSignalSpy finished(&dialog, &SbomImportDialog::importFinished);
    dialog.importFile(file);
    QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 10000);
    QVERIFY(finished.at(0).at(0).toBool());
    auto* table = dialog.findChild<QTableView*>(QStringLiteral("sbomComponents"));
    QCOMPARE(table->model()->rowCount(), 10000);
    const auto displayed = table->model()->data(table->model()->index(0, 0)).toString();
    QVERIFY(displayed.size() < 2000);
    QVERIFY(displayed.endsWith(QStringLiteral("…")));
    QVERIFY(dialog.findChild<QTextBrowser*>()->toPlainText().size() < 2000);
    QCOMPARE(table->model()->data(table->model()->index(9999, 0)).toString(), QStringLiteral("demo-9999"));
    QVERIFY(dialog.close());
}

void Phase03Test::closingDuringImport()
{
    Context context;
    QVERIFY(context.open());
    const auto file = context.temporary.filePath(QStringLiteral("demo.json"));
    QVERIFY(writeFile(file, demoJson()));
    QPointer<SbomImportDialog> dialog = new SbomImportDialog(QStringLiteral("synthetic-project-id"), QStringLiteral("demo"), context.database.filePath(), context.logger);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
    dialog->importFile(file);
    dialog->close();
    QTRY_VERIFY(dialog.isNull());
    // Join the actual bounded worker before the fixture removes its input; no timing sleeps.
    QVERIFY(QThreadPool::globalInstance()->waitForDone(5000));
    SbomImportDialog reopened(QStringLiteral("synthetic-project-id"), QStringLiteral("demo"), context.database.filePath(), context.logger);
    QCOMPARE(reopened.findChild<QTableView*>(QStringLiteral("sbomComponents"))->model()->rowCount(), 0);
}

QTEST_MAIN(Phase03Test)
#include "Phase03Test.moc"
