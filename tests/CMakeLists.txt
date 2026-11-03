
# Unit and integration tests for the FTAD backend.
#
# GoogleTest is fetched at configure time (pinned tag) so the suite
# builds on a machine without a system-wide installation. Configure
# with -DFTAD_BUILD_TESTS=OFF to build the application alone.

include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.16.0
    GIT_SHALLOW    TRUE
)

set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

add_executable(ftad_tests
    test_anomaly_detector.cpp
    test_behavior_analyzer.cpp
    test_csv_import.cpp
    test_detection_pipeline.cpp
    test_detection_settings.cpp
    test_identifiers.cpp
    test_investigation_repository.cpp
    test_risk_scorer.cpp
    test_transaction_repository.cpp
    test_transaction_time.cpp

    support/TestSupport.h
    support/main.cpp
)

target_include_directories(ftad_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(ftad_tests PRIVATE
    FTAD_SAMPLE_CSV="${CMAKE_SOURCE_DIR}/data/samples/test_transactions.csv"
)

target_compile_options(ftad_tests PRIVATE
    -Wall
    -Wextra
)

target_link_libraries(ftad_tests PRIVATE
    ftad_core
    GTest::gtest
)

# DatabaseManager resolves database/schema.sql relative to the running
# binary, so the schema must sit beside the test executable.
add_custom_command(TARGET ftad_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:ftad_tests>/database
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/database/schema.sql
        $<TARGET_FILE_DIR:ftad_tests>/database/schema.sql
    VERBATIM
)

include(GoogleTest)

gtest_discover_tests(ftad_tests
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    DISCOVERY_TIMEOUT 60
)

# Timing benchmark, kept out of CTest because it measures rather than
# asserts. Build explicitly with --target ftad_benchmark.
add_executable(ftad_benchmark
    benchmark.cpp
)

target_compile_options(ftad_benchmark PRIVATE
    -Wall
    -Wextra
)

target_link_libraries(ftad_benchmark PRIVATE
    ftad_core
)

add_custom_command(TARGET ftad_benchmark POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:ftad_benchmark>/database
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/database/schema.sql
        $<TARGET_FILE_DIR:ftad_benchmark>/database/schema.sql
    VERBATIM
)
