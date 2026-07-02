/*
 * flash.c
 *
 * Flash driver for W25Q16 serial Flash
 *
 *
 * Driver Architecture
 *
 * [User App Thread] ──> Calls Flash_Write_NonBlocking()
 *                           │
 *                           ├── Loops through pages (Max 256 bytes)
 *                           ├── Sends Page Program Command via IT or DMA
 *                           ├── Takes Semaphore (Thread goes to sleep, CPU is free)
 *                           │       ^
 *[Hardware Interrupt] ──────┴───────┼─── Releases Semaphore when transfer finishes
 *                                   │
 *                      [Thread wakes up, loops to next page]
 */

#include <stdint.h>
#include "cmsis_os.h"
#include "flash.h"
#include "main.h"

extern SPI_HandleTypeDef hspi1;

uint8_t spiIoBuf[10];

// FreeRTOS Binary Semaphore to signal SPI transfer completion
static osSemaphoreId_t spiTxSemHandle;

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

// Callback triggered by HAL when the non-blocking transfer finishes
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1)
    {
        // Wake up the waiting thread instantly
        osSemaphoreRelease(spiTxSemHandle);
    }
}

// =================
// exposed functions
// =================


void FlashDriverInit(void)
{
    const osSemaphoreAttr_t sem_attributes = { .name = "spiTxSem" };
    spiTxSemHandle = osSemaphoreNew(1, 0, &sem_attributes);
}

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

FlashStatus FlashRead(uint32_t address, void *buffer, uint32_t length)
{
	// argument validation
	if((buffer == NULL) || (length == 0) || (address + length > FLASH_SIZE))
	{
		return FLASH_INVALID_ARGUMENT;
	}

	return FLASH_OK;
}

FlashStatus FlashWrite(uint32_t address, const void *buffer, uint32_t length)
{
	return FLASH_OK;
}
