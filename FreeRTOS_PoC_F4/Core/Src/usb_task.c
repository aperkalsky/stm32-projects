#include "usb_task.h"

#include "cmsis_os.h"

#include "SEGGER_RTT.h"

//#include "usb_driver.h"
#include "protocol.h"

void UsbTask_Run(void *argument)
{
//	TickType_t lastWakeTime = xTaskGetTickCount();

	/* Infinite loop */
	for(;;)
	{
//		static int8_t CDC_Receive_FS(uint8_t* Buf, uint32_t *Len)
//		SEGGER_RTT_WriteString(0, "In USB task\r\n");
		Protocol_Process();
		osDelay(1);
//	  vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(1000));
	}
}
