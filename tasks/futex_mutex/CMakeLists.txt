cmake_minimum_required(VERSION 3.16)

project(futex_mutex)

set(CMAKE_CXX_STANDARD 20)

add_executable(futex_mutex_test test.cpp)
add_executable(futex_mutex_benchmark benchmark.cpp)

target_link_libraries(futex_mutex_test pthread)
target_link_libraries(futex_mutex_benchmark pthread)