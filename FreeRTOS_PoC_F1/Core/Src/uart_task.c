#include "uart_task.h"

#include "cmsis_os.h"

#include "SEGGER_RTT.h"

#include "uart_driver.h"
#include "protocol.h"

void UartTask_Run(void *argument)
{
	char cmd;

	UartDriver_Init();

	for (;;)
	{
		ulTaskNotifyTake(
				pdTRUE,
				portMAX_DELAY);

		if (UartDriver_GetChar(&cmd))
		{
			const char *response;

			SEGGER_RTT_printf(0, "> %c\r\n", cmd);

			response = Protocol_Process(cmd);

			UartDriver_SendString(response);

			SEGGER_RTT_printf(0, "< %s\r\n", response);
		}
	}
}
