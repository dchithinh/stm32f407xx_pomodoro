/* The original code is took from : https://github.com/niekiran/EmbeddedGraphicsLVGL-MCU3/tree/main/Projects
 * The modified code is my learning and hands-on experience with STM32F407xx and LCD.
*/


#include "stm32f4xx_hal.h"
#include "stm32f407xx.h"
#include "stm32f4xx_hal_rcc.h"
#include "stm32f4xx_hal_rcc_ex.h"
#include "stm32f4xx_hal_dma.h"
#include "lcd.h"
#include "hw_def.h"
#include "ili9341_reg.h"


#define SET_SPI_16BIT_MODE(hspi) do { \
    (hspi)->Instance->CR1 |= SPI_CR1_DFF; \
} while(0)

#define SET_SPI_8BIT_MODE(hspi) do { \
    (hspi)->Instance->CR1 &= ~SPI_CR1_DFF; \
} while(0)


#define TRUE 1
#define FALSE 0

#define MADCTL_MY 0x80  ///< Bottom to top
#define MADCTL_MX 0x40  ///< Right to left
#define MADCTL_MV 0x20  ///< Reverse Mode
#define MADCTL_ML 0x10  ///< LCD refresh Bottom to top
#define MADCTL_RGB 0x00 ///< Red-Green-Blue pixel order
#define MADCTL_BGR 0x08 ///< Blue-Green-Red pixel order
#define MADCTL_MH 0x04  ///< LCD refresh right to left

SPI_HandleTypeDef lcd_spi_handle;
DMA_HandleTypeDef lcd_dma_handle;

lcd_handle_t lcd_handle;
lcd_handle_t *hlcd = &lcd_handle;


#define LCD_CS_LOW()    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET)
#define LCD_CS_HIGH()   HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET)
#define LCD_RESX_LOW()  HAL_GPIO_WritePin(LCD_RESX_PORT, LCD_RESX_PIN, GPIO_PIN_RESET)
#define LCD_RESX_HIGH() HAL_GPIO_WritePin(LCD_RESX_PORT, LCD_RESX_PIN, GPIO_PIN_SET)
#define LCD_DCX_LOW()   HAL_GPIO_WritePin(LCD_DCX_PORT, LCD_DCX_PIN, GPIO_PIN_RESET)
#define LCD_DCX_HIGH()  HAL_GPIO_WritePin(LCD_DCX_PORT, LCD_DCX_PIN, GPIO_PIN_SET)



#define DB_SIZE 	(10UL * 1024UL)
uint8_t db[DB_SIZE];
uint8_t wb[DB_SIZE];

static void lcd_pin_init(void);
static void lcd_spi_init(void);

static void lcd_spi_enable(void);
static void lcd_reset(void);
static void lcd_write_command(uint8_t cmd);
static void lcd_write_data(uint8_t *buffer, uint32_t length);
static void lcd_config();
static void lcd_buffer_init(lcd_handle_t *lcd);
static void lcd_set_orientation(uint8_t orientation);
#if USE_DMA_FLUSH_LCD
static void lcd_dma_init(void);
static void lcd_write_dma(uint8_t *buffer, uint32_t length);
#else
void lcd_write(uint8_t *buffer, uint32_t length);
#endif

/* Utils functions*/
static uint32_t copy_to_draw_buffer(lcd_handle_t *hlcd,uint32_t nbytes,uint32_t rgb888);
static uint32_t pixels_to_bytes(uint32_t pixels, uint8_t pixel_format);
static uint32_t bytes_to_pixels(uint32_t nbytes, uint8_t pixel_format);
static uint8_t *get_buff(lcd_handle_t *hlcd);
static void make_area(lcd_area_t *area,uint32_t x_start, uint32_t x_width,uint32_t y_start,uint32_t y_height);
static uint32_t get_total_bytes(lcd_handle_t *hlcd,uint32_t w , uint32_t h);
static uint16_t convert_rgb888_to_rgb565(uint32_t rgb888);
static void lcd_flush(lcd_handle_t *hlcd);
static uint8_t is_lcd_write_allowed(lcd_handle_t *hlcd);

