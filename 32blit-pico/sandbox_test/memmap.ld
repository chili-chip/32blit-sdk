/* QEMU mps2-an505 (Cortex-M33) stand-in for the loader's memory map (sandbox.cpp):
   loader flash at 0x10000000 with APIConst 512 bytes in, the game's image
   elsewhere in flash, shared RAM then the loader's RAM, the loader's stack
   area 0x80008000-0x80010000, the game's RAM after it. */
MEMORY
{
    FLASH(rx)      : ORIGIN = 0x10000000, LENGTH = 2M
    GAME_FLASH(rx) : ORIGIN = 0x10200000, LENGTH = 1M
    RAM(rwx)       : ORIGIN = 0x80000000, LENGTH = 32k
    STACK(rw)      : ORIGIN = 0x80008000, LENGTH = 32k
    GAME_RAM(rwx)  : ORIGIN = 0x80010000, LENGTH = 448k
}

ENTRY(reset_handler)

SECTIONS
{
    .text : {
        KEEP(*(.vectors))
        . = 0x200;
        KEEP(*(.rodata.api_const))
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
        __init_array_start = .;
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        __init_array_end = .;
    } > FLASH

    .ARM.exidx : { *(.ARM.exidx* .gnu.linkonce.armexidx.*) } > FLASH

    .api_data (NOLOAD) : {
        __api_data_start = .;
        KEEP(*(*.api_data))
        . = ALIGN(32);
        KEEP(*(.bss.sandbox_shared*))
        . = ALIGN(32);
        __sandbox_shared_end = .;
    } > RAM

    .data : {
        __data_start__ = .;
        *(.data*)
        . = ALIGN(4);
        __data_end__ = .;
    } > RAM AT> FLASH
    __data_source__ = LOADADDR(.data);
    __flash_binary_end = LOADADDR(.data) + SIZEOF(.data);

    .bss (NOLOAD) : {
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
        end = .;
    } > RAM

    .game_image : {
        __game_image_start = .;
        KEEP(*(.game_text*))
        . = ALIGN(32);
        __game_image_end = .;
    } > GAME_FLASH

    .game_ram (NOLOAD) : { *(.game_bss*) } > GAME_RAM

    __StackLimit = ORIGIN(STACK);
    __StackTop = ORIGIN(STACK) + LENGTH(STACK);
    __HeapLimit = ORIGIN(STACK);
}
