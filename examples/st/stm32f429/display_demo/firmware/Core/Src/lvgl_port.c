#include <string.h>

#include "lvgl_port.h"
#include "lvgl.h"

#define DISP_HOR_RES    240
#define DISP_VER_RES    320
#define DRAW_BUF_LINES  40

/* LTDC Layer0 framebuffer, backed by the emulated SDRAM at 0xD0000000
 * (see MX_LTDC_Init()'s FBStartAdress, RGB565). */
#define FRAMEBUFFER_ADDR ((uint16_t *)0xD0000000U)

static lv_disp_draw_buf_t s_drawBuf;
static lv_color_t s_buf1[DISP_HOR_RES * DRAW_BUF_LINES];
static lv_disp_drv_t s_dispDrv;

static void Display_FlushCb(lv_disp_drv_t *dispDrv, const lv_area_t *area, lv_color_t *colorP)
{
  uint16_t *framebuffer = FRAMEBUFFER_ADDR;
  int32_t rowWidth = area->x2 - area->x1 + 1;

  for (int32_t y = area->y1; y <= area->y2; y++)
  {
    uint16_t *dst = framebuffer + (y * DISP_HOR_RES) + area->x1;
    memcpy(dst, colorP, (size_t)rowWidth * sizeof(uint16_t));
    colorP += rowWidth;
  }

  lv_disp_flush_ready(dispDrv);
}

void LVGL_Init(void)
{
  lv_init();

  lv_disp_draw_buf_init(&s_drawBuf, s_buf1, NULL, DISP_HOR_RES * DRAW_BUF_LINES);

  lv_disp_drv_init(&s_dispDrv);
  s_dispDrv.hor_res = DISP_HOR_RES;
  s_dispDrv.ver_res = DISP_VER_RES;
  s_dispDrv.flush_cb = Display_FlushCb;
  s_dispDrv.draw_buf = &s_drawBuf;
  lv_disp_drv_register(&s_dispDrv);
}

/* Analog clock, built the same way as LVGL's own "meter with needles" demo:
 * one lv_meter scale (0-60, laid out like a clock face) plus three needle
 * indicators for hour/minute/second. A periodic lv_timer advances a virtual
 * time-of-day once a second and re-points the needles - lv_timer_handler()
 * (already called every StartDefaultTask loop iteration) is what actually
 * runs this timer's callback, same as it drives the display's own redraw
 * timers. */
#define CLOCK_SIZE 150
/* Virtual milliseconds (LVGL's own clock, driven by our HAL_GetTick() proxy).
 * In practice this period is no longer the limiting factor for how often the
 * clock visibly ticks: lv_meter fully redraws its whole widget (background +
 * every tick + every needle) on any single indicator change, and at -O0 that
 * full redraw alone takes several real seconds (see LVGL_CreateDemo() below)
 * - far longer than one 1149ms-virtual period - so the timer is always
 * already "due" by the time lv_timer_handler() gets back around to it. The
 * observed cadence is therefore redraw-bound, measured at roughly 3 real
 * seconds/tick, not this period value. */
#define CLOCK_TICK_PERIOD_MS 1149

static lv_obj_t *s_clockMeter;
static lv_meter_indicator_t *s_hourHand;
static lv_meter_indicator_t *s_minHand;
static lv_meter_indicator_t *s_secHand;

static void Clock_TimerCb(lv_timer_t *timer)
{
  (void)timer;
  static uint32_t elapsedSeconds = 0;
  elapsedSeconds++;

  uint32_t totalSeconds = elapsedSeconds % (12U * 3600U); /* wrap every 12 hours */
  uint32_t hours = totalSeconds / 3600U;
  uint32_t minutes = (totalSeconds / 60U) % 60U;
  uint32_t seconds = totalSeconds % 60U;

  if(s_secHand) lv_meter_set_indicator_value(s_clockMeter, s_secHand, (int32_t)seconds);
  if(s_minHand) lv_meter_set_indicator_value(s_clockMeter, s_minHand, (int32_t)minutes);
  /* Hour hand creeps smoothly between hour marks: 5 scale units per hour,
   * plus a fraction of that per elapsed minute (60 minutes -> +5 units). */
  if(s_hourHand) lv_meter_set_indicator_value(s_clockMeter, s_hourHand, (int32_t)((hours * 5U) + (minutes / 12U)));
}

void LVGL_CreateDemo(void)
{
  s_clockMeter = lv_meter_create(lv_scr_act());
  lv_obj_set_size(s_clockMeter, CLOCK_SIZE, CLOCK_SIZE);
  lv_obj_center(s_clockMeter);
  /* LVGL's default theme gives lv_meter a fully round background
   * (LV_STYLE_RADIUS = LV_RADIUS_CIRCLE, see lv_theme_default.c's
   * `styles->circle`). At this widget's size (150px, i.e. ~75px corner
   * radius) LVGL v8.3's corner-radius mask rasterizer (lv_draw_mask.c)
   * takes a huge, apparently far-worse-than-linear amount of CPU time to
   * render at -O0 - confirmed empirically: radius 75 never finished a
   * single redraw in 100+ virtual seconds, while radius 15 finishes in
   * ~19 virtual seconds and then renders normally. Overriding to a small
   * fixed radius (a softly rounded square instead of a perfect circle)
   * keeps rendering cost bounded. */
  lv_obj_set_style_radius(s_clockMeter, 8, 0);

  /* lv_meter fully re-renders the whole widget (background mask + every
   * tick + every needle) on ANY indicator value change - there's no partial/
   * incremental redraw for needle movement in this version. With 60 minor +
   * 12 major ticks each full redraw took ~50 virtual seconds at -O0 (~20-25
   * real seconds/tick); cutting to 13 ticks got that down to ~5-9 real
   * seconds/tick. Cut further to 5 quarter-hour marks (0/15/30/45/60) to
   * keep each redraw - and so each visible tick of the clock - as fast as
   * possible. */
  lv_meter_scale_t *scale = lv_meter_add_scale(s_clockMeter);
  lv_meter_set_scale_ticks(s_clockMeter, scale, 5, 2, 8, lv_color_hex(0x000000));
  lv_meter_set_scale_range(s_clockMeter, scale, 0, 60, 360, 270);

  s_hourHand = lv_meter_add_needle_line(s_clockMeter, scale, 6, lv_color_hex(0x000000), -55);
  s_minHand = lv_meter_add_needle_line(s_clockMeter, scale, 4, lv_color_hex(0x000000), -25);
  s_secHand = lv_meter_add_needle_line(s_clockMeter, scale, 2, lv_color_hex(0xFF0000), -5);

  lv_timer_create(Clock_TimerCb, CLOCK_TICK_PERIOD_MS, NULL);
}
