/* TI MSP432P401R - real, hand-written bare-metal FreeRTOS demo (CMSIS-
 * RTOS v1, no vendor SDK - see target.toml/README.md). Five real tasks:
 * defaultTask (blinks P1.0), uartTask (heartbeat over EUSCI_A0),
 * spiTask (EUSCI_B0 self-loopback), i2cTask (EUSCI_B1 bus probe - this
 * board has no fixed onboard I2C device, so every real transfer NACKs,
 * same honesty as every other TI chip's own I2C demo in this repo), and
 * pwmTask (TIMER_A0.1 duty-cycle sweep - no dedicated PWM peripheral
 * exists on this silicon, real PWM is Timer_A's own compare-mode output,
 * see timer_a.py's own docstring). No CAN task: this chip's own real
 * register map (TI's official CMSIS-SVD) has no on-chip CAN controller
 * at all - modeling one would violate this whole plan's "never claim
 * unsupported capability" rule, so it's genuinely not here rather than
 * silently dropped without comment.
 *
 * Same real, concrete proof standard as every other FreeRTOS demo in
 * this repo: markers only fire from inside a genuinely scheduled task,
 * and on_task_switch (session.py's own real Timeline wiring) must
 * observe at least 2 distinct TCB pointers switching - in practice, with
 * five real tasks now sharing the CPU, this demo's own test looks for
 * more than the bare minimum (see test_msp432p401r_freertos_demo.py). A
 * mutex guards EUSCI_A0 TX since four tasks now transmit through it -
 * without one, a task switch mid-string could genuinely interleave two
 * tasks' own marker characters, the same real hazard any shared UART
 * has on real hardware. */
#include "msp432p401r_regs.h"
#include "cmsis_os.h"

osThreadId defaultTaskHandle;
osThreadId uartTaskHandle;
osThreadId spiTaskHandle;
osThreadId i2cTaskHandle;
osThreadId pwmTaskHandle;

osMutexDef(uartMutex);
osMutexId uartMutexHandle;

static const char kUartTaskMarker[] = "EMULICA_MSP432P401R_FREERTOS_UARTTASK_OK\r\n";
static const char kHeartbeat[] = "EMULICA_MSP432P401R_FREERTOS_HEARTBEAT\r\n";
static const char kSpiMarker[] = "EMULICA_MSP432P401R_FREERTOS_SPITASK_OK\r\n";
static const char kI2cMarker[] = "EMULICA_MSP432P401R_FREERTOS_I2CTASK_OK\r\n";
static const char kPwmMarker[] = "EMULICA_MSP432P401R_FREERTOS_PWMTASK_OK\r\n";

#define SPI_DEMO_BYTE 0xA5U
#define PWM_PERIOD    1000U

static void wdt_hold(void) {
    /* See blink_uart_demo/firmware/Src/main.c's own wdt_hold() comment -
     * same real first step, same "not modeled, harmlessly unmapped"
     * reasoning. */
    *(volatile uint16_t *)(0x40004800UL + 0xC) = 0x5A80;
}

static void gpio_init(void) {
    DIO_PADIR |= P1_PIN0;
}

static void uart0_init(void) {
    EUSCI_A_CTLW0(EUSCI_A0_BASE) = EUSCI_A_CTLW0_SWRST;
    DIO_PASEL0 |= (P1_PIN2 | P1_PIN3);
    EUSCI_A_CTLW0(EUSCI_A0_BASE) |= EUSCI_A_CTLW0_SSEL_SMCLK;
    EUSCI_A_BRW(EUSCI_A0_BASE) = 19;
    EUSCI_A_MCTLW(EUSCI_A0_BASE) = 0x4A00 | 0x0001;
    EUSCI_A_CTLW0(EUSCI_A0_BASE) &= ~EUSCI_A_CTLW0_SWRST;
}

static void uart0_put_string(const char *s) {
    osMutexWait(uartMutexHandle, osWaitForever);
    while (*s) {
        while (!(EUSCI_A_IFG(EUSCI_A0_BASE) & EUSCI_A_IFG_TXIFG)) { }
        EUSCI_A_TXBUF(EUSCI_A0_BASE) = (uint16_t)(unsigned char)(*s++);
    }
    osMutexRelease(uartMutexHandle);
}

static void spi0_init(void) {
    /* Real eUSCI_B0 SPI-master configuration sequence (same real
     * hold-reset / configure / release-reset order eusci0_init() in
     * blink_uart_demo uses for EUSCI_A0) - master mode (UCMST, bit 11)
     * added since UCMODE=0 (3-pin SPI) and UCSYNC=1 are both already the
     * real reset default (see eusci_b.py's own docstring). */
    EUSCI_B_CTLW0(EUSCI_B0_BASE) = EUSCI_B_CTLW0_SWRST;
    DIO_PASEL0 |= (P1_PIN5 | P1_PIN6 | P1_PIN7);
    EUSCI_B_CTLW0(EUSCI_B0_BASE) |= EUSCI_B_CTLW0_SSEL_SMCLK | 0x0800 /* UCMST */;
    EUSCI_B_BRW(EUSCI_B0_BASE) = 2;
    EUSCI_B_CTLW0(EUSCI_B0_BASE) &= ~EUSCI_B_CTLW0_SWRST;
}

