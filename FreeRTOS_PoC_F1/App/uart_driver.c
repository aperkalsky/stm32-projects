#include "uart_driver.h"

#include "main.h"
#include "cmsis_os.h"

#include "FreeRTOS.h"
#include "semphr.h"

#include "SEGGER_RTT.h"

#include <string.h>

extern UART_HandleTypeDef huart1;

#define UART_RX_RING_SIZE 1024

static uint8_t rxByte;
static uint8_t rxRing[UART_RX_RING_SIZE];
static volatile uint16_t rxHead = 0;
static volatile uint16_t rxTail = 0;

static volatile bool charReady = false;

SemaphoreHandle_t txDoneSem;

extern TaskHandle_t uartTaskHandle;

UartDriverStatus_t UartDriver_Init(void)
{
    txDoneSem = xSemaphoreCreateBinary();

    if (txDoneSem == NULL)
    {
        return UART_DRV_SEM_CREATE_FAILED;
    }

    switch(HAL_UART_Receive_IT(&huart1, &rxByte, 1))
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

bool UartDriver_GetByte(uint8_t *byte)
{
    if (rxHead == rxTail)
    {
        return false;
    }

    *byte = rxRing[rxTail];
    rxTail = (rxTail + 1) % UART_RX_RING_SIZE;

    return true;
}

void UartDriver_SendBuffer(const uint8_t *buf, uint16_t len)
{
    HAL_UART_Transmit_DMA(&huart1, (uint8_t *)buf, len);
    xSemaphoreTake(txDoneSem, portMAX_DELAY);
}

void UartDriver_SendString(const char *str)
{
    UartDriver_SendBuffer((const uint8_t *)str, strlen(str));
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (huart->Instance == USART1)
    {
        SEGGER_RTT_printf(0, "> %02X\r\n", rxByte);

        uint16_t nextHead = (rxHead + 1) % UART_RX_RING_SIZE;
        if (nextHead != rxTail)
        {
            rxRing[rxHead] = rxByte;
            rxHead = nextHead;
        }

        vTaskNotifyGiveFromISR(uartTaskHandle, &xHigherPriorityTaskWoken);

        HAL_UART_Receive_IT(&huart1, &rxByte, 1);

        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (huart->Instance == USART1)
    {
        xSemaphoreGiveFromISR(txDoneSem, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}
