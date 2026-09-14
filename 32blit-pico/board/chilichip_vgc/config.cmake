set(BLIT_BOARD_NAME "ChiliChip VGC")

# Force RP2350 platform for Pico 2
set(PICO_PLATFORM rp2350-arm-s)

blit_driver(audio pwm)
blit_driver(display dbi_ssd1351)
blit_driver(input gpio)
blit_driver(led binary)

# Core 1 owns PWM audio mixing so Core 0 is not preempted by the 5 ms
# audio alarm while Citsy is compositing / palettizing a frame. Display
# DMA already runs independently of the CPU (update_display_core1 is a
# no-op for the SSD1351 driver).
set(BLIT_ENABLE_CORE1 TRUE)