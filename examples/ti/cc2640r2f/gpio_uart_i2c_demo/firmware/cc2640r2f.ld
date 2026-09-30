/* Minimal linker script for CC2640R2F - real flash/SRAM sizes (128 KB /
 * 20 KB), matching renoly_examples/ti/cc2640r2f/platform.repl exactly
 * (confirmed against contiki-ng's own real CC13x0/CC26x0-family linker
 * script - no CC2640R2F-specific one was found, see platform.repl's own
 * comment on this caveat). Real SimpleLink parts map flash directly at
 * 0x00000000 (no separate "real" region + alias the way STM32 does). */
MEMORY
{
    FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 128K
    SRAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 20K
}

ENTRY(Reset_Handler)

SECTIONS
{
    .isr_vector :
    {
        KEEP(*(.isr_vector))
    } > FLASH

    .text :
    {
        *(.text*)
        *(.rodata*)
    } > FLASH

    _sidata = LOADADDR(.data);

    .data :
    {
        _sdata = .;
        *(.data*)
        _edata = .;
    } > SRAM AT> FLASH

    .bss :
    {
        _sbss = .;
        *(.bss*)
        *(COMMON)
        _ebss = .;
    } > SRAM

    . = ALIGN(8);
    _estack = ORIGIN(SRAM) + LENGTH(SRAM);
}
