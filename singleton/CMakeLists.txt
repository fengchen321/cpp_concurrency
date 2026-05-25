add_executable(singleton singleton.cpp)
target_link_libraries(singleton PRIVATE Threads::Threads)
set_target_properties(singleton PROPERTIES CXX_STANDARD 11)
