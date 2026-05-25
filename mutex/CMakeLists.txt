add_executable(mutex_lock mutex_lock.cpp)
target_link_libraries(mutex_lock PRIVATE Threads::Threads)

add_executable(unique_lock unique_lock.cpp)
target_link_libraries(unique_lock PRIVATE Threads::Threads)
