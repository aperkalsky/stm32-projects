#ifndef __UART_DRIVER_H
#define __UART_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    UART_DRV_OK = 0,

    UART_DRV_HAL_ERROR,
    UART_DRV_HAL_BUSY,
    UART_DRV_TIMEOUT,
    UART_DRV_SEM_CREATE_FAILED
} UartDriverStatus_t;

UartDriverStatus_t UartDriver_Init(void);

bool UartDriver_GetChar(char *ch);
bool UartDriver_GetByte(uint8_t *byte);

bool UartDriver_SendBuffer(const uint8_t *buf, uint16_t len);
void UartDriver_SendString(const char *str);

#endif
