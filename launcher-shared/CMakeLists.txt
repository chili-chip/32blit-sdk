

add_library(LauncherShared metadata.cpp layout.cpp theme.cpp)
target_link_libraries(LauncherShared BlitEngine)
target_include_directories(LauncherShared PUBLIC ${CMAKE_CURRENT_LIST_DIR})

# host test for the layout maths, desktop builds only
if(NOT CMAKE_CROSSCOMPILING AND NOT EMSCRIPTEN)
    add_executable(launcher-layout-test layout_test.cpp layout.cpp ${CMAKE_CURRENT_LIST_DIR}/../32blit/graphics/font.cpp)
    target_include_directories(launcher-layout-test PRIVATE ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/../32blit)
    add_test(NAME launcher-layout COMMAND launcher-layout-test)
endif()
