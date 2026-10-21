cmake_minimum_required(VERSION 3.20)
project(mem_tac_toe_test VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic)

# Dependencies
find_package(yaml-cpp REQUIRED)

include_directories(include)

# Common library source files
set(MEM_TAC_LIB_SOURCES
    src/reference_model.cpp
    src/scenario.cpp
    src/scenario_library.cpp
    src/profile.cpp
    src/pty_session.cpp
    src/session_runner.cpp
    src/report_generator.cpp
    src/cli.cpp
)

add_library(mem_tac_lib STATIC ${MEM_TAC_LIB_SOURCES})
target_link_libraries(mem_tac_lib PUBLIC yaml-cpp util pthread)

# Main CLI executable
add_executable(mem-tac-review src/main.cpp)
target_link_libraries(mem-tac-review PRIVATE mem_tac_lib)

# Testing
enable_testing()

add_executable(test_reference_model tests/unit/test_reference_model.cpp)
target_link_libraries(test_reference_model PRIVATE mem_tac_lib)
add_test(NAME ReferenceModelTest COMMAND test_reference_model)

add_executable(test_scenario_library tests/unit/test_scenario_library.cpp)
target_link_libraries(test_scenario_library PRIVATE mem_tac_lib)
add_test(NAME ScenarioLibraryTest COMMAND test_scenario_library)

add_executable(test_profile tests/unit/test_profile.cpp)
target_link_libraries(test_profile PRIVATE mem_tac_lib)
add_test(NAME ProfileTest COMMAND test_profile)

add_executable(test_runner tests/unit/test_runner.cpp)
target_link_libraries(test_runner PRIVATE mem_tac_lib)
add_test(NAME RunnerTest COMMAND test_runner)

add_executable(test_report_generator tests/unit/test_report_generator.cpp)
target_link_libraries(test_report_generator PRIVATE mem_tac_lib)
add_test(NAME ReportGeneratorTest COMMAND test_report_generator)
