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
#include <string.h>
#include "cmsis_os.h"
#include "flash.h"
#include "main.h"
#include "SEGGER_RTT.h"

extern SPI_HandleTypeDef hspi1;

static uint8_t spiCmdBuf[10];
static uint8_t spiTxBuf[FLASH_SECTOR_SIZE_4K];
//uint8_t spiRxBuf[10];

// FreeRTOS Binary Semaphore to signal SPI transfer completion. Both Rx and Tx
static osSemaphoreId_t spiIoSemHandle;

static uint32_t txCnt = 0;
static uint32_t rxCnt = 0;
static uint32_t txrxCnt = 0;

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
		txCnt += 1;
//		SEGGER_RTT_WriteString(0, "Tx cplt\r\n");
	}
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if (hspi->Instance == SPI1)
	{
		osSemaphoreRelease(spiIoSemHandle);
		rxCnt += 1;
//		SEGGER_RTT_WriteString(0, "Rx cplt\r\n");
	}
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if (hspi->Instance == SPI1)
	{
		osSemaphoreRelease(spiIoSemHandle);
		txrxCnt += 1;
//		SEGGER_RTT_WriteString(0, "TxRx cplt\r\n");
	}
}

FlashStatus_t FlashWaitUntilReadyNonBlocking(uint32_t timeoutTicks, uint32_t pollDelayMs)
{
	HAL_StatusTypeDef hal_status;
	osStatus_t rtos_status;
	uint8_t statusTxBuf[2];
	uint8_t statusRxBuf[2];

	uint32_t startTime = osKernelGetTickCount();

	statusTxBuf[0] = FLASH_CMD_READ_STAT_REG_1;
	statusTxBuf[1] = 0x00; // Dummy byte to read response

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
		hal_status = HAL_SPI_TransmitReceive_IT(&hspi1, statusTxBuf, statusRxBuf, 2);
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

		// Inspect the received register byte (stored in statusRxBuf[1])
		uint8_t statusRegister = statusRxBuf[1];
		SEGGER_RTT_printf(0, "Flash stat = %02X\r\n", statusRegister);

		if ((statusRegister & STATUS_WIP_BIT) == 0)
		{
			FlashCsDeselect();
			return FLASH_OK; // Success: Flash is clean and ready!
		}

		// Flash is busy. Sleep based on operation type to save CPU power.
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
FlashStatus_t FlashReadNonBlocking(uint32_t flashAddress, uint8_t *pData, uint32_t size, uint32_t timeoutMs)
{
	HAL_StatusTypeDef hal_status;
	osStatus_t rtos_status;

	memset(spiTxBuf, 0x00, size);

	// Prepare standard 4-byte command array [Command, Addr2, Addr1, Addr0]
	spiCmdBuf[0] = FLASH_CMD_READ_DATA;
	spiCmdBuf[1] = (flashAddress >> 16) & 0xFF;
	spiCmdBuf[2] = (flashAddress >> 8)  & 0xFF;
	spiCmdBuf[3] =  flashAddress        & 0xFF;

	// Track total elapsed time using RTOS ticks
	uint32_t timeoutTicks = pdMS_TO_TICKS(timeoutMs);
	uint32_t startTime = osKernelGetTickCount();


	// Assert Chip Select Low to begin SPI transaction
	FlashCsSelect();

	// Send the 4-byte command packet using non-blocking Interrupt mode
	hal_status = HAL_SPI_Transmit_IT(&hspi1, spiCmdBuf, 4);
	if (hal_status != HAL_OK)
	{
		FlashCsDeselect();
		return FLASH_HW_PROBLEM;
	}

	// Block thread until command transmission finishes
	rtos_status = osSemaphoreAcquire(spiIoSemHandle, timeoutTicks);
	if (rtos_status != osOK)
	{
		FlashCsDeselect();
		return FLASH_TIMEOUT;
	}

	// Recalculate remaining timeout for the actual data phase
	uint32_t elapsedTime = osKernelGetTickCount() - startTime;
	if (elapsedTime >= timeoutTicks)
	{
		FlashCsDeselect();
		return FLASH_TIMEOUT;
	}
	uint32_t remainingTimeoutTicks = timeoutTicks - elapsedTime;

	// Receive data payload using optimal peripheral strategy
	if (size >= SPI_DMA_THRESHOLD)
	{
	    hal_status = HAL_SPI_TransmitReceive_DMA(&hspi1, spiTxBuf, pData, size);
	}
	else
	{
	    hal_status = HAL_SPI_TransmitReceive_IT(&hspi1, spiTxBuf, pData, size);
	}

	if (hal_status != HAL_OK)
	{
		FlashCsDeselect();
		return FLASH_HW_PROBLEM;
	}

	// Block thread until data payload reception finishes
	rtos_status = osSemaphoreAcquire(spiIoSemHandle, remainingTimeoutTicks);


	// De-assert Chip Select HIGH immediately to end transaction
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

void FlashWriteEnable(void)
{
	spiCmdBuf[0] = FLASH_CMD_WRITE_ENABLE;
	FlashCsSelect();
	SPI_Write(spiCmdBuf, 1);
	FlashCsDeselect();
}

void FlashWriteDisable(void)
{
	spiCmdBuf[0] = FLASH_CMD_WRITE_DISABLE;
	FlashCsSelect();
	SPI_Write(spiCmdBuf, 1);
	FlashCsDeselect();
}

// Programs one page in non-blocking mode
FlashStatus_t FlashPageProgram(uint32_t address, void *buffer, uint32_t length)
{
	HAL_StatusTypeDef hal_status;
	osStatus_t rtos_status;

	// argument validation
	if((buffer == NULL) || (length == 0) || (length > FLASH_PAGE_SIZE) ||(address + length > FLASH_SIZE))
	{
		return FLASH_INVALID_ARGUMENT;
	}

	if((address & LSB_ADDRESS_MASK) > FLASH_PAGE_SIZE)
	{
		return FLASH_INVALID_ARGUMENT;
	}

	if(!buffer || (length > FLASH_PAGE_SIZE))
	{
		return FLASH_INVALID_ARGUMENT;
	}

	// Unlock Flash for writing
	FlashWriteEnable();

	// Prepare standard 4-byte command array [Command, Addr2, Addr1, Addr0]
	spiCmdBuf[0] = FLASH_CMD_PAGE_PROGRAM;
	spiCmdBuf[1] = (address >> 16) & 0xFF;
	spiCmdBuf[2] = (address >> 8)  & 0xFF;
	spiCmdBuf[3] =  address        & 0xFF;

	// Track total elapsed time using RTOS ticks
	uint32_t timeoutTicks = pdMS_TO_TICKS(PAGE_PROGRAM_TIMEOUT_MS);
	uint32_t startTime = osKernelGetTickCount();


	// Assert Chip Select Low to begin SPI transaction
	FlashCsSelect();

	// Send the 4-byte command packet using non-blocking Interrupt mode
	hal_status = HAL_SPI_Transmit_IT(&hspi1, spiCmdBuf, 4);
	if (hal_status != HAL_OK)
	{
		FlashCsDeselect();
		return FLASH_HW_PROBLEM;
	}

	// Block thread until command transmission finishes
	rtos_status = osSemaphoreAcquire(spiIoSemHandle, timeoutTicks);
	if (rtos_status != osOK)
	{
		FlashCsDeselect();
		return FLASH_TIMEOUT;
	}

	// Recalculate remaining timeout for the actual data phase
	uint32_t elapsedTime = osKernelGetTickCount() - startTime;
	if (elapsedTime >= timeoutTicks)
	{
		FlashCsDeselect();
		return FLASH_TIMEOUT;
	}
	uint32_t remainingTimeoutTicks = timeoutTicks - elapsedTime;

	// Program data payload using optimal peripheral strategy
	if (length >= SPI_DMA_THRESHOLD)
	{
	    hal_status = HAL_SPI_TransmitReceive_DMA(&hspi1, buffer, spiTxBuf, length);
	}
	else
	{
	    hal_status = HAL_SPI_TransmitReceive_IT(&hspi1, buffer, spiTxBuf, length);
	}

	if (hal_status != HAL_OK)
	{
		FlashCsDeselect();
		return FLASH_HW_PROBLEM;
	}

	// Block thread until data I/O finishes
	rtos_status = osSemaphoreAcquire(spiIoSemHandle, remainingTimeoutTicks);

	// De-assert Chip Select HIGH immediately to end transaction
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
	spiCmdBuf[0] = FLASH_CMD_ENABLE_RESET;
	spiCmdBuf[1] = FLASH_CMD_RESET_DEVICE;
	FlashCsSelect();
	SPI_Write(spiCmdBuf, 2);
	FlashCsDeselect();
	osDelay(100);
}

uint32_t FlashReadID(void)
{
	spiCmdBuf[0] = FLASH_CMD_READ_JEDEC_ID;
	FlashCsSelect();
	SPI_Write(spiCmdBuf, 1);
	SPI_Read(spiCmdBuf, 3);
	FlashCsDeselect();
	return ((spiCmdBuf[0]<<16)|(spiCmdBuf[1]<<8)|spiCmdBuf[2]);
}

FlashStatus_t FlashRead(uint32_t address, void *buffer, uint32_t length)
{
	uint32_t num_bytes_to_read;
	uint32_t remaining_length = length;
	FlashStatus_t status;
	uint8_t* pBuf = (uint8_t*)buffer;

//	SEGGER_RTT_printf(0, "FlashRead(%08X, %d)\r\n", address, length);

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

//		SEGGER_RTT_printf(0, "Num bytes to read = %d result = %d \r\n", num_bytes_to_read, status);

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

FlashStatus_t FlashWrite(uint32_t address, const void *buffer, uint32_t length)
{
	return FLASH_OK;
}

// blocking variant
void FlashReadBlocking(uint32_t address, uint32_t size, uint8_t *buffer)
{
	spiCmdBuf[0] = FLASH_CMD_READ_DATA;  // enable Read
	spiCmdBuf[1] = (address>>16)&0xFF;  // MSB of the memory Address
	spiCmdBuf[2] = (address>>8)&0xFF;
	spiCmdBuf[3] = (address)&0xFF; // LSB of the memory Address

	FlashCsSelect();  // pull the CS Low

	SPI_Write(spiCmdBuf, 4);  // send read instruction along with the 24 bit memory address

	SPI_Read(buffer, size);  // Read the data

	FlashCsDeselect();  // pull the CS High
}

// typical erase time is 5 sec, max time - 25 sec
FlashStatus_t FlashChipErase(void)
{
	FlashStatus_t wait_status;
	uint32_t timeoutTicks = pdMS_TO_TICKS(FLASH_CHIP_ERASE_TIMEOUT_MS);

	// open the chip
	FlashWriteEnable();

	// send erase command
	spiCmdBuf[0] = FLASH_CMD_CHIP_ERASE;
	FlashCsSelect();
	SPI_Write(spiCmdBuf, 1);
	FlashCsDeselect();

	// wait for completion
	wait_status = FlashWaitUntilReadyNonBlocking(timeoutTicks, FLASH_CHIP_ERASE_POLL_INTERVAL_MS);

	// close the chip in any case
	FlashWriteDisable();

	return wait_status;
}
