/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : clock_config.h
  * @brief          : Clock configuration header
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

#ifndef __CLOCK_CONFIG_H__
#define __CLOCK_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Clock configuration options -----------------------------------------------*/
/* Uncomment one of the following options to select clock configuration */
// #define USE_HSI_16MHZ    1
#define USE_HSI_84MHZ    1
/* Default: 168MHz HSE configuration */

/* Exported functions prototypes ---------------------------------------------*/
void SystemClock_Config(void);

#ifdef __cplusplus
}
#endif

#endif /* __CLOCK_CONFIG_H__ */