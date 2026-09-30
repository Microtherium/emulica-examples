/* Minimal linker script for MSP432P401R - real flash/SRAM sizes (256 KB
 * / 64 KB), confirmed against TI's own official CMSIS-SVD for this exact
 * part (FLCTL/SRAM address-block sizing) and cross-checked against
 * Zephyr's own dts/arm/ti/msp432p401r.dtsi ("flash0: serial-flash@0 {
 * reg = <0x0 DT_SIZE_K(256)>; }", "sram0: memory@20000000 { reg =
 * <0x20000000 DT_SIZE_K(64)>; }") - both agree. Real MSP432 parts map
 * flash directly at 0x00000000 (no separate "real" region + alias the
 * way STM32 does), same convention as every other TI chip in this
 * repo. */
MEMORY
{
    FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 256K
    SRAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 64K
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
