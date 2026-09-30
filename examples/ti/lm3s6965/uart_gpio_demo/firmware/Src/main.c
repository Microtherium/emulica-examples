/* TI Stellaris LM3S6965EVB - real, hand-written bare-metal demo (no
 * vendor SDK/driverlib - see target.toml/README.md for why). Boot-
 * verification bar matches every other chip in this repo: proves the
 * real PL061/PL011/SysCtl/I2C/ADC/SSI register models genuinely work
 * together through a real compiled+linked+booted firmware image, not a
 * hand-assembled register probe. Mirrors STM32L476's own uart_gpio_demo
 * shape (UART marker -> I2C scan -> ADC internal-channel read -> SPI
 * self-loopback -> GPIO blink loop), the "make it like L476" pass. */
#include "lm3s6965_regs.h"

static const char kMarker[] = "EMULICA_LM3S6965_UART_GPIO_DEMO_OK\r\n";
/* I2C0 bus scan marker - this board has no fixed onboard I2C device
 * (same honesty as stm32l4_i2c.py's own uart_gpio_demo scan), so every
 * probed address is expected to NACK. Proves the real I2C0 MSA/MCS
 * START-condition code path runs to completion without hanging or
 * faulting - see i2c.py's own docstring. */
static const char kI2cScanMarker[] = "EMULICA_LM3S6965_I2C_SCAN_OK\r\n";
static const unsigned char kI2cScanAddresses[] = {0x50, 0x68, 0x76};
/* ADC0 internal temperature-sensor marker - the genuinely-onboard, zero-
 * external-wiring channel this chip has (SSCTL0.TS), matching STM32's
 * own internal-VREFINT-channel honesty for the same class of board. */
static const char kAdcMarker[] = "EMULICA_LM3S6965_ADC_OK\r\n";
/* SSI0 self-loopback marker - no fixed onboard SPI device either (same
 * honest constraint as I2C above); the real, zero-wiring proof is a
 * loopback - the marker only fires if the received byte genuinely
 * matched what was sent, not unconditionally. */
static const char kSpiMarker[] = "EMULICA_LM3S6965_SPI_OK\r\n";
#define SSI0_DEMO_BYTE 0x5AU

static void uart0_init(void) {
    SYSCTL_RCGC1 |= (1U << 0);
    SYSCTL_RCGC2 |= (1U << 0);
    GPIO_AFSEL(GPIOA_BASE) |= (GPIO_PIN_0 | GPIO_PIN_1);
    GPIO_DEN(GPIOA_BASE) |= (GPIO_PIN_0 | GPIO_PIN_1);
    /* 115200 baud at 16 MHz: IBRD=8, FBRD=44 - see this file's own
     * earlier version for the real formula (not functionally exercised
     * by this emulator yet, written correctly anyway). */
    UART_IBRD(UART0_BASE) = 8;
    UART_FBRD(UART0_BASE) = 44;
    UART_LCRH(UART0_BASE) = UART_LCRH_WLEN_8 | UART_LCRH_FEN;
    UART_CTL(UART0_BASE) = UART_CTL_UARTEN | UART_CTL_TXE | UART_CTL_RXE;
}

static void uart0_put_char(char c) {
    while (UART_FR(UART0_BASE) & UART_FR_TXFF) { }
    UART_DR(UART0_BASE) = (uint32_t)(unsigned char)c;
}

static void uart0_put_string(const char *s) {
    while (*s) {
        uart0_put_char(*s++);
    }
}

static void gpiof_init(void) {
    SYSCTL_RCGC2 |= (1U << 5);
    GPIO_DIR(GPIOF_BASE) |= GPIO_PIN_0;
    GPIO_DEN(GPIOF_BASE) |= GPIO_PIN_0;
}

