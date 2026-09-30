/* Real FreeRTOS application hooks (vApplication*) + SystemCoreClock -
 * mirrors lm3s6965/freertos_demo's own freertos_hooks.c, minus the
 * Stellaris-specific comment (this chip's own real post-boot system
 * clock is different - see below). configUSE_IDLE_HOOK/configCHECK_FOR_
 * STACK_OVERFLOW/configUSE_MALLOC_FAILED_HOOK/configSUPPORT_STATIC_
 * ALLOCATION (FreeRTOSConfig.h) all require these to exist - the
 * kernel/heap_4.c won't link without them. */
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"

/* Real CC2640R2F post-boot system clock: 48 MHz - the same value this
 * demo's own sibling (gpio_uart_i2c_demo/firmware/Src/main.c) already
 * used for its UART0 115200-baud IBRD/FBRD calculation. Not functionally
 * exercised by this emulator's own tick delivery yet (MachineTimer drives
 * SysTick independently of the real reload value this feeds into,
 * matching every other chip's demo in this repo) - defined correctly
 * anyway, since vPortSetupTimerInterrupt() (port.c) and
 * configCPU_CLOCK_HZ (FreeRTOSConfig.h) both reference it. */
uint32_t SystemCoreClock = 48000000UL;

void vApplicationIdleHook(void) {
    /* Intentionally empty - see FreeRTOS's own documentation on why an
     * idle hook must never block. */
}

static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory(
    StaticTask_t **ppxIdleTaskTCBBuffer,
    StackType_t **ppxIdleTaskStackBuffer,
    uint32_t *pulIdleTaskStackSize)
{
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
    *ppxIdleTaskStackBuffer = &xIdleStack[0];
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    (void)pcTaskName;
    for (;;) { }
}

void vApplicationMallocFailedHook(void) {
    for (;;) { }
}
