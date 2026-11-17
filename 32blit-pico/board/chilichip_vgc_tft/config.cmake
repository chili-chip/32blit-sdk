# ChiliChip console, main board v3.0.0 with the 1.44" ST7735S TFT
# (vgc-zero-hardware #41). Everything else is the same as chilichip_vgc.
set(BLIT_BOARD_NAME "ChiliChip VGC TFT")
blit_driver(display dbi_st7735s)
include(${CMAKE_CURRENT_LIST_DIR}/../chilichip_vgc/config.cmake)
