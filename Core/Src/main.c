/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include <string.h>
#include "main.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_uart.h"
#include "lvgl.h"
#include "lv_conf.h"
#include "lv_examples.h"
#include "tft.h"
#include "touchpad.h"
#include "main_screen.h"
#include "debug_utils.h"
#include "clock_config.h"

UART_HandleTypeDef huart2;

/**
  * @brief  Initialize UART2 for debugging
  * @note   Pin Configuration (configured in HAL_UART_MspInit):
  *         - PA2: USART2_TX (Alternate Function AF7)
  *         - PA3: USART2_RX (Alternate Function AF7)
  *         - GPIO Mode: GPIO_MODE_AF_PP (Alternate Function Push-Pull)
  *         - GPIO Pull: GPIO_PULLUP
  *         - GPIO Speed: GPIO_SPEED_FREQ_VERY_HIGH
  * @retval None
  */
static void UART2_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;      // 115200 bps for debug output
    huart2.Init.WordLength = UART_WORDLENGTH_8B;  // 8 data bits
    huart2.Init.StopBits = UART_STOPBITS_1;       // 1 stop bit
    huart2.Init.Parity = UART_PARITY_NONE;        // No parity
    huart2.Init.Mode = UART_MODE_TX_RX;           // TX and RX enabled
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;  // No hardware flow control
    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  UART2_Init();
  lv_init();
  lv_port_log_init();
  tft_init();
  touchpad_init();

  ui_main_screen(lv_scr_act());
  
  while (1)
  {
	  lv_timer_handler();
    HAL_Delay(5);
  }

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
