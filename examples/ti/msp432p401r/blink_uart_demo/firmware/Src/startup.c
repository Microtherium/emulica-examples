/* Minimal, hand-written Cortex-M4 startup for the MSP432P401R - no
 * vendor SDK/generator applies to this chip (see target.toml/README.md).
 * Standard GCC-for-ARM bare-metal pattern, identical in shape to every
 * other TI chip's own startup.c in this repo (the first 16 vector-table
 * entries are the same on every Cortex-M core): the vector table as a
 * real array in its own linker-placed section, a Reset_Handler that
 * copies .data from flash to SRAM and zeroes .bss before calling main(),
 * and default handlers for every other exception (an infinite loop - no
 * demo here uses interrupts yet, matching this chip's "initial"
 * promotion stage). */
#include <stdint.h>

extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss, _estack;
extern int main(void);

void Reset_Handler(void);
void Default_Handler(void);

void NMI_Handler(void) __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void) __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void) __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void) __attribute__((weak, alias("Default_Handler")));

/* Real Cortex-M4F vector table. Only [0] (initial SP) and [1]
 * (Reset_Handler, Thumb bit set by the linker/compiler automatically for
 * a function pointer) are actually consulted at cold boot by this
 * emulator's own RescParser "start" command (reads word[0]/word[1]
 * directly from flash at 0x00000000, the real MSP432 boot address - same
 * convention as every other TI chip). The rest is real, standard content
 * anyway, not filler. */
__attribute__((section(".isr_vector")))
void (* const g_pfnVectors[])(void) = {
    (void (*)(void))&_estack,
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    0, 0, 0, 0,
    SVC_Handler,
    DebugMon_Handler,
    0,
    PendSV_Handler,
    SysTick_Handler,
};

void Reset_Handler(void) {
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;
    while (dst < &_edata) {
        *dst++ = *src++;
    }
    dst = &_sbss;
    while (dst < &_ebss) {
        *dst++ = 0;
    }
    main();
    while (1) { }
}

void Default_Handler(void) {
    while (1) { }
}
