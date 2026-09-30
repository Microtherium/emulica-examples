/* Minimal, hand-written register-address header for TI MSP432P401R - no
 * vendor DriverLib/SDK is used for this chip (see target.toml/README.md:
 * same "hand-write native peripherals" decision as every other TI chip
 * in this plan). Addresses match renoly/hw/ti/msp432/{dio,eusci}.py's
 * own docstrings exactly (verified against TI's own official CMSIS-SVD
 * for this exact part, cross-checked against Zephyr's real
 * drivers/serial/uart_msp432p4xx.c + dts/arm/ti/msp432p4xx.dtsi - see
 * those files for the full citation trail). Just hardware addresses, not
 * driver code - no vendor copyright concern.
 *
 * Genuinely different real IP from every other TI chip's own regs.h in
 * this repo: MSP432's DIO is one peripheral for every port (not one GPIO
 * instance per port like Stellaris/Tiva/SimpleLink), every register here
 * is natively 16-bit (HWREG16, not HWREG32), and this chip has no
 * per-peripheral clock-gate register at all - see dio.py's own
 * docstring points 1-3. Only Port A (P1/P2) and EUSCI_A0 are declared -
 * this demo's own scope, matching the plan's "UART + GPIO" estimate for
 * this chip. */
#ifndef MSP432P401R_REGS_H
#define MSP432P401R_REGS_H

#include <stdint.h>

#define HWREG16(addr) (*(volatile uint16_t *)(addr))

/* ---- DIO - one peripheral, every port's registers at their own real
 * offset within it (see dio.py's own docstring). Only Port A (P1/P2),
 * this demo's own scope. ---- */
#define DIO_BASE        0x40004C00UL

#define DIO_PAIN        HWREG16(DIO_BASE + 0x00) /* P1IN low byte, P2IN high byte */
#define DIO_PAOUT       HWREG16(DIO_BASE + 0x02)
#define DIO_PADIR       HWREG16(DIO_BASE + 0x04)
#define DIO_PAREN       HWREG16(DIO_BASE + 0x06)
#define DIO_PASEL0      HWREG16(DIO_BASE + 0x0A)
#define DIO_PASEL1      HWREG16(DIO_BASE + 0x0C)
#define DIO_P1IV        HWREG16(DIO_BASE + 0x0E)
#define DIO_PAIES       HWREG16(DIO_BASE + 0x18)
#define DIO_PAIE        HWREG16(DIO_BASE + 0x1A)
#define DIO_PAIFG       HWREG16(DIO_BASE + 0x1C)
#define DIO_P2IV        HWREG16(DIO_BASE + 0x1E)

/* P1.0 - the real MSP-EXP432P401R LaunchPad's red LED1 (standard,
 * widely-documented board wiring; P2.0/P2.1/P2.2 are LED2's RGB channels,
 * not used by this demo, same "keep it minimal" choice every other
 * chip's own blink demo in this repo makes). Low byte of the Port A pair
 * = P1, so no shift needed. */
#define P1_PIN0  0x0001

/* P1.2/P1.3 - the real EUSCI_A0 UART pins on this board (confirmed
 * against Zephyr's own uart_msp432p4xx_init(), which selects exactly
 * these two pins via driverlib for its own UART0 - see eusci.py's own
 * docstring). */
#define P1_PIN2  0x0004
#define P1_PIN3  0x0008

/* ---- EUSCI_A0, configured for UART mode ---- */
#define EUSCI_A0_BASE   0x40001000UL

#define EUSCI_A_CTLW0(base)  HWREG16((base) + 0x00)
#define EUSCI_A_BRW(base)    HWREG16((base) + 0x06)
#define EUSCI_A_MCTLW(base)  HWREG16((base) + 0x08)
#define EUSCI_A_RXBUF(base)  HWREG16((base) + 0x0C)
#define EUSCI_A_TXBUF(base)  HWREG16((base) + 0x0E)
#define EUSCI_A_IE(base)     HWREG16((base) + 0x1A)
#define EUSCI_A_IFG(base)    HWREG16((base) + 0x1C)

#define EUSCI_A_CTLW0_SWRST   0x0001
#define EUSCI_A_CTLW0_SSEL_SMCLK 0x0080 /* UCSSEL = 2 (SMCLK) */
#define EUSCI_A_IE_TXIE       0x0002
#define EUSCI_A_IFG_TXIFG     0x0002

#endif /* MSP432P401R_REGS_H */
