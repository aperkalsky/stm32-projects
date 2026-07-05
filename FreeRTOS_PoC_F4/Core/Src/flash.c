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
#include "SEGGER_RTT.h"

extern SPI_HandleTypeDef hspi1;

uint8_t spiIoBuf[10];
uint8_t spiRxBuf[10];

// FreeRTOS Binary Semaphore to signal SPI transfer completion. Both Rx and Tx
static osSemaphoreId_t spiIoSemHandle;

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

// Callbacks triggered by HAL when the non-blocking transfer finishes
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if (hspi->Instance == SPI1)
	{
		osSemaphoreRelease(spiIoSemHandle);
	}
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if (hspi->Instance == SPI1)
	{
		osSemaphoreRelease(spiIoSemHandle);
	}
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if (hspi->Instance == SPI1)
	{
		osSemaphoreRelease(spiIoSemHandle);
	}
}

FlashStatus FlashWaitUntilReadyNonBlocking(uint32_t timeoutTicks, uint32_t pollDelayMs)
{
	HAL_StatusTypeDef hal_status;
	osStatus_t rtos_status;

	uint32_t startTime = osKernelGetTickCount();

	spiIoBuf[0] = FLASH_CMD_READ_STAT_REG_1;
	spiIoBuf[1] = 0x00; // Dummy byte to read response

	while (1)
	{
		// Enforce global safety timeout
		if ((osKernelGetTickCount() - startTime) >= timeoutTicks)
		{
			return FLASH_TIMEOUT;
		}

		// Pull Chip Select LOW to start the command
		FlashCsSelect();

		// Start non-blocking 2-byte transfer
		hal_status = HAL_SPI_TransmitReceive_IT(&hspi1, spiIoBuf, spiRxBuf, 2);
		if (hal_status != HAL_OK)
		{
			FlashCsDeselect();
			return FLASH_HW_PROBLEM;
		}

		// Sleep the thread until the SPI hardware finishes reading the byte. Delay in ms
		rtos_status = osSemaphoreAcquire(spiIoSemHandle, 10);
		if (rtos_status != osOK)
		{
			FlashCsDeselect();
			return FLASH_TIMEOUT;
		}

		// 5. Inspect the received register byte (stored in spiRxBuf[1])
		uint8_t statusRegister = spiRxBuf[1];
		SEGGER_RTT_printf(0, "Flash stat = %02X\r\n", statusRegister);

		if ((statusRegister & STATUS_WIP_BIT) == 0)
		{
			FlashCsDeselect();
			return FLASH_OK; // Success: Flash is clean and ready!
		}

		// 6. Flash is busy. Sleep based on operation type to save CPU power.
		osDelay(pollDelayMs);
	}
}

/**
 * @brief  Reads a block of data from the SPI Flash memory asynchronously.
 * @param  flashAddress: 24-bit physical start address in flash.
 * @param  pData: Pointer to the destination buffer where data will be stored.
 * @param  size: Number of bytes to read. Up to 64K
 * @param  timeoutMs: Maximum time allowed for the operation to complete.
 * @retval FlashStatus: OK on success, error code otherwise.
 */
