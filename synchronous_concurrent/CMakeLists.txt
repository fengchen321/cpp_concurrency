set(SYNC_CXX_STANDARD 14)

add_executable(condition_var condition_var.cpp)
target_link_libraries(condition_var PRIVATE Threads::Threads)
set_target_properties(condition_var PROPERTIES CXX_STANDARD ${SYNC_CXX_STANDARD})

add_executable(async_feature async_feature.cpp)
target_link_libraries(async_feature PRIVATE Threads::Threads)
set_target_properties(async_feature PROPERTIES CXX_STANDARD ${SYNC_CXX_STANDARD})

add_executable(thread_pool thread_pool.cpp)
target_link_libraries(thread_pool PRIVATE Threads::Threads)
set_target_properties(thread_pool PROPERTIES CXX_STANDARD ${SYNC_CXX_STANDARD})

add_executable(quick_sort quick_sort.cpp)
target_link_libraries(quick_sort PRIVATE Threads::Threads)
set_target_properties(quick_sort PROPERTIES CXX_STANDARD ${SYNC_CXX_STANDARD})

add_executable(csp_producer_consumer csp_producer_consumer.cpp)
target_link_libraries(csp_producer_consumer PRIVATE Threads::Threads)
set_target_properties(csp_producer_consumer PROPERTIES CXX_STANDARD ${SYNC_CXX_STANDARD})

add_executable(qa_test qa_test.cpp)
target_link_libraries(qa_test PRIVATE Threads::Threads)
set_target_properties(qa_test PROPERTIES CXX_STANDARD ${SYNC_CXX_STANDARD})

add_subdirectory(csp_ATM)
