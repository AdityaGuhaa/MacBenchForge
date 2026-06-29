cmake_minimum_required(VERSION 3.20)
project(MacBenchForge VERSION 0.1.0 LANGUAGES CXX)

# ─── C++ Standard ─────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ─── macOS Apple Silicon Target ───────────────────────────────────────────────
set(CMAKE_OSX_ARCHITECTURES "arm64")
set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0")  # Ventura minimum -- good M1 baseline

# ─── Output Directories ───────────────────────────────────────────────────────
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ─── Homebrew Prefix (Apple Silicon) ──────────────────────────────────────────
# Homebrew on Apple Silicon installs to /opt/homebrew, NOT /usr/local
# This avoids picking up miniconda's libcurl accidentally
set(HOMEBREW_PREFIX "/opt/homebrew")

find_package(CURL REQUIRED)
message(STATUS "libcurl found: ${CURL_LIBRARIES}")

find_package(OpenSSL REQUIRED)
message(STATUS "OpenSSL found: ${OPENSSL_LIBRARIES}")

# ─── Third Party: cpp-httplib (header-only + OpenSSL for HTTPS) ───────────────
add_library(httplib INTERFACE)
target_include_directories(httplib INTERFACE
    ${CMAKE_SOURCE_DIR}/third_party/cpp-httplib)
target_include_directories(httplib INTERFACE
    ${OPENSSL_INCLUDE_DIR})
# Enable HTTPS support in cpp-httplib
target_compile_definitions(httplib INTERFACE CPPHTTPLIB_OPENSSL_SUPPORT)

# ─── Third Party: nlohmann/json (header-only) ────────────────────────────────
add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE
    ${CMAKE_SOURCE_DIR}/third_party/nlohmann/include)

# ─── Third Party: toml++ (header-only) ───────────────────────────────────────
add_library(tomlplusplus INTERFACE)
target_include_directories(tomlplusplus INTERFACE
    ${CMAKE_SOURCE_DIR}/third_party/tomlplusplus/include)

# ─── Third Party: SQLiteCpp ───────────────────────────────────────────────────
set(SQLITECPP_RUN_CPPLINT   OFF CACHE BOOL "" FORCE)
set(SQLITECPP_RUN_CPPCHECK  OFF CACHE BOOL "" FORCE)
set(SQLITE_ENABLE_COLUMN_METADATA OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/SQLiteCpp)

# ─── Source Files ─────────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/config/config.cpp
    src/db/database.cpp
    src/db/crud.cpp
    src/discovery/scanner.cpp
    src/hardware/hw_info.cpp
    src/huggingface/hf_client.cpp
    src/huggingface/downloader.cpp
    src/benchmark/runner.cpp
    src/benchmark/metrics.cpp
    src/export/exporter.cpp
    src/server/server.cpp
    src/server/routes.cpp
)

# ─── Executable ───────────────────────────────────────────────────────────────
add_executable(MacBenchForge ${SOURCES})

# ─── Include Directories ──────────────────────────────────────────────────────
target_include_directories(MacBenchForge PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party/cpp-httplib
    ${CMAKE_SOURCE_DIR}/third_party/nlohmann/include
    ${CMAKE_SOURCE_DIR}/third_party/tomlplusplus/include
    ${CURL_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
)

# ─── Link Libraries ───────────────────────────────────────────────────────────
target_link_libraries(MacBenchForge PRIVATE
    httplib
    nlohmann_json
    tomlplusplus
    SQLiteCpp
    ${CURL_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
)

# ─── macOS Frameworks ─────────────────────────────────────────────────────────
# IOKit     -- GPU core count via AGXAccelerator
# CoreFoundation -- required by IOKit
# Foundation     -- macOS base framework
target_link_libraries(MacBenchForge PRIVATE
    "-framework IOKit"
    "-framework CoreFoundation"
    "-framework Foundation"
)

# ─── RPATH for bundled dylibs ─────────────────────────────────────────────────
# Tells the binary to look in .app/Contents/Frameworks/ at runtime
# instead of /opt/homebrew/lib (which won't exist on the user's machine)
set_target_properties(MacBenchForge PROPERTIES
    BUILD_RPATH          "${HOMEBREW_PREFIX}/lib"
    INSTALL_RPATH        "@executable_path/../Frameworks"
    BUILD_WITH_INSTALL_RPATH OFF
)

# ─── Compiler Flags ───────────────────────────────────────────────────────────
target_compile_options(MacBenchForge PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-deprecated-declarations
    -O2
)

# ─── Enable HTTPS in cpp-httplib ──────────────────────────────────────────────
target_compile_definitions(MacBenchForge PRIVATE
    CPPHTTPLIB_OPENSSL_SUPPORT
)

# ─── Post-Build: Copy Frontend ────────────────────────────────────────────────
add_custom_command(TARGET MacBenchForge POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/frontend
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/frontend
    COMMENT "Copying frontend to build output..."
)

# ─── Post-Build: Copy config.toml ────────────────────────────────────────────
add_custom_command(TARGET MacBenchForge POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/config.toml
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config.toml
    COMMENT "Copying config.toml to build output..."
)

# ─── Post-Build: Copy reference texts ────────────────────────────────────────
add_custom_command(TARGET MacBenchForge POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/reference
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/reference
    COMMENT "Copying reference texts to build output..."
)

# ─── Build Info ───────────────────────────────────────────────────────────────
message(STATUS "")
message(STATUS "=== MacBenchForge v${PROJECT_VERSION} ===")
message(STATUS "Architecture : ${CMAKE_OSX_ARCHITECTURES}")
message(STATUS "Min macOS    : ${CMAKE_OSX_DEPLOYMENT_TARGET}")
message(STATUS "Compiler     : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build type   : ${CMAKE_BUILD_TYPE}")
message(STATUS "Output dir   : ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "libcurl      : ${CURL_LIB}")
message(STATUS "OpenSSL      : ${OPENSSL_SSL_LIB}")
message(STATUS "")