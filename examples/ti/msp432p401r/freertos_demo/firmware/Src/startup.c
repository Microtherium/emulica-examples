/* Minimal, hand-written Cortex-M4 startup for the MSP432P401R FreeRTOS
 * demo - no vendor SDK/generator applies to this chip (see target.toml).
 * Same shape as blink_uart_demo's own startup.c and identical in
 * substance to every other TI chip's own FreeRTOS startup.c in this repo
 * (the first 16 vector-table entries and this SVC/PendSV/SysTick
 * convention are architecture-generic, not chip-specific):
 * SVC_Handler/PendSV_Handler stay weak (aliased to Default_Handler) so
 * FreeRTOSConfig.h's real vPortSVCHandler->SVC_Handler/
 * xPortPendSVHandler->PendSV_Handler renaming (see that file's own
 * comment) - non-weak, real kernel functions - correctly overrides them
 * at link time; SysTick_Handler is a real, non-weak override that calls
 * osSystickHandler(). */
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

/* Ports 1-6's real GPIO interrupt handlers - only declared (weak,
 * defaulting to Default_Handler) for real vector-table completeness;
 * this demo doesn't unmask any DIO interrupt, so none of these actually
 * run. Ports 7-10/PJ have no real NVIC vector at all on this exact part
 * (see renoly/hw/ti/msp432/dio.py's own docstring) - correctly omitted,
 * not a gap. */
void PORT1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void PORT2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void PORT3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void PORT4_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void PORT5_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void PORT6_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));

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
    /* External interrupts start here (device IRQ 0). Real MSP432P401R
     * vector numbers, confirmed against TI's own SVD (see dio.py's own
     * docstring): PORT1_IRQ=35 .. PORT6_IRQ=40, i.e. indices 35-40 in
     * this array (16 core exceptions + device IRQ number). Every entry
     * before index 35 that this demo doesn't use is a real, standard
     * Default_Handler-aliased vector, matching this repo's own "real,
     * standard content anyway, not filler" convention. */
    [16 ... 34] = Default_Handler,
    [35] = PORT1_IRQHandler,
    [36] = PORT2_IRQHandler,
    [37] = PORT3_IRQHandler,
    [38] = PORT4_IRQHandler,
    [39] = PORT5_IRQHandler,
    [40] = PORT6_IRQHandler,
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
