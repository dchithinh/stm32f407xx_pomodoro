/**
 * @file XPT2046.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "XPT2046.h"
#include "hw_def.h"
#include "stm32f4xx.h"

#if USE_XPT2046

#include <stddef.h>
#include <stdbool.h>
#include LV_DRV_INDEV_INCLUDE
#include LV_DRV_DELAY_INCLUDE

/*********************
 *      DEFINES
 *********************/
#define CMD_X_READ  0b10010000
#define CMD_Y_READ  0b11010000

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void xpt2046_corr(int16_t * x, int16_t * y);
static void xpt2046_avg(int16_t * x, int16_t * y);
static void xpt2046_avg_reset(void);
static void xpt2046_gpio_init(void);
static void MX_SPI3_Init(void);

/**********************
 *  STATIC VARIABLES
 **********************/
static SPI_HandleTypeDef hspi3;

int16_t avg_buf_x[XPT2046_AVG];
int16_t avg_buf_y[XPT2046_AVG];
uint8_t avg_last;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

uint8_t xpt2046_xchg_value(uint8_t val)
{
	uint8_t rcv_data;
	HAL_SPI_TransmitReceive(&hspi3, &val, &rcv_data, 1, HAL_MAX_DELAY);
	return rcv_data;
}

void xpt2046_cs(uint8_t val)
{
	HAL_GPIO_WritePin(XPT2046_CS_PORT, XPT2046_CS_PIN,
	                  val ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void HAL_SPI_MspInit(SPI_HandleTypeDef * hspi)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	if (hspi->Instance != XPT2046_SPI) {
		return;
	}

	__HAL_RCC_SPI3_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();

	GPIO_InitStruct.Pin = XPT2046_SCK_PIN | XPT2046_MISO_PIN | XPT2046_MOSI_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

static void MX_SPI3_Init(void)
{
	hspi3.Instance = XPT2046_SPI;
	hspi3.Init.Mode = SPI_MODE_MASTER;
	hspi3.Init.Direction = SPI_DIRECTION_2LINES;
	hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi3.Init.NSS = SPI_NSS_SOFT;
	hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
	hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi3.Init.CRCPolynomial = 10;
	if (HAL_SPI_Init(&hspi3) != HAL_OK) {
		while (1) {
		}
	}
}

static void xpt2046_gpio_init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	__HAL_RCC_GPIOE_CLK_ENABLE();

	GPIO_InitStruct.Pin = XPT2046_CS_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(XPT2046_CS_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = XPT2046_IRQ_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(XPT2046_IRQ_PORT, &GPIO_InitStruct);

	xpt2046_cs(1);
}

/**
 * Initialize the XPT2046
 */
void xpt2046_init(void)
{
	xpt2046_gpio_init();
	MX_SPI3_Init();

	LV_DRV_INDEV_SPI_CS(0);
	LV_DRV_INDEV_SPI_XCHG_BYTE(0x80);
	LV_DRV_INDEV_SPI_XCHG_BYTE(0);
	LV_DRV_INDEV_SPI_XCHG_BYTE(0);
	LV_DRV_INDEV_SPI_CS(1);
}

uint8_t xpt2046_irq_pin_read(void)
{
	return (uint8_t)HAL_GPIO_ReadPin(XPT2046_IRQ_PORT, XPT2046_IRQ_PIN);
}

/**
 * Get the current position and state of the touchpad
 */
void xpt2046_read(lv_indev_t * indev, lv_indev_data_t * data)
{
	static int16_t last_x = 0;
	static int16_t last_y = 0;
	static bool last_pressed = false;

	bool pressed = (LV_DRV_INDEV_IRQ_READ == 0);

	/* Drop averaged samples from the previous touch so a new tap is not pulled toward the old point. */
	if (!pressed) {
		if (last_pressed) {
			xpt2046_avg_reset();
		}
	}
	else if (!last_pressed) {
		xpt2046_avg_reset();
	}

	if (pressed) {
		LV_DRV_INDEV_SPI_CS(0);

		LV_DRV_INDEV_SPI_XCHG_BYTE(CMD_X_READ);
		uint8_t buf = LV_DRV_INDEV_SPI_XCHG_BYTE(0);
		int16_t x = buf << 8;
		buf = LV_DRV_INDEV_SPI_XCHG_BYTE(CMD_Y_READ);
		x += buf;

		buf = LV_DRV_INDEV_SPI_XCHG_BYTE(0);
		int16_t y = buf << 8;
		buf = LV_DRV_INDEV_SPI_XCHG_BYTE(0);
		y += buf;

		LV_DRV_INDEV_SPI_CS(1);

		x >>= 3;
		y >>= 3;

		xpt2046_corr(&x, &y);
		xpt2046_avg(&x, &y);

		last_x = x;
		last_y = y;
	}

	data->point.x = last_x;
	data->point.y = last_y;
	data->state   = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;

	if (pressed && !last_pressed)
		LV_LOG_USER("[TOUCH] PRESSED (%d,%d)", last_x, last_y);
	else if (!pressed && last_pressed)
		LV_LOG_USER("[TOUCH] RELEASED (%d,%d)", last_x, last_y);

	last_pressed = pressed;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void xpt2046_corr(int16_t * x, int16_t * y)
{
#if XPT2046_XY_SWAP != 0
	int16_t swap_tmp;
	swap_tmp = *x;
	*x = *y;
	*y = swap_tmp;
#endif

	if ((*x) > XPT2046_X_MIN)
		(*x) -= XPT2046_X_MIN;
	else
		(*x) = 0;

	if ((*y) > XPT2046_Y_MIN)
		(*y) -= XPT2046_Y_MIN;
	else
		(*y) = 0;

	(*x) = (uint32_t)((uint32_t)(*x) * XPT2046_HOR_RES) /
	       (XPT2046_X_MAX - XPT2046_X_MIN);

	(*y) = (uint32_t)((uint32_t)(*y) * XPT2046_VER_RES) /
	       (XPT2046_Y_MAX - XPT2046_Y_MIN);

#if XPT2046_X_INV != 0
	(*x) = XPT2046_HOR_RES - (*x);
#endif

#if XPT2046_Y_INV != 0
	(*y) = XPT2046_VER_RES - (*y);
#endif
}

static void xpt2046_avg_reset(void)
{
	avg_last = 0;
}

static void xpt2046_avg(int16_t * x, int16_t * y)
{
	uint8_t i;
	for (i = XPT2046_AVG - 1; i > 0; i--) {
		avg_buf_x[i] = avg_buf_x[i - 1];
		avg_buf_y[i] = avg_buf_y[i - 1];
	}

	avg_buf_x[0] = *x;
	avg_buf_y[0] = *y;
	if (avg_last < XPT2046_AVG)
		avg_last++;

	int32_t x_sum = 0;
	int32_t y_sum = 0;
	for (i = 0; i < avg_last; i++) {
		x_sum += avg_buf_x[i];
		y_sum += avg_buf_y[i];
	}

	(*x) = (int32_t)x_sum / avg_last;
	(*y) = (int32_t)y_sum / avg_last;
}

#endif
