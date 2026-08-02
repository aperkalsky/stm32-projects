#include "main.h"
#include "sd.h"
#include "SEGGER_RTT.h"

extern SD_HandleTypeDef hsd;

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
