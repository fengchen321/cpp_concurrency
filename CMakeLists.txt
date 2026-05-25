cmake_minimum_required(VERSION 3.14)
project(cpp_concurrency LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)

add_subdirectory(Thread_Management)
add_subdirectory(mutex)
add_subdirectory(ThreadSafeContainer)
add_subdirectory(atomic)
add_subdirectory(circularque)
add_subdirectory(singleton)
add_subdirectory(synchronous_concurrent)
