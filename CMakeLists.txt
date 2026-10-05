cmake_minimum_required(VERSION 3.30)

project(SupplyChainRiskAssessment VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 6.11.2 EXACT REQUIRED COMPONENTS Widgets Sql Concurrent Network)

# Parsing and quality analysis remain independent of Widgets, SQL and application services.
add_library(SbomParsing STATIC src/SbomDocument.h src/CycloneDxParser.h src/CycloneDxParser.cpp
    src/SbomQualityAnalyzer.h src/SbomQualityAnalyzer.cpp)
target_include_directories(SbomParsing PUBLIC src)
target_link_libraries(SbomParsing PUBLIC Qt6::Core)

add_library(DependencyAnalysis STATIC src/DependencySnapshot.h src/DependencyAnalyzer.h src/DependencyAnalyzer.cpp)
target_include_directories(DependencyAnalysis PUBLIC src)
target_link_libraries(DependencyAnalysis PUBLIC Qt6::Core)

# Shared by the application and its tests; components have current responsibilities.
add_library(AppFoundation STATIC
    src/AppPaths.h src/AppPaths.cpp
    src/AppSettings.h src/AppSettings.cpp
    src/AppLogger.h src/AppLogger.cpp
    src/AppDatabase.h src/AppDatabase.cpp
    src/Project.h
    src/ProjectRepository.h src/ProjectRepository.cpp
    src/Component.h src/ComponentRepository.h src/ComponentRepository.cpp
)
target_include_directories(AppFoundation PUBLIC src)
target_link_libraries(AppFoundation PUBLIC Qt6::Core Qt6::Sql)

# Identity, response interpretation and cache format are Core-only.
add_library(VulnerabilityCore STATIC src/CveIdentity.h src/CveIdentity.cpp src/PackageIdentity.h src/PackageIdentity.cpp
    src/OsvResponseParser.h src/OsvResponseParser.cpp src/OsvCache.h src/OsvCache.cpp
    src/Semver.h src/Semver.cpp src/VersionApplicability.h src/VersionApplicability.cpp)
target_include_directories(VulnerabilityCore PUBLIC src)
target_link_libraries(VulnerabilityCore PUBLIC Qt6::Core)
add_library(RiskEvidenceCore STATIC src/RiskEvidence.h src/RiskEvidence.cpp src/RiskPriority.h src/RiskPriority.cpp)
target_link_libraries(RiskEvidenceCore PUBLIC VulnerabilityCore DependencyAnalysis)
add_library(PresentationCore STATIC src/PresentationText.h src/PresentationText.cpp)
target_link_libraries(PresentationCore PUBLIC RiskEvidenceCore Qt6::Core)
# Every mapped enum must remain exhaustive when a domain enum grows.
if(MSVC)
    target_compile_options(PresentationCore PRIVATE /we4061 /we4062)
else()
    target_compile_options(PresentationCore PRIVATE -Werror=switch-enum -Werror=switch)
endif()
add_library(PresentationWidgets STATIC src/RiskPresentationView.h src/RiskPresentationView.cpp)
target_link_libraries(PresentationWidgets PUBLIC PresentationCore Qt6::Widgets)
add_library(FindingReportCore STATIC src/FindingReport.h src/FindingReport.cpp)
target_link_libraries(FindingReportCore PUBLIC PresentationCore)
add_library(ValidationCore STATIC src/ValidationDataset.h src/ValidationDataset.cpp
    src/ValidationSelection.h src/ValidationSelection.cpp
    src/ValidationExperiment.h src/ValidationExperiment.cpp src/ValidationSynthetic.cpp)
target_link_libraries(ValidationCore PUBLIC RiskEvidenceCore Qt6::Core)
execute_process(COMMAND git rev-parse HEAD WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE validation_source_commit OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND git status --porcelain WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE validation_source_status OUTPUT_STRIP_TRAILING_WHITESPACE)
set(validation_source_dirty 0)
if(validation_source_status)
    set(validation_source_dirty 1)
endif()
if(NOT validation_source_commit)
    set(validation_source_commit "unknown")
