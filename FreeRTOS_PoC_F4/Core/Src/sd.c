#include "main.h"
#include "sd.h"
#include "SEGGER_RTT.h"

extern SD_HandleTypeDef hsd;

static uint8_t sectBuf[SD_SECTOR_SIZE];

void SD_PrintCardInfo()
{
	HAL_SD_CardInfoTypeDef info;

	HAL_SD_GetCardInfo(&hsd, &info);

	SEGGER_RTT_printf(0, "CardType: %d\r\n", info.CardType);
	SEGGER_RTT_printf(0, "CardVersion: %d\r\n", info.CardVersion);
	SEGGER_RTT_printf(0, "Class: %d\r\n", info.Class);
	SEGGER_RTT_printf(0, "RelCardAdd: %d\r\n", info.RelCardAdd);
	SEGGER_RTT_printf(0, "BlockNbr: %d\r\n", info.BlockNbr);
	SEGGER_RTT_printf(0, "BlockSize: %d\r\n", info.BlockSize);
	SEGGER_RTT_printf(0, "LogBlockNbr: %d\r\n", info.LogBlockNbr);
	SEGGER_RTT_printf(0, "LogBlockSize: %d\r\n", info.LogBlockSize);
}

void SD_PrintBootSector()
{
	HAL_StatusTypeDef result;
	HAL_SD_CardStateTypeDef state;

	state = HAL_SD_GetCardState(&hsd);
	SEGGER_RTT_printf(0, "HAL state = %08X\r\n", state);

	result = HAL_SD_ReadBlocks(&hsd, sectBuf, 32, 1, HAL_MAX_DELAY);

	if(result == HAL_OK)
	{
		for(uint16_t i = 0; i < SD_SECTOR_SIZE; i++)
		{
			SEGGER_RTT_printf(0, "%04X: %02X\r\n", i, sectBuf[i]);
		}
	}
	else
	{
		SEGGER_RTT_printf(0, "HAL status = %d\r\n", result);
		SEGGER_RTT_printf(0, "SD Error   = 0x%08lX\r\n", HAL_SD_GetError(&hsd));
	}
}
