add_executable(atomics_data_race data_race.cpp)
target_link_libraries(atomics_data_race PRIVATE Threads::Threads)
target_compile_options(atomics_data_race PRIVATE -fsanitize=thread)
target_link_options(atomics_data_race PRIVATE -fsanitize=thread)

add_executable(race_condition race_condition.cpp)
target_link_libraries(race_condition PRIVATE Threads::Threads)
target_compile_options(race_condition PRIVATE -fsanitize=thread)
target_link_options(race_condition PRIVATE -fsanitize=thread)

add_executable(atomic_operations atomic_operations.cpp)
target_link_libraries(atomic_operations PRIVATE Threads::Threads)

add_executable(cas cas.cpp)
target_link_libraries(cas PRIVATE Threads::Threads)
