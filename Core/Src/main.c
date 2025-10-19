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
#include <stdio.h>
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

  /* Enable DWT cycle counter for CPU measurement */
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;  // Enable trace
  DWT->CYCCNT = 0;                                  // Reset counter
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;             // Enable counter

  ui_main_screen(lv_scr_act());
  
  /* CPU usage monitoring variables */
  extern volatile uint32_t dma_irq_counter;
  uint32_t last_irq_count = 0;
  uint32_t last_report_time = HAL_GetTick();
  
  /* For CPU measurement - use recent average instead of long accumulation */
  uint32_t recent_active = 0;
  uint32_t recent_idle = 0;
  uint32_t measurement_count = 0;
  
  while (1)
  {
    uint32_t cycle_start = DWT->CYCCNT;
    
	  lv_timer_handler();
    
    uint32_t cycle_end = DWT->CYCCNT;
    uint32_t active_cycles = cycle_end - cycle_start;
    
    /* Measure idle time - reduce delay to call LVGL more frequently */
    uint32_t idle_start = DWT->CYCCNT;
    HAL_Delay(5);
    uint32_t idle_end = DWT->CYCCNT;
    uint32_t idle_cycles = idle_end - idle_start;
    
    /* Accumulate for averaging (prevent overflow by limiting samples) */
    if(measurement_count < 100) {
      recent_active += active_cycles;
      recent_idle += idle_cycles;
      measurement_count++;
    }
    
    /* Report DMA and CPU statistics every 10 seconds */
    uint32_t now = HAL_GetTick();
    if(now - last_report_time >= 10000) {
      uint32_t irq_delta = dma_irq_counter - last_irq_count;
      
      /* Calculate actual CPU usage based on recent measurements */
      uint32_t cpu_percent = 0;
      uint32_t avg_active = 0;
      uint32_t avg_idle = 0;
      
      if(measurement_count > 0) {
        avg_active = recent_active / measurement_count;
        avg_idle = recent_idle / measurement_count;
        uint32_t avg_total = avg_active + avg_idle;
        
        if(avg_total > 0) {
          cpu_percent = (avg_active * 100) / avg_total;
        }
      }
      
      /* Convert cycles to microseconds (84 MHz = 84 cycles/us) */
      uint32_t active_us = avg_active / 84;
      uint32_t idle_us = avg_idle / 84;
      uint32_t total_us = active_us + idle_us;
      
      LV_LOG_USER("=== Performance Report ===");
      LV_LOG_USER("DMA IRQs: %lu total, %lu in 10s (%lu/sec)", 
                  (unsigned long)dma_irq_counter,
                  (unsigned long)irq_delta,
                  (unsigned long)(irq_delta / 10));
      LV_LOG_USER("CPU Usage: %lu%% (active=%luus, idle=%luus, total=%luus)", 
                  (unsigned long)cpu_percent,
                  (unsigned long)active_us,
                  (unsigned long)idle_us,
                  (unsigned long)total_us);
      LV_LOG_USER("Expected loop time: ~5ms, actual: %luus", (unsigned long)total_us);
      
      last_irq_count = dma_irq_counter;
      last_report_time = now;
      recent_active = 0;
      recent_idle = 0;
      measurement_count = 0;
    }
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
