# sandbox.cpp and sandbox_gate.S on QEMU's Cortex-M33 board (see README.md).
# Built with BLIT_SANDBOX=ON; `make run-sandbox-test` needs qemu-system-arm.
add_executable(blit-sandbox-test
    test.cpp
    game.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../sandbox.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../sandbox_gate.S
)
target_include_directories(blit-sandbox-test PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/..
    ${CMAKE_CURRENT_LIST_DIR}/../../32blit
)
target_compile_definitions(blit-sandbox-test PRIVATE
    BLIT_SANDBOX=1 BUILD_LOADER
    SANDBOX_GAME_RAM_END=0x80080000u
)
target_compile_options(blit-sandbox-test PRIVATE -O2 -fno-exceptions)
# only headers from the pico-sdk: the test has its own start-up and stand-ins
target_link_libraries(blit-sandbox-test hardware_exception_headers hardware_sync_headers hardware_structs pico_platform_headers)
target_link_options(blit-sandbox-test PRIVATE
    -nostartfiles --specs=nano.specs --specs=nosys.specs
    -T${CMAKE_CURRENT_LIST_DIR}/memmap.ld
)
set_target_properties(blit-sandbox-test PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/memmap.ld)

add_custom_target(run-sandbox-test
    COMMAND timeout 60 qemu-system-arm -machine mps2-an505 -nographic -semihosting-config enable=on,target=native -kernel $<TARGET_FILE:blit-sandbox-test>
    DEPENDS blit-sandbox-test
    USES_TERMINAL
)
