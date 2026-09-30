/* TI Tiva C TM4C123GH6PM - real, hand-written bare-metal FreeRTOS demo
 * (CMSIS-RTOS v1, no vendor SDK - see target.toml/README.md). Two real
 * tasks (defaultTask blinks GPIOF PF1, uartTask transmits over UART0),
 * mirroring lm3s6965/freertos_demo's own shape and the same real,
 * concrete proof standard: markers only fire from inside a genuinely
 * scheduled task, and on_task_switch (session.py's own real Timeline
 * wiring) must observe at least 2 distinct TCB pointers switching. Real
 * clock-gate registers are this chip's own genuinely different
 * RCGCGPIO/RCGCUART (not LM3S6965's RCGC1/RCGC2) - see
 * tiva/sysctl.py's own docstring. */
#include "tm4c123_regs.h"
#include "cmsis_os.h"

osThreadId defaultTaskHandle;
osThreadId uartTaskHandle;

static const char kUartTaskMarker[] = "EMULICA_TM4C123_FREERTOS_UARTTASK_OK\r\n";
static const char kHeartbeat[] = "EMULICA_TM4C123_FREERTOS_HEARTBEAT\r\n";

static void gpiof_init(void) {
    SYSCTL_RCGCGPIO |= (1U << 5); /* GPIOF */
    GPIO_DIR(GPIOF_BASE) |= GPIO_PIN_1;
    GPIO_DEN(GPIOF_BASE) |= GPIO_PIN_1;
}

static void uart0_init(void) {
    SYSCTL_RCGCUART |= (1U << 0); /* UART0 */
    SYSCTL_RCGCGPIO |= (1U << 0); /* GPIOA */
    GPIO_AFSEL(GPIOA_BASE) |= (GPIO_PIN_0 | GPIO_PIN_1);
    GPIO_DEN(GPIOA_BASE) |= (GPIO_PIN_0 | GPIO_PIN_1);
    UART_IBRD(UART0_BASE) = 8;
    UART_FBRD(UART0_BASE) = 44;
    UART_LCRH(UART0_BASE) = UART_LCRH_WLEN_8 | UART_LCRH_FEN;
    UART_CTL(UART0_BASE) = UART_CTL_UARTEN | UART_CTL_TXE | UART_CTL_RXE;
}

static void uart0_put_string(const char *s) {
    while (*s) {
        while (UART_FR(UART0_BASE) & UART_FR_TXFF) { }
        UART_DR(UART0_BASE) = (uint32_t)(unsigned char)(*s++);
    }
}

void StartDefaultTask(void const * argument) {
    for (;;) {
        GPIO_DATA(GPIOF_BASE, GPIO_PIN_1) ^= GPIO_PIN_1;
        osDelay(500);
    }
}

void StartUartTask(void const * argument) {
    uart0_put_string(kUartTaskMarker);
    for (;;) {
        uart0_put_string(kHeartbeat);
        osDelay(750);
    }
}

int main(void) {
    gpiof_init();
    uart0_init();

    osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
    defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

    osThreadDef(uartTask, StartUartTask, osPriorityNormal, 0, 128);
    uartTaskHandle = osThreadCreate(osThread(uartTask), NULL);

    osKernelStart();

    /* Should never reach here. */
    for (;;) { }
}
