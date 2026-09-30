# STM32F407 CAN RTOS Demo

A firmware example for the **STM32F407** microcontroller that exercises the
CAN, I2C, SPI, and ADC peripherals under a **minimal cooperative RTOS**
scheduler. It is intended to be loaded and run inside the **Replica / Emulica**
simulation environment (powered by Renoly).

## What it does

The `main()` function initialises all peripherals, then hands control to
`MiniRTOS_Run()` – a lightweight cooperative round-robin scheduler implemented
in `can_rtos.c`.  Six tasks are registered before the scheduler starts:

| Period | Task | Peripheral / Action |
|--------|------|---------------------|
| 0 ms (every tick) | `MX_USB_HOST_Process` | USB host maintenance |
| 10 ms | `CAN_Rx_Task` | Poll CAN1 FIFO0, drain SW RX queue, toggle LD4 (green) |
| 100 ms | `CAN_Tx_Task` | Send heartbeat frame (ID 0x123, payload "CAN\<seq\>"), toggle LD3 (orange) |
| 1000 ms | `I2C_Demo` | Transmit a byte to TMP117-compatible sensor at address 0x25; toggle LD6 (blue) |
| 1000 ms | `ADC_Demo` | Read ADC1 channel 1 (12-bit), forward result over I2C |
| 1000 ms | `SPI_Demo` | Write-Enable + Page-Program command to SPI flash; toggle LD5 (red) |

### CAN1 configuration

| Parameter | Value |
|-----------|-------|
| Pins | PD0 = CAN1\_RX, PD1 = CAN1\_TX (AF9) |
| Baud rate | 500 kbps |
| APB1 clock | 42 MHz |
| BTR | BRP=6, TS1=11, TS2=2, SJW=1 |
| Filter | Bank 0, 32-bit mask, accept-all, FIFO0 |

### Mini-RTOS scheduler

`MiniRTOS_Run()` is a cooperative (non-preemptive) scheduler.  Each task is
invoked when `HAL_GetTick() – last_run_tick >= period_ms`.  Tasks must not
block indefinitely; the scheduler itself sleeps `MINI_RTOS_TICK_MS` (5 ms) at
the end of every pass.

Extend the design with a preemptive RTOS (FreeRTOS, Zephyr) when sub-ms
latency or priority inversion protection is required.

### Emulica test mode

Activated when `*(uint32_t*)0x10000000 == 0xCAFEBABE`:

* CAN hardware initialisation (GPIO, clock gate, BTR, filters) is **skipped**.
  The CAN1 register block is mapped as `MappedMemory` in `platform.repl`, so
  register reads/writes succeed without stalling.
* `CAN_Tx_Task` still writes every frame to TX mailbox 0, triggering the
  `sim.resc` watchpoint at `0x40006580` (CAN1 TI0R).
* One synthetic RX frame (`ID=0x456`, payload `"SIM\x01"`) is pre-loaded into
  the software RX queue so the receive path runs on the first `CAN_Rx_Task`
  invocation.
* I2C and SPI tasks use fixed sentinel values (`0x74` and `0x06`) for
  deterministic assertions.

## Project structure

```
.
├── example.toml              Replica example manifest
├── sim.resc                  Renoly script – BMP180 sensor, CAN + I2C watchpoints
├── platform.repl             STM32F407 platform description (includes CAN1 stub)
└── firmware/                 STM32CubeIDE project (HAL-based)
    ├── Core/
    │   ├── Inc/
    │   │   ├── main.h        Pin definitions, peripheral handles, CAN pin defines
    │   │   └── can_rtos.h    Mini-RTOS + CAN types and public API
    │   └── Src/
    │       ├── main.c        Peripheral init + task registration + demo functions
    │       └── can_rtos.c    Cooperative scheduler + CAN1 LL driver + queue
    ├── Drivers/              CMSIS + STM32F4xx HAL driver sources
    ├── Middlewares/          ST USB Host library
    ├── sim.resc              Minimal Renoly script (firmware-only)
    └── Debug/                Build artefacts (makefile, subdir.mk, *.elf)
```

## Build

Open the `firmware/` directory as an **STM32CubeIDE** workspace and build
the **Debug** configuration.  The output ELF is placed at:

```
firmware/Debug/stm32f407_example.elf
```

The `example.toml` manifest copies it to `firmware/firmware.elf` when launched
through the Replica extension.

## Running in simulation

Launch via the **Replica VS Code extension** (select the `sim` configuration)
or run Renode directly:

```bash
renode sim.resc
```

The script:
1. Creates an STM32F407 machine from `platform.repl` (includes the CAN1
   register stub at `0x40006400`).
2. Attaches a simulated **BMP180** I2C sensor at address `0x25`.
3. Stubs `HAL_Delay`, `MX_USB_HOST_Init`, and `MX_USB_HOST_Process` so the
   simulation does not stall.
4. Adds watchpoints that print to the Renoly monitor:
   - **I2C**: START/STOP conditions and each transmitted byte.
   - **CAN TX**: standard ID and TXRQ flag on every TX mailbox 0 write; data
     bytes 0–3 on every TDLR write.

### Example Renoly output

```
[CAN TX] ID=0x123 TXRQ=1
[CAN TX] DLC=4
[CAN TX Data] B0=C B1=A B2=N B3=0x00
[I2C] START condition
[I2C Byte] 0x4A
[I2C] STOP condition
```
