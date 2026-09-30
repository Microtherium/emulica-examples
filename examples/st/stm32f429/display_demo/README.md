# STM32F429 Display Demo

Demonstrates a real LTDC/DMA2D/LVGL display path - an analog clock face
(`lv_meter` with hour/minute/second needles) rendered into a 240x320 RGB565
screen - run through the Emulica/Renoly emulator.

This project was split off from `st/stm32f429/pwm_demo` specifically because
the clock widget is expensive enough (see gotcha #5 below) that it can stall
the sole FreeRTOS task for a very long time inside its first redraw, which
made `pwm_demo`'s RTOS Timeline view look permanently frozen. Keeping the
clock here isolates that cost so `pwm_demo` stays fast enough to show real
task switching.

## What's modeled

- **LTDC** (`0x40016800`): register storage only (SSCR/BPCR/AWCR/TWCR, GCR,
  SRCR, Layer0 window/pixel-format/framebuffer-address registers, etc.).
  `HAL_LTDC_Init`/`HAL_LTDC_ConfigLayer` only write these registers and never
  block on a hardware flag, so no side effects are needed - there is no
  physical panel in this emulator to synchronously drive.
- **DMA2D** (`0x4002B000`): `CR.START` is modeled with a real synchronous
  fill (R2M) or copy (M2M) against the sysbus, performed inside the register
  write callback (the same pattern `STM32F4_SPI` uses for its DR loopback -
  there is no interrupt-delivery path anywhere in this emulator, so every
  peripheral side effect happens synchronously on the triggering write). The
  transfer-complete flag (`ISR.TCIF`) is set immediately, so
  `HAL_DMA2D_PollForTransfer` resolves on its first check.
- **SDRAM framebuffer** (`0xD0000000`, 2MB `MappedMemory`): backs the FMC
  bank 2 address range the firmware places its LTDC Layer0 framebuffer at
  (RGB565, 240x320, matches `MX_LTDC_Init()`'s `FBStartAdress`).
- **LVGL v8.3.11** (vendored under `firmware/Middlewares/Third_Party/lvgl/`):
  a real graphics library driving an actual widget, ported via
  `firmware/Core/Src/lvgl_port.c`. `LVGL_Init()`/`LVGL_CreateDemo()` run once
  from `main()`; `lv_timer_handler()` runs every iteration of
  `StartDefaultTask`'s loop. `LVGL_CreateDemo()` builds an `lv_meter` clock
  face (0-60 scale, quarter-hour ticks) plus a periodic `lv_timer`
  (`Clock_TimerCb`, ~1.1s period) that advances a virtual time-of-day and
  re-points the hour/minute/second needles. LVGL's display flush writes
  pixels into the framebuffer with a plain `memcpy` (real CPU store
  instructions, not a peripheral register write) - see the "Reading the
  framebuffer back" gotcha below.

## What's NOT modeled

- The FMC/SDRAM controller registers themselves - `MX_FMC_Init` is still
  patched to a no-op in `sim.resc`, since only the backing address range is
  needed for the framebuffer to be writable.
- DMA2D CLUT loading and blending arithmetic.
- A real screen - there is no GUI/hardware sink in this project, so "the
  framebuffer memory holds the correct pixel bytes" is the complete,
  sufficient proof that the display path works; `tools/framebuffer_viewer.py`
  renders a snapshot of it in a Qt window for humans to look at.

## Gotchas found while building this demo

These cost real debugging time - documented so nobody re-discovers them the
hard way:

1. **`HAL_DMA2D_Start`'s `pdata` for R2M mode is `0x00RRGGBB` (8 bits per
   channel), not a pre-packed value in the output `ColorMode`.** HAL converts
   it down to RGB565 internally before writing `OCOLR`. Passing a raw RGB565
   value directly (e.g. `0x0000001F` for blue) is silently misinterpreted as
   R=0x00,G=0x00,B=0x1F and produces `0x1F >> 3 = 3` - a near-black color,
   not an error.
2. **Reading the framebuffer back after a run requires Unicorn's live
   memory, not the Python-side `MappedMemory` object.** Once
   `sync_memory_map()` copies a `MappedMemory` region's initial content into
   Unicorn, Unicorn keeps its own copy for plain-memory regions (see
   `UnicornCore._hook_mem_access` - anything with `memory_view()` bypasses
   the Python callback for speed). A real CPU write - like LVGL's flush
   callback doing `memcpy` into the framebuffer - only ever lands in
   Unicorn's memory; it's never reflected back into the Python object.
   `tools/framebuffer_viewer.py`'s `read_framebuffer()` reads via
   `core.mu.mem_read()` for exactly this reason. This does NOT affect
   register reads on genuine MMIO peripherals (LTDC/DMA2D) - those always
   round-trip through the Python peripheral object by design, so `sysbus`
   reads of registers are fine; it's specifically bulk *memory* content that
   needs the Unicorn-native read path. (Conversely, `Display_DemoFill()`'s
   DMA2D-driven fill, a Python-callback write, updates the Python-side
   object but NOT Unicorn's copy - the two demo mechanisms are only
   consistent with each other when read via the correct path.)
3. **`osThreadCreate`'s stack size and the FreeRTOS heap are easy to
   silently exhaust together.** `defaultTask`'s stack is allocated *from*
   `configTOTAL_HEAP_SIZE` (`FreeRTOSConfig.h`) via `pvPortMalloc`. Bumping
   the task's stack without bumping the heap to match makes `xTaskCreate`
   fail silently (`osThreadCreate`'s return value isn't checked in this
   demo) - the idle task then runs forever from boot, with no crash, no
   error, and no sign anything is wrong beyond "nothing in the app ever
   executes". Heap is `98304` (96KB) and the task stack is `12288` words
   (48KB) here specifically to leave headroom for LVGL's default theme/font
   rendering.
4. **LVGL must be compiled at `-O0` in this emulator.** At `-O1` or `-O2`,
   the firmware reliably crashes a few virtual-seconds in with a jump to
   PC=0 inside LVGL's style-resolution code (`get_prop_core` /
   `_lv_style_get_prop_group` in `lv_obj_style.c`/`lv_style.c`), regardless
   of stack/heap size (tested up to 48KB stack / 96KB heap with no change).
   This looks like a genuine miscompilation or undefined-behavior-only-at-
   optimization bug specific to this GCC 12.3.rel1 + Cortex-M4 + LVGL v8.3
   combination, not a logic error in the port - it wasn't root-caused
   further since `-O0` matches this debug build's existing convention
   anyway (see `Middlewares/Third_Party/lvgl/subdir.mk`'s `-O0` flag).
5. **`lv_meter`'s default circular background (`LV_RADIUS_CIRCLE`, applied by
   the default theme) is pathologically slow to rasterize at `-O0`, and
   `lv_meter` fully redraws its entire widget on any single indicator value
   change.** Root cause: LVGL's corner-radius mask rasterizer
   (`lv_draw_mask.c`, `lv_draw_mask_radius`/`circ_calc_aa4`) took *worse
   than linear* time as radius grew - a 150px `lv_meter`'s ~75px default
   corner radius never finished a single redraw in 100+ virtual seconds,
   while an explicit `lv_obj_set_style_radius(obj, 15, 0)` override finished
   in ~19 virtual seconds. Separately, `lv_meter` has no partial/incremental
   redraw for needle movement - every `lv_meter_set_indicator_value()` call
   invalidates and redraws the *whole* widget (background mask + every tick
   + every needle), so tick-mark count directly multiplies per-update cost.
   `LVGL_CreateDemo()` applies both mitigations available - `radius = 8`
   (down from the default ~75px) and only 5 scale ticks (down from 60 minor
   + 12 major) - but **even with both applied, the very first redraw can
   still take on the order of a minute or more of wall-clock time to
   complete** when driven through the GUI's ~15ms-per-tick stepping budget
   (`live_source.py`'s `STEP_BUDGET_SECONDS`), since that budget is shared
   with everything else the emulator does per tick. Until that first redraw
   returns, `lv_timer_handler()` - and therefore `defaultTask` - never
   yields, so **do not expect FreeRTOS task-switch telemetry (RTOS Timeline/
   CPU Utilization) to show anything moving in this project**; that's
   exactly why the clock lives here instead of in `pwm_demo`. This is not a
   timing/calibration problem - no `lv_timer` period tuning fixes a
   `lv_timer_handler()` call that hasn't returned yet.

## How to verify

- Watch for `[LTDC] GCR LTDCEN set` and `[DMA2D] CR_START fired` /
  `[DMA2D] Transfer complete ...` in the simulation log for the one-time
  DMA2D self-test (`Display_DemoFill()`, a solid blue fill via DMA2D R2M,
  called from `main()` right after all peripheral inits - proof LTDC/DMA2D
  work independently of LVGL; LVGL's first redraw overwrites it once that
  redraw finally completes, which is expected).
- Run `pixi run view-framebuffer -- --project st/stm32f429/display_demo --timeout N`
  with `N` generously large (see gotcha #5 - budget for at least a minute
  of real wall-clock time, more on a loaded machine) to render a snapshot of
  the framebuffer in a Qt window. Before the first redraw completes you'll
  only see the DMA2D self-test's solid blue fill; once it completes you'll
  see the clock face with its needles, and re-running with a larger timeout
  should show the needles having advanced.

## Rebuilding the firmware

After changing `firmware/Core/Src/main.c` (or anything else under
`firmware/`), rebuild via `project_runner.py` rather than calling `make`
directly - it regenerates `objects.list` from all current `.mk` fragments,
which a plain `make all` won't do if a new source file's object entry isn't
already in that (gitignored, locally-generated) file:

```
cd Emulica/src/client
python project_runner.py build --project st/stm32f429/display_demo --config debug
cp ../client/examples/st/stm32f429/display_demo/firmware/Debug/firmware.elf \
   ../client/examples/st/stm32f429/display_demo/firmware/firmware.elf
```

(A plain `cd firmware/Debug && make all` still works for iterating on
existing files - e.g. `main.c` - where no new object files are introduced.)

## Vendoring/updating LVGL

`firmware/Middlewares/Third_Party/lvgl/` was vendored from
`https://github.com/lvgl/lvgl` (tag `v8.3.11`), keeping only `src/`,
`lvgl.h`, `lv_conf_template.h`, and the license file (no `demos/`,
`examples/`, `tests/`, `docs/`, `scripts/`). `firmware/Debug/Middlewares/Third_Party/lvgl/subdir.mk`
is a **hand-written, not CubeIDE-generated** build fragment - LVGL has
dozens of internal subdirectories, which would be tedious to enumerate the
CubeIDE way (see the fragment's header comment for why its `C_SRCS`/`OBJS`
lists are still literal text rather than a `$(shell find ...)` - a dynamic
list isn't compatible with `project_runner.py`'s `_ensure_objects_list()`
regex-based scraper). If LVGL is upgraded or files are added/removed,
regenerate this fragment's `C_SRCS`/`OBJS`/`C_DEPS` lists (a directory walk
over `firmware/Middlewares/Third_Party/lvgl/src` producing the same literal
format) rather than hand-editing it.
