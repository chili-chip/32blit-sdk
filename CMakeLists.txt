cmake_minimum_required(VERSION 3.15...3.31)

# has to be before project
include(32blit-pico/sdk_import.cmake)

project(32blit)

if(MSVC)
    add_compile_options("/W4" "/wd4244" "/wd4324" "/wd4458" "/wd4100")
else()
    add_compile_options("-Wall" "-Wextra" "-Wdouble-promotion" "-Wno-unused-parameter")
endif()

find_package (32BLIT CONFIG REQUIRED PATHS .)

add_subdirectory(launcher-shared)

add_subdirectory(utilities)

if(32BLIT_HW)
    add_subdirectory(32blit-stm32)
    add_subdirectory(32blit-stm32/firmware)
    add_subdirectory(32blit-stm32/firmware-update)
elseif(32BLIT_PICO)
    add_subdirectory(32blit-pico)
    add_subdirectory(32blit-pico/loader)
endif()

add_subdirectory(launcher)
add_subdirectory(vgc-os)

# host tests, on desktop builds only
if(NOT 32BLIT_HW AND NOT 32BLIT_PICO AND NOT EMSCRIPTEN AND NOT CMAKE_CROSSCOMPILING)
    enable_testing()
    add_subdirectory(tests)
endif()

# if the examples are there, build them too
if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/examples/CMakeLists.txt)
    add_subdirectory(examples)
endif()

# include dist files in install
install(DIRECTORY
    ${CMAKE_CURRENT_LIST_DIR}/dist/
    DESTINATION .
)

# setup release packages
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_GENERATOR "ZIP" "TGZ")
include(CPack)
