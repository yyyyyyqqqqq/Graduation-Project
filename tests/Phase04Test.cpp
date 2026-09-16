#include "AppDatabase.h"
#include "AppLogger.h"
#include "CycloneDxParser.h"
#include "ProjectRepository.h"
#include "SbomImportDialog.h"
#include "SbomQualityAnalyzer.h"

#include <QDataStream>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPointer>
#include <QSignalSpy>
#include <QSqlQuery>
#include <QTableView>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTextBrowser>
#include <QThreadPool>
#include <tuple>

using Code = QualityIssueCode;
using Severity = QualitySeverity;
using Scope = QualityScope;

namespace
{
// Artificial identifiers only. These same fixtures generate ignored manual inputs.
QJsonObject cleanObject()
{
    return QJsonDocument::fromJson(R"({
        "bomFormat":"CycloneDX", "specVersion":"1.6", "version":1,
        "metadata":{"component":{"bom-ref":"root","type":"application","name":"synthetic-app",
                    "version":"1.0","purl":"pkg:generic/synthetic-app@1.0"}},
        "components":[
            {"bom-ref":"a","type":"library","name":"synthetic-a","version":"1.0","purl":"pkg:generic/synthetic-a@1.0"},
            {"bom-ref":"b","type":"library","name":"synthetic-b","version":"1.0","purl":"pkg:generic/synthetic-b@1.0"},
            {"bom-ref":"c","type":"library","name":"synthetic-c","version":"1.0","purl":"pkg:generic/synthetic-c@1.0"},
            {"bom-ref":"d","type":"library","name":"synthetic-d","version":"1.0","purl":"pkg:generic/synthetic-d@1.0"}
        ],
        "dependencies":[{"ref":"root","dependsOn":["a","b","c","d"]},{"ref":"b","dependsOn":[]}]
    })").object();
}

QJsonObject issuesObject()
{
    auto object = cleanObject();
    auto components = object["components"].toArray();
    auto b = components[1].toObject();
    b.remove(QStringLiteral("version"));
    components[1] = b;
    auto c = components[2].toObject();
    c.remove(QStringLiteral("purl"));
    c["bom-ref"] = "a";
    components[2] = c;
    auto d = components[3].toObject();
    d.remove(QStringLiteral("bom-ref"));
    components[3] = d;
    object["components"] = components;
    object["dependencies"] = QJsonArray{QJsonObject{{"ref", "root"}, {"dependsOn", QJsonArray{"a", "b"}}},
        QJsonObject{{"ref", "b"}, {"dependsOn", QJsonArray{"synthetic-unknown", "b"}}}};
    return object;
}

QByteArray encode(const QJsonObject& object) { return QJsonDocument(object).toJson(); }
SbomDocument cleanDocument() { return CycloneDxParser::parse(encode(cleanObject())).document; }
bool writeFile(const QString& path, const QByteArray& bytes)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
}

QList<Code> codes(const SbomQualityReport& report)
{
    QList<Code> result;
    for (const auto& issue : report.issues()) result.append(issue.code);
    return result;
}

QByteArray documentSnapshot(const SbomDocument& document)
{
    QByteArray bytes;
    QDataStream out(&bytes, QIODevice::WriteOnly);
    out << document.bomFormat << document.specVersion << document.serialNumber
        << document.bomVersion.has_value() << document.bomVersion.value_or(0)
        << document.metadata.timestamp << document.metadata.component.has_value();
    const auto component = [&](const SbomComponent& value) {
        out << value.bomRef << value.type << value.name << value.version << value.purl;
    };
    if (document.metadata.component) component(*document.metadata.component);
    out << document.components.size();
    for (const auto& value : document.components) component(value);
    out << document.dependencies.size();
    for (const auto& value : document.dependencies) out << value.ref << value.dependsOn;
    return bytes;
}

