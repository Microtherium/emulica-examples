/* TI SimpleLink CC2652R1 - real, hand-written bare-metal demo (no vendor
 * driverlib - see target.toml/README.md), fourth TI chip under this
 * plan, second SimpleLink-family one. GPIO blink (DIO7 - the real green
 * LED on the CC26X2R1 LaunchPad) + UART0 print + I2C0 bus scan - matching
 * CC2640R2F's own initial-scope demo exactly, since every peripheral
 * involved is the exact same real register set (see the renoly/hw/ti/
 * simplelink module's own docstrings). Real 802.15.4/BLE radio hardware
 * is deliberately untouched. */
#include "cc2652r1_regs.h"

static const char kMarker[] = "EMULICA_CC2652R1_GPIO_UART_I2C_DEMO_OK\r\n";
/* I2C0 bus scan marker - this board has no fixed onboard I2C device
 * (same honesty as every other TI chip's own scan in this repo), so
 * every probed address is expected to NACK. Proves the real I2C0 MSA/
 * MCTRL START-condition code path runs to completion without hanging or
 * faulting. */
static const char kI2cScanMarker[] = "EMULICA_CC2652R1_I2C_SCAN_OK\r\n";
static const unsigned char kI2cScanAddresses[] = {0x50, 0x68, 0x76};

/* DIO7 is the real green LED on the CC26X2R1 LaunchPad (DIO6 is the real
 * red LED - a genuinely different board layout from CC2640R2F's own
 * single-LED board, confirmed via contiki-ng's own real
 * CC26X2R1_LAUNCHXL.h board header). DIO3/DIO2 for UART0 TX/RX and
 * DIO4/DIO5 for I2C0 SCL/SDA are this demo's own pin choice, matching
 * CC2640R2F's own sibling demo's choice for consistency (not claimed to
 * match this board's own real silkscreen for these three). */
#define LED_DIO   7U
#define UART_TX_DIO 3U
#define UART_RX_DIO 2U
#define I2C_SCL_DIO 4U
#define I2C_SDA_DIO 5U

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
    /* 115200 baud at 48 MHz (real CC2652R1 system clock after boot, same
     * real value as CC2640R2F's own - not functionally exercised by this
     * emulator yet, written correctly anyway). */
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

static void i2c0_init(void) {
    PRCM_ENABLE(PRCM_I2CCLKGR);
    PRCM_ENABLE(PRCM_GPIOCLKGR);
    IOCFG(I2C_SCL_DIO) = IOC_IOCFG_PORT_ID_I2C_MSSCL | IOC_IOCFG_IE | IOC_IOCFG_PULL_CTL_UP;
    IOCFG(I2C_SDA_DIO) = IOC_IOCFG_PORT_ID_I2C_MSSDA | IOC_IOCFG_IE | IOC_IOCFG_PULL_CTL_UP;
    I2C0_MTPR = 0x17; /* real ~100 kHz divisor at 48 MHz - not functionally
                        * exercised yet, written correctly anyway. */
}

static void i2c0_scan(void) {
    for (unsigned int i = 0; i < sizeof(kI2cScanAddresses); i++) {
        I2C0_MSA = (uint32_t)(kI2cScanAddresses[i] << 1);
        I2C0_MCTRL = I2C_MCTRL_RUN | I2C_MCTRL_START | I2C_MCTRL_STOP;
        while (I2C0_MCTRL & 0x1) { } /* real BUSY bit, bit0 on read */
    }
    uart0_put_string(kI2cScanMarker);
}

static void spin_delay(volatile uint32_t count) {
    while (count--) { }
}

int main(void) {
    gpio_led_init();
    uart0_init();
    i2c0_init();

    uart0_put_string(kMarker);
    i2c0_scan();

    for (;;) {
        GPIO_DOUTTGL31_0 = (1U << LED_DIO);
        spin_delay(50000);
    }
}
