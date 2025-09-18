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
static void tft_flush(lv_display_t * drv, const lv_area_t * area, uint8_t * color_p);

/*LCD*/


/**********************
 *  STATIC VARIABLES
 **********************/


static lv_display_t *display;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

static volatile uint32_t t_saved = 0;
void monitor_cb(lv_event_t *e)
{
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

void tft_init(void)
{
    uint8_t *draw_buf1;
    uint8_t *draw_buf2;

    lcd_init();
    draw_buf1 = (uint8_t*)lcd_get_draw_buffer1_addr();
    draw_buf2 = (uint8_t*)lcd_get_draw_buffer2_addr();

    display = lv_display_create(TFT_HOR_RES, TFT_VER_RES);

    // buf_size is number of pixels
    uint32_t buf_size = (10UL * 1024UL) / 2;  // 2 bytes per pixel in RGB565

    lv_display_set_buffers(display, draw_buf1, draw_buf2, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);

    // Set flush callback
    lv_display_set_flush_cb(display, tft_flush);

    // Optional: attach monitor callback
    lv_display_add_event_cb(display, monitor_cb, LV_EVENT_FLUSH_FINISH, NULL);

    lv_display_set_rotation(display, LV_DISPLAY_ROTATION_270);

    // Store user data if needed
    lv_display_set_user_data(display, (void *)&lcd_handle);
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
static void tft_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * color_p)
{
    if(area->x2 < 0 || area->y2 < 0 || area->x1 > (TFT_HOR_RES - 1) || area->y1 > (TFT_VER_RES - 1)) {
        lv_disp_flush_ready(disp);
        return;
    }

    int32_t act_x1 = area->x1 < 0 ? 0 : area->x1;
    int32_t act_y1 = area->y1 < 0 ? 0 : area->y1;
    int32_t act_x2 = area->x2 > TFT_HOR_RES - 1 ? TFT_HOR_RES - 1 : area->x2;
    int32_t act_y2 = area->y2 > TFT_VER_RES - 1 ? TFT_VER_RES - 1 : area->y2;

    lcd_set_display_area(act_x1, act_x2, act_y1, act_y2);
    lcd_send_cmd_mem_write();

    /* Calculate total pixels in the draw buffer */
    uint32_t width  = (area->x2 - area->x1 + 1);
    uint32_t height = (area->y2 - area->y1 + 1);
    uint32_t total_bytes = width * height * 2;  // RGB565

    /* Write all pixels at once */
    lcd_write(color_p, total_bytes);

    lv_disp_flush_ready(disp);
}