struct Context
{
    QTemporaryDir temporary;
    AppLogger logger;
    AppDatabase database;
    ProjectRepository repository{database, logger};
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

// Includes all rows/columns in both existing tables; content checks complement
// total_changes(), which alone would not detect writes from another connection.
QByteArray databaseSnapshot(AppDatabase& database)
{
    QByteArray bytes;
    QDataStream out(&bytes, QIODevice::WriteOnly);
    for (const auto& sql : {QStringLiteral("SELECT key,value FROM app_meta ORDER BY key"),
                           QStringLiteral("SELECT id,name,description,created_at FROM projects ORDER BY id")}) {
        QSqlQuery query(database.connection());
        if (!query.exec(sql)) return {};
        const int columns = sql.contains(QStringLiteral("app_meta")) ? 2 : 4;
        while (query.next()) {
            for (int i = 0; i < columns; ++i) out << query.value(i);
        }
    }
    return bytes;
}
}

class Phase04Test final : public QObject
{
    Q_OBJECT
private slots:
    void clean();
    void missingFields_data();
    void missingFields();
    void duplicateIdentifiers();
    void emptyDocument();
    void dependencyRules_data();
    void dependencyRules();
    void deterministicReport();
    void documentUnchanged();
    void parseQualitySeparation();
    void qualityUi();
    void largeInput();
    void databaseUnchanged();
};

void Phase04Test::clean()
{
    auto document = cleanDocument();
    // References to metadata root and forward component references are valid.
    document.dependencies.append({QStringLiteral("a"), {QStringLiteral("root"), QStringLiteral("d")}});
    const auto report = SbomQualityAnalyzer::analyze(document);
    QVERIFY(report.issues().isEmpty());
    QCOMPARE(report.count(Severity::Error), 0);
    QCOMPARE(report.count(Severity::Warning), 0);
    QCOMPARE(report.count(Severity::Info), 0);
    QVERIFY(document.serialNumber.isEmpty()); // Optional metadata does not create noise.
    QVERIFY(document.metadata.timestamp.isEmpty());
}

void Phase04Test::missingFields_data()
{
    QTest::addColumn<QString>("field");
    QTest::addColumn<QString>("value");
    QTest::addColumn<bool>("root");
    QTest::addColumn<Code>("code");
    QTest::addColumn<Severity>("severity");
    const QList<std::tuple<QString, Code, Severity>> fields{
        {"name", Code::MissingComponentName, Severity::Error},
        {"version", Code::MissingComponentVersion, Severity::Warning},
        {"purl", Code::MissingComponentPurl, Severity::Warning},
        {"bom-ref", Code::MissingBomRef, Severity::Warning},
        {"type", Code::MissingComponentType, Severity::Warning}};
    for (const auto& [field, code, severity] : fields) {
        for (bool root : {false, true}) {
            for (bool blank : {false, true}) {
                const auto name = QStringLiteral("%1-%2-%3").arg(field).arg(root).arg(blank).toLatin1();
                QTest::newRow(name.constData()) << field << (blank ? QStringLiteral(" \t\n\u3000 ") : QString())
                                              << root << code << severity;
            }
        }
    }
}

void Phase04Test::missingFields()
{
    QFETCH(QString, field);
    QFETCH(QString, value);
    QFETCH(bool, root);
    QFETCH(Code, code);
    QFETCH(Severity, severity);
    auto object = cleanObject();
    object.remove(QStringLiteral("dependencies")); // Test missing field independently of dangling references.
    if (root) {
        auto metadata = object["metadata"].toObject();
        auto component = metadata["component"].toObject();
        component[field] = value;
        metadata["component"] = component;
        object["metadata"] = metadata;
    } else {
        auto components = object["components"].toArray();
        auto component = components[1].toObject();
        component[field] = value;
        components[1] = component;
        object["components"] = components;
    }
    const auto parsed = CycloneDxParser::parse(encode(object));
    QVERIFY(parsed.ok());
    const auto report = SbomQualityAnalyzer::analyze(parsed.document);
    QCOMPARE(report.issues().size(), 1);
    const auto& issue = report.issues().first();
    QCOMPARE(issue.code, code);
    QCOMPARE(issue.severity(), severity);
    QCOMPARE(issue.scope, root ? Scope::MetadataComponent : Scope::Component);
    QCOMPARE(issue.componentIndex, root ? -1 : 1);
    QCOMPARE(issue.dependencyIndex, -1);
    QVERIFY(!issue.message().isEmpty());
    QCOMPARE(report.count(severity), 1);
}

