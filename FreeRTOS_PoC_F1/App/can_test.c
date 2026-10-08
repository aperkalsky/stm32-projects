#include "main.h"
#include "my_rtt.h"
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
		RTT_printf(0, "CAN RX ERROR\r\n");
		return;
	}

	RTT_printf(0, "CAN RX: ID=0x%03lX DLC=%lu DATA=", rxHeader.StdId, rxHeader.DLC);

	for (uint32_t i = 0; i < rxHeader.DLC; i++)
	{
		RTT_printf(0, "%02X ", rxData[i]);
	}

	RTT_printf(0, "\r\n");

	// toggle RX indicator LED

	if(HAL_GPIO_ReadPin(LED_CAN_RX_GPIO_Port, LED_CAN_RX_Pin) != GPIO_PIN_SET)
	{
	  HAL_GPIO_WritePin(LED_CAN_RX_GPIO_Port, LED_CAN_RX_Pin, GPIO_PIN_SET);
	}
	else
	{
	  HAL_GPIO_WritePin(LED_CAN_RX_GPIO_Port, LED_CAN_RX_Pin, GPIO_PIN_RESET);
	}
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

static void CAN_TxInit(void)
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

static HAL_StatusTypeDef CAN1_Send(void)
{
    return HAL_CAN_AddTxMessage(&hcan, &txHeader, txData, &txMailbox);
}

#endif
