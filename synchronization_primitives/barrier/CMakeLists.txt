add_executable(barrier barrier.cpp)
target_link_libraries(barrier PRIVATE Threads::Threads)

add_executable(latch latch.cpp)
target_link_libraries(latch PRIVATE Threads::Threads)