# STM32F407 I2C Demo

A firmware example for the **STM32F407** microcontroller that exercises the I2C, SPI, and ADC peripherals. It is intended to be loaded and run inside the **Replica / Emulica** simulation environment (powered by Renoly).

## What it does

The main loop runs three demos every second:

| Demo | Peripheral | Description |
|------|-----------|-------------|
| `I2C_Demo` | I2C1 @ 100 kHz | Transmits a byte to a TMP117-compatible sensor at address `0x25`; toggles the blue LED (LD6) on success |
| `ADC_Demo` | ADC1 (12-bit) | Reads channel 1 and forwards the two-byte result over I2C to the same address |
| `SPI_Demo` | SPI1 | Sends a Write-Enable + Page-Program command sequence to an SPI flash device; toggles the orange LED (LD5) on success |

An **Emulica test mode** is activated when `*(uint32_t*)0x10000000 == 0xCAFEBABE`; in that mode fixed sentinel values (`0x74` for I2C, `0x06` for SPI) are used instead of live data, enabling deterministic automated testing.

## Project structure

```
i2c_demo/
├── example.toml          # Replica example manifest (id, target, build config)
├── sim.resc              # Renoly simulation script – attaches a BMP180 sensor,
│                         # patches USB host stubs, adds I2C watchpoints
└── firmware/             # STM32CubeIDE project (HAL-based)
    ├── .cproject / .project
    ├── Core/
    │   ├── Inc/main.h    # Pin definitions, peripheral handles
    │   └── Src/main.c    # Application entry point + I2C/ADC/SPI demo functions
    ├── Drivers/          # CMSIS + STM32F4xx HAL driver sources
    ├── Middlewares/      # ST USB Host library
    ├── sim.resc          # Minimal Renoly script (firmware-only, no sensor model)
    └── Debug/            # Build artefacts (makefile, *.cyclo, *.elf)
```

## Build

Open the `firmware/` directory as an **STM32CubeIDE** workspace and build the **Debug** configuration. The output ELF is placed at:

```
firmware/Debug/stm32f407_example.elf
```

The `example.toml` manifest copies it to `firmware/firmware.elf` when launched through the Replica extension.

## Running in simulation

Launch via the **Replica VS Code extension** (select the `sim` configuration) or run Renode directly:

```bash
renode sim.resc
```

The script:
1. Creates an STM32F407 machine from `../platform.repl`
2. Attaches a simulated **BMP180** I2C sensor at address `0x25`
3. Stubs out `HAL_Delay`, `MX_USB_HOST_Init`, and `MX_USB_HOST_Process` so the simulation does not stall
4. Adds watchpoints that print I2C START/STOP conditions and each transmitted byte to the Renoly monitor
