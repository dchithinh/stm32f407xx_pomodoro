/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : debug_utils.h
  * @brief          : Debug utilities header
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

#ifndef __DEBUG_UTILS_H__
#define __DEBUG_UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "lvgl.h"

/* Exported functions prototypes ---------------------------------------------*/
void my_log_cb(lv_log_level_t level, const char * buf);
void lv_port_log_init(void);
void create_touch_cursor(void);

#ifdef __cplusplus
}
#endif

#endif /* __DEBUG_UTILS_H__ */