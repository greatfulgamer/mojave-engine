cmake_minimum_required(VERSION 3.20)
project(mojave-engine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MOJAVE_BUILD_TESTS "Build Mojave tests" ON)
option(MOJAVE_HEADLESS "Build without GUI deps (server/sim only)" OFF)

# ── Core library ──────────────────────────────────────────────
add_library(mojave_core STATIC
    src/platform/AssetLocator.cpp
    src/platform/Vdf.cpp
)
target_include_directories(mojave_core PUBLIC src)
target_compile_options(mojave_core PRIVATE -Wall -Wextra -Wpedantic)

# ── Executable ────────────────────────────────────────────────
add_executable(mojave src/main.cpp)
target_link_libraries(mojave PRIVATE mojave_core)

# ── Tests ─────────────────────────────────────────────────────
if(MOJAVE_BUILD_TESTS)
    enable_testing()
    add_executable(mojave_tests
        tests/test_assetlocator.cpp
        tests/test_vdf.cpp
    )
    target_link_libraries(mojave_tests PRIVATE mojave_core)
    add_test(NAME assetlocator COMMAND mojave_tests)
endif()
