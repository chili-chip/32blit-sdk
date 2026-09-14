set(BLIT_BOARD_NAME "ChiliChip VGC")

# Force RP2350 platform for Pico 2
set(PICO_PLATFORM rp2350-arm-s)

# Hardware PWM DAC on PICO_AUDIO_PWM_MONO_PIN (GP22). Not the beep driver.
blit_driver(audio pwm)
blit_driver(display dbi_ssd1351)
blit_driver(input gpio)
blit_driver(led binary)

# Core 1 owns PWM audio mixing so Core 0 is not preempted by the 5 ms
# audio alarm while Citsy is compositing / palettizing a frame. Display
# DMA already runs independently of the CPU (update_display_core1 is a
# no-op for the SSD1351 driver).
set(BLIT_ENABLE_CORE1 TRUE)

# Default Pico memmap puts core 0's stack in 4 KiB scratch Y. Citsy needs more.
set(BLIT_LINKER_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/memmap_ram_stack.ld)
