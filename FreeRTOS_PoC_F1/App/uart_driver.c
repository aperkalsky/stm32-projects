#include "uart_driver.h"

#include "main.h"
#include "cmsis_os.h"

#include "FreeRTOS.h"
#include "semphr.h"

#include "SEGGER_RTT.h"

#include <string.h>

extern UART_HandleTypeDef huart1;

static uint8_t rxByte;

static char receivedChar;
static volatile bool charReady = false;

SemaphoreHandle_t txDoneSem;

extern TaskHandle_t uartTaskHandle;

UartDriverStatus_t UartDriver_Init(void)
{
	// Clear any pending status by reading SR then DR. Just in case
	volatile uint32_t tmp;

	tmp = USART1->SR;
	tmp = USART1->DR;
	(void)tmp;

	txDoneSem = xSemaphoreCreateBinary();

	if (txDoneSem == NULL)
	{
		return UART_DRV_SEM_CREATE_FAILED;
	}

	switch(HAL_UART_Receive_IT(&huart1,	&rxByte, 1))
	{
	case HAL_OK:
		return UART_DRV_OK;

	case HAL_BUSY:
		return UART_DRV_HAL_BUSY;

	case HAL_ERROR:
		return UART_DRV_HAL_ERROR;

	default:
		return UART_DRV_HAL_ERROR;
	}
}

bool UartDriver_GetChar(char *ch)
{
	if(!charReady)
		return false;

	*ch = receivedChar;

	SEGGER_RTT_printf(0, "Got %c\r\n", *ch);

	charReady = false;

	return true;
}

void UartDriver_SendString(const char *str)
{
	HAL_UART_Transmit_DMA(
			&huart1,
			(uint8_t*)str,
			strlen(str));

	xSemaphoreTake(
			txDoneSem,
			portMAX_DELAY);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	if(huart->Instance == USART1)
	{
		receivedChar = rxByte;

		charReady = true;

		vTaskNotifyGiveFromISR(
				uartTaskHandle,
				&xHigherPriorityTaskWoken);

		HAL_UART_Receive_IT(
				&huart1,
				&rxByte,
				1);

		portYIELD_FROM_ISR(
				xHigherPriorityTaskWoken);
	}
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	if(huart->Instance == USART1)
	{
		xSemaphoreGiveFromISR(
				txDoneSem,
				&xHigherPriorityTaskWoken);

		portYIELD_FROM_ISR(
				xHigherPriorityTaskWoken);
	}
}