void lcd_init(void)
{
    lcd_pin_init();
    lcd_spi_init();
    lcd_spi_enable();
#if USE_DMA_FLUSH_LCD
	lcd_dma_init();
#endif

    lcd_reset();
    lcd_config();

    hlcd->display_area.x1 = 0;
	hlcd->display_area.x2 = LCD_ACTIVE_WIDTH - 1;
	hlcd->display_area.y1 = 0;
	hlcd->display_area.y2 = LCD_ACTIVE_HEIGHT - 1;
	hlcd->orientation = LCD_ORIENTATION;
	hlcd->pixel_format = LCD_PIXEL_FORMAT;

	uint16_t x1 = hlcd->display_area.x1;
	uint16_t x2 = hlcd->display_area.x2;
	uint16_t y1 = hlcd->display_area.y1;
	uint16_t y2 = hlcd->display_area.y2;
	lcd_set_orientation(hlcd->orientation);
	lcd_set_display_area(x1, x2, y1, y2);
	
	lcd_buffer_init(hlcd);
}

void lcd_pin_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    GPIO_InitStruct.Pin = LCD_CS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LCD_CS_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LCD_RESX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LCD_RESX_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LCD_DCX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LCD_DCX_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LCD_SCLK_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(LCD_SCLK_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LCD_MOSI_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(LCD_MOSI_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LCD_MISO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(LCD_MISO_PORT, &GPIO_InitStruct);

    // Set all pins to High initially
    LCD_CS_HIGH();
    LCD_RESX_HIGH();
    LCD_DCX_HIGH();
}

static void lcd_spi_init(void)
{
    __HAL_RCC_SPI2_CLK_ENABLE();

    lcd_spi_handle.Instance = LCD_SPI;
    lcd_spi_handle.Init.Mode = SPI_MODE_MASTER;
    lcd_spi_handle.Init.Direction = SPI_DIRECTION_2LINES;
    lcd_spi_handle.Init.DataSize = SPI_DATASIZE_8BIT;
    lcd_spi_handle.Init.CLKPolarity = SPI_POLARITY_LOW;
    lcd_spi_handle.Init.CLKPhase = SPI_PHASE_1EDGE;
    lcd_spi_handle.Init.NSS = SPI_NSS_SOFT;
    lcd_spi_handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    lcd_spi_handle.Init.FirstBit = SPI_FIRSTBIT_MSB;
    lcd_spi_handle.Init.TIMode = SPI_TIMODE_DISABLE;

    if (HAL_SPI_Init(&lcd_spi_handle) != HAL_OK)
    {
        // Initialization Error
        printf("LCD SPI init failed\n");
        while(1);
    }    
}

static void lcd_spi_enable(void)
{
    __HAL_SPI_ENABLE(&lcd_spi_handle);
}

#if USE_DMA_FLUSH_LCD
static void lcd_dma_init(void)
{
	__HAL_RCC_DMA1_CLK_ENABLE();

	//Enable IRQ in NVIC side
	HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);

	// NVIC_SetPendingIRQ(DMA1_Stream4_IRQn);

	/* SPI2 DMA Init */
    /* SPI2_TX Init */
    lcd_dma_handle.Instance = DMA1_Stream4;
    lcd_dma_handle.Init.Channel = DMA_CHANNEL_0;
    lcd_dma_handle.Init.Direction = DMA_MEMORY_TO_PERIPH;
    lcd_dma_handle.Init.PeriphInc = DMA_PINC_DISABLE;
    lcd_dma_handle.Init.MemInc = DMA_MINC_ENABLE;
    lcd_dma_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    lcd_dma_handle.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    lcd_dma_handle.Init.Mode = DMA_NORMAL;
    lcd_dma_handle.Init.Priority = DMA_PRIORITY_VERY_HIGH;
    lcd_dma_handle.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&lcd_dma_handle) != HAL_OK)
    {
    //   Error_Handler();
		printf("LCD DMA init failed\n");
		while(1);
    }

    __HAL_LINKDMA(&lcd_spi_handle, hdmatx, lcd_dma_handle);

}
#endif // USE_DMA_FLUSH_LCD

