add_executable(LockFreeStack LockFreeStack.cpp)
target_link_libraries(LockFreeStack PRIVATE Threads::Threads atomic)
