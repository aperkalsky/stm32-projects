/*
 * flash.c
 *
 * Flash driver for W25Q16 serial Flash
 *
 */

#include <stdint.h>
#include "cmsis_os.h"
#include "flash.h"
#include "main.h"

extern SPI_HandleTypeDef hspi1;

uint8_t spiIoBuf[10];


// internal functions
// ==================

void FlashCsSelect(void)
{
	HAL_GPIO_WritePin (GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
}

void FlashCsDeselect(void)
{
	HAL_GPIO_WritePin (GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
}

void SPI_Write(uint8_t *data, uint8_t len)
{
	HAL_SPI_Transmit(&hspi1, data, len, 2000);
}

void SPI_Read(uint8_t *data, uint8_t len)
{
	HAL_SPI_Receive(&hspi1, data, len, 5000);
}

// exposed functions
// =================

void FlashReset(void)
{
	spiIoBuf[0] = FLASH_CMD_ENABLE_RESET;
	spiIoBuf[1] = FLASH_CMD_RESET_DEVICE;
	FlashCsSelect();
	SPI_Write(spiIoBuf, 2);
	FlashCsDeselect();
	osDelay(100);
}

uint32_t FlashReadID(void)
{
	spiIoBuf[0] = FLASH_CMD_READ_JEDEC_ID;
	FlashCsSelect();
	SPI_Write(spiIoBuf, 1);
	SPI_Read(spiIoBuf, 3);
	FlashCsDeselect();
	return ((spiIoBuf[0]<<16)|(spiIoBuf[1]<<8)|spiIoBuf[2]);
}
