/* Minimal, hand-written register-address header for TI MSP432P401R - no
 * vendor DriverLib/SDK is used for this chip (see target.toml/README.md:
 * same "hand-write native peripherals" decision as every other TI chip
 * in this plan). Addresses match renoly/hw/ti/msp432/{dio,eusci,
 * eusci_b,timer_a}.py's own docstrings exactly (verified against TI's
 * own official CMSIS-SVD for this exact part, cross-checked against
 * Zephyr's real drivers/serial/uart_msp432p4xx.c + dts/arm/ti/
 * msp432p4xx.dtsi, and - for the real board pin assignments below -
 * against TI's own real MSP-EXP432P401R LaunchPad support files
 * (Energia's msp432-core variant, variants/MSP_EXP432P401R/{pins.c,
 * Board_init.c}) - see those files for the full citation trail). Just
 * hardware addresses, not driver code - no vendor copyright concern.
 *
 * Genuinely different real IP from every other TI chip's own regs.h in
 * this repo: MSP432's DIO is one peripheral for every port (not one GPIO
 * instance per port like Stellaris/Tiva/SimpleLink), every register here
 * is natively 16-bit (HWREG16, not HWREG32), and this chip has no
 * per-peripheral clock-gate register at all - see dio.py's own
 * docstring points 1-3. This FreeRTOS demo's own scope extends beyond
 * blink_uart_demo's "UART + GPIO": EUSCI_B0 (SPI), EUSCI_B1 (I2C), and
 * TIMER_A0 (this chip's real PWM source - no dedicated PWM peripheral
 * exists on this silicon) are also declared here. */
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

/* P1.5/P1.6/P1.7 - the real EUSCI_B0 SPI pins (SPICLK/SPIMOSI/SPIMISO)
 * on this board, confirmed against pins.c's own comments ("P1.5_SPICLK",
 * "P1.6_SPIMOSI", "P1.7_SPIMISO") in TI's real Energia board support
 * files for the MSP-EXP432P401R LaunchPad. */
#define P1_PIN5  0x0020
#define P1_PIN6  0x0040
#define P1_PIN7  0x0080

/* ---- EUSCI_B0, configured for SPI mode, and EUSCI_B1, configured for
 * I2C mode - same real IP block, mode picked at runtime via UCMODEx (see
 * eusci_b.py's own docstring). ---- */
#define EUSCI_B0_BASE   0x40002000UL
#define EUSCI_B1_BASE   0x40002400UL

#define EUSCI_B_CTLW0(base)  HWREG16((base) + 0x00)
#define EUSCI_B_BRW(base)    HWREG16((base) + 0x06)
#define EUSCI_B_RXBUF(base)  HWREG16((base) + 0x0C)
#define EUSCI_B_TXBUF(base)  HWREG16((base) + 0x0E)
#define EUSCI_B_IE(base)     HWREG16((base) + 0x2A)
#define EUSCI_B_IFG(base)    HWREG16((base) + 0x2C)

#define EUSCI_B_CTLW0_SWRST      0x0001
#define EUSCI_B_CTLW0_UCTXSTT    0x0002
#define EUSCI_B_CTLW0_UCTXSTP    0x0004
#define EUSCI_B_CTLW0_UCMODE_I2C 0x0600 /* UCMODEx = 3 (bits 10:9) */
#define EUSCI_B_CTLW0_SSEL_SMCLK 0x00C0 /* UCSSEL = 3 (SMCLK) */
#define EUSCI_B_IFG_RXIFG0    0x0001
#define EUSCI_B_IFG_TXIFG0    0x0002
#define EUSCI_B_IFG_NACKIFG   0x0020

/* P6.4/P6.5 - the real EUSCI_B1 I2C pins (SDA/SCL) on this board,
 * confirmed against pins.c's own comments ("P6.4_I2CSDA", "P6.5_I2CSCL")
 * and Board_init.c's own i2cMSP432HWAttrs[] entry (.baseAddr =
 * EUSCI_B1_BASE) - real, TI-authored board configuration for this exact
 * LaunchPad, not assumed from a generic MSP432 pin table. Port F pair
 * (P6/... - see dio.py's own docstring for the real pair layout) is not
 * modeled by this chip's own DIO class today (only Port A is - see that
 * class's own docstring); this demo's I2C task therefore does not
 * configure P6 GPIO SEL bits at all (nothing here would enforce them
 * either way), matching this whole codebase's existing "pin-mux is
 * stored, not behaviorally enforced" convention for peripherals whose
 * GPIO port isn't modeled. */

/* ---- TIMER_A0 - this chip's real PWM source (no dedicated PWM
 * peripheral exists on this silicon - see timer_a.py's own docstring).
 * Real P2.4 pin, confirmed against pins.c's own "P2.4_PWM" comment - the
 * exact real GPIO_SEL encoding for routing TA0.1 to P2.4 is a device-
 * datasheet pin-function-table fact this session did not independently
 * re-derive from a machine-readable source (the SVD has no pinmux
 * tables at all - see dio.py's own docstring on what the SVD actually
 * covers); Port B (P2 - see dio.py's own real pair layout) is also not
 * modeled by this chip's own DIO class today, so this demo's PWM task
 * does not attempt a P2SEL0 write either, for the same reason noted
 * above for I2C's P6 pins. ---- */
#define TIMER_A0_BASE   0x40000000UL

#define TA_CTL(base)     HWREG16((base) + 0x00)
#define TA_CCTL1(base)   HWREG16((base) + 0x04)
#define TA_CCR0(base)    HWREG16((base) + 0x12)
#define TA_CCR1(base)    HWREG16((base) + 0x14)

#define TA_CTL_TASSEL_SMCLK  0x0200 /* TASSEL = 2 */
#define TA_CTL_MC_UP         0x0010 /* MC = 1 (up mode) */
#define TA_CTL_TACLR         0x0004
#define TA_CCTL_OUTMOD_RESET_SET 0x00E0 /* OUTMOD = 7 */

#endif /* MSP432P401R_REGS_H */
