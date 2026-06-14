#include "uart_driver.h"

#include "main.h"
#include "cmsis_os.h"

#include "FreeRTOS.h"
#include "semphr.h"

#include <string.h>

extern UART_HandleTypeDef huart1;

static uint8_t rxByte;

static char receivedChar;
static volatile bool charReady = false;

SemaphoreHandle_t txDoneSem;

extern TaskHandle_t uartTaskHandle;

void UartDriver_Init(void)
{
	txDoneSem = xSemaphoreCreateBinary();

	HAL_UART_Receive_IT(
			&huart1,
			&rxByte,
			1);
}

bool UartDriver_GetChar(char *ch)
{
	if(!charReady)
		return false;

	*ch = receivedChar;

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
