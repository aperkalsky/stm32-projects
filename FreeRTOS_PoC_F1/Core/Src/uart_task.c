#include "uart_task.h"

#include "cmsis_os.h"

#include "SEGGER_RTT.h"

#include "uart_driver.h"
#include "protocol.h"

void UartTask_Run(void *argument)
{
    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        Protocol_Process();
    }
}
