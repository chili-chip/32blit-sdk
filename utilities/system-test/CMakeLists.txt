cmake_minimum_required(VERSION 3.15...3.31)
project (system-test)
find_package (32BLIT CONFIG REQUIRED PATHS ../..)

blit_executable (system-test system-test.cpp)
blit_metadata (system-test metadata.yml)