static void lcd_reset(void)
{
    LCD_RESX_LOW();
    HAL_Delay(50); // Wait for 50 ms
    LCD_RESX_HIGH();
    HAL_Delay(50); // Wait for 50 ms


}


static void lcd_write_command(uint8_t cmd)
{
#ifndef USE_STM32_API
	SPI_TypeDef *pSPI = LCD_SPI;
#endif
    LCD_CS_LOW();
    LCD_DCX_LOW(); // Command mode
	
	/*When using STM32 API, the config failed and LCD can't display correct color*/

#if USE_STM32_API
    HAL_SPI_Transmit(&lcd_spi_handle, &cmd, 1, HAL_MAX_DELAY);
	
#else
	while (!(pSPI->SR & SPI_SR_TXE));
    pSPI->DR = cmd;
    while (!(pSPI->SR & SPI_SR_TXE));
    while (pSPI->SR & SPI_SR_BSY);
#endif

    LCD_DCX_HIGH(); //Back to default state
    LCD_CS_HIGH();
}

static void lcd_write_data(uint8_t *buffer, uint32_t length)
{
#ifndef USE_STM32_API
	SPI_TypeDef *pSPI = LCD_SPI;
#endif

    LCD_DCX_HIGH(); // Data mode
	LCD_CS_LOW();
	
	/*When using STM32 API, the config failed and LCD can't display correct color*/
#if USE_STM32_API
    HAL_SPI_Transmit(&lcd_spi_handle, buffer, length, HAL_MAX_DELAY);
#else
	for (uint32_t i = 0; i < length; i++) {
        while (!(pSPI->SR & SPI_SR_TXE));      // Wait until TX buffer is empty
        pSPI->DR = buffer[i];                  // Send byte
    }
    while (!(pSPI->SR & SPI_SR_TXE));          // Ensure last byte moved to shift register
    while (pSPI->SR & SPI_SR_BSY);             // Wait until SPI is not busy
#endif

    LCD_CS_HIGH();
}

