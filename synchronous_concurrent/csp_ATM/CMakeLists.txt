add_executable(csp_ATM csp_ATM.cpp)
target_link_libraries(csp_ATM PRIVATE Threads::Threads)
set_target_properties(csp_ATM PROPERTIES CXX_STANDARD 11)
