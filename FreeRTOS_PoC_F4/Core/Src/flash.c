/*
 * flash.c
 *
 * Flash driver for W25Q16 serial Flash
 *
 */

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "cmsis_os.h"
#include "flash.h"
#include "main.h"
#include "SEGGER_RTT.h"
#include "debug_io.h"

extern SPI_HandleTypeDef hspi1;

static uint8_t spiCmdBuf[MAX_FLASH_CMD_LENGTH + 1];	// buffer for short commands
static uint8_t spiTxRxBuf[FLASH_SECTOR_SIZE_4K + MAX_FLASH_CMD_LENGTH]; // for non-blocking I/O, mostly read/write
static uint8_t sectorBuf[FLASH_SECTOR_SIZE_4K];	// buffer for (re)programming a sector

// FreeRTOS Binary Semaphore to signal SPI transfer completion
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

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if (hspi->Instance == SPI1)
	{
		osSemaphoreRelease(spiIoSemHandle);
	}
}

FlashStatus_t FlashWaitUntilReady(uint32_t timeoutMs, uint32_t pollDelayMs)
{
	HAL_StatusTypeDef hal_status;
	uint8_t statusTxBuf[2];
	uint8_t statusRxBuf[2];

	uint32_t startTime = osKernelGetTickCount();

	// a separate buffer for Tx is used, as we want to send the same command more than once
	statusTxBuf[0] = FLASH_CMD_READ_STAT_REG_1;
	statusTxBuf[1] = 0x00; // Dummy byte to read response

	while (1)
	{
		// Enforce global safety timeout
		if ((osKernelGetTickCount() - startTime) >= pdMS_TO_TICKS(timeoutMs))
		{
			return FLASH_TIMEOUT;
		}

		// Pull Chip Select LOW to start the command
		FlashCsSelect();

		// Start non-blocking 2-byte transfer
		hal_status = HAL_SPI_TransmitReceive(&hspi1, statusTxBuf, statusRxBuf, 2, timeoutMs);
		if (hal_status != HAL_OK)
		{
			FlashCsDeselect();
			return FLASH_HW_PROBLEM;
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

	// Prepare standard 4-byte command array [Command, Addr2, Addr1, Addr0]
	spiTxRxBuf[0] = FLASH_CMD_READ_DATA;
	spiTxRxBuf[1] = (flashAddress >> 16) & 0xFF;
	spiTxRxBuf[2] = (flashAddress >> 8)  & 0xFF;
	spiTxRxBuf[3] =  flashAddress        & 0xFF;

	FlashCsSelect();

	// use the same buffer for Tx/Rx
	hal_status = HAL_SPI_TransmitReceive_DMA(&hspi1, spiTxRxBuf, spiTxRxBuf, size + MAX_FLASH_CMD_LENGTH);

	if (hal_status != HAL_OK)
	{
		FlashCsDeselect();
		return FLASH_HW_PROBLEM;
	}

	// Block thread until command transmission finishes
	rtos_status = osSemaphoreAcquire(spiIoSemHandle, pdMS_TO_TICKS(timeoutMs));

	FlashCsDeselect();

	if (rtos_status != osOK)
	{
		return FLASH_TIMEOUT;
	}
	else
	{
		// copy received data to user buffer
		memcpy(pData, &spiTxRxBuf[MAX_FLASH_CMD_LENGTH], size);

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
FlashStatus_t FlashPageProgram(uint32_t address, const void *buffer, uint32_t length)
{
	HAL_StatusTypeDef hal_status;
	osStatus_t rtos_status;

//	SEGGER_RTT_printf(0, "FlashPageProgram 0x%08X, %d\r\n", address, length);

	// argument validation
	if((buffer == NULL) || (length == 0) || (length > FLASH_PAGE_SIZE) ||(address + length > FLASH_SIZE))
	{
		return FLASH_INVALID_ARGUMENT;
	}

	if(!buffer || (length > FLASH_PAGE_SIZE))
	{
		return FLASH_INVALID_ARGUMENT;
	}

	// Prepare standard 4-byte command array [Command, Addr2, Addr1, Addr0]
	spiTxRxBuf[0] = FLASH_CMD_PAGE_PROGRAM;
	spiTxRxBuf[1] = (address >> 16) & 0xFF;
	spiTxRxBuf[2] = (address >> 8)  & 0xFF;
	spiTxRxBuf[3] =  address        & 0xFF;

	// fill data to be written
	memcpy(&spiTxRxBuf[MAX_FLASH_CMD_LENGTH], buffer,  length);

	// Unlock Flash for writing
	FlashWriteEnable();

	// Assert Chip Select Low to begin SPI transaction
	FlashCsSelect();

	// Send the 4-byte command packet using non-blocking TxRx Interrupt mode
	hal_status = HAL_SPI_TransmitReceive_DMA(&hspi1, spiTxRxBuf, spiTxRxBuf, length + MAX_FLASH_CMD_LENGTH);

	if (hal_status != HAL_OK)
	{
		FlashCsDeselect();
		return FLASH_HW_PROBLEM;
	}

	// Block thread until command transmission finishes
	rtos_status = osSemaphoreAcquire(spiIoSemHandle, pdMS_TO_TICKS(FLASH_PAGE_PROG_TIMEOUT_MS));

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

static inline uint32_t FlashOffsetInPage(uint32_t address)
{
    return address & (FLASH_PAGE_SIZE - 1U);
}

static inline uint32_t FlashSectorStart(uint32_t address)
{
    return address & ~(FLASH_SECTOR_SIZE_4K - 1U);
}

static inline uint32_t FlashOffsetInSector(uint32_t address)
{
    return address & (FLASH_SECTOR_SIZE_4K - 1U);
}

// Checks whether programming the supplied data would require a sector erase
// Returns true if any bit would need to change from 0 -> 1.
static bool FlashSectorNeedsErase(uint32_t offset, const uint8_t *src, uint32_t length)
{
	uint32_t i;

	for (i = 0; i < length; i++)
	{
		uint8_t oldByte = sectorBuf[offset + i];
		uint8_t newByte = src[i];

		if ((oldByte & newByte) != newByte)
		{
			return true;
		}
	}

	return false;
}

static FlashStatus_t FlashSectorErase(uint32_t sectorStart)
{
	FlashStatus_t wait_status;

//	SEGGER_RTT_printf(0, "FlashSectorErase 0x%08X\r\n", sectorStart);

	// open the chip
	FlashWriteEnable();

	// send erase command
	spiCmdBuf[0] = FLASH_CMD_SECT_ERASE_4K;
	spiCmdBuf[1] = (sectorStart >> 16) & 0xFF;
	spiCmdBuf[2] = (sectorStart >> 8)  & 0xFF;
	spiCmdBuf[3] =  sectorStart        & 0xFF;

	FlashCsSelect();
	SPI_Write(spiCmdBuf, 4);
	FlashCsDeselect();

	// wait for completion
	wait_status = FlashWaitUntilReady(FLASH_SECTOR_ERASE_TIMEOUT_MS, FLASH_SECTOR_ERASE_POLL_INTERVAL_MS);

	// No need for Write disable: Note that the WEL bit is automatically reset after Power-up and upon
	// completion of the Write Status Register, Erase/Program Security Registers, Page Program, Quad Page
	// Program, Sector Erase, Block Erase, Chip Erase and Reset instructions

	return wait_status;
}

static FlashStatus_t FlashWriteSectorPartial(uint32_t sectorStart, uint32_t offset, const uint8_t *src, uint32_t length)
{
	FlashStatus_t status = FLASH_OK;
	uint8_t pageIndex;

//	SEGGER_RTT_printf(0, "FlashWriteSectorPartial(0x%08X off=%d len=%d)\r\n", sectorStart, offset, length);

	if(offset + length > FLASH_SECTOR_SIZE_4K)
	{
		return FLASH_INVALID_ARGUMENT;
	}

	// read entire sector to internal buffer
	status = FlashReadNonBlocking(sectorStart, sectorBuf, FLASH_SECTOR_SIZE_4K, FLASH_SECTOR_READ_TIMEOUT_MS);

	if(status != FLASH_OK)
	{
		return status;
	}

	// determine dirty pages
	uint16_t dirtyPages = 0;
	uint32_t firstPage = offset / FLASH_PAGE_SIZE;
	uint32_t lastPage = (offset + length - 1) / FLASH_PAGE_SIZE;

	for(pageIndex = firstPage; pageIndex <= lastPage; pageIndex++)
	{
//		SEGGER_RTT_printf(0, "Page %d dirty\r\n", pageIndex);
		dirtyPages |= (1u << pageIndex);
	}

	// do we need to erase this sector?
	bool eraseNeeded = FlashSectorNeedsErase(offset, src, length);
//	SEGGER_RTT_printf(0, "Need erase = %d\r\n", eraseNeeded);

	if(!eraseNeeded)
	{
		// program the pages directly
		uint32_t remaining = length;
		uint32_t flashAddr = sectorStart + offset;
		const uint8_t *p = src;

		while(remaining)
		{
			uint32_t pageOffset = FlashOffsetInPage(flashAddr);
			uint32_t bytesThisPage = FLASH_PAGE_SIZE - pageOffset;

			if(bytesThisPage > remaining)
			{
				bytesThisPage = remaining;
			}

			status = FlashPageProgram(flashAddr, p, bytesThisPage);

			if(status != FLASH_OK)
			{
				return status;
			}

			flashAddr += bytesThisPage;
			p += bytesThisPage;
			remaining -= bytesThisPage;
		}
	}
	else
	{
		// sector erase is needed
		// ----------------------

		// merge new data
		memcpy(&sectorBuf[offset], src, length);

		status = FlashSectorErase(sectorStart);

		if(status != FLASH_OK)
		{
			return status;
		}

		// rewrite pages
		for(pageIndex = 0; pageIndex < NUM_PAGES_IN_4k_SECTOR; pageIndex++)
		{
			if((dirtyPages & (1u << pageIndex)) == 0)
			{
				continue;
			}

			status = FlashPageProgram(sectorStart + (pageIndex * FLASH_PAGE_SIZE), &sectorBuf[pageIndex * FLASH_PAGE_SIZE], FLASH_PAGE_SIZE);

			if(status != FLASH_OK)
			{
				return status;
			}
		}
	}

	return status;
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

/*
FlashStatus_t FlashWrite(uint32_t address, const void *buffer, uint32_t length)
{
	// for meantime call page program directly
	return FlashPageProgram(address, (void*)buffer, length);
} */

// blocking variant
void FlashReadBlocking(uint32_t address, uint32_t length, uint8_t *buffer)
{
	spiCmdBuf[0] = FLASH_CMD_READ_DATA;  // enable Read
	spiCmdBuf[1] = (address>>16)&0xFF;  // MSB of the memory Address
	spiCmdBuf[2] = (address>>8)&0xFF;
	spiCmdBuf[3] = (address)&0xFF; // LSB of the memory Address

	FlashCsSelect();  // pull the CS Low

	SPI_Write(spiCmdBuf, 4);  // send read instruction along with the 24 bit memory address

	SPI_Read(buffer, length);  // Read the data

	FlashCsDeselect();  // pull the CS High
}

// typical erase time is 5 sec, max time - 25 sec
FlashStatus_t FlashChipErase(void)
{
	FlashStatus_t wait_status;

	// open the chip
	FlashWriteEnable();

	// send erase command
	spiCmdBuf[0] = FLASH_CMD_CHIP_ERASE;
	FlashCsSelect();
	SPI_Write(spiCmdBuf, 1);
	FlashCsDeselect();

	// wait for completion
	wait_status = FlashWaitUntilReady(FLASH_CHIP_ERASE_TIMEOUT_MS, FLASH_CHIP_ERASE_POLL_INTERVAL_MS);

	// No need fro Write disable: Note that the WEL bit is automatically reset after Power-up and upon
	// completion of the Write Status Register, Erase/Program Security Registers, Page Program, Quad Page
	// Program, Sector Erase, Block Erase, Chip Erase and Reset instructions

	return wait_status;
}

/*
 * FlashWrite()
 *
 * Writes data to the specified address, internally performs erase if needed
 *
 * Accept data lengths up to 64K (HAL limit)
 *
 * The design:
 * -----------
 * Walk in steps of one 4K sector
 * Read sector to internal sector buffer
 * Create bitmap of dirty pages (those that overlap with input data)
 * Check if sector erase is needed
 * If erase is not needed:
 *   For each page in dirty bitmap - program the page directly
 * Else (erase is needed):
 *   Merge new data
 *   Erase sector
 *   Program this sector page by page
 */
FlashStatus_t FlashWrite(uint32_t address, const void *buffer, uint32_t length)
{
	FlashStatus_t status = FLASH_OK;
	const uint8_t *pSrc = (const uint8_t *)buffer;

//	SEGGER_RTT_printf(0, "FlashWrite 0x%08X, %d\r\n", address, length);

	if ((buffer == NULL) ||	(length == 0U) ||	((address + length) > FLASH_SIZE))
	{
		return FLASH_INVALID_ARGUMENT;
	}

	while (length > 0U)
	{
		uint32_t sectorStart;
		uint32_t offsetInSector;
		uint32_t bytesThisSector;

		sectorStart    = FlashSectorStart(address);
		offsetInSector = FlashOffsetInSector(address);

		bytesThisSector = FLASH_SECTOR_SIZE_4K - offsetInSector;

		if (bytesThisSector > length)
		{
			bytesThisSector = length;
		}

		status = FlashWriteSectorPartial(sectorStart,	offsetInSector, pSrc, bytesThisSector);

		if (status != FLASH_OK)
		{
			break;
		}

		address += bytesThisSector;
		pSrc += bytesThisSector;
		length -= bytesThisSector;
	}

	return status;
}
