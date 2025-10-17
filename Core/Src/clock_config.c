/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : clock_config.c
  * @brief          : Clock configuration implementation
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
#include "clock_config.h"
#include "main.h"

/* Private function prototypes -----------------------------------------------*/
static void Configure_Power_And_Voltage(void);
static HAL_StatusTypeDef Configure_HSI_16MHz(void);
static HAL_StatusTypeDef Configure_HSE_84MHz(void);
static HAL_StatusTypeDef Configure_HSE_168MHz(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Configure power regulator and voltage scaling
  * @retval None
  */
static void Configure_Power_And_Voltage(void)
{
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
}

/**
  * @brief  Configure HSI 16MHz clock
  * @retval HAL status
  */
static HAL_StatusTypeDef Configure_HSI_16MHz(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /* Configure PLL multiplication factor and source */
  __HAL_RCC_PLL_PLLM_CONFIG(8);
  __HAL_RCC_PLL_PLLSOURCE_CONFIG(RCC_PLLSOURCE_HSE);

  Configure_Power_And_Voltage();

  /* Initialize oscillators */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    return HAL_ERROR;
  }

  /* Initialize CPU, AHB and APB buses clocks */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  return HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);
}

/**
  * @brief  Configure HSE 84MHz clock
  * @retval HAL status
  */
static HAL_StatusTypeDef Configure_HSE_84MHz(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  Configure_Power_And_Voltage();

  /* HSE = 8 MHz → PLL → 84 MHz SYSCLK */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM       = 8;      // 8 MHz / 8 = 1 MHz VCO in
  RCC_OscInitStruct.PLL.PLLN       = 168;    // 1 MHz × 168 = 168 MHz VCO
  RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV2; // 168 / 2 = 84 MHz SYSCLK
  RCC_OscInitStruct.PLL.PLLQ       = 7;      // optional USB 48 MHz
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    return HAL_ERROR;
  }

  RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_SYSCLK |
                                     RCC_CLOCKTYPE_HCLK   |
                                     RCC_CLOCKTYPE_PCLK1  |
                                     RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;   // 84 MHz
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;     // 42 MHz → SPI2 OK
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;     // 84 MHz APB2

  HAL_StatusTypeDef status = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
  if (status == HAL_OK)
  {
    SystemCoreClockUpdate();
  }
  return status;
}

/**
  * @brief  Configure HSE 168MHz clock (default)
  * @retval HAL status
  */
static HAL_StatusTypeDef Configure_HSE_168MHz(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  Configure_Power_And_Voltage();

  /* Initialize oscillators */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    return HAL_ERROR;
  }

  /* Initialize CPU, AHB and APB buses clocks */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  return HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
}

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  HAL_StatusTypeDef status = HAL_ERROR;

#if (USE_HSI_16MHZ == 1)
  status = Configure_HSI_16MHz();
#elif (USE_HSI_84MHZ == 1)
  status = Configure_HSE_84MHz();
#else
  status = Configure_HSE_168MHz();
#endif

  if (status != HAL_OK)
  {
    Error_Handler();
  }
}