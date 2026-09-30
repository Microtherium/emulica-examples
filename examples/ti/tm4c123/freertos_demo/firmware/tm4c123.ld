/* Minimal linker script for TM4C123GH6PM - real flash/SRAM sizes (256 KB
 * / 32 KB, confirmed against Apache NuttX's own tm4c123g-launchpad board
 * linker script), matching renoly_examples/ti/tm4c123/platform.repl
 * exactly. 32 KB SRAM, not LM3S6965's 64 KB - a real, smaller-SRAM part
 * despite being the newer Cortex-M4 chip; not assumed to scale up just
 * because it's newer. Real Tiva C parts still map flash directly at
 * 0x00000000 (no separate "real" region + alias the way STM32 does). */
MEMORY
{
    FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 256K
    SRAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 32K
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
