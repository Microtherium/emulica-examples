/* Minimal, hand-written register-address header for TI Tiva C TM4C123GH6PM -
 * no vendor driverlib/SDK is used for this chip (see target.toml/README.md:
 * same "hand-write native peripherals" decision as LM3S6965 - TI's
 * SysConfig tool doesn't cover this older Tiva C generation either).
 * Addresses match renoly/hw/ti/tiva/{gpio,uart,sysctl}.py's own
 * docstrings exactly (verified against Apache NuttX's real
 * arch/arm/src/tiva/hardware/tm4c/{tm4c_memorymap.h,tm4c123_sysctrl.h} -
 * no QEMU model exists for any Tiva C part, unlike LM3S6965). Just
 * hardware addresses, not driver code - no vendor copyright concern.
 *
 * Genuinely different from LM3S6965's own lm3s6965_regs.h: this chip's
 * real SysCtl clock-gating lives on a newer register generation
 * (RCGCGPIO/RCGCUART, not RCGC1/RCGC2) - see tiva/sysctl.py's own
 * docstring for why. */
#ifndef TM4C123_REGS_H
#define TM4C123_REGS_H

#include <stdint.h>

#define HWREG32(addr) (*(volatile uint32_t *)(addr))

/* ---- System Control (real TI-proprietary, not ARM PrimeCell) ---- */
#define SYSCTL_BASE     0x400FE000UL
#define SYSCTL_RCGCGPIO HWREG32(SYSCTL_BASE + 0x608) /* GPIOA=bit0 .. GPIOF=bit5 */
#define SYSCTL_RCGCUART HWREG32(SYSCTL_BASE + 0x618) /* UART0=bit0 .. UART7=bit7 */
#define SYSCTL_RCGCSSI  HWREG32(SYSCTL_BASE + 0x61C) /* SSI0=bit0 .. SSI3=bit3 */
#define SYSCTL_RCGCI2C  HWREG32(SYSCTL_BASE + 0x620) /* I2C0=bit0 .. I2C3=bit3 */
#define SYSCTL_RCGCADC  HWREG32(SYSCTL_BASE + 0x638) /* ADC0=bit0, ADC1=bit1 */

/* ---- GPIO (ARM PL061) - one base per port, DATA is address-masked ---- */
#define GPIOA_BASE      0x40004000UL
#define GPIOB_BASE      0x40005000UL
#define GPIOC_BASE      0x40006000UL
#define GPIOD_BASE      0x40007000UL
#define GPIOE_BASE      0x40024000UL
#define GPIOF_BASE      0x40025000UL

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

/* ---- UART (ARM PL011) ---- */
#define UART0_BASE      0x4000C000UL

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
#define SSI_SR_RNE       (1U << 2)
#define SSI_SR_BSY       (1U << 4)

#endif /* TM4C123_REGS_H */
