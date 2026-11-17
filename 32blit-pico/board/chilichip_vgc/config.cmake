# ChiliChip console, main board v3.0.0 with the OLED display.
# chilichip_vgc_tft is the same board with the TFT display.
if(NOT BLIT_BOARD_NAME)
    set(BLIT_BOARD_NAME "ChiliChip VGC")
endif()

# Force RP2350 platform for Pico 2
set(PICO_PLATFORM rp2350-arm-s)

# MAX98357A on the audio module, over I2S.
blit_driver(audio i2s)
blit_driver(display dbi_ssd1351)
blit_driver(input gpio)

# Core 1 owns audio mixing so Core 0 is not preempted while Citsy is
# compositing / palettizing a frame. Display DMA already runs independently
# of the CPU (update_display_core1 is a no-op for the SSD1351 driver).
set(BLIT_ENABLE_CORE1 TRUE)

# Default Pico memmap puts core 0's stack in 4 KiB scratch Y. Citsy needs more.
set(BLIT_LINKER_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/memmap_ram_stack.ld)

# Loader + .blit games: the loader's core 0 stack is shared with the running
# game, so it needs the same 32 KiB as standalone builds. The loader takes the
# first 64 KiB of RAM (data, heap, stack) and games start after it.
set(BLIT_LOADER_LINKER_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/memmap_blit_loader.ld)
set(BLIT_LOADER_RAM_KB 64)
