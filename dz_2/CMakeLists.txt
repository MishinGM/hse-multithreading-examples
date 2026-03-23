cmake_minimum_required(VERSION 3.16)
project(futex_cv_demo CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(test_cv test_cv.cpp)
target_link_libraries(test_cv pthread)