void Phase04Test::duplicateIdentifiers()
{
    auto document = cleanDocument();
    document.dependencies.clear();
    document.components[1].bomRef = document.components[0].bomRef;
    document.components[2].bomRef = document.components[0].bomRef;
    document.components[3].bomRef = document.metadata.component->bomRef;
    document.components[2].purl = document.components[0].purl;
    document.components[3].purl = document.metadata.component->purl;
    auto report = SbomQualityAnalyzer::analyze(document);
    QCOMPARE(codes(report), (QList<Code>{Code::DuplicateBomRef, Code::DuplicateBomRef, Code::DuplicatePurl,
                                       Code::DuplicateBomRef, Code::DuplicatePurl}));
    QCOMPARE(report.count(Severity::Error), 3);
    QCOMPARE(report.count(Severity::Warning), 2);
    QCOMPARE(report.issues()[0].componentIndex, 1);
    document = cleanDocument();
    document.dependencies.clear();
    for (auto& c : document.components) { c.bomRef.clear(); c.purl.clear(); }
    report = SbomQualityAnalyzer::analyze(document);
    QCOMPARE(report.issues().size(), 8);
    QVERIFY(!codes(report).contains(Code::DuplicateBomRef));
    QVERIFY(!codes(report).contains(Code::DuplicatePurl));

    document = cleanDocument();
    document.dependencies.clear();
    // Equal name/version are not identity. Nonblank identifiers are never normalized.
    document.components[1].name = document.components[0].name;
    document.components[1].version = document.components[0].version;
    document.components[1].purl = document.components[0].purl.toUpper();
    document.components[2].purl = u' ' + document.components[0].purl + u' ';
    document.components[1].bomRef = QStringLiteral("A");
    document.components[2].bomRef = QStringLiteral(" a ");
    QVERIFY(SbomQualityAnalyzer::analyze(document).issues().isEmpty());
}

void Phase04Test::emptyDocument()
{
    const auto parsed = CycloneDxParser::parse(R"({"bomFormat":"CycloneDX","specVersion":"1.6"})");
    QVERIFY(parsed.ok());
    auto report = SbomQualityAnalyzer::analyze(parsed.document);
    QCOMPARE(codes(report), (QList<Code>{Code::MissingMetadataComponent, Code::NoComponents}));
    QCOMPARE(report.count(Severity::Info), 1);
    QCOMPARE(report.count(Severity::Warning), 1);
    QCOMPARE(report.count(Severity::Error), 0);
    auto document = cleanDocument();
    document.metadata.component.reset();
    document.dependencies.clear();
    report = SbomQualityAnalyzer::analyze(document);
    QCOMPARE(codes(report), QList<Code>{Code::MissingMetadataComponent});
    document = cleanDocument();
    document.components.clear();
    document.dependencies.clear();
    QCOMPARE(codes(SbomQualityAnalyzer::analyze(document)), QList<Code>{Code::NoComponents});
}

void Phase04Test::dependencyRules_data()
{
    QTest::addColumn<QString>("ref");
    QTest::addColumn<QStringList>("targets");
    QTest::addColumn<QList<Code>>("expected");
    QTest::addColumn<int>("errors");
    QTest::addColumn<int>("warnings");
    QTest::newRow("valid") << QStringLiteral("a") << QStringList{"b", "root"} << QList<Code>{} << 0 << 0;
    QTest::newRow("missing") << QString() << QStringList{"b"} << QList<Code>{Code::MissingDependencyRef} << 1 << 0;
    QTest::newRow("blank") << QStringLiteral(" \t ") << QStringList{"b"} << QList<Code>{Code::MissingDependencyRef} << 1 << 0;
    QTest::newRow("unknown") << QStringLiteral("unknown") << QStringList{"b"} << QList<Code>{Code::UnknownDependencyRef} << 1 << 0;
    QTest::newRow("exact-ref") << QStringLiteral(" a ") << QStringList{"b"} << QList<Code>{Code::UnknownDependencyRef} << 1 << 0;
    QTest::newRow("unknown-target") << QStringLiteral("a") << QStringList{"unknown"} << QList<Code>{Code::UnknownDependsOnRef} << 1 << 0;
    QTest::newRow("empty-targets") << QStringLiteral("a") << QStringList{"", " \t ", ""}
        << QList<Code>{Code::EmptyDependsOnRef, Code::EmptyDependsOnRef, Code::EmptyDependsOnRef} << 3 << 0;
    QTest::newRow("empty-not-self") << QString() << QStringList{""}
        << QList<Code>{Code::MissingDependencyRef, Code::EmptyDependsOnRef} << 2 << 0;
    QTest::newRow("self") << QStringLiteral("a") << QStringList{"a"} << QList<Code>{Code::SelfDependency} << 0 << 1;
    QTest::newRow("duplicate-target") << QStringLiteral("a") << QStringList{"b", "b", "b"}
        << QList<Code>{Code::DuplicateDependsOnTarget, Code::DuplicateDependsOnTarget} << 0 << 2;
    QTest::newRow("duplicate-unknown") << QStringLiteral("a") << QStringList{"unknown", "unknown"}
        << QList<Code>{Code::UnknownDependsOnRef, Code::UnknownDependsOnRef, Code::DuplicateDependsOnTarget} << 2 << 1;
}

