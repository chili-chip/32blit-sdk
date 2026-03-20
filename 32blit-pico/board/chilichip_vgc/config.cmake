set(BLIT_BOARD_NAME "ChiliChip VGC")

# Force RP2350 platform for Pico 2
set(PICO_PLATFORM rp2350-arm-s)

blit_driver(audio beep)
blit_driver(display dbi_ssd1351)
blit_driver(input gpio)
