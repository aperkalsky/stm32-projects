#include "touch.h"
#include "main.h"
#include "SEGGER_RTT.h"
//#include "cmsis_os.h"

static osThreadId_t touchTaskHandle;

void Touch_Init(osThreadId_t taskHandle)
{
    touchTaskHandle = taskHandle;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (GPIO_Pin == T_PEN_Pin)
	{
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;

		vTaskNotifyGiveFromISR(touchTaskHandle,	&xHigherPriorityTaskWoken);

		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}

void TouchTask_Run(void *argument)
{
	for (;;)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		// T_PEN went low:
		// perform ADS7843 X/Y acquisition
		SEGGER_RTT_WriteString(0, "Screen touch\r\n");
	}
}