void Phase04Test::dependencyRules()
{
    QFETCH(QString, ref);
    QFETCH(QStringList, targets);
    QFETCH(QList<Code>, expected);
    QFETCH(int, errors);
    QFETCH(int, warnings);
    auto document = cleanDocument();
    document.dependencies = {{ref, targets}};
    auto report = SbomQualityAnalyzer::analyze(document);
    QCOMPARE(codes(report), expected);
    QCOMPARE(report.count(Severity::Error), errors);
    QCOMPARE(report.count(Severity::Warning), warnings);
    QCOMPARE(report.count(Severity::Info), 0);
    for (const auto& issue : report.issues()) {
        QCOMPARE(issue.scope, Scope::Dependency);
        QCOMPARE(issue.dependencyIndex, 0);
        QCOMPARE(issue.componentIndex, -1);
    }
    document.dependencies = {{"a", {"b"}}, {"a", {"b"}}, {"a", {}}, {"", {}}, {"", {}}};
    report = SbomQualityAnalyzer::analyze(document);
    QCOMPARE(codes(report), (QList<Code>{Code::DuplicateDependencyRef, Code::DuplicateDependencyRef,
                                       Code::MissingDependencyRef, Code::MissingDependencyRef}));
    QCOMPARE(report.issues()[0].dependencyIndex, 1);
    QCOMPARE(report.issues()[1].dependencyIndex, 2);
    document.dependencies = {{"a", {"b", "b"}}};
    QCOMPARE(SbomQualityAnalyzer::analyze(document).issues().first().targetIndex, 1);
}

void Phase04Test::deterministicReport()
{
    auto document = CycloneDxParser::parse(encode(issuesObject())).document;
    document.metadata.component.reset();
    const auto expectedCodes = QList<Code>{Code::MissingMetadataComponent, Code::MissingComponentVersion,
        Code::MissingComponentPurl, Code::DuplicateBomRef, Code::MissingBomRef,
        Code::UnknownDependencyRef, Code::UnknownDependsOnRef, Code::SelfDependency};
    const auto expectedNames = QStringList{"MissingMetadataComponent", "MissingComponentVersion",
        "MissingComponentPurl", "DuplicateBomRef", "MissingBomRef", "UnknownDependencyRef",
        "UnknownDependsOnRef", "SelfDependency"};
    const auto first = SbomQualityAnalyzer::analyze(document);
    QCOMPARE(codes(first), expectedCodes);
    QCOMPARE(first.count(Severity::Error), 3);
    QCOMPARE(first.count(Severity::Warning), 4);
    QCOMPARE(first.count(Severity::Info), 1);
    QStringList names;
    for (const auto& issue : first.issues()) names.append(issue.codeName());
    QCOMPARE(names, expectedNames);
    for (int i = 0; i < 10; ++i) {
        const auto report = SbomQualityAnalyzer::analyze(document);
        QCOMPARE(report.issues(), first.issues());
    }
}

void Phase04Test::documentUnchanged()
{
    auto object = issuesObject();
    object["serialNumber"] = "synthetic-serial";
    auto metadata = object["metadata"].toObject();
    metadata["timestamp"] = "synthetic-timestamp";
    object["metadata"] = metadata;
    const auto document = CycloneDxParser::parse(encode(object)).document;
    const auto before = documentSnapshot(document);
    const auto report = SbomQualityAnalyzer::analyze(document);
    QCOMPARE(report.issues().size(), 6);
    QCOMPARE(documentSnapshot(document), before);
    QCOMPARE(document.components.size(), 4);
    QVERIFY(document.components[1].version.isEmpty());
    QVERIFY(document.components[2].purl.isEmpty());
    QVERIFY(document.components[3].bomRef.isEmpty());
    QCOMPARE(document.dependencies[1].dependsOn, (QStringList{"synthetic-unknown", "b"}));
}

