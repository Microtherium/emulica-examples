/* TI SimpleLink CC2652R1 - real, hand-written bare-metal FreeRTOS demo
 * (CMSIS-RTOS v1, no vendor SDK - see target.toml/README.md). Three real
 * tasks (defaultTask blinks DIO7 - the real green LED on the CC26X2R1
 * LaunchPad, uartTask transmits over UART0, bleTask drives the RF Core
 * stub through a real driverlib-shaped boot+advertise+fake-connect
 * sequence), mirroring cc2640r2f/freertos_demo's own shape exactly -
 * every peripheral involved (GPIO/UART0/RF Core) is the exact same real
 * register set, confirmed via direct comparison against TI's own
 * coresdk_cc13xx_cc26xx headers, not assumed from family-name
 * similarity. I2C0 is deliberately left out of this demo (already
 * covered by gpio_uart_i2c_demo).
 *
 * **BLE is a STUB, not real over-the-air Bluetooth** - see
 * renoly/hw/ti/simplelink/rf_core.py's own docstring (server-side). Real
 * BLE/802.15.4 radio hardware is explicitly out of scope for this whole
 * plan - see target.toml's own note. The same real client-PC BLE bridge
 * CC2640R2F's own demo has for CMD_BLE_ADV applies here unchanged too
 * (same real RF Core register set, same server-side hook). */
#include "cc2652r1_regs.h"
#include "cmsis_os.h"

osThreadId defaultTaskHandle;
osThreadId uartTaskHandle;
osThreadId bleTaskHandle;

/* Real, standard CMSIS-RTOS v1 mutex, guarding UART0 access - needed
 * since two tasks (uartTask and bleTask) both call uart0_put_string(),
 * same real race CC2640R2F's own freertos_demo already found and fixed
 * this way. */
osMutexDef(uartMutex);
osMutexId uartMutexId;

static const char kUartTaskMarker[] = "EMULICA_CC2652R1_FREERTOS_UARTTASK_OK\r\n";
static const char kHeartbeat[] = "EMULICA_CC2652R1_FREERTOS_HEARTBEAT\r\n";
static const char kBleAdvMarker[] = "EMULICA_CC2652R1_FREERTOS_BLE_ADV_OK\r\n";
static const char kBleFakeConnectedMarker[] = "EMULICA_CC2652R1_FREERTOS_BLE_FAKE_CONNECTED_OK\r\n";
static const char kBleHeartbeat[] = "EMULICA_CC2652R1_FREERTOS_BLE_HEARTBEAT\r\n";

/* DIO7 is the real green LED on the CC26X2R1 LaunchPad (DIO6 is the real
 * red LED - see gpio_uart_i2c_demo's own main.c comment on this board's
 * genuinely different two-LED layout vs. CC2640R2F's own single-LED
 * board). DIO3/DIO2 for UART0 TX/RX are this demo's own pin choice. */
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
    /* 115200 baud at 48 MHz (real CC2652R1 system clock after boot) -
     * same real IBRD/FBRD calculation as CC2640R2F's own. */
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

/* Mutex-guarded wrapper - see this file's own header comment. */
static void uart0_put_string_safe(const char *s) {
    osMutexWait(uartMutexId, osWaitForever);
    uart0_put_string(s);
    osMutexRelease(uartMutexId);
}

/* ---- RF Core (BLE stub) - identical real sequence to CC2640R2F's own,
 * same real register set. ---- */

static void rf_core_boot(void) {
    PRCM_ENABLE(PRCM_RFCCLKG);
    RFC_PWMCLKEN = RFC_PWMCLKEN_RFC | RFC_PWMCLKEN_CPE | RFC_PWMCLKEN_CPERAM;
    while (!(RFC_RFCPEIFG & (RFC_RFCPEIFG_BOOT_DONE | RFC_RFCPEIFG_MODULES_UNLOCKED))) { }
}

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
 * CMD_BLE_ADV sequence, then a firmware-side-only fake "connection"
 * (CMD_BLE_SLAVE) - see cc2640r2f/freertos_demo's own main.c for the
 * full real-vs-fake reasoning, identical here since it's the exact same
 * real RF Core register set. With the real client-PC BLE bridge attached
 * server-side, CMD_BLE_ADV genuinely waits for this machine's own real
 * Bluetooth adapter to actually advertise and for a real central to
 * connect - not a fixed delay. */
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

    osKernelStart();

    /* Should never reach here. */
    for (;;) { }
}
