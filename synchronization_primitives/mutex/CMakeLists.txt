add_executable(mutex mutex.cpp)
target_link_libraries(mutex PRIVATE Threads::Threads)

add_executable(lock_guard lock_guard.cpp)
target_link_libraries(lock_guard PRIVATE Threads::Threads)

add_executable(deadlock deadlock.cpp)
target_link_libraries(deadlock PRIVATE Threads::Threads)

add_executable(livelock livelock.cpp)
target_link_libraries(livelock PRIVATE Threads::Threads)

add_executable(recursive_mutex recursive_mutex.cpp)
target_link_libraries(recursive_mutex PRIVATE Threads::Threads)

add_executable(shared_mutex shared_mutex.cpp)
target_link_libraries(shared_mutex PRIVATE Threads::Threads)

add_executable(spinlock spinlock.cpp)
target_link_libraries(spinlock PRIVATE Threads::Threads)
target_compile_options(spinlock PRIVATE -fsanitize=thread)
target_link_options(spinlock PRIVATE -fsanitize=thread)