/* Minimal linker script for CC2652R1 - identical to
 * gpio_uart_i2c_demo/firmware/cc2652r1.ld (same real flash/SRAM sizes,
 * 352 KB / 80 KB, matching renoly_examples/ti/cc2652r1/platform.repl -
 * see that sibling file's own comment for the real-source citations on
 * these sizes). Real SimpleLink parts map flash directly at 0x00000000
 * (no separate "real" region + alias the way STM32 does). */
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