static void i2c0_init(void) {
    /* Real eUSCI_B1 I2C-master configuration - UCMODE=3 (I2C) and UCMST
     * are both real, non-default fields that must be set explicitly
     * (unlike SPI's own mostly-already-right reset state above). P6.4/
     * P6.5 GPIO SEL bits are deliberately not touched here - see
     * msp432p401r_regs.h's own comment on why (Port B/P6 isn't modeled
     * by this chip's own DIO class yet). */
    EUSCI_B_CTLW0(EUSCI_B1_BASE) = EUSCI_B_CTLW0_SWRST | EUSCI_B_CTLW0_UCMODE_I2C;
    EUSCI_B_CTLW0(EUSCI_B1_BASE) |= EUSCI_B_CTLW0_SSEL_SMCLK | 0x0800 /* UCMST */;
    EUSCI_B_BRW(EUSCI_B1_BASE) = 30;
    EUSCI_B_CTLW0(EUSCI_B1_BASE) &= ~EUSCI_B_CTLW0_SWRST;
}

static void pwm0_init(void) {
    /* Real Timer_A up-mode PWM (TI driverlib's own Timer_A_generatePWM()
     * does exactly this): TA0CCR0 sets the real period, TA0CCTL1's
     * OUTMOD=7 (reset/set) makes TA0.1's output high at CCR1 count value
     * and low at the CCR0 rollover - genuine real register configuration
     * (see timer_a.py's own docstring on why the emulator doesn't
     * simulate the resulting analog waveform itself). P2.4 GPIO SEL bits
     * are deliberately not touched here - see msp432p401r_regs.h's own
     * comment (Port B/P2 isn't modeled by this chip's own DIO class
     * yet). */
    TA_CCR0(TIMER_A0_BASE) = PWM_PERIOD;
    TA_CCTL1(TIMER_A0_BASE) = TA_CCTL_OUTMOD_RESET_SET;
    TA_CCR1(TIMER_A0_BASE) = PWM_PERIOD / 4;
    TA_CTL(TIMER_A0_BASE) = TA_CTL_TASSEL_SMCLK | TA_CTL_MC_UP | TA_CTL_TACLR;
}

void StartDefaultTask(void const * argument) {
    for (;;) {
        DIO_PAOUT ^= P1_PIN0;
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

void StartSpiTask(void const * argument) {
    EUSCI_B_TXBUF(EUSCI_B0_BASE) = SPI_DEMO_BYTE;
    while (!(EUSCI_B_IFG(EUSCI_B0_BASE) & EUSCI_B_IFG_RXIFG0)) { }
    if ((EUSCI_B_RXBUF(EUSCI_B0_BASE) & 0xFF) == SPI_DEMO_BYTE) {
        uart0_put_string(kSpiMarker);
    }
    for (;;) {
        EUSCI_B_TXBUF(EUSCI_B0_BASE) = SPI_DEMO_BYTE;
        osDelay(900);
    }
}

void StartI2cTask(void const * argument) {
    EUSCI_B_CTLW0(EUSCI_B1_BASE) |= EUSCI_B_CTLW0_UCTXSTT | EUSCI_B_CTLW0_UCTXSTP;
    if (EUSCI_B_IFG(EUSCI_B1_BASE) & EUSCI_B_IFG_NACKIFG) {
        uart0_put_string(kI2cMarker);
    }
    for (;;) {
        EUSCI_B_CTLW0(EUSCI_B1_BASE) |= EUSCI_B_CTLW0_UCTXSTT | EUSCI_B_CTLW0_UCTXSTP;
        osDelay(1000);
    }
}

void StartPwmTask(void const * argument) {
    uart0_put_string(kPwmMarker);
    uint16_t duty = PWM_PERIOD / 4;
    for (;;) {
        duty = (duty + 100) % PWM_PERIOD;
        TA_CCR1(TIMER_A0_BASE) = duty;
        osDelay(600);
    }
}

int main(void) {
    wdt_hold();
    gpio_init();
    uart0_init();
    spi0_init();
    i2c0_init();
    pwm0_init();

    uartMutexHandle = osMutexCreate(osMutex(uartMutex));

    osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
    defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

    osThreadDef(uartTask, StartUartTask, osPriorityNormal, 0, 128);
    uartTaskHandle = osThreadCreate(osThread(uartTask), NULL);

    osThreadDef(spiTask, StartSpiTask, osPriorityNormal, 0, 128);
    spiTaskHandle = osThreadCreate(osThread(spiTask), NULL);

    osThreadDef(i2cTask, StartI2cTask, osPriorityNormal, 0, 128);
    i2cTaskHandle = osThreadCreate(osThread(i2cTask), NULL);

    osThreadDef(pwmTask, StartPwmTask, osPriorityNormal, 0, 128);
    pwmTaskHandle = osThreadCreate(osThread(pwmTask), NULL);

    osKernelStart();

    /* Should never reach here. */
    for (;;) { }
}
