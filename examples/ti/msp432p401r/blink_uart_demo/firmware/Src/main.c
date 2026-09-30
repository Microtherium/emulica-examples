/* TI MSP432P401R (MSP-EXP432P401R LaunchPad) - real, hand-written
 * bare-metal demo (no vendor DriverLib - see target.toml/README.md),
 * fifth TI chip under this plan and the first with a genuinely new
 * peripheral family (eUSCI serial, unified DIO GPIO) since Stellaris/
 * Tiva/SimpleLink. GPIO blink (P1.0, the real onboard red LED1) + UART
 * print via EUSCI_A0 (P1.2/P1.3, the real backchannel UART pins on this
 * board - see msp432p401r_regs.h's own citation trail). This chip's own
 * scope per the plan is "UART + GPIO" - no I2C/ADC/SSI, unlike TM4C123's
 * own extended demo. */
#include "msp432p401r_regs.h"

static const char kMarker[] = "EMULICA_MSP432P401R_BLINK_UART_DEMO_OK\r\n";

static void wdt_hold(void) {
    /* Real MSP432 watchdog is running by default out of reset and must
     * be stopped (or serviced) by firmware - the same first step
     * virtually every real MSP432 program takes. WDTCTL is not modeled
     * by this emulator (renoly/hw/ti/msp432/ has no WDT_A class - no
     * demo here needs a real countdown/reset behavior, matching this
     * whole codebase's "only model what a demo exercises" convention),
     * so this write lands on SystemBus's own generic "unmapped" fallback
     * (a harmless logged warning, not a fault) rather than a real
     * peripheral - written here anyway because real firmware does this,
     * and a missing wdt_hold() would be a misleading demo, not because
     * skipping it would actually break anything in this emulator today.
     * Real WDTCTL = base 0x40004800 + offset 0xC; WDTPW (bits[15:8]) =
     * 0x5A, WDTHOLD = bit 7 - both confirmed against TI's own SVD (see
     * renoly/hw/ti/msp432/dio.py's docstring for the SVD citation). */
    *(volatile uint16_t *)(0x40004800UL + 0xC) = 0x5A80;
}

static void gpio_init(void) {
    /* P1.0 as output, GPIO function (PASEL0/1 already 0 at reset). */
    DIO_PADIR |= P1_PIN0;
}

static void uart0_init(void) {
    /* Real eUSCI_A configuration sequence (TI's own driverlib does the
     * same three steps in this order): hold the module in reset, select
     * the pins' peripheral function, configure, then release reset. */
    EUSCI_A_CTLW0(EUSCI_A0_BASE) = EUSCI_A_CTLW0_SWRST;
    DIO_PASEL0 |= (P1_PIN2 | P1_PIN3); /* primary module function = EUSCI_A0 */

    /* 9600 baud from a 3 MHz SMCLK (real MSP432 default DCO-derived
     * SMCLK after reset) - real oversampling-mode BRW/MCTLW values from
     * TI's own eUSCI eusci_a_uart.c#UART_initModule reference tables.
     * Not functionally exercised by this emulator's own TX path yet
     * (matches every other UART class in this repo's "always ready, no
     * real shift-register timing" stub - see eusci.py's own docstring),
     * written correctly anyway. */
    EUSCI_A_CTLW0(EUSCI_A0_BASE) |= EUSCI_A_CTLW0_SSEL_SMCLK;
    EUSCI_A_BRW(EUSCI_A0_BASE) = 19;
    EUSCI_A_MCTLW(EUSCI_A0_BASE) = 0x4A00 | 0x0001; /* UCBRF=10, UCOS16=1 */

    EUSCI_A_CTLW0(EUSCI_A0_BASE) &= ~EUSCI_A_CTLW0_SWRST;
}

static void uart0_put_char(char c) {
    while (!(EUSCI_A_IFG(EUSCI_A0_BASE) & EUSCI_A_IFG_TXIFG)) { }
    EUSCI_A_TXBUF(EUSCI_A0_BASE) = (uint16_t)(unsigned char)c;
}

static void uart0_put_string(const char *s) {
    while (*s) {
        uart0_put_char(*s++);
    }
}

static void spin_delay(volatile uint32_t count) {
    while (count--) { }
}

int main(void) {
    wdt_hold();
    gpio_init();
    uart0_init();

    uart0_put_string(kMarker);

    for (;;) {
        DIO_PAOUT ^= P1_PIN0;
        spin_delay(50000);
    }
}
