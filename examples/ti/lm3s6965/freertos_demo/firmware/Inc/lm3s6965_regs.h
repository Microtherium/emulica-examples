/* Minimal, hand-written register-address header for TI Stellaris LM3S6965 -
 * no vendor driverlib/SDK is used for this chip (see target.toml/README.md:
 * Stellaris predates TI's SysConfig tool, and its old CCS/StellarisWare
 * workflow isn't a practical local dependency to pin - matches
 * plans/mcu-emulation/ti.md's "Decided: (b), hand-write native
 * peripherals" for the emulator side, applied here to the demo firmware
 * too). Addresses match renoly/hw/ti/stellaris/{gpio,uart,sysctl}.py's own
 * docstrings exactly (verified against QEMU's hw/gpio/pl061.c, hw/char/
 * pl011.c, hw/arm/stellaris.c - this repo's reference-manual-equivalent
 * oracle for this chip). Just hardware addresses, not driver code - no
 * vendor copyright concern. */
#ifndef LM3S6965_REGS_H
#define LM3S6965_REGS_H

#include <stdint.h>

#define HWREG32(addr) (*(volatile uint32_t *)(addr))

/* ---- System Control (real TI-proprietary, not ARM PrimeCell) ---- */
#define SYSCTL_BASE     0x400FE000UL
#define SYSCTL_RCGC0    HWREG32(SYSCTL_BASE + 0x100) /* ADC=bit16 */
#define SYSCTL_RCGC1    HWREG32(SYSCTL_BASE + 0x104) /* UART0=bit0, SSI0=bit4, I2C0=bit12 */
#define SYSCTL_RCGC2    HWREG32(SYSCTL_BASE + 0x108) /* GPIOA=bit0 .. GPIOG=bit6 */

/* ---- GPIO (ARM PL061) - one base per port, DATA is address-masked ---- */
#define GPIOA_BASE      0x40004000UL
#define GPIOB_BASE      0x40005000UL
#define GPIOC_BASE      0x40006000UL
#define GPIOD_BASE      0x40007000UL
#define GPIOE_BASE      0x40024000UL
#define GPIOF_BASE      0x40025000UL
#define GPIOG_BASE      0x40026000UL

/* Real PL061 idiom: GPIODATA isn't one fixed register - address bits
 * [9:2] act as a live per-access bitmask selecting which pins the access
 * affects. GPIO_DATA(base, pinmask) computes the right address, matching
 * TI's own GPIOPinWrite/GPIOPinRead (HWREG(base + (pins << 2))). */
#define GPIO_DATA(base, pinmask) HWREG32((base) + ((uint32_t)(pinmask) << 2))
#define GPIO_DIR(base)    HWREG32((base) + 0x400)
#define GPIO_AFSEL(base)  HWREG32((base) + 0x420)
#define GPIO_DEN(base)    HWREG32((base) + 0x51C)

#define GPIO_PIN_0  0x01
#define GPIO_PIN_1  0x02
#define GPIO_PIN_2  0x04
#define GPIO_PIN_3  0x08

/* ---- UART (ARM PL011, "Luminary" variant) ---- */
#define UART0_BASE      0x4000C000UL
#define UART1_BASE      0x4000D000UL
#define UART2_BASE      0x4000E000UL

#define UART_DR(base)     HWREG32((base) + 0x00)
#define UART_FR(base)     HWREG32((base) + 0x18)
#define UART_IBRD(base)   HWREG32((base) + 0x24)
#define UART_FBRD(base)   HWREG32((base) + 0x28)
#define UART_LCRH(base)   HWREG32((base) + 0x2C)
#define UART_CTL(base)    HWREG32((base) + 0x30)

#define UART_FR_TXFF    (1U << 5)  /* transmit FIFO full */
#define UART_LCRH_FEN   (1U << 4)  /* FIFO enable */
#define UART_LCRH_WLEN_8 (0x3U << 5)
#define UART_CTL_UARTEN (1U << 0)
#define UART_CTL_TXE    (1U << 8)
#define UART_CTL_RXE    (1U << 9)

/* ---- I2C (real TI-proprietary master+slave block) ---- */
#define I2C0_BASE       0x40020000UL
#define I2C0_MSA        HWREG32(I2C0_BASE + 0x00)
#define I2C0_MCS        HWREG32(I2C0_BASE + 0x04)
#define I2C0_MDR        HWREG32(I2C0_BASE + 0x08)
#define I2C0_MTPR       HWREG32(I2C0_BASE + 0x0C)

#define I2C_MCS_RUN     (1U << 0)
#define I2C_MCS_START   (1U << 1)
#define I2C_MCS_STOP    (1U << 2)
#define I2C_MCS_ERROR   (1U << 1) /* on read: address/data NACK */

/* ---- ADC (real TI-proprietary, 4 independent sample sequencers) ---- */
#define ADC0_BASE       0x40038000UL
#define ADC0_ACTSS      HWREG32(ADC0_BASE + 0x00)
#define ADC0_RIS        HWREG32(ADC0_BASE + 0x04)
#define ADC0_ISC        HWREG32(ADC0_BASE + 0x0C)
#define ADC0_PSSI       HWREG32(ADC0_BASE + 0x28)
#define ADC0_SSMUX0     HWREG32(ADC0_BASE + 0x40)
#define ADC0_SSCTL0     HWREG32(ADC0_BASE + 0x44)
#define ADC0_SSFIFO0    HWREG32(ADC0_BASE + 0x48)

#define ADC_ACTSS_ASEN0 (1U << 0)
#define ADC_SSCTL0_TS0  (1U << 3) /* sample the internal temperature sensor */
#define ADC_SSCTL0_IE0  (1U << 2)
#define ADC_SSCTL0_END0 (1U << 1)
#define ADC_RIS_INR0    (1U << 0)
#define ADC_ISC_IN0     (1U << 0)
#define ADC_PSSI_SS0    (1U << 0)

/* ---- SSI (ARM PL022) ---- */
#define SSI0_BASE       0x40008000UL
#define SSI_CR0(base)   HWREG32((base) + 0x00)
#define SSI_CR1(base)   HWREG32((base) + 0x04)
#define SSI_DR(base)    HWREG32((base) + 0x08)
#define SSI_SR(base)    HWREG32((base) + 0x0C)
#define SSI_CPSR(base)  HWREG32((base) + 0x10)

#define SSI_CR0_FRF_MOTO (0U << 4) /* Freescale SPI frame format */
#define SSI_CR0_DSS_8BIT (7U)      /* 8-bit data */
#define SSI_CR1_SSE      (1U << 1) /* SSI enable */
#define SSI_SR_TFE       (1U << 0)
#define SSI_SR_RNE       (1U << 2)
#define SSI_SR_BSY       (1U << 4)

#endif /* LM3S6965_REGS_H */
