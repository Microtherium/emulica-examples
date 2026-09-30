/* Real FreeRTOS application hooks (vApplication*) + SystemCoreClock -
 * mirrors every other FreeRTOS demo's own freertos_hooks.c exactly (this
 * file is architecture-agnostic - no chip-specific code beyond the
 * SystemCoreClock comment below). configUSE_IDLE_HOOK/configCHECK_FOR_
 * STACK_OVERFLOW/configUSE_MALLOC_FAILED_HOOK/
 * configSUPPORT_STATIC_ALLOCATION (FreeRTOSConfig.h) all require these
 * to exist - the kernel/heap_4.c won't link without them. */
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"

/* Real CC2652R1 post-boot system clock: 48 MHz - the same real value
 * CC2640R2F's own demo already used for its own UART0 baud calculation
 * (both chips share the same real SimpleLink clock architecture). Not
 * functionally exercised by this emulator's own tick delivery yet
 * (MachineTimer drives SysTick independently of the real reload value
 * this feeds into, matching every other chip's demo in this repo) -
 * defined correctly anyway, since vPortSetupTimerInterrupt() (port.c)
 * and configCPU_CLOCK_HZ (FreeRTOSConfig.h) both reference it. */
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
