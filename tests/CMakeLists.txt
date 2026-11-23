# Host tests for small pieces of the SDK that need no device.
add_executable(save_dir_test save_dir_test.cpp)
target_include_directories(save_dir_test PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../32blit)
add_test(NAME save_dir COMMAND save_dir_test)
