/**
 * @file disp.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_conf.h"
#include "lvgl/lvgl.h"
#include <string.h>

#include "tft.h"
#include "stm32f4xx.h"


extern  lcd_handle_t lcd_handle;

/*********************
 *      DEFINES
 *********************/


/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/*These 3 functions are needed by LittlevGL*/
static void tft_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_p);

/*LCD*/


/**********************
 *  STATIC VARIABLES
 **********************/


static lv_disp_drv_t disp_drv;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

static volatile uint32_t t_saved = 0;
void monitor_cb(lv_disp_drv_t * d, uint32_t time, uint32_t px)
{
	t_saved = time;
	LV_LOG_USER("Frame flushed: %lu px in %lu ms", px, time);

    // Optional: measure FPS
    static uint32_t frame_cnt = 0;
    static uint32_t last_ms = 0;
    frame_cnt++;

    uint32_t now = lv_tick_get();
    if(now - last_ms > 1000) {
        LV_LOG_USER("FPS: %lu", frame_cnt);
        frame_cnt = 0;
        last_ms = now;
    }
}

/**
 * Initialize your display here
 */
void tft_init(void)
{
	static lv_disp_draw_buf_t buf;
	lv_color_t *draw_buf1;
	lv_color_t *draw_buf2;

	lcd_init();
	draw_buf1 = (lv_color_t*)lcd_get_draw_buffer1_addr();
	draw_buf2 = (lv_color_t*)lcd_get_draw_buffer2_addr();
	lv_disp_draw_buf_init(&buf, draw_buf1, draw_buf2, (10UL * 1024UL)/2);
	lv_disp_drv_init(&disp_drv);

	disp_drv.draw_buf = &buf;
	disp_drv.flush_cb = tft_flush;
	disp_drv.monitor_cb = monitor_cb;
	disp_drv.hor_res = TFT_HOR_RES;
	disp_drv.ver_res = TFT_VER_RES;
	disp_drv.sw_rotate = 1;
	disp_drv.user_data = (void*)&lcd_handle;
	lv_disp_drv_register(&disp_drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * Flush a color buffer
 * @param x1 left coordinate of the rectangle
 * @param x2 right coordinate of the rectangle
 * @param y1 top coordinate of the rectangle
 * @param y2 bottom coordinate of the rectangle
 * @param color_p pointer to an array of colors
 */
static void tft_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_p)
{
  if(area->x2 < 0 || area->y2 < 0 || area->x1 > (TFT_HOR_RES  - 1) || area->y1 > (TFT_VER_RES  - 1)) {
		lv_disp_flush_ready(drv);
		return;
	}

	/*Return if the area is out the screen*/
	if(area->x2 < 0) return;
	if(area->y2 < 0) return;
	if(area->x1 > TFT_HOR_RES - 1) return;
	if(area->y1 > TFT_VER_RES - 1) return;

	/*Truncate the area to the screen*/
	int32_t act_x1 = area->x1 < 0 ? 0 : area->x1;
	int32_t act_y1 = area->y1 < 0 ? 0 : area->y1;
	int32_t act_x2 = area->x2 > TFT_HOR_RES - 1 ? TFT_HOR_RES - 1 : area->x2;
	int32_t act_y2 = area->y2 > TFT_VER_RES - 1 ? TFT_VER_RES - 1 : area->y2;

	lv_coord_t full_w = (area->x2 - area->x1) + 1;                // requested width
	lv_coord_t act_w  = (act_x2 - act_x1 + 1);                    // truncated width
	uint32_t len = act_w * 2ul;
	lcd_set_display_area(act_x1, act_x2, act_y1, act_y2);
	lcd_send_cmd_mem_write();

	for(uint32_t y = act_y1; y <= act_y2; y++) {
		lcd_write((uint8_t*)color_p, len);
		color_p += full_w;    // advance by original buffer width, not truncated
	}

	lv_disp_flush_ready(&disp_drv);
}

