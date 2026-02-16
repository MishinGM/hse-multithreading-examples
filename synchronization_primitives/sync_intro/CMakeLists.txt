add_executable(data_race data_race.cpp)
target_link_libraries(data_race PRIVATE Threads::Threads)

add_executable(data_race_tsan data_race.cpp)
target_link_libraries(data_race_tsan PRIVATE Threads::Threads)
target_compile_options(data_race_tsan PRIVATE -fsanitize=thread)
target_link_options(data_race_tsan PRIVATE -fsanitize=thread)