endif()
# Bind an uncommitted implementation to its actual source, in addition to its base commit.
file(GLOB validation_source_files CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "src/*.h" "src/*.cpp" "src/*.in" "tools/*.cpp" "tools/*.py" "tests/*.cpp")
list(APPEND validation_source_files CMakeLists.txt)
list(SORT validation_source_files)
set(validation_source_fingerprints "")
foreach(source_file IN LISTS validation_source_files)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${source_file}")
    file(SHA256 "${CMAKE_CURRENT_SOURCE_DIR}/${source_file}" source_file_hash)
    string(APPEND validation_source_fingerprints "${source_file}:${source_file_hash}\n")
endforeach()
string(SHA256 validation_source_hash "${validation_source_fingerprints}")
configure_file(src/ValidationBuildInfo.h.in generated/ValidationBuildInfo.h @ONLY)
target_include_directories(ValidationCore PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
add_library(ValidationWidgets STATIC src/ValidationPage.h src/ValidationPage.cpp)
target_link_libraries(ValidationWidgets PUBLIC ValidationCore PresentationWidgets Qt6::Widgets Qt6::Concurrent)
add_executable(ValidationReplay tools/ValidationReplay.cpp)
target_link_libraries(ValidationReplay PRIVATE ValidationCore)

# Explicit deployment support, never a product page or alternative analysis path.
option(BUILD_DELIVERY_SUPPORT "Build the standalone deployment verification probe" OFF)
if(BUILD_DELIVERY_SUPPORT)
    add_executable(DeploymentProbe tools/DeploymentProbe.cpp)
    target_link_libraries(DeploymentProbe PRIVATE AppFoundation Qt6::Network)
    target_include_directories(DeploymentProbe PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
endif()

# The product carries only the final Dataset. Tests retain the full research audit.
function(add_validation_resources target full_audit)
    set(validation_files validation/phase11-validation-dataset-v1.json
        validation/phase11-validation-dataset-v1.json.sha256)
    if(full_audit)
        list(APPEND validation_files validation/candidate-pool-manifest-v1.json
            validation/candidate-pool-manifest-v1.json.sha256
            validation/candidate-selection-index-v1.json validation/candidate-selection-index-v1.json.sha256)
    endif()
    qt_add_resources(${target} "validation_files_${target}" BIG_RESOURCES PREFIX "/validation" BASE "validation"
        FILES ${validation_files})
endfunction()

add_library(RiskEvidenceServices STATIC src/RiskEvidenceCache.h src/RiskEvidenceCache.cpp
    src/EvidenceClients.h src/EvidenceClients.cpp src/RiskEvidenceController.h src/RiskEvidenceController.cpp)
target_link_libraries(RiskEvidenceServices PUBLIC RiskEvidenceCore AppFoundation Qt6::Network Qt6::Concurrent)
add_library(OsvMatching STATIC src/OsvClient.h src/OsvClient.cpp
    src/VulnerabilityController.h src/VulnerabilityController.cpp)
target_include_directories(OsvMatching PUBLIC src)
target_link_libraries(OsvMatching PUBLIC VulnerabilityCore RiskEvidenceServices AppFoundation Qt6::Network Qt6::Concurrent)

add_executable(SupplyChainRiskAssessment WIN32
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/OverviewPage.h src/OverviewPage.cpp
    src/ProjectPage.h src/ProjectPage.cpp
    src/DependencyPage.h src/DependencyPage.cpp
    src/VulnerabilityPage.h src/VulnerabilityPage.cpp
    src/SbomImportDialog.h src/SbomImportDialog.cpp
)
target_link_libraries(SupplyChainRiskAssessment PRIVATE Qt6::Widgets Qt6::Concurrent AppFoundation SbomParsing DependencyAnalysis OsvMatching)
target_link_libraries(SupplyChainRiskAssessment PRIVATE ValidationWidgets PresentationWidgets)
target_link_libraries(SupplyChainRiskAssessment PRIVATE FindingReportCore)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/validation/phase11-validation-dataset-v1.json")
    add_validation_resources(SupplyChainRiskAssessment FALSE)
endif()
target_compile_definitions(SupplyChainRiskAssessment PRIVATE APPLICATION_VERSION="${PROJECT_VERSION}")

# Research preparation is opt-in and has no product GUI entry point.
option(BUILD_VALIDATION_PREPARATION "Build the explicit Phase 11 public fixture preparation utility" OFF)
if(BUILD_VALIDATION_PREPARATION)
    add_executable(ValidationPrepare tools/ValidationPrepare.cpp)
    target_link_libraries(ValidationPrepare PRIVATE ValidationCore RiskEvidenceServices)
endif()

include(CTest)
if(BUILD_TESTING)
    enable_testing()
    find_package(Qt6 6.11.2 EXACT REQUIRED COMPONENTS Test Sql)
    find_program(GRAPHVIZ_DOT_EXECUTABLE NAMES dot
        HINTS "C:/Program Files/Graphviz/bin"
        DOC "Graphviz dot executable used by the Phase 00 smoke test"
        REQUIRED
    )

    add_executable(Phase00SmokeTest
        tests/Phase00SmokeTest.cpp
        src/MainWindow.h
        src/MainWindow.cpp
    )
    target_include_directories(Phase00SmokeTest PRIVATE src)
    target_link_libraries(Phase00SmokeTest PRIVATE Qt6::Widgets Qt6::Test Qt6::Sql)
    target_compile_definitions(Phase00SmokeTest PRIVATE
        GRAPHVIZ_DOT_EXECUTABLE="${GRAPHVIZ_DOT_EXECUTABLE}"
    )

    add_executable(Phase01Test
        tests/Phase01Test.cpp
        src/MainWindow.h src/MainWindow.cpp
    )
    target_link_libraries(Phase01Test PRIVATE Qt6::Widgets Qt6::Test AppFoundation)
    target_compile_definitions(Phase01Test PRIVATE
        APPLICATION_EXECUTABLE="$<TARGET_FILE:SupplyChainRiskAssessment>"
    )
    add_dependencies(Phase01Test SupplyChainRiskAssessment)

    add_executable(Phase02Test
        tests/Phase02Test.cpp
        src/MainWindow.h src/MainWindow.cpp
        src/ProjectPage.h src/ProjectPage.cpp
        src/DependencyPage.h src/DependencyPage.cpp
        src/VulnerabilityPage.h src/VulnerabilityPage.cpp
        src/SbomImportDialog.h src/SbomImportDialog.cpp
    )
    target_link_libraries(Phase02Test PRIVATE Qt6::Widgets Qt6::Test Qt6::Concurrent AppFoundation SbomParsing DependencyAnalysis OsvMatching)

    add_executable(Phase03Test
        tests/Phase03Test.cpp
        src/MainWindow.h src/MainWindow.cpp
        src/ProjectPage.h src/ProjectPage.cpp
        src/DependencyPage.h src/DependencyPage.cpp
        src/VulnerabilityPage.h src/VulnerabilityPage.cpp
        src/SbomImportDialog.h src/SbomImportDialog.cpp
    )
    target_link_libraries(Phase03Test PRIVATE Qt6::Widgets Qt6::Test Qt6::Concurrent AppFoundation SbomParsing DependencyAnalysis OsvMatching)

    add_executable(Phase04Test
        tests/Phase04Test.cpp
        src/SbomImportDialog.h src/SbomImportDialog.cpp
    )
    target_link_libraries(Phase04Test PRIVATE Qt6::Widgets Qt6::Test Qt6::Concurrent AppFoundation SbomParsing)

    add_executable(Phase05Test tests/Phase05Test.cpp
        src/MainWindow.h src/MainWindow.cpp
        src/ProjectPage.h src/ProjectPage.cpp
        src/DependencyPage.h src/DependencyPage.cpp
        src/VulnerabilityPage.h src/VulnerabilityPage.cpp
        src/SbomImportDialog.h src/SbomImportDialog.cpp)
    target_link_libraries(Phase05Test PRIVATE Qt6::Widgets Qt6::Test Qt6::Concurrent AppFoundation SbomParsing DependencyAnalysis OsvMatching)

    add_executable(Phase06Test tests/Phase06Test.cpp
        src/MainWindow.h src/MainWindow.cpp
        src/ProjectPage.h src/ProjectPage.cpp
        src/DependencyPage.h src/DependencyPage.cpp
        src/VulnerabilityPage.h src/VulnerabilityPage.cpp
        src/SbomImportDialog.h src/SbomImportDialog.cpp)
    target_link_libraries(Phase06Test PRIVATE Qt6::Widgets Qt6::Test Qt6::Concurrent AppFoundation SbomParsing DependencyAnalysis OsvMatching)

    add_executable(Phase07Test tests/Phase07Test.cpp
        src/ProjectPage.h src/ProjectPage.cpp src/DependencyPage.h src/DependencyPage.cpp
        src/VulnerabilityPage.h src/VulnerabilityPage.cpp src/SbomImportDialog.h src/SbomImportDialog.cpp)
    target_link_libraries(Phase07Test PRIVATE Qt6::Widgets Qt6::Test OsvMatching SbomParsing DependencyAnalysis)

    add_executable(Phase08Test tests/Phase08Test.cpp
        src/ProjectPage.h src/ProjectPage.cpp src/DependencyPage.h src/DependencyPage.cpp
        src/VulnerabilityPage.h src/VulnerabilityPage.cpp src/SbomImportDialog.h src/SbomImportDialog.cpp)
    target_link_libraries(Phase08Test PRIVATE Qt6::Widgets Qt6::Test OsvMatching SbomParsing DependencyAnalysis)
    get_filename_component(phase08_compiler_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)
    target_compile_definitions(Phase08Test PRIVATE PHASE08_COMPILER_BIN="${phase08_compiler_bin}")

    add_executable(Phase09Test tests/Phase09Test.cpp src/VulnerabilityPage.h src/VulnerabilityPage.cpp
        src/ProjectPage.h src/ProjectPage.cpp src/DependencyPage.h src/DependencyPage.cpp
        src/SbomImportDialog.h src/SbomImportDialog.cpp)
    target_link_libraries(Phase09Test PRIVATE Qt6::Widgets Qt6::Test OsvMatching SbomParsing DependencyAnalysis)
    target_compile_definitions(Phase09Test PRIVATE PHASE09_COMPILER_BIN="${phase08_compiler_bin}")

    add_executable(Phase10Test tests/Phase10Test.cpp src/VulnerabilityPage.h src/VulnerabilityPage.cpp)
    target_link_libraries(Phase10Test PRIVATE Qt6::Widgets Qt6::Test OsvMatching)
    target_compile_definitions(Phase10Test PRIVATE PHASE10_COMPILER_BIN="${phase08_compiler_bin}")

    add_executable(Phase11Test tests/Phase11Test.cpp src/MainWindow.h src/MainWindow.cpp)
    target_link_libraries(Phase11Test PRIVATE ValidationWidgets Qt6::Test)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/validation/phase11-validation-dataset-v1.json")
        add_validation_resources(Phase11Test TRUE)
    endif()

    add_executable(Phase12Test tests/Phase12Test.cpp
        src/MainWindow.h src/MainWindow.cpp src/OverviewPage.h src/OverviewPage.cpp
        src/ProjectPage.h src/ProjectPage.cpp src/DependencyPage.h src/DependencyPage.cpp
        src/VulnerabilityPage.h src/VulnerabilityPage.cpp src/SbomImportDialog.h src/SbomImportDialog.cpp)
    target_link_libraries(Phase12Test PRIVATE ValidationWidgets Qt6::Test OsvMatching SbomParsing)
    add_validation_resources(Phase12Test FALSE)
    target_compile_definitions(Phase12Test PRIVATE PHASE12_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

    add_executable(Phase13Test tests/Phase13Test.cpp
        src/ProjectPage.h src/ProjectPage.cpp src/DependencyPage.h src/DependencyPage.cpp
        src/VulnerabilityPage.h src/VulnerabilityPage.cpp src/SbomImportDialog.h src/SbomImportDialog.cpp)
    target_link_libraries(Phase13Test PRIVATE FindingReportCore PresentationWidgets Qt6::Test OsvMatching SbomParsing)
    target_compile_definitions(Phase13Test PRIVATE PHASE13_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

    foreach(target IN ITEMS Phase02Test Phase03Test Phase05Test Phase06Test Phase07Test Phase08Test Phase09Test Phase12Test Phase13Test)
        target_link_libraries(${target} PRIVATE FindingReportCore)
        target_compile_definitions(${target} PRIVATE APPLICATION_VERSION="${PROJECT_VERSION}")
    endforeach()

    foreach(target IN ITEMS Phase00SmokeTest Phase01Test Phase02Test Phase03Test Phase05Test Phase06Test Phase11Test Phase12Test)
        target_link_libraries(${target} PRIVATE AppFoundation PresentationCore)
        target_compile_definitions(${target} PRIVATE APPLICATION_VERSION="${PROJECT_VERSION}")
    endforeach()
    foreach(target IN ITEMS Phase02Test Phase03Test Phase05Test Phase06Test Phase07Test Phase08Test Phase09Test Phase10Test Phase12Test)
        target_link_libraries(${target} PRIVATE PresentationWidgets)
    endforeach()

    function(add_qt_test phase target test_case)
        # File output retains Qt Test diagnostics even when Windows stdout is unavailable.
        add_test(NAME ${phase}.${test_case} COMMAND ${target} ${test_case}
            -o "${CMAKE_CURRENT_BINARY_DIR}/${phase}.${test_case}.txt,txt")
        set_tests_properties(${phase}.${test_case} PROPERTIES TIMEOUT 40)
        if(WIN32)
            # CTest can find the existing Qt/MinGW DLLs without a deployment package
            # or any changes to the user's system PATH.
            get_filename_component(compiler_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)
            set_tests_properties(${phase}.${test_case} PROPERTIES
                ENVIRONMENT_MODIFICATION
                    "PATH=path_list_prepend:${compiler_bin};PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
            )
        endif()
    endfunction()
    add_executable(Phase14Test tests/Phase14Test.cpp)
    target_link_libraries(Phase14Test PRIVATE AppFoundation SbomParsing DependencyAnalysis VulnerabilityCore Qt6::Test)
    target_compile_definitions(Phase14Test PRIVATE DELIVERY_DEMO_FILE="${CMAKE_CURRENT_SOURCE_DIR}/demo/defense-sbom.json")
    add_qt_test(Phase14 Phase14Test demoCurrentState)
    foreach(test_case IN ITEMS semantics presentationConsistency multiCve noCve freshness htmlSafety privacy
            staticTime provenance candidateCapture eligibility retainedRisk ownership projectFailure
            saveCancel saveSnapshot singleFlight workerLifetime fileSafety noSideEffects resolvedPath scale architecture)
        add_qt_test(Phase13 Phase13Test ${test_case})
    endforeach()
    foreach(test_case IN ITEMS vocabulary exactThreshold forbiddenSemantics acquisition kevNotQueryable
            freshnessDrift statusFreshness cveJoin noCve missingAssociation multiCvePartial
            selectionOwnership initialSynchronization projectSwitch createDelete successfulApply
            overviewSummary largeSummary summaryError overviewNavigation settingsInfo validationVocabulary
            uiLifetime markupSafety structuralContract vulnerabilityPresentation)
        add_qt_test(Phase12 Phase12Test ${test_case})
    endforeach()
    foreach(test_case IN ITEMS sharedEvaluator synthetic explanation profileCodec contractFailures
            hashContract timeContract provenanceContract duplicateSamples coverageGate aliasShared aliasPrimary
            aliasTransitive aliasPackageScope aliasRelations aliasOrder exactVersions productionVersionValidity frozenIdentityCompatibility representativeDeterminism
            noAliasEnrichment duplicateAdvisory strataFailures orderingContract frozenDataset sensitivity replay exportArtifact ui uiRejected uiLifetime
            captureAttempts failureAuditConsistency failedCandidateMetadata successCandidateMetadata
            coverageN29 coverageMissingS4 coverageMissingS5 runtimeLoader runtimeContract)
        add_qt_test(Phase11 Phase11Test ${test_case})
        # Contract mutation cases repeatedly validate the complete public construction audit.
        set_tests_properties(Phase11.${test_case} PROPERTIES TIMEOUT 180)
    endforeach()
    foreach(test_case IN ITEMS priorityRules unavailableEvidence freshnessBoundaries temporalInvalidity
            staleFallback naturalExpiry corroborationExpiry multiCveDeterminism supportCoverage provenance
            reasonsAndExplanation contextIsolation defensiveInputs timerExpiry timerCorroboration refreshOverlap
            publication timerInvalidation staleTimerGeneration lateTimer destruction ui scale manualFixtures)
        add_qt_test(Phase10 Phase10Test ${test_case})
    endforeach()
    foreach(test_case IN ITEMS ownership requestLookupScale kevCveBoundaries severityScopes severityValidation epssChunks epssEnvelope epssRows
            epssCache kevValidation kevCache dependencyStates transport privacy cacheOnly consent liveAtomic
            partialChunks fallback cancellation lifecycle staleDependency destruction ui scale manualFixtures providerFailures projectApply)
        add_qt_test(Phase09 Phase09Test ${test_case})
    endforeach()
    if(WIN32)
        add_qt_test(Phase09 Phase09Test epssCacheTransientRename)
        add_qt_test(Phase09 Phase09Test epssCachePermanentRenameDenial)
    endif()
    foreach(test_case IN ITEMS qtCppSmoke sqliteReadWrite graphvizSvg)
        add_qt_test(Phase00 Phase00SmokeTest ${test_case})
    endforeach()
    foreach(test_case IN ITEMS paths settings database databaseRejectsUnknownSchema logging navigation applicationStartup)
        add_qt_test(Phase01 Phase01Test ${test_case})
    endforeach()
    foreach(test_case IN ITEMS freshDatabase migrateV1 migrationRollback migrationConflict createAndFind
            validation stableOrder persistenceAndDelete databaseErrors projectUi uiValidation uiErrors)
        add_qt_test(Phase02 Phase02Test ${test_case})
    endforeach()
    foreach(test_case IN ITEMS minimal supportedVersions components metadata dependencies invalidJson
            invalidHeaders invalidStructures utf8 limits fileRead previewAtomicity projectIntegration
            largePreview closingDuringImport)
        add_qt_test(Phase03 Phase03Test ${test_case})
    endforeach()
    foreach(test_case IN ITEMS clean missingFields duplicateIdentifiers emptyDocument dependencyRules
            deterministicReport documentUnchanged parseQualitySeparation qualityUi largeInput databaseUnchanged)
        add_qt_test(Phase04 Phase04Test ${test_case})
    endforeach()
    foreach(test_case IN ITEMS schema migrateV2 migrationFailures foreignKeys sourcePreservation replacement rollback
            repositoryErrors previewOnly explicitApply parserFailures applyFailureRetry projectUi largeReplacement largeApply workerLifetime)
        add_qt_test(Phase05 Phase05Test ${test_case})
    endforeach()
    foreach(test_case IN ITEMS schema migration migrationFailures captureRaw atomicReplace resolution graphTraversal metricsSelf consistentSnapshot largeData
            previewApply dependencyUi graphReuse staleResults projectIntegration largeUi workerLifetime manualFixtures)
        add_qt_test(Phase06 Phase06Test ${test_case})
    endforeach()
    # Offline only. liveSmoke is deliberately invoked separately, never by CTest.
    foreach(test_case IN ITEMS identityRules identityInvalid versions parser parserInvalid merge
            networkErrors pagination paginationFailure limits cancellation requestPrivacy
            cache cacheInvalid cacheAtomicClear controllerStates cacheRefresh staleResults
            clearLifecycle workerLifecycle hiddenProjectSwitch destruction ui projectIntegration largeInput manualFixtures)
        add_qt_test(Phase07 Phase07Test ${test_case})
    endforeach()
    foreach(test_case IN ITEMS semver exactVersions semverBoundaries invalidEvents eventOrdering limits
            packageMatching wildcard aggregation providerGate withdrawn findings cacheAndLive lifecycle
            cancellation destruction projectApply largeSnapshot ui manualFixtures)
        add_qt_test(Phase08 Phase08Test ${test_case})
    endforeach()
endif()

message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} (${CMAKE_CXX_COMPILER})")
message(STATUS "Qt: ${Qt6_VERSION} (${Qt6_DIR})")
