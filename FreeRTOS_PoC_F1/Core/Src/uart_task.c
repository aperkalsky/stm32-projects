#include "uart_task.h"

#include "cmsis_os.h"

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

			response = Protocol_Process(cmd);

			UartDriver_SendString(response);
		}
	}
}
