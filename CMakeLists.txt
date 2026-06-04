cmake_minimum_required(VERSION 3.20)
project(RivenCompiler VERSION 0.1.0 LANGUAGES CXX)

# ── C++20 required ────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Compiler warnings ─────────────────────────
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror -Wshadow)
endif()

# ── Build types ───────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# ── Shared lexer library ──────────────────────
add_library(riven_lexer STATIC
    src/token.cpp
    src/lexer.cpp
)
target_include_directories(riven_lexer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ── Main compiler driver (stub for now) ───────
add_executable(rivenc
    src/main.cpp
)
target_link_libraries(rivenc PRIVATE riven_lexer)

# ── Tests ─────────────────────────────────────
enable_testing()
add_subdirectory(tests)
