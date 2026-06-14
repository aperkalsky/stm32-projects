#ifndef __UART_DRIVER_H
#define __UART_DRIVER_H

#include <stdbool.h>

void UartDriver_Init(void);

bool UartDriver_GetChar(char *ch);

void UartDriver_SendString(const char *str);

#endif