FlashStatus FlashReadNonBlocking(uint32_t flashAddress, uint8_t *pData, uint32_t size, uint32_t timeoutMs)
{
	HAL_StatusTypeDef hal_status;
	FlashStatus wait_status;
	osStatus_t rtos_status;

	// 1. Prepare standard 4-byte command array [Command, Addr2, Addr1, Addr0]
	spiIoBuf[0] = FLASH_CMD_READ_DATA;
	spiIoBuf[1] = (flashAddress >> 16) & 0xFF;
	spiIoBuf[2] = (flashAddress >> 8)  & 0xFF;
	spiIoBuf[3] =  flashAddress        & 0xFF;

	// 2. Track total elapsed time using RTOS ticks
	uint32_t startTime = osKernelGetTickCount();
	uint32_t timeoutTicks = pdMS_TO_TICKS(timeoutMs);

	// 3. Ensure the flash chip is not busy before starting a read
	wait_status = FlashWaitUntilReadyNonBlocking(timeoutTicks, 1);
	if (wait_status != FLASH_OK)
	{
		return wait_status;
	}

	// Recalculate remaining timeout after waiting for busy state
	uint32_t elapsedTime = osKernelGetTickCount() - startTime;
	if (elapsedTime >= timeoutTicks)
	{
		return FLASH_TIMEOUT;
	}
	uint32_t remainingTimeoutTicks = timeoutTicks - elapsedTime;

	// 4. Assert Chip Select Low to begin SPI transaction
	FlashCsSelect();

	// 5. Send the 4-byte command packet using non-blocking Interrupt mode
	hal_status = HAL_SPI_Transmit_IT(&hspi1, spiIoBuf, 4);
	if (hal_status != HAL_OK)
	{
		FlashCsDeselect();
		return FLASH_HW_PROBLEM;
	}

	// 6. Block thread until command transmission finishes
	rtos_status = osSemaphoreAcquire(spiIoSemHandle, remainingTimeoutTicks);
	if (rtos_status != osOK)
	{
		FlashCsDeselect();
		return FLASH_TIMEOUT;
	}

	// Recalculate remaining timeout for the actual data phase
	elapsedTime = osKernelGetTickCount() - startTime;
	if (elapsedTime >= timeoutTicks)
	{
		FlashCsDeselect();
		return FLASH_TIMEOUT;
	}
	remainingTimeoutTicks = timeoutTicks - elapsedTime;

	// 7. Receive data payload using optimal peripheral strategy
	if (size >= SPI_DMA_THRESHOLD)
	{
		hal_status = HAL_SPI_Receive_DMA(&hspi1, pData, size);
	} else {
		hal_status = HAL_SPI_Receive_IT(&hspi1, pData, size);
	}

	if (hal_status != HAL_OK)
	{
		FlashCsDeselect();
		return FLASH_HW_PROBLEM;
	}

	// 8. Block thread until data payload reception finishes
	rtos_status = osSemaphoreAcquire(spiIoSemHandle, remainingTimeoutTicks);

	// 9. De-assert Chip Select HIGH immediately to end transaction
	FlashCsDeselect();

	if (rtos_status != osOK)
	{
		return FLASH_TIMEOUT;
	}
	else
	{
		return FLASH_OK;
	}
}

// =================
// exposed functions
// =================


void FlashDriverInit(void)
{
	const osSemaphoreAttr_t sem_attributes = { .name = "spiIoSem" };
	spiIoSemHandle = osSemaphoreNew(1, 0, &sem_attributes);
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
	uint32_t num_bytes_to_read;
	uint32_t remaining_length = length;
	FlashStatus status;
	uint8_t* pBuf = (uint8_t*)buffer;

	SEGGER_RTT_printf(0, "FlashRead(%08X, %d)\r\n", address, length);

	// argument validation
	if((buffer == NULL) || (length == 0) || (address + length > FLASH_SIZE))
	{
		return FLASH_INVALID_ARGUMENT;
	}

	// read data in chunks of page size
	while(remaining_length > 0)
	{
		if(remaining_length <= FLASH_PAGE_SIZE)
		{
			num_bytes_to_read = remaining_length;
		}
		else
		{
			num_bytes_to_read = FLASH_PAGE_SIZE;
		}

		status = FlashReadNonBlocking(address, pBuf, num_bytes_to_read, 200);

		SEGGER_RTT_printf(0, "Num bytes to read = %d result = %d \r\n", num_bytes_to_read, status);

		if(status != FLASH_OK)
		{
			return status;
		}
		else
		{
			remaining_length -= num_bytes_to_read;	// will become 0 after last chunk is read
			pBuf = (uint8_t*)((uint32_t)pBuf + num_bytes_to_read);
			address += num_bytes_to_read;
		}
	}

	return FLASH_OK;
}

FlashStatus FlashWrite(uint32_t address, const void *buffer, uint32_t length)
{
	return FLASH_OK;
}

// blocking variant
void FlashTestRead(uint32_t address, uint32_t size, uint8_t *buffer)
{
	uint8_t tData[5];

	tData[0] = 0x03;  // enable Read
	tData[1] = (address>>16)&0xFF;  // MSB of the memory Address
	tData[2] = (address>>8)&0xFF;
	tData[3] = (address)&0xFF; // LSB of the memory Address

	FlashCsSelect();  // pull the CS Low

	SPI_Write(tData, 4);  // send read instruction along with the 24 bit memory address

	SPI_Read(buffer, size);  // Read the data

	FlashCsDeselect();  // pull the CS High
}

