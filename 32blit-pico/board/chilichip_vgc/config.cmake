set(BLIT_BOARD_NAME "ChiliChip VGC")

# Force RP2350 platform for Pico 2
set(PICO_PLATFORM rp2350-arm-s)

blit_driver(audio pwm)
blit_driver(display dbi_st7735s)
blit_driver(input gpio)
