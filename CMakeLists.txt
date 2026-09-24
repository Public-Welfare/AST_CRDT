cmake_minimum_required(VERSION 3.20)
project(TreeCRDT VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler optimization flags
if(MSVC)
    add_compile_options(/O2 /W4 /permissive-)
else()
    add_compile_options(-O3 -Wall -Wextra -Wpedantic)
endif()

# Core static library
add_library(tree_crdt_core STATIC
    src/order_statistic_tree.cpp
    src/yata_engine.cpp
    src/delta_encoder.cpp
    src/document.cpp
)

target_include_directories(tree_crdt_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Automated Test Suite executable
add_executable(tree_crdt_tests
    tests/test_main.cpp
)
target_link_libraries(tree_crdt_tests PRIVATE tree_crdt_core)

# Performance Benchmark executable
add_executable(tree_crdt_benchmark
    benchmarks/benchmark_main.cpp
)
target_link_libraries(tree_crdt_benchmark PRIVATE tree_crdt_core)
