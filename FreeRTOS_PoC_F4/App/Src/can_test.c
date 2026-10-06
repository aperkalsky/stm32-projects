#include "main.h"
#include "SEGGER_RTT.h"
#include "can_test.h"
#include "cmsis_os.h"


extern CAN_HandleTypeDef hcan;

void CAN_StartDriver()
{
	if (HAL_CAN_Start(&hcan) != HAL_OK)
	{
		Error_Handler();
	}
}

#ifdef DO_CAN_RX

// Rx part
// =======

static CAN_RxHeaderTypeDef rxHeader;
static uint8_t rxData[8];

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	if(hcan == NULL)
	{
		return;
	}

	if (hcan->Instance != CAN1)
	{
		return;
	}

	if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rxHeader,	rxData) != HAL_OK)
	{
		SEGGER_RTT_printf(0, "CAN RX ERROR\r\n");
		return;
	}

	SEGGER_RTT_printf(0, "CAN RX: ID=0x%03lX DLC=%lu DATA=", rxHeader.StdId, rxHeader.DLC);

	for (uint32_t i = 0; i < rxHeader.DLC; i++)
	{
		SEGGER_RTT_printf(0, "%02X ", rxData[i]);
	}

	SEGGER_RTT_printf(0, "\r\n");
}

static void CAN_FilterInit(void)
{
	CAN_FilterTypeDef filter;

	filter.FilterBank = 0;
	filter.FilterMode = CAN_FILTERMODE_IDMASK;
	filter.FilterScale = CAN_FILTERSCALE_32BIT;

	filter.FilterIdHigh = 0x0000;
	filter.FilterIdLow = 0x0000;

	filter.FilterMaskIdHigh = 0x0000;
	filter.FilterMaskIdLow = 0x0000;

	filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;

	filter.FilterActivation = ENABLE;

#if defined(STM32F4)
	filter.SlaveStartFilterBank = 14;
#endif

	if (HAL_CAN_ConfigFilter(&hcan, &filter) != HAL_OK)
	{
		Error_Handler();
	}
}

void CAN_RxInit(void)
{
	CAN_FilterInit();

	if (HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
	{
		Error_Handler();
	}
}

#endif

#ifdef DO_CAN_TX

// Tx part
// =======

static CAN_TxHeaderTypeDef txHeader;
static uint32_t txMailbox;
static uint8_t txData[8];

void CAN_TxInit(void)
{
	txHeader.StdId = 0x123;
	txHeader.ExtId = 0;
	txHeader.IDE = CAN_ID_STD;
	txHeader.RTR = CAN_RTR_DATA;
	txHeader.DLC = 8;
	txHeader.TransmitGlobalTime = DISABLE;

	txData[0] = 0;
	txData[1] = 0x02;
	txData[2] = 0x03;
	txData[3] = 0x04;
	txData[4] = 0x05;
	txData[5] = 0x06;
	txData[6] = 0x07;
	txData[7] = 0x08;
}

HAL_StatusTypeDef CAN_Send(void)
{
	uint32_t freeLevel;

	freeLevel = HAL_CAN_GetTxMailboxesFreeLevel(&hcan);

	if (freeLevel > 0)
	{
		SEGGER_RTT_printf(0, "TX mailboxes free: %d\r\n", freeLevel);

		if (HAL_CAN_AddTxMessage(&hcan, &txHeader, txData, &txMailbox) != HAL_OK)
		{
			uint32_t error = HAL_CAN_GetError(&hcan);

			SEGGER_RTT_printf(0, "CAN TX ERROR: 0x%08lX\r\n", error);
			return HAL_ERROR;
		}
		else
		{
			SEGGER_RTT_WriteString(0, "CAN TX: frame added to mailbox\r\n");
			return HAL_OK;
		}
	}
	else
	{
		uint32_t error = HAL_CAN_GetError(&hcan);

		SEGGER_RTT_WriteString(0, "CAN TX: No free mailbox\r\n");
		SEGGER_RTT_printf(0, "CAN state: %d\r\n", HAL_CAN_GetState(&hcan));
		SEGGER_RTT_printf(0, "CAN error: 0x%08lX\r\n", error);
		SEGGER_RTT_printf(0, "TSR = 0x%08lX\r\n", hcan.Instance->TSR);
		SEGGER_RTT_printf(0, "ESR = 0x%08lX\r\n", hcan.Instance->ESR);

		return HAL_BUSY;
	}
}

void CAN_TestModifyTxData()
{
	txData[0]++;
}

#endif
