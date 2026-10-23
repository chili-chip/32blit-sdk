# Examples used by the documentation (the Examples and Tutorials pages).
#
# Build them all for the desktop with:
#   cmake -B build -S docs/examples && cmake --build build
# or for the VGC Zero by adding the usual Pico arguments (see docs/getting-started/build-console.md).
cmake_minimum_required(VERSION 3.15...3.31)
project(sdk-examples LANGUAGES C CXX ASM)

if(MSVC)
  add_compile_options("/W4" "/wd4244" "/wd4324" "/wd4458" "/wd4100")
else()
  add_compile_options("-Wall" "-Wextra" "-Wdouble-promotion" "-Wno-unused-parameter")
endif()

find_package(32BLIT CONFIG REQUIRED PATHS ${CMAKE_CURRENT_LIST_DIR}/../..)

add_subdirectory(hello-world)
add_subdirectory(shapes)
add_subdirectory(input)
add_subdirectory(sprites)
add_subdirectory(text)
add_subdirectory(audio)
add_subdirectory(saves)
add_subdirectory(timers-tweens)
add_subdirectory(tilemap)
add_subdirectory(particles)
add_subdirectory(system-info)
add_subdirectory(chili-catch)