static void lcd_config(void)
{
     uint8_t params[15];
	lcd_write_command(ILI9341_SWRESET);
	lcd_write_command(ILI9341_POWERB);
	params[0] = 0x00;
	params[1] = 0xD9;
	params[2] = 0x30;
	lcd_write_data(params, 3);

	lcd_write_command(ILI9341_POWER_SEQ);
	params[0]= 0x64;
	params[1]= 0x03;
	params[2]= 0X12;
	params[3]= 0X81;
	lcd_write_data(params, 4);

	lcd_write_command(ILI9341_DTCA);
	params[0]= 0x85;
	params[1]= 0x10;
	params[2]= 0x7A;
	lcd_write_data(params, 3);

	lcd_write_command(ILI9341_POWERA);
	params[0]= 0x39;
	params[1]= 0x2C;
	params[2]= 0x00;
	params[3]= 0x34;
	params[4]= 0x02;
	lcd_write_data(params, 5);

	lcd_write_command(ILI9341_PRC);
	params[0]= 0x20;
	lcd_write_data(params, 1);

	lcd_write_command(ILI9341_DTCB);
	params[0]= 0x00;
	params[1]= 0x00;
	lcd_write_data(params, 2);

	lcd_write_command(ILI9341_POWER1);
	params[0]= 0x1B;
	lcd_write_data(params, 1);

	lcd_write_command(ILI9341_POWER2);
	params[0]= 0x12;
	lcd_write_data(params, 1);

	lcd_write_command(ILI9341_VCOM1);
	params[0]= 0x08;
	params[1]= 0x26;
	lcd_write_data(params, 2);

	lcd_write_command(ILI9341_VCOM2);
	params[0]= 0XB7;
	lcd_write_data(params, 1);


	lcd_write_command(ILI9341_PIXEL_FORMAT);
	params[0]= 0x55; //select RGB565
	lcd_write_data(params, 1);

	lcd_write_command(ILI9341_FRMCTR1);
	params[0]= 0x00;
	params[1]= 0x1B;//frame rate = 70
	lcd_write_data(params, 2);

	lcd_write_command(ILI9341_DFC);    // Display Function Control
	params[0]= 0x0A;
	params[1]= 0xA2;
	lcd_write_data(params, 2);

	lcd_write_command(ILI9341_3GAMMA_EN);    // 3Gamma Function Disable
	params[0]= 0x02; //LCD_WR_DATA(0x00);
	lcd_write_data(params, 1);

	lcd_write_command(ILI9341_GAMMA);
	params[0]= 0x01;
	lcd_write_data(params, 1);

	lcd_write_command(ILI9341_PGAMMA);    //Set Gamma
	params[0]= 0x0F;
	params[1]= 0x1D;
	params[2]= 0x1A;
	params[3]= 0x0A;
	params[4]= 0x0D;
	params[5]= 0x07;
	params[6]= 0x49;
	params[7]= 0X66;
	params[8]= 0x3B;
	params[9]= 0x07;
	params[10]= 0x11;
	params[11]= 0x01;
	params[12]= 0x09;
	params[13]= 0x05;
	params[14]= 0x04;
	lcd_write_data(params, 15);

	lcd_write_command(ILI9341_NGAMMA);
	params[0]= 0x00;
	params[1]= 0x18;
	params[2]= 0x1D;
	params[3]= 0x02;
	params[4]= 0x0F;
	params[5]= 0x04;
	params[6]= 0x36;
	params[7]= 0x13;
	params[8]= 0x4C;
	params[9]= 0x07;
	params[10]= 0x13;
	params[11]= 0x0F;
	params[12]= 0x2E;
	params[13]= 0x2F;
	params[14]= 0x05;
	lcd_write_data(params, 15);

	lcd_write_command(ILI9341_SLEEP_OUT); //Exit Sleep
	HAL_Delay(50); // Wait for 50 ms
	HAL_Delay(50); // Wait for 50 ms
	lcd_write_command(ILI9341_DISPLAY_ON); //display on
}



void lcd_set_background_color(uint32_t rgb888)
{
    lcd_fill_rect(rgb888, 0, (LCD_ACTIVE_WIDTH), 0, (LCD_ACTIVE_HEIGHT));
}

void lcd_fill_rect(uint32_t rgb888, uint32_t x_start, uint32_t x_width, uint32_t y_start,uint32_t y_height)
{
    uint32_t total_bytes_to_write = 0;
	uint32_t bytes_sent_so_far = 0;
	uint32_t remaining_bytes = 0;;
	uint32_t npix;
	uint32_t pixels_sent = 0;
	uint32_t x1,y1;
	uint32_t pixel_per_line = x_width;

	if((x_start+x_width) > LCD_ACTIVE_WIDTH) return;
	if((y_start+y_height) > LCD_ACTIVE_HEIGHT) return;

	//1. calculate total number of bytes written in to DB
	total_bytes_to_write = get_total_bytes(hlcd,x_width,y_height);
	remaining_bytes = total_bytes_to_write;
	while(remaining_bytes)
	{
		x1 = x_start+(pixels_sent % pixel_per_line);
		y1 = y_start+(pixels_sent / pixel_per_line);

		make_area(&hlcd->display_area,x1,x_width,y1,y_height);

		if(x1 != x_start)
		{
			npix = x_start+x_width - x1;
		}
		else
		{
			npix = bytes_to_pixels(remaining_bytes,hlcd->pixel_format);
		}

		bytes_sent_so_far  +=  copy_to_draw_buffer(hlcd,pixels_to_bytes(npix,hlcd->pixel_format),rgb888);
		pixels_sent = bytes_to_pixels(bytes_sent_so_far,hlcd->pixel_format);
		remaining_bytes = total_bytes_to_write - bytes_sent_so_far;
	}
}


