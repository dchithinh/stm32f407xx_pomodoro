#ifndef __HW_DEF_H__
#define __HW_DEF_H__

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_spi.h"
#include "stm32f4xx_hal_gpio.h"

#define LCD_SPI             SPI2

#define LCD_SCLK_PORT       GPIOB
#define LCD_SCLK_PIN        GPIO_PIN_13

#define LCD_MISO_PORT       GPIOC           //SDO
#define LCD_MISO_PIN        GPIO_PIN_2

#define LCD_MOSI_PORT       GPIOB           //SDI
#define LCD_MOSI_PIN        GPIO_PIN_15

#define LCD_CS_PORT         GPIOB
#define LCD_CS_PIN          GPIO_PIN_9

#define LCD_RESX_PORT		GPIOD
#define LCD_RESX_PIN		GPIO_PIN_10

#define LCD_DCX_PORT		GPIOD
#define LCD_DCX_PIN			GPIO_PIN_9

/* XPT2046 on stm32f407_tft_2.4inch_hat (SPI3, shared with microSD) */
#define XPT2046_SPI         SPI3

#define XPT2046_SCK_PORT    GPIOC
#define XPT2046_SCK_PIN     GPIO_PIN_10
#define XPT2046_MISO_PORT   GPIOC
#define XPT2046_MISO_PIN    GPIO_PIN_11
#define XPT2046_MOSI_PORT   GPIOC
#define XPT2046_MOSI_PIN    GPIO_PIN_12

#define XPT2046_CS_PORT     GPIOE
#define XPT2046_CS_PIN      GPIO_PIN_3
#define XPT2046_IRQ_PORT    GPIOE
#define XPT2046_IRQ_PIN     GPIO_PIN_5

#endif /* __HW_DEF_H__ */