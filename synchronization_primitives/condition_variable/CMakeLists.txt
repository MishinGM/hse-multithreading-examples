add_executable(condvar_intro condvar_intro.cpp)
target_link_libraries(condvar_intro PRIVATE Threads::Threads)
target_compile_options(condvar_intro PRIVATE -fsanitize=thread)
target_link_options(condvar_intro PRIVATE -fsanitize=thread)

add_executable(condition_variable_any condition_variable_any.cpp)
target_link_libraries(condition_variable_any PRIVATE Threads::Threads)
target_compile_options(condition_variable_any PRIVATE -fsanitize=thread)
target_link_options(condition_variable_any PRIVATE -fsanitize=thread)
