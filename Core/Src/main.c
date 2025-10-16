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


#define RGB888(r,g,b)  (((r) << 16) | ((g) << 8) | (b))

#define VIOLET   	RGB888(148,0,211)
#define INDIGO   	RGB888(75,0,130)
#define BLUE   		RGB888(0,0,255)
#define GREEN   	RGB888(0,255,0)
#define YELLOW   	RGB888(255,255,0)
#define ORANGE   	RGB888(255,127,0)
#define RED   		RGB888(255,0,0)
#define WHITE   	RGB888(255,255,255)
#define BLACK		  RGB888(0,0,0)


/* make linker symbols visible to debugger & C */
extern char __HeapBase, __HeapLimit, __StackTop;

volatile uintptr_t heap_base_addr;
volatile uintptr_t heap_limit_addr;
volatile uintptr_t stack_top_addr;
volatile uintptr_t sp_value;
register char *sp asm("sp"); /* the current stack pointer register */

UART_HandleTypeDef huart2;

void SystemClock_Config(void);


static void UART2_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }
}

#define LOG_BUFFER_SIZE 1024   // adjust for your needs

static volatile uint16_t log_head = 0;
static volatile uint16_t log_tail = 0;
static uint8_t log_buf[LOG_BUFFER_SIZE];
static volatile uint8_t uart_tx_busy = 0;

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        if (log_tail != log_head) {
            uint8_t c = log_buf[log_tail];
            log_tail = (log_tail + 1) % LOG_BUFFER_SIZE;
            HAL_UART_Transmit_IT(&huart2, &c, 1);
        } else {
            uart_tx_busy = 0; // transmission done
        }
    }
}

void usart2_puts_it(const char *s)
{
    while (*s) {
        uint16_t next = (log_head + 1) % LOG_BUFFER_SIZE;

        if (next == log_tail) {
            // Buffer full -> drop character OR spin-wait
            // For logs, usually dropping is safer (avoid blocking LVGL)
            return;
        }

        log_buf[log_head] = (uint8_t)*s++;
        log_head = next;
    }

    if (!uart_tx_busy) {
        uart_tx_busy = 1;
        uint8_t c = log_buf[log_tail];
        log_tail = (log_tail + 1) % LOG_BUFFER_SIZE;
        HAL_UART_Transmit_IT(&huart2, &c, 1);
    }
}

extern char _end;   // from linker
extern char _estack;

void hard_fault_handler_c(uint32_t *stack) {
    // uint32_t msp = __get_MSP();
    // uint32_t psp = __get_PSP();

    // LV_LOG_USER("\n[HardFault]\n");
    // LV_LOG_USER("R0=0x%08lX R1=0x%08lX R2=0x%08lX R3=0x%08lX\n",
    //        (unsigned long)stack[0], (unsigned long)stack[1],
    //        (unsigned long)stack[2], (unsigned long)stack[3]);
    // LV_LOG_USER("R12=0x%08lX LR=0x%08lX PC=0x%08lX PSR=0x%08lX\n",
    //        (unsigned long)stack[4], (unsigned long)stack[5],
    //        (unsigned long)stack[6], (unsigned long)stack[7]);

    // LV_LOG_USER("MSP=0x%08lX PSP=0x%08lX &_end=0x%08lX _estack=0x%08lX\n",
    //        (unsigned long)msp, (unsigned long)psp,
    //        (unsigned long)&_end, (unsigned long)&_estack);

    // LV_LOG_USER("Free approx (MSP - &_end) = %lu bytes\n",
    //        (unsigned long)(msp - (uint32_t)&_end));

    // LV_LOG_USER("SCB->CFSR=0x%08lX SCB->HFSR=0x%08lX SCB->BFAR=0x%08lX SCB->MMFAR=0x%08lX\n",
    //        (unsigned long)SCB->CFSR, (unsigned long)SCB->HFSR,
    //        (unsigned long)SCB->BFAR, (unsigned long)SCB->MMFAR);

    // // Helpful: dump a small region of stack memory around SP
    // uint32_t *sp = (uint32_t*)msp;
    // LV_LOG_USER("Stack dump (top 16 words):\n");
    // for (int i = 0; i < 16; i++) {
    //     LV_LOG_USER(" %02d: 0x%08lX\n", i, (unsigned long)sp[i]);
    // }

  register uint32_t sp_val __asm("sp");
  extern char __HeapLimit, __StackTop;

  LV_LOG_USER("\nHardFault! SP=%08lx  HeapLimit=%08lx  StackTop=%08lx\n",
          sp_val, (uint32_t)&__HeapLimit, (uint32_t)&__StackTop);


    while(1);
}

void my_log_cb(lv_log_level_t level, const char * buf)
{
    // usart2_puts_it(buf);
    // usart2_puts_it("\r\r\n");  
    if (buf) {
        // Blocking transmit to avoid overlap
        HAL_UART_Transmit(&huart2, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);
    }
}


void lv_port_log_init(void)
{ 
  lv_log_register_print_cb(my_log_cb); 
}

void test_uart_log(void)
{
    my_log_cb(LV_LOG_LEVEL_INFO, "Hello from my_log_cb!");
}

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

void create_touch_cursor(void)
{
    lv_obj_t * cursor = lv_label_create(lv_screen_active());
    lv_label_set_text(cursor, "+");

    // Attach cursor as user data
    lv_timer_create(touch_cursor_cb, 30, cursor);
}

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  heap_base_addr  = (uintptr_t)&__HeapBase;
  heap_limit_addr = (uintptr_t)&__HeapLimit;
  stack_top_addr  = (uintptr_t)&__StackTop;
  sp_value        = (uintptr_t)sp;
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  UART2_Init();
  test_uart_log();
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

#if (USE_HSI_16MHZ == 1)

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Macro to configure the PLL multiplication factor
  */
  __HAL_RCC_PLL_PLLM_CONFIG(8);

  /** Macro to configure the PLL clock source
  */
  __HAL_RCC_PLL_PLLSOURCE_CONFIG(RCC_PLLSOURCE_HSE);

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}
#elif (USE_HSI_84MHZ == 1)
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

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
        Error_Handler();

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_SYSCLK |
                                       RCC_CLOCKTYPE_HCLK   |
                                       RCC_CLOCKTYPE_PCLK1  |
                                       RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;   // 84 MHz
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;     // 42 MHz → SPI2 OK
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;     // 84 MHz APB2

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
        Error_Handler();

    SystemCoreClockUpdate();
}

#else
/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
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
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}
#endif

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
