#ifndef LVGL_PORT_H
#define LVGL_PORT_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Initializes LVGL and registers the display driver that flushes into the
 * LTDC Layer0 framebuffer (SDRAM bank2, 0xD0000000, RGB565, 240x320). */
void LVGL_Init(void);

/* Creates a small demo widget with a continuous bouncing animation. */
void LVGL_CreateDemo(void);

#ifdef __cplusplus
}
#endif

#endif /* LVGL_PORT_H */
