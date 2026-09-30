/* Real FreeRTOS application hooks (vApplication*) + SystemCoreClock -
 * mirrors stm32l476/freertos_demo's own freertos.c, minus the STM32-
 * specific bits (no HAL, no CubeMX-generated boilerplate - this chip has
 * neither, see target.toml). configUSE_IDLE_HOOK/configCHECK_FOR_STACK_
 * OVERFLOW/configUSE_MALLOC_FAILED_HOOK/configSUPPORT_STATIC_ALLOCATION
 * (FreeRTOSConfig.h) all require these to exist - the kernel/heap_4.c
 * won't link without them. */
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"

/* Real Stellaris LM3S6965 default clock after reset: the internal 16 MHz
 * precision oscillator (no PLL/crystal configured - this demo doesn't
 * touch RCC/RCC2 at all, matching sysctl.py's own real POR reset values).
 * Not functionally exercised by this emulator's own tick delivery yet
 * (MachineTimer drives SysTick independently of the real reload value
 * this feeds into, matching every other chip's demo in this repo) -
 * defined correctly anyway, since vPortSetupTimerInterrupt() (port.c)
 * and configCPU_CLOCK_HZ (FreeRTOSConfig.h) both reference it. */
uint32_t SystemCoreClock = 16000000UL;

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
