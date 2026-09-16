cmake_minimum_required(VERSION 3.30)

project(SupplyChainRiskAssessment VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 6.11.2 EXACT REQUIRED COMPONENTS Widgets)

add_executable(SupplyChainRiskAssessment WIN32
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
)
target_link_libraries(SupplyChainRiskAssessment PRIVATE Qt6::Widgets)

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

    foreach(test_case IN ITEMS qtCppSmoke sqliteReadWrite graphvizSvg)
        add_test(NAME Phase00.${test_case} COMMAND Phase00SmokeTest ${test_case})
        set_tests_properties(Phase00.${test_case} PROPERTIES TIMEOUT 40)
        if(WIN32)
            # CTest can find the existing Qt/MinGW DLLs without a deployment package
            # or any changes to the user's system PATH.
            get_filename_component(compiler_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)
            set_tests_properties(Phase00.${test_case} PROPERTIES
                ENVIRONMENT_MODIFICATION
                    "PATH=path_list_prepend:${compiler_bin};PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
            )
        endif()
    endforeach()
endif()

message(STATUS "Phase 00 compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} (${CMAKE_CXX_COMPILER})")
message(STATUS "Phase 00 Qt: ${Qt6_VERSION} (${Qt6_DIR})")
