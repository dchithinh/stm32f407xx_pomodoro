#ifndef __LCD_H__
#define __LCD_H__

#include <stdint.h>

/* This will use HAL API in lcd_write_command()and lcd_write_data() */
#define USE_STM32_API 		1

/* This will use HSI 16MHz sysclock and
 * LCD SPI clock can be configured by
 * SPI_BAUDRATEPRESCALER_2 in lcd_spi_init()
 * Otherwise, sysclock will be PLL 168MHz
 */
#define USE_HSI_16MHZ 		1	


#define HIGH_16(x) (((uint16_t)x >> 0x8U) & 0xFFU)
#define LOW_16(x)  (((uint16_t)x >> 0x0U) & 0xFFU)

#define PORTRAIT        0
#define LANDSCAPE       1
#define LCD_ORIENTATION PORTRAIT

#define LCD_ACTIVE_WIDTH                240
#define LCD_ACTIVE_HEIGHT               320

#define LCD_PIXEL_FORMAT_L8             1
#define LCD_PIXEL_FORMAT_RGB565         2
#define LCD_PIXEL_FORMAT_RGB666         3
#define LCD_PIXEL_FORMAT_RGB888         4
#define LCD_PIXEL_FORMAT                LCD_PIXEL_FORMAT_RGB565

typedef struct{
 	uint16_t x1;
 	uint16_t x2;
 	uint16_t y1;
 	uint16_t y2;
 }lcd_area_t;

typedef struct{
 	uint8_t orientation;
 	uint8_t pixel_format;
 	uint8_t * draw_buffer1;
 	uint8_t * draw_buffer2;
 	uint32_t write_length;
 	uint8_t *buff_to_draw;
 	uint8_t *buff_to_flush;
 	lcd_area_t display_area;
 } lcd_handle_t;

/* Exported functions prototypes */
void lcd_init(void);
void lcd_set_background_color(uint32_t rgb888);
void lcd_fill_rect(uint32_t rgb888, uint32_t x_start, uint32_t x_width, uint32_t y_start,uint32_t y_height);
void ili9341_test_draw_color_bars(void);

#endif /* __LCD_H__ */