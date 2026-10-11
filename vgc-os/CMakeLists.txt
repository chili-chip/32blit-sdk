cmake_minimum_required(VERSION 3.15...3.31)
project (vgc-os)
find_package (32BLIT CONFIG REQUIRED PATHS ..)

blit_executable(vgc-os
    games.cpp home.cpp settings.cpp ui.cpp
)

target_link_libraries(vgc-os LauncherShared)
blit_metadata(vgc-os metadata.yml)