void lcd_set_orientation(uint8_t orientation)
{
	uint8_t param;

	if(orientation == LANDSCAPE){
		param = MADCTL_MV | MADCTL_MY | MADCTL_BGR; /*Memory Access Control <Landscape setting>*/
	}else if(orientation == PORTRAIT){
		param = MADCTL_MY | MADCTL_MX | MADCTL_RGB;  /* Memory Access Control <portrait setting> */
	}

	lcd_write_command(ILI9341_MAC);    // Memory Access Control command
	lcd_write_data(&param, 1);
}

void lcd_set_display_area(uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2)
{
	uint8_t params[4];
	//Set the column address
	params[0] = HIGH_16(x1);
	params[1] = LOW_16(x1);
	params[2] = HIGH_16(x2);
	params[3] = LOW_16(x2);
	lcd_write_command(ILI9341_CASET);
	lcd_write_data(params, 4);

	//Set the row address
	params[0] = HIGH_16(y1);
	params[1] = LOW_16(y1);
	params[2] = HIGH_16(y2);
	params[3] = LOW_16(y2);
	lcd_write_command(ILI9341_RASET);
	lcd_write_data(params, 4);
	
}

static void lcd_buffer_init(lcd_handle_t *lcd)
{
	lcd->draw_buffer1 = db;
	lcd->draw_buffer2 = wb;
	lcd->buff_to_draw = NULL;
	lcd->buff_to_flush = NULL;
}

static uint8_t is_lcd_write_allowed(lcd_handle_t *hlcd)
{
	__disable_irq();
	if(!hlcd->buff_to_flush)
		return TRUE;
	__enable_irq();

	return FALSE;
}

void lcd_send_cmd_mem_write(void)
{
	lcd_write_command(ILI9341_GRAM);
}

static void lcd_flush(lcd_handle_t *hlcd)
{
	uint16_t x1 = hlcd->display_area.x1;
	uint16_t x2 = hlcd->display_area.x2;
	uint16_t y1 = hlcd->display_area.y1;
	uint16_t y2 = hlcd->display_area.y2;
	lcd_set_display_area(x1, x2, y1, y2);
	lcd_send_cmd_mem_write();

#if USE_DMA_FLUSH_LCD
	lcd_write_dma(hlcd->buff_to_flush, hlcd->write_length);
#else
	lcd_write(hlcd->buff_to_flush, hlcd->write_length);
#endif
	hlcd->buff_to_flush = NULL;
}

static uint16_t convert_rgb888_to_rgb565(uint32_t rgb888)
{
    uint16_t r,g,b;
	r = (rgb888 >> 19) & 0x1FU; // (A) 1111 1111 | (R) 1111 1111 | (G) 1111 1111 | (B) 1111 1111 -> 000 0000 0000 0000 0000 1111 1111 | 1111 1
	g = (rgb888 >> 10) & 0x3FU;
	b = (rgb888 >> 3)  & 0x1FU;
	return (uint16_t)((r << 11) | (g << 5) | b);
}

static uint32_t get_total_bytes(lcd_handle_t *hlcd,uint32_t w , uint32_t h)
{
    //TODO: Add support for other pixel formats, Now just RGB565 is supported
	uint8_t bytes_per_pixel = 2;
	if(hlcd->pixel_format == LCD_PIXEL_FORMAT){
		bytes_per_pixel = 2;
	}
    else {
        printf("Unsupported pixel format: %d\n", hlcd->pixel_format);
    }
	return (w * h * bytes_per_pixel);
}

static void make_area(lcd_area_t *area,uint32_t x_start, uint32_t x_width,uint32_t y_start,uint32_t y_height){

	uint16_t lcd_total_width,lcd_total_height;

	lcd_total_width =  LCD_ACTIVE_WIDTH-1;
	lcd_total_height = LCD_ACTIVE_HEIGHT -1;

	area->x1 = x_start;
	area->x2 = x_start + x_width -1;
	area->y1 = y_start;
	area->y2 = y_start + y_height -1;

	area->x2 = (area->x2 > lcd_total_width) ? lcd_total_width :area->x2;
	area->y2 = (area->y2 > lcd_total_height) ? lcd_total_height : area->y2;

}