static void i2c0_init(void) {
    /* I2C0 clock gate is RCGC1 bit12; its SCL/SDA pins are PB2/PB3 on
     * this board - real, standard Stellaris/Tiva pin assignment. */
    SYSCTL_RCGC1 |= (1U << 12);
    SYSCTL_RCGC2 |= (1U << 1); /* GPIOB */
    GPIO_AFSEL(GPIOB_BASE) |= (GPIO_PIN_2 | GPIO_PIN_3);
    GPIO_DEN(GPIOB_BASE) |= (GPIO_PIN_2 | GPIO_PIN_3);
    I2C0_MTPR = 0x07; /* real 100 kHz divisor at 16 MHz - not functionally
                        * exercised by this emulator yet, written correctly
                        * anyway (matches every other unpatched-timing
                        * register in this repo's demos). */
}

static void i2c0_scan(void) {
    for (unsigned int i = 0; i < sizeof(kI2cScanAddresses); i++) {
        I2C0_MSA = (uint32_t)(kI2cScanAddresses[i] << 1);
        I2C0_MCS = I2C_MCS_RUN | I2C_MCS_START | I2C_MCS_STOP;
        while (I2C0_MCS & 0x1) { } /* real BUSY bit, bit0 on read */
    }
    uart0_put_string(kI2cScanMarker);
}

static void adc0_init(void) {
    SYSCTL_RCGC0 |= (1U << 16);
    ADC0_ACTSS &= ~ADC_ACTSS_ASEN0; /* disable SS0 while configuring, real requirement */
    ADC0_SSMUX0 = 0;
    ADC0_SSCTL0 = ADC_SSCTL0_TS0 | ADC_SSCTL0_IE0 | ADC_SSCTL0_END0;
    ADC0_ACTSS |= ADC_ACTSS_ASEN0;
}

static void adc0_read_temp(void) {
    ADC0_PSSI = ADC_PSSI_SS0;
    while (!(ADC0_RIS & ADC_RIS_INR0)) { }
    (void)ADC0_SSFIFO0;
    ADC0_ISC = ADC_ISC_IN0;
    uart0_put_string(kAdcMarker);
}

static void ssi0_init(void) {
    /* SSI0 clock gate is RCGC1 bit4; CLK/FSS/RX/TX are PA2-PA5 on this
     * board - real, standard Stellaris/Tiva pin assignment. */
    SYSCTL_RCGC1 |= (1U << 4);
    GPIO_AFSEL(GPIOA_BASE) |= (GPIO_PIN_2 | GPIO_PIN_3);
    GPIO_AFSEL(GPIOA_BASE) |= 0x30; /* PA4/PA5 - beyond the header's named PIN_0/1 constants */
    GPIO_DEN(GPIOA_BASE) |= (GPIO_PIN_2 | GPIO_PIN_3) | 0x30;

    SSI_CR1(SSI0_BASE) &= ~SSI_CR1_SSE; /* disable while configuring, real requirement */
    SSI_CPSR(SSI0_BASE) = 8; /* real clock prescale - not functionally exercised yet */
    SSI_CR0(SSI0_BASE) = SSI_CR0_FRF_MOTO | SSI_CR0_DSS_8BIT;
    SSI_CR1(SSI0_BASE) |= SSI_CR1_SSE;
}

static void ssi0_loopback(void) {
    while (SSI_SR(SSI0_BASE) & SSI_SR_BSY) { }
    SSI_DR(SSI0_BASE) = SSI0_DEMO_BYTE;
    while (!(SSI_SR(SSI0_BASE) & SSI_SR_RNE)) { }
    uint32_t rx = SSI_DR(SSI0_BASE);
    if ((rx & 0xFF) == SSI0_DEMO_BYTE) {
        uart0_put_string(kSpiMarker);
    }
}

static void spin_delay(volatile uint32_t count) {
    while (count--) { }
}

int main(void) {
    gpiof_init();
    uart0_init();
    i2c0_init();
    adc0_init();
    ssi0_init();

    uart0_put_string(kMarker);
    i2c0_scan();
    adc0_read_temp();
    ssi0_loopback();

    for (;;) {
        GPIO_DATA(GPIOF_BASE, GPIO_PIN_0) ^= GPIO_PIN_0;
        spin_delay(50000);
    }
}
