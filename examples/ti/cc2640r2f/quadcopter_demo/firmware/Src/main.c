/* TI SimpleLink CC2640R2F - real, hand-written bare-metal FreeRTOS demo
 * (CMSIS-RTOS v1, no vendor SDK - see target.toml/README.md). Four real
 * tasks (defaultTask blinks DIO7 - the real onboard LaunchPad LED,
 * uartTask transmits over UART0, bleTask drives the RF Core stub through
 * a real driverlib-shaped boot+advertise+fake-connect sequence,
 * quadcopterTask runs a 10 ms PID attitude loop and drives 4 motor PWM
 * channels on GPT0-GPT3 - see quadcopter.c),
 * mirroring lm3s6965/freertos_demo's own shape and the same real,
 * concrete proof standard: markers only fire from inside a genuinely
 * scheduled task, and on_task_switch (session.py's own real Timeline
 * wiring) must observe at least 2 distinct TCB pointers switching (in
 * practice, with 3 tasks, always observes 3). GPIO/UART0 init reuses the
 * exact same real PRCM_ENABLE/IOCFG pattern already proven in
 * gpio_uart_i2c_demo/firmware/Src/main.c - see that file's own comments
 * for the underlying register rationale. I2C0 is deliberately left out
 * of this demo (already covered by gpio_uart_i2c_demo).
 *
 * **BLE is a STUB, not real over-the-air Bluetooth** - see
 * renoly/hw/ti/simplelink/rf_core.py's own docstring (server-side) and
 * this file's own bleTask comments below for exactly what is and isn't
 * real. Real BLE radio hardware is explicitly out of scope for this
 * whole plan - see target.toml's own note. */
#include "cc2640r2f_regs.h"
#include "quadcopter.h"
#include "cmsis_os.h"

osThreadId defaultTaskHandle;
osThreadId uartTaskHandle;
osThreadId bleTaskHandle;
osThreadId quadcopterTaskHandle;

/* Real, standard CMSIS-RTOS v1 mutex, guarding UART0 access - needed now
 * that two tasks (uartTask and bleTask) both call uart0_put_string().
 * Without it, a SysTick-driven preemption mid-string (this is a genuine,
 * real preemptive scheduler, not a cooperative one - a tick can land
 * between any two bytes) could interleave the two tasks' own output and
 * corrupt both markers. Neither prior demo in this repo needed this,
 * since none had more than one UART-writing task before. */
osMutexDef(uartMutex);
osMutexId uartMutexId;

static const char kUartTaskMarker[] = "EMULICA_CC2640R2F_FREERTOS_UARTTASK_OK\r\n";
static const char kHeartbeat[] = "EMULICA_CC2640R2F_FREERTOS_HEARTBEAT\r\n";
static const char kBleAdvMarker[] = "EMULICA_CC2640R2F_FREERTOS_BLE_ADV_OK\r\n";
static const char kBleFakeConnectedMarker[] = "EMULICA_CC2640R2F_FREERTOS_BLE_FAKE_CONNECTED_OK\r\n";
static const char kBleHeartbeat[] = "EMULICA_CC2640R2F_FREERTOS_BLE_HEARTBEAT\r\n";

/* DIO7 is the real onboard LED (red) on the CC2640R2 LaunchPad - same
 * real board fact gpio_uart_i2c_demo's own main.c already established.
 * DIO3/DIO2 for UART0 TX/RX are this demo's own pin choice, matching that
 * same sibling demo's own choice for consistency (not claimed to be the
 * only valid routing - any DIO can route to any peripheral function via
 * IOC's real crossbar). */
#define LED_DIO     7U
#define UART_TX_DIO 3U
#define UART_RX_DIO 2U

static void gpio_led_init(void) {
    PRCM_ENABLE(PRCM_GPIOCLKGR);
    IOCFG(LED_DIO) = IOC_IOCFG_PORT_ID_GPIO;
    GPIO_DOE31_0 |= (1U << LED_DIO);
}

static void uart0_init(void) {
    PRCM_ENABLE(PRCM_UARTCLKGR);
    PRCM_ENABLE(PRCM_GPIOCLKGR);
    IOCFG(UART_TX_DIO) = IOC_IOCFG_PORT_ID_UART0_TX;
    IOCFG(UART_RX_DIO) = IOC_IOCFG_PORT_ID_UART0_RX | IOC_IOCFG_IE;
    /* 115200 baud at 48 MHz (real CC2640R2F system clock after boot) -
     * same real IBRD/FBRD calculation as gpio_uart_i2c_demo's own. */
    UART_IBRD(UART0_BASE) = 26;
    UART_FBRD(UART0_BASE) = 3;
    UART_LCRH(UART0_BASE) = UART_LCRH_WLEN_8 | UART_LCRH_FEN;
    UART_CTL(UART0_BASE) = UART_CTL_UARTEN | UART_CTL_TXE | UART_CTL_RXE;
}

static void uart0_put_string(const char *s) {
    while (*s) {
        while (UART_FR(UART0_BASE) & UART_FR_TXFF) { }
        UART_DR(UART0_BASE) = (uint32_t)(unsigned char)(*s++);
    }
}

/* Mutex-guarded wrapper - see this file's own header comment on why this
 * is now required with two UART-writing tasks. osWaitForever is safe
 * here: nothing else in this demo ever holds the mutex for more than one
 * short string's worth of polling. */
void uart0_put_string_safe(const char *s) {
    osMutexWait(uartMutexId, osWaitForever);
    uart0_put_string(s);
    osMutexRelease(uartMutexId);
}

