
/* This will use HAL API in lcd_write_command()and lcd_write_data() */
#define USE_STM32_API 		1

/* This will use HSI 16MHz sysclock
 * Otherwise, sysclock will be PLL 168MHz
 * LCD SPI clock can be configured by
 * SPI_BAUDRATEPRESCALER_X in lcd_spi_init()
 */
#define USE_HSI_16MHZ 		0


#define USE_DMA_FLUSH_LCD	    1
#ifdef USE_DMA_FLUSH_LCD
/* Should define 1 of these macro to select DMA API*/
    // #define USE_DMA_IN_POLLING_MODE 1
    // #define USE_DMA_IN_IT_MODE      1
#endif // USE_DMA_FLUSH_LCD