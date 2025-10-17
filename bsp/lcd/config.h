
/* This will use HAL API in lcd_write_command()and lcd_write_data() */
// #define USE_STM32_API 		1

#define USE_DMA_FLUSH_LCD	    0
#if USE_DMA_FLUSH_LCD
/* Should define 1 of these macro to select DMA API*/
    // #define USE_DMA_IN_POLLING_MODE 1
    // #define USE_DMA_IN_IT_MODE      1
#endif // USE_DMA_FLUSH_LCD
