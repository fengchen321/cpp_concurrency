add_executable(test_atomic test_atomic.cpp)
target_link_libraries(test_atomic PRIVATE Threads::Threads)
