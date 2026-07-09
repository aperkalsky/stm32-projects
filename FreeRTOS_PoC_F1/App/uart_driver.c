#include "uart_driver.h"

#include "main.h"
#include "cmsis_os.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "SEGGER_RTT.h"

#include <string.h>

extern UART_HandleTypeDef huart1;

#define UART_RX_RING_SIZE 1024
#define UART_TX_RING_SIZE 512

static uint8_t rxByte;
static uint8_t rxRing[UART_RX_RING_SIZE];
static volatile uint16_t rxHead = 0;
static volatile uint16_t rxTail = 0;

static uint8_t txRing[UART_TX_RING_SIZE];
static volatile uint16_t txHead = 0;
static volatile uint16_t txTail = 0;
static volatile uint16_t txPending = 0;
static volatile uint16_t txInFlight = 0;
static volatile bool txBusy = false;

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
    taskENTER_CRITICAL();

    if (rxHead == rxTail)
    {
        taskEXIT_CRITICAL();
        return false;
    }

    *byte = rxRing[rxTail];
    rxTail = (rxTail + 1) % UART_RX_RING_SIZE;

    taskEXIT_CRITICAL();
    return true;
}

static void UartDriver_StartTx(void)
{
    uint16_t chunk = 0;
    uint16_t startTail = txTail;

    if (txBusy || (txPending == 0))
    {
        return;
    }

    chunk = txPending;
    if (chunk > (UART_TX_RING_SIZE - txTail))
    {
        chunk = UART_TX_RING_SIZE - txTail;
    }

    txBusy = true;
    txInFlight = chunk;
    txTail = (txTail + chunk) % UART_TX_RING_SIZE;
    HAL_UART_Transmit_DMA(&huart1, &txRing[startTail], chunk);
}

bool UartDriver_SendBuffer(const uint8_t *buf, uint16_t len)
{
    uint16_t i;

    while (1)
    {
        taskENTER_CRITICAL();

        if (len <= (UART_TX_RING_SIZE - txPending - 1))
        {
            for (i = 0; i < len; ++i)
            {
                txRing[txHead] = buf[i];
                txHead = (txHead + 1) % UART_TX_RING_SIZE;
                txPending++;
            }

            taskEXIT_CRITICAL();

            if (!txBusy)
            {
                UartDriver_StartTx();
            }

            return true;
        }

        taskEXIT_CRITICAL();
        taskYIELD();
    }
}

void UartDriver_SendString(const char *str)
{
    (void)UartDriver_SendBuffer((const uint8_t *)str, strlen(str));
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (huart->Instance == USART1)
    {
        UBaseType_t irqMask = taskENTER_CRITICAL_FROM_ISR();
        uint16_t nextHead = (rxHead + 1) % UART_RX_RING_SIZE;
        if (nextHead != rxTail)
        {
            rxRing[rxHead] = rxByte;
            rxHead = nextHead;
        }
        taskEXIT_CRITICAL_FROM_ISR(irqMask);

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
        UBaseType_t irqMask = taskENTER_CRITICAL_FROM_ISR();
        if (txBusy)
        {
            txPending -= txInFlight;
            txInFlight = 0;
            txBusy = false;
        }
        taskEXIT_CRITICAL_FROM_ISR(irqMask);

        if (txPending != 0)
        {
            UartDriver_StartTx();
        }

        xSemaphoreGiveFromISR(txDoneSem, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}