static uint8_t *get_buff(lcd_handle_t *hlcd)
{
	uint32_t buf1 = (uint32_t)hlcd->draw_buffer1;
	uint32_t buf2 = (uint32_t)hlcd->draw_buffer2;

	__disable_irq();
	if(hlcd->buff_to_draw == NULL && hlcd->buff_to_flush == NULL){
		return  hlcd->draw_buffer1;
	}else if((uint32_t)hlcd->buff_to_flush == buf1 && hlcd->buff_to_draw == NULL ){
		return  hlcd->draw_buffer2;
	}else if ((uint32_t)hlcd->buff_to_flush == buf2 && hlcd->buff_to_draw == NULL){
		return  hlcd->draw_buffer1;
	}
	__enable_irq();

	return NULL;
}

static uint32_t bytes_to_pixels(uint32_t nbytes, uint8_t pixel_format)
{
	UNUSED(pixel_format);
	return nbytes/2;

}

static uint32_t pixels_to_bytes(uint32_t pixels, uint8_t pixel_format)
{
	UNUSED(pixel_format);
	return pixels * 2UL;
}

static uint32_t copy_to_draw_buffer(lcd_handle_t *hlcd,uint32_t nbytes,uint32_t rgb888)
{
	uint16_t *fb_ptr = NULL;
	uint32_t npixels;
	hlcd->buff_to_draw = get_buff(hlcd);
	fb_ptr = (uint16_t*)hlcd->buff_to_draw;
	nbytes =  ((nbytes > DB_SIZE)?DB_SIZE:nbytes);
	npixels= bytes_to_pixels(nbytes,hlcd->pixel_format);
	if(hlcd->buff_to_draw != NULL){
		for(uint32_t i = 0 ; i < npixels ;i++){
			*fb_ptr = convert_rgb888_to_rgb565(rgb888);
			fb_ptr++;
		}
		hlcd->write_length = pixels_to_bytes(npixels,hlcd->pixel_format);
		while(!is_lcd_write_allowed(hlcd));
		hlcd->buff_to_flush = hlcd->buff_to_draw;
		hlcd->buff_to_draw = NULL;
		lcd_flush(hlcd);
		return pixels_to_bytes(npixels,hlcd->pixel_format);
	}

	return 0;
}

#if USE_DMA_FLUSH_LCD
static void lcd_write_dma(uint8_t *buffer, uint32_t length)
{
	// Modify SPI data size to 16 bits
	__HAL_SPI_DISABLE(&lcd_spi_handle);
	SET_SPI_16BIT_MODE(&lcd_spi_handle);
	__HAL_SPI_ENABLE(&lcd_spi_handle);    

	uint16_t *data_ptr = (uint16_t *)buffer;
	LCD_CS_LOW();

#ifdef USE_DMA_IN_POLLING_MODE //This mode is for learning purpose only
	/* This will enable SPI request to DMA to start put data in SPI2->DR,
	 * When SPI TX buffer is empty.
	 */
    SET_BIT(lcd_spi_handle.Instance->CR2, SPI_CR2_TXDMAEN);
	HAL_DMA_Start(&lcd_dma_handle, (uint32_t)data_ptr, (uint32_t)&LCD_SPI->DR, length/2);
	HAL_DMA_PollForTransfer(&lcd_dma_handle, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);

	// Restore back to 8-bit mode
	__HAL_SPI_DISABLE(&lcd_spi_handle);
	SET_SPI_8BIT_MODE(&lcd_spi_handle);
	__HAL_SPI_ENABLE(&lcd_spi_handle);
#elif USE_DMA_IN_IT_MODE
	
#else
// Make sure previous flags are cleared before starting DMA


	HAL_SPI_Transmit_DMA(&lcd_spi_handle, (uint8_t *)buffer, length/2);
#endif
}

