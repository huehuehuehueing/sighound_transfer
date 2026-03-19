# tests/CMakeLists.txt
add_executable(unit_tests
    test_rate_planner.cpp
    test_bandwidth.cpp
    test_cli_options.cpp
    test_sigmf_metadata.cpp
    test_raw_writer.cpp
)

target_include_directories(unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${SIGNALHOUND_BB_INCLUDE_DIR}
)

target_link_libraries(unit_tests PRIVATE
    Catch2::Catch2WithMain
    nlohmann_json::nlohmann_json
)

# Link the same source files used by main (hardware-independent ones only)
target_sources(unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/cli/cli_options.cpp
    ${CMAKE_SOURCE_DIR}/src/core/rate_planner.cpp
    ${CMAKE_SOURCE_DIR}/src/core/bandwidth_table.cpp
    ${CMAKE_SOURCE_DIR}/src/io/sigmf_writer.cpp
    ${CMAKE_SOURCE_DIR}/src/io/raw_writer.cpp
    ${CMAKE_SOURCE_DIR}/src/util/progress.cpp
)

# NOTE: include(Catch) works because FetchContent_MakeAvailable(Catch2) in
# the root CMakeLists.txt propagates Catch2's extras/ dir into CMAKE_MODULE_PATH.
# The root's add_subdirectory(tests) must come AFTER FetchContent_MakeAvailable(Catch2).
include(Catch)
catch_discover_tests(unit_tests)
