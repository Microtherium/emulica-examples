# STM32F429 PWM Demo

Demonstrates PWM signal generation (TIM2 channel 1, PA5) on top of FreeRTOS,
run through the Emulica/Renoly emulator. This project intentionally has no
display/LVGL code - see `st/stm32f429/display_demo` for that - so that
`defaultTask` never blocks on anything expensive and the RTOS scheduler
switches tasks (`defaultTask` <-> `IDLE`) promptly and often, which makes
this a good project for exercising FreeRTOS-aware tooling (e.g. an RTOS
Timeline view).

## What's modeled

- **TIM2 PWM** (`0x40000000`): `MX_TIM2_Init()` configures channel 1
  (`PA5`, `TIM_OCMODE_PWM1`, `ARR` = 999) and `HAL_TIM_PWM_Start()` is called
  from `main()` right after peripheral init. `StartDefaultTask`'s loop then
  ramps the duty cycle (`CCR1`, via `__HAL_TIM_SET_COMPARE`) up from 0 to
  999 and back down every iteration, a triangle wave that produces a
  continuously changing PWM duty cycle. `TIM1` is initialized as a plain
  base timer (no output-compare channel is ever configured on it) and
  `TIM3` isn't used by this firmware at all - `sim.resc`'s watchpoints only
  trace `TIM2`, since that's the only timer whose `CR1`/`ARR`/`CCR1` writes
  are ever observable.
- **SPI5**: `defaultTask` sends a one-time 4-byte handshake, then a 4-byte
  status message every 10th loop iteration - observable via `sim.resc`'s
  `[SPI5 TX]` watchpoint on `SPI5->DR`.
- **FreeRTOS**: a single `defaultTask` plus the idle task. `defaultTask`
  calls `osDelay(1)` every loop iteration, so it yields to `IDLE` on (almost)
  every pass - real, frequent task switching, not a single long-running task.

## Gotchas found while building this demo

1. **`osThreadCreate`'s stack size and the FreeRTOS heap are easy to
   silently exhaust together.** `defaultTask`'s stack is allocated *from*
   `configTOTAL_HEAP_SIZE` (`FreeRTOSConfig.h`) via `pvPortMalloc`. Bumping
   the task's stack without bumping the heap to match makes `xTaskCreate`
   fail silently (`osThreadCreate`'s return value isn't checked in this
   demo) - the idle task then runs forever from boot, with no crash, no
   error, and no sign anything is wrong beyond "nothing in the app ever
   executes".
2. **This project used to include an LVGL display demo (bouncing box, then
   later an analog clock widget) sharing `defaultTask`'s loop with the PWM
   code.** Both were removed: LVGL rendering at the `-O0` build this
   emulator requires is expensive enough (even for a small widget) that
   `lv_timer_handler()`'s first call could run for minutes of wall-clock
   time without returning, during which `defaultTask` never reaches
   `osDelay()` and the RTOS scheduler never switches tasks - see
   `display_demo`'s README for the full writeup of just how expensive this
   gets with an `lv_meter` widget in particular. If you're tempted to add
   display/graphics code back into this project, put it in `display_demo`
   instead so this one stays a fast, reliably-switching RTOS+PWM demo.

## How to verify

- Watch for `[PWM TIM2] Timer STARTED`, `[PWM TIM2] Period ARR=...`, and a
  stream of `[PWM TIM2] CH1 Duty CCR1=...` lines ramping up then back down
  in the simulation log - that's the triangle-wave duty cycle sweep from
  `StartDefaultTask`'s loop.
- Watch for repeating `[SPI5 TX] DR=...` lines - the one-time handshake
  followed by a status byte every 10th loop iteration.
- If you're driving this through an RTOS-aware tool (e.g. reading
  `pxCurrentTCB` directly out of memory once per tick, the way
  `src/gui/live_source.py`'s `LiveSession._poll_current_task` does), you
  should see frequent switching between `defaultTask` and `IDLE` from very
  early in the run - within a couple of virtual seconds, not something you
  need to wait tens of seconds or minutes for.

## Rebuilding the firmware

After changing `firmware/Core/Src/main.c` (or anything else under
`firmware/`), rebuild via `project_runner.py` rather than calling `make`
directly - it regenerates `objects.list` from all current `.mk` fragments,
which a plain `make all` won't do if a new source file's object entry isn't
already in that (gitignored, locally-generated) file:

```
cd Emulica/src/client
python project_runner.py build --project st/stm32f429/pwm_demo --config debug
cp ../client/examples/st/stm32f429/pwm_demo/firmware/Debug/firmware.elf \
   ../client/examples/st/stm32f429/pwm_demo/firmware/firmware.elf
```

(A plain `cd firmware/Debug && make all` still works for iterating on
existing files - e.g. `main.c` - where no new object files are introduced.)
