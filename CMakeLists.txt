
cmake_minimum_required(VERSION 3.16)

project(FTAD VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql Charts)

option(FTAD_BUILD_TESTS
    "Build the unit and integration test suite" ON)
option(FTAD_ENABLE_COVERAGE
    "Instrument the code for gcov coverage" OFF)

# The backend is built as a library so that the application and the
# test suite compile it exactly once and share the same flags.
add_library(ftad_core STATIC
    backend/src/DatabaseManager.cpp
    backend/src/TransactionRepository.cpp
    backend/src/CsvImportService.cpp
    backend/src/DetectionSettings.cpp
    backend/src/DetectionSettingsStore.cpp
    backend/src/BehaviorAnalyzer.cpp
    backend/src/RiskScorer.cpp
    backend/src/AnomalyDetector.cpp
    backend/src/DetectionRepository.cpp
    backend/src/DetectionService.cpp
    backend/src/InvestigationRepository.cpp
    backend/src/MlFraudModel.cpp

    backend/include/TransactionTime.h
    backend/include/MlFraudModel.h
    backend/include/DetectionSettingsStore.h
)

target_include_directories(ftad_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/backend/include
)

target_compile_options(ftad_core PRIVATE
    -Wall
    -Wextra
)

target_link_libraries(ftad_core PUBLIC
    Qt6::Core
    Qt6::Sql
)

if (FTAD_ENABLE_COVERAGE)
    target_compile_options(ftad_core PUBLIC --coverage -O0 -g)
    target_link_options(ftad_core PUBLIC --coverage)
endif()

add_executable(FTAD
    main.cpp

    frontend/src/MainWindow.cpp
    frontend/src/HomePage.cpp
    frontend/src/BasePage.cpp
    frontend/src/TransactionsPage.cpp
    frontend/src/DetectionPage.cpp
    frontend/src/InvestigationsPage.cpp
    frontend/src/ReportsPage.cpp
    frontend/src/SettingsPage.cpp

    frontend/include/MainWindow.h
    frontend/include/HomePage.h
    frontend/include/BasePage.h
    frontend/include/TransactionsPage.h
    frontend/include/DetectionPage.h
    frontend/include/InvestigationsPage.h
    frontend/include/ReportsPage.h
    frontend/include/SettingsPage.h
)

target_include_directories(FTAD PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/frontend/include
)

target_compile_options(FTAD PRIVATE
    -Wall
    -Wextra
)

target_link_libraries(FTAD PRIVATE
    ftad_core
    Qt6::Widgets
    Qt6::Charts
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/database/schema.sql
    ${CMAKE_CURRENT_BINARY_DIR}/database/schema.sql
    COPYONLY
)

if (FTAD_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
