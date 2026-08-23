#include "touch.h"
#include "main.h"
#include "SEGGER_RTT.h"

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (GPIO_Pin == T_PEN_Pin)
	{
		SEGGER_RTT_WriteString(0, "Screen touch\r\n");
		/*        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        vTaskNotifyGiveFromISR(
            touchTaskHandle,
            &xHigherPriorityTaskWoken
        );

        portYIELD_FROM_ISR(xHigherPriorityTaskWoken); */
	}
}
