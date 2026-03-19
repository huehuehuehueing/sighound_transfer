# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(sighound_transfer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# SDK discovery
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(SignalHoundBB REQUIRED)

# Dependencies via FetchContent
include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.2
)
FetchContent_MakeAvailable(Catch2)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# Main executable
add_executable(sighound_transfer
    src/main.cpp
    src/cli/cli_options.cpp
    src/core/rate_planner.cpp
    src/core/bandwidth_table.cpp
    src/core/capture_engine.cpp
    src/bb/bb_device.cpp
    src/io/raw_writer.cpp
    src/io/sigmf_writer.cpp
    src/util/signal_handler.cpp
    src/util/progress.cpp
)

target_include_directories(sighound_transfer PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${SIGNALHOUND_BB_INCLUDE_DIR}
)

target_link_libraries(sighound_transfer PRIVATE
    ${SIGNALHOUND_BB_LIBRARY}
    nlohmann_json::nlohmann_json
)

if(UNIX)
    target_link_libraries(sighound_transfer PRIVATE usb-1.0)
endif()

# Install
install(TARGETS sighound_transfer DESTINATION bin)

# Tests
enable_testing()
add_subdirectory(tests)
