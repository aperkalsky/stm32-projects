/*
 * flash.h
 *
 * Flash driver for W25Q16 serial Flash
 *
 */

#ifndef _FLASH_H_
#define _FLASH_H_

#define FLASH_PAGE_SIZE      256
#define	FLASH_SIZE           (uint32_t)(2*1024*1024)
#define SPI_DMA_THRESHOLD    16	// if less than this, use interrupts for data transfer

// Return statuses
typedef enum{
	FLASH_OK,
	FLASH_TIMEOUT,
	FLASH_INVALID_ARGUMENT,
	FLASH_HW_PROBLEM,
	FLASH_BUSY
}FlashStatus;

// Exposed functions
void FlashDriverInit(void);
void FlashReset(void);
uint32_t FlashReadID(void);
void FlashReadBlocking(uint32_t address, uint32_t size, uint8_t *buffer);

FlashStatus FlashRead(uint32_t address, void *buffer, uint32_t length);
FlashStatus FlashWrite(uint32_t address, const void *buffer, uint32_t length);

// Flash commands
#define FLASH_CMD_WRITE_ENABLE				0x06
#define FLASH_CMD_VOL_SR_WRITE_ENABLE	0x50
#define FLASH_CMD_WRITE_DISABLE				0x04

#define FLASH_CMD_RELEASE_PWR_DOWN		0xAB
#define FLASH_CMD_MANUF_ID						0x90
#define FLASH_CMD_READ_JEDEC_ID				0x9F
#define FLASH_CMD_READ_UNIQUE_ID			0x4B

#define FLASH_CMD_READ_DATA						0x03
#define FLASH_CMD_FAST_READ						0x03

#define FLASH_CMD_PAGE_PROGRAM				0x02

#define FLASH_CMD_BLK_ERASE_64K				0xD8
#define FLASH_CMD_BLK_ERASE_32K				0x52
#define FLASH_CMD_SECT_ERASE_4K				0x20
#define FLASH_CMD_CHIP_ERASE					0xC7

#define FLASH_CMD_READ_STAT_REG_1			0x05
#define FLASH_CMD_WRITE_STAT_REG_1		0x05
#define FLASH_CMD_READ_STAT_REG_2			0x35
#define FLASH_CMD_WRITE_STAT_REG_2		0x31
#define FLASH_CMD_READ_STAT_REG_3			0x15
#define FLASH_CMD_WRITE_STAT_REG_3		0x11

#define FLASH_CMD_READ_SFDP_REG				0x5A
#define FLASH_CMD_ERASE_SEC_REG				0x44
#define FLASH_CMD_PROGRAM_SEC_REG			0x42
#define FLASH_CMD_READ_SEC_REG				0x48

#define FLASH_CMD_GLOB_BLK_LOCK				0x7E
#define FLASH_CMD_GLOB_BLK_UNLOCK			0x98
#define FLASH_CMD_READ_BLK_LOCK				0x3D
#define FLASH_CMD_INDIV_BLK_LOCK			0x36
#define FLASH_CMD_INDIV_BLK_UNLOCK		0x39

#define FLASH_CMD_ERASE_PROG_SUSPEND	0x75
#define FLASH_CMD_ERASE_PROG_RESUME		0x7A
#define FLASH_CMD_POWER_DOWN					0xB9

#define FLASH_CMD_ENABLE_RESET				0x66
#define FLASH_CMD_RESET_DEVICE				0x99

// status register bits
#define STATUS_WIP_BIT                0x01

#endif
