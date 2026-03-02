cmake_minimum_required(VERSION 3.25)

include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
  benchmark
  URL https://github.com/google/benchmark/archive/refs/tags/v1.8.5.zip
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_DOWNLOAD_DEPENDENCIES ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(benchmark)

enable_testing()

add_library(apply_function INTERFACE)
target_include_directories(apply_function INTERFACE ${CMAKE_CURRENT_LIST_DIR})

add_executable(apply_function_tests apply_function_tests.cpp)
target_link_libraries(apply_function_tests PRIVATE apply_function GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(apply_function_tests)

add_executable(apply_function_bench apply_function_bench.cpp)
target_link_libraries(apply_function_bench PRIVATE apply_function benchmark::benchmark)