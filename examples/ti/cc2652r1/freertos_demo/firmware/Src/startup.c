/* Minimal, hand-written Cortex-M4F startup for the CC2652R1 FreeRTOS
 * demo - no vendor SDK/generator applies to this chip (see target.toml).
 * Same shape as gpio_uart_i2c_demo's own startup.c and identical in
 * substance to every other FreeRTOS demo's own (the first 16 vector-
 * table entries and this SVC/PendSV/SysTick convention are architecture-
 * generic, not chip-specific): SVC_Handler/PendSV_Handler stay weak
 * (aliased to Default_Handler) so FreeRTOSConfig.h's real
 * vPortSVCHandler->SVC_Handler/xPortPendSVHandler->PendSV_Handler
 * renaming (see that file's own comment) - non-weak, real kernel
 * functions - correctly overrides them at link time; SysTick_Handler is
 * a real, non-weak override that calls osSystickHandler(). */
#include <stdint.h>
#include "cmsis_os.h"

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
void SysTick_Handler(void);

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

/* Real, non-weak override - see this file's own header comment. */
void SysTick_Handler(void) {
    osSystickHandler();
}
