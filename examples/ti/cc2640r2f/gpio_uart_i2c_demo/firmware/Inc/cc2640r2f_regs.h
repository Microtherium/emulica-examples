/* Minimal, hand-written register-address header for TI SimpleLink
 * CC2640R2F - no vendor driverlib/SDK is used for this chip (see
 * target.toml/README.md: TI's SysConfig tool, the real code generator
 * for this family, isn't a practical local dependency to pin - matches
 * every other TI chip's own "hand-write native peripherals" decision).
 * Addresses match renoly/hw/ti/simplelink/{gpio,uart,i2c,prcm,ioc}.py's
 * own docstrings exactly (verified against TI's own real, official
 * CC2640R2 SDK header files, mirrored by contiki-ng/cc2640r2-sdk - no
 * QEMU model or NuttX/Zephyr hardware support exists for this exact
 * part). Just hardware addresses, not driver code - no vendor copyright
 * concern. */
#ifndef CC2640R2F_REGS_H
#define CC2640R2F_REGS_H

#include <stdint.h>

#define HWREG32(addr) (*(volatile uint32_t *)(addr))

/* ---- PRCM (Power, Reset, and Clock Management) ---- */
#define PRCM_BASE        0x40082000UL
#define PRCM_CLKLOADCTL  HWREG32(PRCM_BASE + 0x28)
#define PRCM_GPIOCLKGR   HWREG32(PRCM_BASE + 0x48)
#define PRCM_I2CCLKGR    HWREG32(PRCM_BASE + 0x60)
#define PRCM_UARTCLKGR   HWREG32(PRCM_BASE + 0x6C)

#define PRCM_CLKLOADCTL_LOAD      (1U << 0)
#define PRCM_CLKLOADCTL_LOAD_DONE (1U << 1)

/* Real, standard driverlib idiom: set a *CLKGR bit, commit it via
 * CLKLOADCTL.LOAD, then poll LOAD_DONE - see prcm.py's own docstring on
 * why this emulator's model doesn't require the commit step but real
 * firmware still performs it correctly (safe either way). */
#define PRCM_ENABLE(reg) do { \
    (reg) |= 1U; \
    PRCM_CLKLOADCTL = PRCM_CLKLOADCTL_LOAD; \
    while (!(PRCM_CLKLOADCTL & PRCM_CLKLOADCTL_LOAD_DONE)) { } \
} while (0)

/* ---- IOC (I/O Controller - pin mux) ---- */
#define IOC_BASE 0x40081000UL
#define IOCFG(n) HWREG32(IOC_BASE + ((n) * 4U))

#define IOC_IOCFG_IE           (1U << 29)
#define IOC_IOCFG_PULL_CTL_UP  0x00004000U
#define IOC_IOCFG_PULL_CTL_DIS 0x00006000U
#define IOC_IOCFG_PORT_ID_GPIO       0x00U
#define IOC_IOCFG_PORT_ID_UART0_TX   0x10U
#define IOC_IOCFG_PORT_ID_UART0_RX   0x0FU
#define IOC_IOCFG_PORT_ID_I2C_MSSCL  0x0EU
#define IOC_IOCFG_PORT_ID_I2C_MSSDA  0x0DU

/* ---- GPIO (single unified peripheral, one bit per DIO) ---- */
#define GPIO_BASE       0x40022000UL
#define GPIO_DOUT31_0    HWREG32(GPIO_BASE + 0x80)
#define GPIO_DOUTSET31_0 HWREG32(GPIO_BASE + 0x90)
#define GPIO_DOUTCLR31_0 HWREG32(GPIO_BASE + 0xA0)
#define GPIO_DOUTTGL31_0 HWREG32(GPIO_BASE + 0xB0)
#define GPIO_DIN31_0     HWREG32(GPIO_BASE + 0xC0)
#define GPIO_DOE31_0     HWREG32(GPIO_BASE + 0xD0)

/* ---- UART0 (ARM PL011, same real IP as Stellaris/Tiva's own) ---- */
#define UART0_BASE  0x40001000UL
#define UART_DR(base)     HWREG32((base) + 0x00)
#define UART_FR(base)     HWREG32((base) + 0x18)
#define UART_IBRD(base)   HWREG32((base) + 0x24)
#define UART_FBRD(base)   HWREG32((base) + 0x28)
#define UART_LCRH(base)   HWREG32((base) + 0x2C)
#define UART_CTL(base)    HWREG32((base) + 0x30)

#define UART_FR_TXFF    (1U << 5)
#define UART_LCRH_FEN   (1U << 4)
#define UART_LCRH_WLEN_8 (0x3U << 5)
#define UART_CTL_UARTEN (1U << 0)
#define UART_CTL_TXE    (1U << 8)
#define UART_CTL_RXE    (1U << 9)

/* ---- I2C0 (same real TI I2C IP as Stellaris/Tiva, Master bank at a
 * genuinely different offset - 0x800, not 0x000 - see i2c.py's own
 * docstring) ---- */
#define I2C0_BASE   0x40002000UL
#define I2C0_MSA    HWREG32(I2C0_BASE + 0x800)
#define I2C0_MCTRL  HWREG32(I2C0_BASE + 0x804)
#define I2C0_MDR    HWREG32(I2C0_BASE + 0x808)
#define I2C0_MTPR   HWREG32(I2C0_BASE + 0x80C)

#define I2C_MCTRL_RUN   (1U << 0)
#define I2C_MCTRL_START (1U << 1)
#define I2C_MCTRL_STOP  (1U << 2)

#endif /* CC2640R2F_REGS_H */
