add_executable(ThreadSafeContainer ThreadSafeContainer.cpp)
target_link_libraries(ThreadSafeContainer PRIVATE Threads::Threads)

add_executable(ThreadSafeLookupTable ThreadSafeLookupTable.cpp)
target_link_libraries(ThreadSafeLookupTable PRIVATE Threads::Threads)

add_executable(ThreadSafeList ThreadSafeList.cpp)
target_link_libraries(ThreadSafeList PRIVATE Threads::Threads)