#else
void lcd_write(uint8_t *buffer, uint32_t length)
{
    // Switch SPI to 16-bit mode
    __HAL_SPI_DISABLE(&lcd_spi_handle);
    SET_SPI_16BIT_MODE(&lcd_spi_handle);
    __HAL_SPI_ENABLE(&lcd_spi_handle);

    LCD_CS_LOW();

    uint16_t *data_ptr = (uint16_t *)buffer;
    uint32_t count = length / 2;   // number of 16-bit words

    for(uint32_t i = 0; i < count; i++) {
        // Wait until TX buffer empty
        while(!(lcd_spi_handle.Instance->SR & SPI_SR_TXE));

        // Write next 16-bit pixel
        lcd_spi_handle.Instance->DR = *data_ptr++;
    }

    // Wait for last transmission to fully complete
    while(!(lcd_spi_handle.Instance->SR & SPI_SR_TXE));
    while(lcd_spi_handle.Instance->SR & SPI_SR_BSY);

    LCD_CS_HIGH();

    // Restore back to 8-bit mode
    __HAL_SPI_DISABLE(&lcd_spi_handle);
    SET_SPI_8BIT_MODE(&lcd_spi_handle);
    __HAL_SPI_ENABLE(&lcd_spi_handle);
}


#endif // USE_DMA_FLUSH_LCD

void ili9341_test_draw_color_bars(void)
{
    // === Set Column Address (0 to 239) ===
    lcd_write_command(0x2A);
    uint8_t col_data[4] = { 0x00, 0x00, 0x00, 0xEF };  // 0 to 239 (0xEF)
    lcd_write_data(col_data, 4);

    // === Set Page Address (0 to 319) ===
    lcd_write_command(0x2B);
    uint8_t row_data[4] = { 0x00, 0x00, 0x01, 0x3F };  // 0 to 319 (0x13F)
    lcd_write_data(row_data, 4);

    // === Write Memory Command ===
    lcd_write_command(0x2C);

    // Prepare color lines (80 pixels each)
    uint8_t red_line[80 * 2];
    uint8_t green_line[80 * 2];
    uint8_t blue_line[80 * 2];

    for (int i = 0; i < 80; i++) {
        // Red (RGB565: 0xF800)
        red_line[2 * i]     = 0xF8;
        red_line[2 * i + 1] = 0x00;

        // Green (RGB565: 0x07E0)
        green_line[2 * i]     = 0x07;
        green_line[2 * i + 1] = 0xE0;

        // Blue (RGB565: 0x001F)
        blue_line[2 * i]     = 0x00;
        blue_line[2 * i + 1] = 0x1F;
    }

    // === Draw 320 lines (rows) ===
    LCD_DCX_HIGH(); // Data mode
    LCD_CS_LOW();

    for (int y = 0; y < 320; y++) {
        HAL_SPI_Transmit(&lcd_spi_handle, red_line, sizeof(red_line), HAL_MAX_DELAY);
        HAL_SPI_Transmit(&lcd_spi_handle, green_line, sizeof(green_line), HAL_MAX_DELAY);
        HAL_SPI_Transmit(&lcd_spi_handle, blue_line, sizeof(blue_line), HAL_MAX_DELAY);
    }

    LCD_CS_HIGH();
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
	__HAL_DMA_CLEAR_FLAG(&lcd_dma_handle, __HAL_DMA_GET_TC_FLAG_INDEX(&lcd_dma_handle));
	__HAL_DMA_CLEAR_FLAG(&lcd_dma_handle, __HAL_DMA_GET_TC_FLAG_INDEX(&lcd_dma_handle));
	__HAL_DMA_CLEAR_FLAG(&lcd_dma_handle, __HAL_DMA_GET_TC_FLAG_INDEX(&lcd_dma_handle));

	LCD_CS_HIGH();

	__HAL_SPI_DISABLE(hspi);
	SET_SPI_8BIT_MODE(hspi);
	__HAL_SPI_ENABLE(hspi);
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
	// Handle SPI error here
	printf("SPI Error occurred\n");
	while(1);
}

void HAL_SPI_TxHalfCpltCallback(SPI_HandleTypeDef *hspi)
{
	// Handle half transfer complete here if needed
	// This is optional and can be used for debugging or other purposes
}

void *lcd_get_draw_buffer1_addr(void)
{
    return (void*)hlcd->draw_buffer1;
}
void *lcd_get_draw_buffer2_addr(void)
{
	return (void*)hlcd->draw_buffer2;
}
