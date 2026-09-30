/* TI Tiva C TM4C123GH6PM (EK-TM4C123GXL LaunchPad) - real, hand-written
 * bare-metal demo (no vendor driverlib - see target.toml/README.md),
 * second TI chip under this plan, first Cortex-M4 one. Extended to match
 * LM3S6965's own uart_gpio_demo scope ("make it like LM3S6965"): GPIOF
 * blink (real onboard RGB LED, PF1=red) + UART0 print + I2C0 bus scan +
 * ADC0 internal-channel read + SSI0 self-loopback - proves the real
 * PL061/PL011/SysCtl/I2C/ADC/SSI register models (including this chip's
 * own genuinely-different RCGCGPIO/RCGCUART/RCGCSSI/RCGCI2C/RCGCADC
 * clock-gate registers) genuinely work together through a real
 * compiled+linked+booted firmware image. */
#include "tm4c123_regs.h"

static const char kMarker[] = "EMULICA_TM4C123_BLINK_UART_DEMO_OK\r\n";
/* I2C0 bus scan marker - this board has no fixed onboard I2C device
 * (same honesty as stellaris/i2c.py's own scan on LM3S6965), so every
 * probed address is expected to NACK. Proves the real I2C0 MSA/MCS
 * START-condition code path runs to completion without hanging or
 * faulting. */
static const char kI2cScanMarker[] = "EMULICA_TM4C123_I2C_SCAN_OK\r\n";
static const unsigned char kI2cScanAddresses[] = {0x50, 0x68, 0x76};
/* ADC0 internal temperature-sensor marker - the genuinely-onboard,
 * zero-external-wiring channel this chip has (SSCTL0.TS), same real
 * ADC IP as LM3S6965's own. */
static const char kAdcMarker[] = "EMULICA_TM4C123_ADC_OK\r\n";
/* SSI0 self-loopback marker - no fixed onboard SPI device either; the
 * real, zero-wiring proof is a loopback - the marker only fires if the
 * received byte genuinely matched what was sent. */
static const char kSpiMarker[] = "EMULICA_TM4C123_SPI_OK\r\n";
#define SSI0_DEMO_BYTE 0x5AU

static void uart0_init(void) {
    /* Real TM4C123 clock-gate registers - RCGCGPIO/RCGCUART, NOT
     * LM3S6965's own RCGC1/RCGC2 (see tiva/sysctl.py's own docstring on
     * why this genuinely differs from Stellaris). */
    SYSCTL_RCGCUART |= (1U << 0); /* UART0 */
    SYSCTL_RCGCGPIO |= (1U << 0); /* GPIOA */
    GPIO_AFSEL(GPIOA_BASE) |= (GPIO_PIN_0 | GPIO_PIN_1);
    GPIO_DEN(GPIOA_BASE) |= (GPIO_PIN_0 | GPIO_PIN_1);
    /* 115200 baud at 16 MHz: same real IBRD/FBRD values as LM3S6965's own
     * demo (identical default internal-oscillator frequency and PL011
     * baud-rate formula) - not functionally exercised by this emulator
     * yet, written correctly anyway. */
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
    /* PF1 - the real onboard RGB LED's red channel on the EK-TM4C123GXL
     * LaunchPad (standard, widely-documented board wiring; PF2=blue,
     * PF3=green, not used by this demo). Deliberately not PF0 - that pin
     * shares a NMI/wake function on real TM4C123 silicon and requires the
     * GPIOLOCK/GPIOCR commit-unlock sequence before DIR can be changed;
     * PF1 needs no such dance, keeping this demo's own scope minimal. */
    SYSCTL_RCGCGPIO |= (1U << 5); /* GPIOF */
    GPIO_DIR(GPIOF_BASE) |= GPIO_PIN_1;
    GPIO_DEN(GPIOF_BASE) |= GPIO_PIN_1;
}

static void i2c0_init(void) {
    /* I2C0 clock gate is RCGCI2C bit0 (NOT LM3S6965's RCGC1 bit12 - see
     * tiva/sysctl.py's own docstring); its SCL/SDA pins are PB2/PB3 on
     * this board, same real, standard Stellaris/Tiva pin assignment. */
    SYSCTL_RCGCI2C |= (1U << 0);
    SYSCTL_RCGCGPIO |= (1U << 1); /* GPIOB */
    GPIO_AFSEL(GPIOB_BASE) |= (GPIO_PIN_2 | GPIO_PIN_3);
    GPIO_DEN(GPIOB_BASE) |= (GPIO_PIN_2 | GPIO_PIN_3);
    I2C0_MTPR = 0x07; /* real 100 kHz divisor at 16 MHz - not functionally
                        * exercised by this emulator yet, written correctly
                        * anyway. */
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
    SYSCTL_RCGCADC |= (1U << 0); /* ADC0 - RCGCADC bit0, not LM3S6965's RCGC0 bit16 */
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
    /* SSI0 clock gate is RCGCSSI bit0 (NOT LM3S6965's RCGC1 bit4); CLK/
     * FSS/RX/TX are PA2-PA5 on this board, same real, standard
     * Stellaris/Tiva pin assignment. */
    SYSCTL_RCGCSSI |= (1U << 0);
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
        GPIO_DATA(GPIOF_BASE, GPIO_PIN_1) ^= GPIO_PIN_1;
        spin_delay(50000);
    }
}
