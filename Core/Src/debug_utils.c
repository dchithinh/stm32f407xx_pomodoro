/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : debug_utils.c
  * @brief          : Debug utilities implementation
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "debug_utils.h"
#include "main.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_uart.h"
#include "lvgl.h"

/* External variables --------------------------------------------------------*/
extern UART_HandleTypeDef huart2;

/* Private function prototypes -----------------------------------------------*/
static void touch_cursor_cb(lv_timer_t * t);

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  LVGL log callback function
  * @param  level: Log level
  * @param  buf: Log buffer
  * @retval None
  */
void my_log_cb(lv_log_level_t level, const char * buf)
{  
    if (buf) {
        // Blocking transmit to avoid overlap
        HAL_UART_Transmit(&huart2, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);
    }
}

/**
  * @brief  Initialize LVGL logging
  * @retval None
  */
void lv_port_log_init(void)
{ 
  lv_log_register_print_cb(my_log_cb); 
}

/**
  * @brief  Touch cursor timer callback
  * @param  t: Timer handle
  * @retval None
  */
static void touch_cursor_cb(lv_timer_t * t)
{
    LV_UNUSED(t);

    lv_indev_t * indev = lv_indev_get_next(NULL);
    if(indev) {
        lv_point_t p;
        lv_indev_get_point(indev, &p);

        lv_obj_t * cursor = (lv_obj_t *)lv_timer_get_user_data(t);
        lv_obj_set_pos(cursor, p.x, p.y);
    }
}

/**
  * @brief  Create touch cursor for debugging
  * @retval None
  */
void create_touch_cursor(void)
{
    lv_obj_t * cursor = lv_label_create(lv_screen_active());
    lv_label_set_text(cursor, "+");

    // Attach cursor as user data
    lv_timer_create(touch_cursor_cb, 30, cursor);
}