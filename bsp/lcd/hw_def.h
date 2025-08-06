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


#endif /* __HW_DEF_H__ */