void Phase04Test::parseQualitySeparation()
{
    for (const auto& version : {QStringLiteral("1.4"), QStringLiteral("1.5"), QStringLiteral("1.6")}) {
        auto object = issuesObject();
        object["specVersion"] = version;
        const auto parsed = CycloneDxParser::parse(encode(object));
        QVERIFY(parsed.ok());
        QCOMPARE(parsed.error, SbomError::None);
        const auto report = SbomQualityAnalyzer::analyze(parsed.document);
        QCOMPARE(report.issues().size(), 6);
        QCOMPARE(report.count(Severity::Error), 2);
        QCOMPARE(report.count(Severity::Warning), 4);
        QVERIFY(codes(report).contains(Code::MissingComponentVersion));
        QVERIFY(codes(report).contains(Code::MissingComponentPurl));
        QCOMPARE(parsed.document.components.size(), 4);
    }
    QCOMPARE(CycloneDxParser::parse("{not-json").error, SbomError::InvalidJson);
    QCOMPARE(CycloneDxParser::parse("{}").error, SbomError::NotCycloneDx);
    auto object = cleanObject();
    object["components"] = QJsonArray{QJsonObject{{"version", 1}}};
    QCOMPARE(CycloneDxParser::parse(encode(object)).error, SbomError::InvalidStructure);
}

