# st.stm32f429.freertos_demo

FreeRTOS feature-coverage demo for the **STM32F429ZI** (Cortex-M4), designed to
run inside the [Emulica / Replica](../../../../) simulation environment. Every
major FreeRTOS synchronization primitive gets its own task pair so the activity
is independently visible in Emulica's **Timeline** panel.

## What it demonstrates

| Primitive | Tasks |
|---|---|
| Queue | `ProducerTask` → `ConsumerTask` |
| Binary semaphore | `PeriodicTimer` (software timer) → `SemaphoreTask` |
| Counting semaphore | `PoolTaskA/B/C` contending for a 2-slot resource pool |
| Mutex | `MutexTaskA` / `MutexTaskB` incrementing a shared counter |
| Software timer | `PeriodicTimer` — 700 ms auto-reload |
| Event group | `EventSetterA/B` → `EventWaiterTask` (wait-for-all) |
| Task notifications | `NotifySenderTask` → `NotifyReceiverTask` |
| Dynamic create/delete | `SpawnerTask` → `OneShotTask` (self-deletes) |

Task activity is observable two ways:
- **LD3 / LD4** onboard LEDs toggle on key events.
- **USART1** (115200 8N1) transmits one byte per queue item consumed.

## Project structure

```
freertos_demo/
├── example.toml              # Emulica example metadata & build config
├── sim.resc                  # Renoly simulation script
└── firmware/                 # STM32CubeIDE project (GNU Tools for STM32 12.3.rel1)
    ├── Core/
    │   ├── Inc/
    │   │   ├── rtos_demo.h   # Public init declaration
    │   │   ├── FreeRTOSConfig.h
    │   │   └── main.h / lv_conf.h / lvgl_port.h / stm32f4xx_*
    │   ├── Src/
    │   │   ├── rtos_demo.c   # ← all demo tasks and RTOS objects
    │   │   ├── main.c        # HAL init, clock/GPIO/UART, calls RTOS_Demo_Init()
    │   │   └── freertos.c    # CMSIS-RTOS hooks (idle, stack overflow, malloc)
    │   └── Startup/          # startup_stm32f429zitx.s
    ├── Drivers/              # CMSIS + STM32F4xx HAL
    ├── Middlewares/          # FreeRTOS, LVGL, USB Host
    └── Debug/                # Build output (Makefile-driven)
```

## Building

**Toolchain required:** `arm-none-eabi-gcc` from
[GNU Tools for STM32](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)
(v12.3.rel1 or later).

```bash
cd firmware/Debug
make all
# Output: firmware/Debug/firmware.elf
```

Or let Emulica drive the build — `example.toml` defines the build command:

```toml
[configs.debug.build]
workdir = "firmware/Debug"
command = ["make", "all"]
artifact = "firmware/Debug/firmware.elf"
```

## Running in simulation

Open the example in Emulica (VS Code extension) with target `st.stm32f429` and
select the `debug` configuration. Emulica will build the firmware if needed,
then launch `sim.resc` in Renoly, which:

1. Loads `firmware.elf` onto the simulated STM32F429.
2. Patches stubs for FMC, USB HOST, and timing HAL functions that have no
   emulation model.
3. Starts the simulation — task switches appear immediately in the Timeline.
