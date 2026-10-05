#include "AppDatabase.h"
#include "AppLogger.h"
#include "ProjectRepository.h"
#include "ComponentRepository.h"
#include "CycloneDxParser.h"
#include "SbomQualityAnalyzer.h"
#include "PackageIdentity.h"
#include <QTemporaryDir>
#include <QTest>

class Phase14Test : public QObject {
    Q_OBJECT
private slots:
    void demoCurrentState();
};

void Phase14Test::demoCurrentState()
{
    // The shipped synthetic demo must survive the real parser/quality/current-state boundary.
    const auto parsed = CycloneDxParser::parseFile(QStringLiteral(DELIVERY_DEMO_FILE));
    QVERIFY(parsed.ok());
    const auto& document = parsed.document;
    QCOMPARE(document.components.size(), 3);
    QCOMPARE(document.dependencies.size(), 4);
    QVERIFY(document.metadata.component.has_value());
    const auto quality = SbomQualityAnalyzer::analyze(document);
    QCOMPARE(quality.issues().size(), 0);
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    AppDatabase database;
    QString error;
    QVERIFY(database.open(temporary.filePath("demo.sqlite"), error));
    AppLogger logger;
    QVERIFY(logger.open(temporary.filePath("demo.log")));
    ProjectRepository projects(database, logger);
    Project project;
    QVERIFY(projects.create("Synthetic defense", "Public package identities; synthetic topology", project).ok());
    ComponentRepository components(database);
    QVERIFY(components.replaceForProject(project.id, document).ok());
    DependencySnapshot snapshot;
    QVERIFY(components.readSnapshot(project.id, snapshot).ok());
    QCOMPARE(snapshot.components.size(), 4); // metadata root is separately persisted
    for (const auto& component : snapshot.components)
        QCOMPARE(PackageIdentity::resolve(component).state, IdentityState::Resolved);
    ProjectOverviewSummary summary;
    QVERIFY(components.readOverviewSummary(project.id, summary).ok());
    QCOMPARE(summary.ordinaryComponentCount, 3);
    QVERIFY(summary.dependencyCaptured);
}

QTEST_GUILESS_MAIN(Phase14Test)
#include "Phase14Test.moc"
