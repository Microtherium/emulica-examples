/* TI SimpleLink CC2640R2F - real, hand-written bare-metal demo (no
 * vendor driverlib - see target.toml/README.md), third TI chip under
 * this plan, first SimpleLink-family one. GPIO blink (DIO7 - the real
 * onboard LED on the CC2640R2 LaunchPad) + UART0 print + I2C0 bus scan -
 * matching every other TI chip's own "GPIO+UART(+I2C)" initial-scope
 * demo, proving the real, genuinely different unified-GPIO/IOC/PRCM
 * architecture (see renoly/hw/ti/simplelink - gpio.py/ioc.py/prcm.py -
 * own docstrings) actually works end to end through a real compiled+linked+booted
 * firmware image, not a hand-assembled register probe. Real BLE radio
 * hardware is deliberately untouched - see target.toml's own note. */
#include "cc2640r2f_regs.h"

static const char kMarker[] = "EMULICA_CC2640R2F_GPIO_UART_I2C_DEMO_OK\r\n";
/* I2C0 bus scan marker - this board has no fixed onboard I2C device
 * (same honesty as every other TI chip's own scan in this repo), so
 * every probed address is expected to NACK. Proves the real I2C0 MSA/
 * MCTRL START-condition code path runs to completion without hanging or
 * faulting. */
static const char kI2cScanMarker[] = "EMULICA_CC2640R2F_I2C_SCAN_OK\r\n";
static const unsigned char kI2cScanAddresses[] = {0x50, 0x68, 0x76};

/* DIO7 is the real onboard LED (red) on the CC2640R2 LaunchPad - a
 * standard, widely-documented board wiring. DIO3/DIO2 for UART0 TX/RX
 * and DIO4/DIO5 for I2C0 SCL/SDA are this demo's own pin choice (any DIO
 * can route to any peripheral function via IOC's real crossbar - see
 * cc2640r2f_regs.h's own PORT_ID comment - not claimed to match a
 * specific LaunchPad's silkscreen for these three). */
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
    /* 115200 baud at 48 MHz (real CC2640R2F system clock after boot) -
     * not functionally exercised by this emulator yet, written correctly
     * anyway, same convention as every other UART demo in this repo. */
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