void Phase04Test::qualityUi()
{
    Context context;
    QVERIFY(context.open());
    const auto file = context.temporary.filePath(QStringLiteral("synthetic.json"));
    QVERIFY(writeFile(file, encode(issuesObject())));
    SbomImportDialog dialog(QStringLiteral("synthetic-project"), context.logger);
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    auto* components = dialog.findChild<QTableView*>(QStringLiteral("sbomComponents"));
    auto* issues = dialog.findChild<QTableView*>(QStringLiteral("sbomQualityIssues"));
    auto* summary = dialog.findChild<QLabel*>(QStringLiteral("sbomQualitySummary"));
    auto* parsedSummary = dialog.findChild<QTextBrowser*>(QStringLiteral("sbomSummary"));
    auto* tabs = dialog.findChild<QTabWidget*>(QStringLiteral("sbomTabs"));
    QVERIFY(components && issues && summary && parsedSummary && tabs);
    QVERIFY(summary->text().contains(QStringLiteral("尚无")));
    QSignalSpy finished(&dialog, &SbomImportDialog::importFinished);
    dialog.importFile(file);
    QTRY_COMPARE(finished.count(), 1);
    QVERIFY(finished[0][0].toBool()); // Quality errors never become import failures.
    QCOMPARE(components->model()->rowCount(), 4);
    QCOMPARE(issues->model()->rowCount(), 6);
    QCOMPARE(issues->model()->columnCount(), 4);
    QVERIFY(summary->text().contains(QStringLiteral("总问题：6　Error：2　Warning：4　Info：0")));
    QCOMPARE(issues->editTriggers(), QAbstractItemView::NoEditTriggers);
    QVERIFY(!(issues->model()->flags(issues->model()->index(0, 0)) & Qt::ItemIsEditable));
    const QStringList expectedCodes{"MissingComponentVersion", "MissingComponentPurl", "DuplicateBomRef",
                                    "MissingBomRef", "UnknownDependsOnRef", "SelfDependency"};
    const QStringList expectedLocations{"Component #2", "Component #3", "Component #3", "Component #4",
                                        "Dependency #2 / target #1", "Dependency #2 / target #2"};
    for (int row = 0; row < expectedCodes.size(); ++row) {
        const auto* model = issues->model();
        QCOMPARE(model->data(model->index(row, 1)).toString(), expectedCodes[row]);
        QCOMPARE(model->data(model->index(row, 2)).toString(), expectedLocations[row]);
        QVERIFY(!model->data(model->index(row, 3)).toString().isEmpty());
    }
    QCOMPARE(issues->model()->data(issues->model()->index(2, 0)).toString(), QStringLiteral("Error / 错误"));
    tabs->setCurrentIndex(1);
    QCoreApplication::processEvents();
    const QDir output(QCoreApplication::applicationDirPath());
    QVERIFY(dialog.grab().save(output.filePath(QStringLiteral("phase04-issues.png"))));
    dialog.resize(560, 400);
    QCoreApplication::processEvents();
    QCOMPARE(dialog.size(), QSize(560, 400));
    QVERIFY(issues->viewport()->height() > 20);
    QVERIFY(dialog.grab().save(output.filePath(QStringLiteral("phase04-small.png"))));
    const auto oldQuality = summary->text();
    const auto oldSummary = parsedSummary->toPlainText();
    int count = 1;
    for (const QByteArray& bad : {QByteArray("{not-json"), QByteArray("{}")}) {
        QVERIFY(writeFile(file, bad));
        dialog.importFile(file);
        ++count;
        QTRY_COMPARE(finished.count(), count);
        QVERIFY(!finished.last()[0].toBool());
        QCOMPARE(summary->text(), oldQuality);
        QCOMPARE(parsedSummary->toPlainText(), oldSummary);
        QCOMPARE(issues->model()->rowCount(), 6);
        QCOMPARE(components->model()->rowCount(), 4);
    }
    dialog.importFile({});
    QCOMPARE(summary->text(), oldQuality);
    QVERIFY(writeFile(file, encode(cleanObject())));
    dialog.importFile(file);
    QTRY_COMPARE(finished.count(), 4);
    QVERIFY(finished.last()[0].toBool());
    QVERIFY(summary->text().contains(QStringLiteral("当前诊断规则未发现问题。")));
    QVERIFY(summary->text().contains(QStringLiteral("总问题：0　Error：0　Warning：0　Info：0")));
    QCOMPARE(issues->model()->rowCount(), 0);
    QCOMPARE(components->model()->rowCount(), 4);
    dialog.resize(900, 600);
    QCoreApplication::processEvents();
    QVERIFY(dialog.grab().save(output.filePath(QStringLiteral("phase04-clean.png"))));
    QFile log(context.temporary.filePath(QStringLiteral("test.log")));
    QVERIFY(log.open(QIODevice::ReadOnly));
    const auto bytes = log.readAll();
    QVERIFY(bytes.contains("quality analysis completed, errorCount=2, warningCount=4, infoCount=0"));
    for (const auto& forbidden : {QByteArray("synthetic-a"), QByteArray("pkg:generic"), QByteArray("synthetic-unknown"),
                                 QByteArray("MissingComponentVersion"), file.toUtf8()})
        QVERIFY(!bytes.contains(forbidden));
    QVERIFY(dialog.close());
    SbomImportDialog reopened(QStringLiteral("synthetic"), context.logger);
    QVERIFY(reopened.findChild<QLabel*>(QStringLiteral("sbomQualitySummary"))->text().contains(QStringLiteral("尚无")));
    QCOMPARE(reopened.findChild<QTableView*>(QStringLiteral("sbomQualityIssues"))->model()->rowCount(), 0);
    QVERIFY(writeFile(output.filePath(QStringLiteral("phase04-clean-sbom.json")), encode(cleanObject())));
    QVERIFY(writeFile(output.filePath(QStringLiteral("phase04-quality-issues.json")), encode(issuesObject())));
}

