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
#define PRCM_RFCCLKG     HWREG32(PRCM_BASE + 0x2C)
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

/* ---- RF Core (RFC_PWR + RFC_DBELL) - a STUB, not a real BLE radio. See
 * renoly/hw/ti/simplelink/rf_core.py's own docstring for exactly what
 * real register-level behavior is and isn't modeled by the emulator on
 * the other end of these addresses; this header only declares the real
 * addresses/bit positions/struct layout themselves (TI's own official
 * inc/hw_rfc_pwr.h, inc/hw_rfc_dbell.h, driverlib/rf_mailbox.h,
 * driverlib/rf_common_cmd.h, driverlib/rf_ble_cmd.h - mirrored by
 * contiki-ng/cc2640r2-sdk). Real BLE radio hardware itself is explicitly
 * out of scope for this whole plan - see target.toml's own note; nothing
 * transmitted via these registers goes anywhere real. */
#define RFC_PWR_BASE     0x40040000UL
#define RFC_PWMCLKEN     HWREG32(RFC_PWR_BASE + 0x00)
#define RFC_PWMCLKEN_RFC    (1U << 0)  /* real HW: can't be cleared */
#define RFC_PWMCLKEN_CPE    (1U << 1)
#define RFC_PWMCLKEN_CPERAM (1U << 2)

#define RFC_DBELL_BASE   0x40041000UL
#define RFC_CMDR         HWREG32(RFC_DBELL_BASE + 0x00)
#define RFC_CMDSTA       HWREG32(RFC_DBELL_BASE + 0x04)
#define RFC_RFCPEIFG     HWREG32(RFC_DBELL_BASE + 0x10)
#define RFC_RFCPEIEN     HWREG32(RFC_DBELL_BASE + 0x14)

#define RFC_RFCPEIFG_COMMAND_DONE      (1U << 0)
#define RFC_RFCPEIFG_LAST_COMMAND_DONE (1U << 1)
#define RFC_RFCPEIFG_MODULES_UNLOCKED  (1U << 29)
#define RFC_RFCPEIFG_BOOT_DONE         (1U << 30)

#define RFC_CMDSTA_DONE  0x01U

/* Real direct-command encoding (driverlib/rf_mailbox.h CMDR_DIR_CMD) -
 * not used by this demo (it only ever submits pointer-format radio-op
 * commands below), kept for completeness/future reuse. */
#define RFC_CMDR_DIR_CMD(cmdId) (((uint32_t)(cmdId) << 16) | 1U)

/* Real TI BLE radio-op command IDs (driverlib/rf_common_cmd.h,
 * rf_ble_cmd.h) - genuinely real opcodes, not invented. This stub never
 * inspects anything beyond commandNo (see rf_core.py's own docstring),
 * so using the real IDs costs nothing and documents actual TI hardware
 * behavior accurately for anyone reading this firmware later. */
#define CMD_RADIO_SETUP  0x0802U
#define CMD_BLE_ADV      0x1803U
#define CMD_BLE_SLAVE    0x1801U  /* real "connected, acting as a BLE
                                    * peripheral" role - see this demo's
                                    * own main.c comment on why entering
                                    * it here is a firmware-side fiction,
                                    * not a real received connection. */

/* Minimal, real-shaped radio-op command header - every real TI radio-op
 * command structure begins with exactly these two fields at these exact
 * offsets (driverlib/rf_common_cmd.h, confirmed identical across every
 * command struct in that file and rf_ble_cmd.h). `reserved` stands in
 * for the real per-command fields (pNextOp/startTime/startTrigger/
 * condition/command-specific parameters) this stub never inspects or
 * needs populated - see rf_core.py's own docstring. */
typedef struct {
    uint16_t commandNo;
    uint16_t status;
    uint8_t  reserved[12];
} __attribute__((aligned(4))) rf_op_stub_t;

/* ---- GPT (General Purpose Timers 0-3) - 16-bit split PWM mode ----
 * CC2640R2F has four GPTs at consecutive 0x1000-spaced addresses.
 * Offsets match TI TRM SWCU117H Table 20-32 (identical to Tiva-C GPT
 * layout - same IP, different base addresses).  Only Timer A registers
 * used here; Timer B left unconfigured. */
#define GPT0_BASE 0x40010000UL
#define GPT1_BASE 0x40011000UL
#define GPT2_BASE 0x40012000UL
#define GPT3_BASE 0x40013000UL

#define GPT_CFG(base)      HWREG32((base) + 0x000) /* GPTMCFG             */
#define GPT_TAMR(base)     HWREG32((base) + 0x004) /* GPTMTAMR            */
#define GPT_CTL(base)      HWREG32((base) + 0x00C) /* GPTMCTL             */
#define GPT_TAILR(base)    HWREG32((base) + 0x028) /* GPTMTAILR (period)  */
#define GPT_TAMATCHR(base) HWREG32((base) + 0x030) /* GPTMTAMATCHR (duty) */
#define GPT_TAPR(base)     HWREG32((base) + 0x038) /* GPTMTAPR (prescale) */

/* GPTMCFG: 0x4 = 16-bit split mode (two independent 16-bit timers) */
#define GPT_CFG_16BIT 0x4U

/* GPTMTAMR: bit[1:0]=0b10 (periodic), bit[3] TAAMS=1 (PWM alternate)
 * → 0b1010 = 0xA enables PWM mode on Timer A. */
#define GPT_TAMR_PWM  0x0000000AUL

/* GPTMCTL bits */
#define GPT_CTL_TAEN    (1U << 0)  /* Timer A enable */
#define GPT_CTL_TAPWML  (1U << 6)  /* Timer A PWM output level (invert) */

/* PRCM GPT clock gating register - bits[3:0] gate GPT0-GPT3 respectively */
#define PRCM_GPTCLKGR  HWREG32(PRCM_BASE + 0x54)

#endif /* CC2640R2F_REGS_H */
