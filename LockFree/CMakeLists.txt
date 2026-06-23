add_executable(LockFreeStack LockFreeStack.cpp)
target_link_libraries(LockFreeStack PRIVATE Threads::Threads atomic)

add_executable(LockFreeQueue LockFreeQueue.cpp)
target_link_libraries(LockFreeQueue PRIVATE Threads::Threads atomic)