void Phase04Test::largeInput()
{
    auto document = cleanDocument();
    document.components.clear();
    document.dependencies.clear();
    constexpr int size = 100000;
    for (int i = 0; i < size; ++i) {
        const auto id = QStringLiteral("synthetic-%1").arg(i);
        document.components.append({id, "library", id, "1.0", QStringLiteral("pkg:generic/%1@1.0").arg(id)});
        document.dependencies.append({id, {QStringLiteral("root")}});
    }
    QElapsedTimer timer;
    timer.start();
    auto report = SbomQualityAnalyzer::analyze(document);
    const auto cleanMs = timer.elapsed();
    QVERIFY(report.issues().isEmpty());
    QVERIFY2(cleanMs < 15000, "100k clean analysis exceeded the generous regression bound");
    for (auto& component : document.components) component = {};
    for (auto& dependency : document.dependencies) dependency = {"unknown", {QString()}};
    timer.restart();
    report = SbomQualityAnalyzer::analyze(document);
    const auto issuesMs = timer.elapsed();
    QCOMPARE(report.issues().size(), 799999);
    QCOMPARE(report.count(Severity::Error), 300000);
    QCOMPARE(report.count(Severity::Warning), 499999);
    QVERIFY2(issuesMs < 15000, "100k issue-heavy analysis exceeded the generous regression bound");
    qInfo() << "100000 components + 100000 entries + 100000 targets; clean ms=" << cleanMs
            << "; 799999 issues ms=" << issuesMs;

    Context context;
    QVERIFY(context.open());
    auto object = cleanObject();
    QJsonArray many;
    for (int i = 0; i < size; ++i) many.append(QJsonObject{});
    object["components"] = many;
    object.remove(QStringLiteral("dependencies"));
    const auto file = context.temporary.filePath(QStringLiteral("large.json"));
    QVERIFY(writeFile(file, encode(object)));
    SbomImportDialog dialog(QStringLiteral("synthetic-large"), context.logger);
    dialog.show();
    QSignalSpy finished(&dialog, &SbomImportDialog::importFinished);
    dialog.importFile(file);
    QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 20000);
    QVERIFY(finished.last()[0].toBool());
    auto* issues = dialog.findChild<QTableView*>(QStringLiteral("sbomQualityIssues"));
    QCOMPARE(issues->model()->rowCount(), 500000);
    QCOMPARE(issues->model()->data(issues->model()->index(499999, 1)).toString(), QStringLiteral("MissingComponentType"));
    QCOMPARE(issues->model()->data(issues->model()->index(499999, 2)).toString(), QStringLiteral("Component #100000"));
    dialog.findChild<QTabWidget*>()->setCurrentIndex(1);
    issues->scrollToBottom();
    QCoreApplication::processEvents();
    QVERIFY(dialog.close());
    QPointer<SbomImportDialog> closing = new SbomImportDialog(QStringLiteral("synthetic"), context.logger);
    closing->setAttribute(Qt::WA_DeleteOnClose);
    closing->show();
    closing->importFile(file);
    closing->close();
    QTRY_VERIFY(closing.isNull());
    QVERIFY(QThreadPool::globalInstance()->waitForDone(20000));
}

void Phase04Test::databaseUnchanged()
{
    Context context;
    QVERIFY(context.open());
    Project project;
    QVERIFY(context.repository.create(QStringLiteral("synthetic-project"), QStringLiteral("synthetic-description"), project).ok());
    const auto changes = scalar(context.database, QStringLiteral("SELECT total_changes()"));
    QVERIFY(changes.isValid());
    const auto before = databaseSnapshot(context.database);
    QVERIFY(!before.isEmpty());
    const auto file = context.temporary.filePath(QStringLiteral("synthetic.json"));
    SbomImportDialog dialog(project.name, context.logger);
    QSignalSpy finished(&dialog, &SbomImportDialog::importFinished);
    int count = 0;
    for (const auto& object : {cleanObject(), issuesObject()}) {
        const auto parsed = CycloneDxParser::parse(encode(object));
        QVERIFY(parsed.ok());
        SbomQualityAnalyzer::analyze(parsed.document);
        QVERIFY(writeFile(file, encode(object)));
        dialog.importFile(file);
        ++count;
        QTRY_COMPARE(finished.count(), count);
        QVERIFY(finished.last()[0].toBool());
        QCOMPARE(databaseSnapshot(context.database), before);
        QCOMPARE(scalar(context.database, QStringLiteral("SELECT total_changes()")), changes);
    }
    QCOMPARE(scalar(context.database, QStringLiteral("SELECT value FROM app_meta WHERE key='schema_version'")).toString(), QStringLiteral("2"));
    auto tables = context.database.connection().tables();
    tables.sort();
    QCOMPARE(tables, (QStringList{"app_meta", "projects"}));
    Project found;
    QVERIFY(context.repository.findById(project.id, found).ok());
    QCOMPARE(found.id, project.id);
    QCOMPARE(found.name, project.name);
    QCOMPARE(found.description, project.description);
    QCOMPARE(found.createdAt, project.createdAt);
}

QTEST_MAIN(Phase04Test)
#include "Phase04Test.moc"