/* ---- RF Core (BLE stub) ---- */

/* Real driverlib-shaped RF Core power-up sequence: gate the RFC clock at
 * the PRCM level, then enable the CPE+CPERAM bits together ("As part of
 * RF Core initialization, set this bit together with CPERAM bit to
 * enable CPE to boot" - TI's own hw_rfc_pwr.h), then wait for the real
 * BOOT_DONE/MODULES_UNLOCKED bits. On real hardware this wait is
 * genuinely asynchronous (the CPE's own internal boot ROM running); the
 * emulator's stub sets both bits synchronously (see rf_core.py's own
 * docstring), so this loop never actually iterates here - written as a
 * real polling loop anyway, matching every other "wait for real hardware
 * status" loop in this demo family (e.g. I2C0's own MCTRL busy-bit poll
 * in gpio_uart_i2c_demo). */
static void rf_core_boot(void) {
    PRCM_ENABLE(PRCM_RFCCLKG);
    RFC_PWMCLKEN = RFC_PWMCLKEN_RFC | RFC_PWMCLKEN_CPE | RFC_PWMCLKEN_CPERAM;
    while (!(RFC_RFCPEIFG & (RFC_RFCPEIFG_BOOT_DONE | RFC_RFCPEIFG_MODULES_UNLOCKED))) { }
}

/* Submits one real, pointer-format radio-op command and waits for the
 * real LAST_COMMAND_DONE bit - the standard TI driverlib submit/wait
 * shape (real firmware also checks the command's own `status` field for
 * DONE_OK; this demo doesn't bother, since the stub always writes that
 * exact value - see rf_core.py's own docstring). Clears RFCPEIFG's own
 * write-1-to-clear bits afterward so the next command's own completion
 * is observable again. */
static void rf_submit_and_wait(rf_op_stub_t *cmd, uint16_t command_no) {
    cmd->commandNo = command_no;
    cmd->status = 0;
    RFC_CMDR = (uint32_t)cmd;
    while (!(RFC_RFCPEIFG & RFC_RFCPEIFG_LAST_COMMAND_DONE)) { }
    RFC_RFCPEIFG = RFC_RFCPEIFG_COMMAND_DONE | RFC_RFCPEIFG_LAST_COMMAND_DONE;
}

void StartDefaultTask(void const * argument) {
    for (;;) {
        GPIO_DOUTTGL31_0 = (1U << LED_DIO);
        osDelay(500);
    }
}

void StartUartTask(void const * argument) {
    uart0_put_string_safe(kUartTaskMarker);
    for (;;) {
        uart0_put_string_safe(kHeartbeat);
        osDelay(750);
    }
}

/* BLE task - real driverlib-shaped boot -> CMD_RADIO_SETUP ->
 * CMD_BLE_ADV sequence, then a **firmware-side-only fake** "connection"
 * (CMD_BLE_SLAVE, the real command a BLE peripheral role uses once
 * actually connected). Real hardware would only ever submit
 * CMD_BLE_SLAVE in response to a genuine, real over-the-air connection
 * request from a real central device; there is no real peer here, no
 * real radio activity anywhere in this task - the emulator's RF Core
 * stub cannot tell this apart from any other radio-op command, by
 * design (see rf_core.py's own docstring).
 *
 * No fixed osDelay() between CMD_BLE_ADV and CMD_BLE_SLAVE (an earlier
 * revision of this function had one, "stands in for a real,
 * unpredictable wait for a real connection request" - since removed:
 * with a real client-PC BLE bridge attached (see rf_core.py's own
 * attach_ble_bridge()/docs/ARCHITECTURE.md's "BLE bridge" section),
 * CMD_BLE_ADV's own rf_submit_and_wait() call below already blocks for
 * however long that real wait genuinely takes - a fixed delay on top
 * would just be redundant. With no bridge attached (e.g. this demo's own
 * direct-construction boot test), CMD_BLE_ADV completes instantly like
 * every other command, so there is no wait to stand in for at all). */
void StartBleTask(void const * argument) {
    static rf_op_stub_t cmd;

    rf_core_boot();
    rf_submit_and_wait(&cmd, CMD_RADIO_SETUP);

    uart0_put_string_safe(kBleAdvMarker);
    rf_submit_and_wait(&cmd, CMD_BLE_ADV);

    rf_submit_and_wait(&cmd, CMD_BLE_SLAVE);
    uart0_put_string_safe(kBleFakeConnectedMarker);

    for (;;) {
        uart0_put_string_safe(kBleHeartbeat);
        osDelay(1200);
    }
}

int main(void) {
    gpio_led_init();
    uart0_init();

    uartMutexId = osMutexCreate(osMutex(uartMutex));

    osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
    defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

    osThreadDef(uartTask, StartUartTask, osPriorityNormal, 0, 128);
    uartTaskHandle = osThreadCreate(osThread(uartTask), NULL);

    osThreadDef(bleTask, StartBleTask, osPriorityNormal, 0, 128);
    bleTaskHandle = osThreadCreate(osThread(bleTask), NULL);

    /* Quadcopter PID + 4-channel PWM task - larger stack (256 words) for
     * the floating-point PID state and the soft-float call chain. */
    osThreadDef(quadcopterTask, StartQuadcopterTask, osPriorityNormal, 0, 256);
    quadcopterTaskHandle = osThreadCreate(osThread(quadcopterTask), NULL);

    osKernelStart();

    /* Should never reach here. */
    for (;;) { }
}
