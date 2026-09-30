/* Minimal linker script for CC2652R1 - real flash/SRAM sizes (352 KB /
 * 80 KB), matching renoly_examples/ti/cc2652r1/platform.repl exactly -
 * cross-checked against two independent real sources: TI's own official
 * CC2652R1 product listing and RS Components' real distributor part
 * listing ("...ARM Cortex M4F...352kB FLASH..."). Real SimpleLink parts
 * map flash directly at 0x00000000 (same convention as CC2640R2F's own,
 * no separate "real" region + alias the way STM32 does). */
MEMORY
{
    FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 352K
    SRAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 80K
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
