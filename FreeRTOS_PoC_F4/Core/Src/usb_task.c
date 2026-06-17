#include "usb_task.h"

#include "cmsis_os.h"

//#include "SEGGER_RTT.h"

//#include "usb_driver.h"
//#include "protocol.h"

void UsbTask_Run(void *argument)
{
	char cmd;

	for (;;)
	{
	  /* Infinite loop */
	  for(;;)
	  {
	    osDelay(1);
	  }
	}
}
