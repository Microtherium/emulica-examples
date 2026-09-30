/* Minimal linker script for CC2640R2F - identical to
 * gpio_uart_i2c_demo/firmware/cc2640r2f.ld (same real flash/SRAM sizes,
 * 128 KB / 20 KB, matching renoly_examples/ti/cc2640r2f/platform.repl -
 * see that sibling file's own comment for the real-source caveat on these
 * sizes). Real SimpleLink parts map flash directly at 0x00000000 (no
 * separate "real" region + alias the way STM32 does). */
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
