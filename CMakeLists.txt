cmake_minimum_required(VERSION 3.30)

project(SupplyChainRiskAssessment VERSION 0.6.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 6.11.2 EXACT REQUIRED COMPONENTS Widgets Sql Concurrent)

# Parsing and quality analysis remain independent of Widgets, SQL and application services.
add_library(SbomParsing STATIC src/SbomDocument.h src/CycloneDxParser.h src/CycloneDxParser.cpp
    src/SbomQualityAnalyzer.h src/SbomQualityAnalyzer.cpp)
target_include_directories(SbomParsing PUBLIC src)
target_link_libraries(SbomParsing PUBLIC Qt6::Core)

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

add_executable(SupplyChainRiskAssessment WIN32
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/ProjectPage.h src/ProjectPage.cpp
    src/SbomImportDialog.h src/SbomImportDialog.cpp
)
target_link_libraries(SupplyChainRiskAssessment PRIVATE Qt6::Widgets Qt6::Concurrent AppFoundation SbomParsing)

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
        src/SbomImportDialog.h src/SbomImportDialog.cpp
    )
    target_link_libraries(Phase02Test PRIVATE Qt6::Widgets Qt6::Test Qt6::Concurrent AppFoundation SbomParsing)

    add_executable(Phase03Test
        tests/Phase03Test.cpp
        src/MainWindow.h src/MainWindow.cpp
        src/ProjectPage.h src/ProjectPage.cpp
        src/SbomImportDialog.h src/SbomImportDialog.cpp
    )
    target_link_libraries(Phase03Test PRIVATE Qt6::Widgets Qt6::Test Qt6::Concurrent AppFoundation SbomParsing)

    add_executable(Phase04Test
        tests/Phase04Test.cpp
        src/SbomImportDialog.h src/SbomImportDialog.cpp
    )
    target_link_libraries(Phase04Test PRIVATE Qt6::Widgets Qt6::Test Qt6::Concurrent AppFoundation SbomParsing)

    add_executable(Phase05Test tests/Phase05Test.cpp
        src/MainWindow.h src/MainWindow.cpp
        src/ProjectPage.h src/ProjectPage.cpp
        src/SbomImportDialog.h src/SbomImportDialog.cpp)
    target_link_libraries(Phase05Test PRIVATE Qt6::Widgets Qt6::Test Qt6::Concurrent AppFoundation SbomParsing)

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
endif()

message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} (${CMAKE_CXX_COMPILER})")
message(STATUS "Qt: ${Qt6_VERSION} (${Qt6_DIR})")
