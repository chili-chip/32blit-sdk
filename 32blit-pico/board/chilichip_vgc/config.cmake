set(BLIT_BOARD_NAME "ChiliChip VGC")

# Force RP2350 platform for Pico 2
set(PICO_PLATFORM rp2350-arm-s)

blit_driver(audio none)
blit_driver(display dbi)
blit_driver(input gpio)
