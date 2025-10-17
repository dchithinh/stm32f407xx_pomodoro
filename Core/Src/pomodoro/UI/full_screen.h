#ifndef __H_FULL_SCREEN_H__
#define __H_FULL_SCREEN_H__

#include "stdint.h"
#include "lvgl.h"

void show_fullscreen_timer(lv_obj_t *parent);
void update_fullscreen_timer(uint32_t remaining);
void hide_fullscreen_timer(void);

#endif/* __H_FULL_SCREEN_H__ */
