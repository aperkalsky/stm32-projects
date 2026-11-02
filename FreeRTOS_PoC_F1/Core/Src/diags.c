// Utilities for RTOS diagnostics
// Pre-requisites (set in MX):
//
// #define configUSE_TRACE_FACILITY              1	(task info)
// #define configUSE_STATS_FORMATTING_FUNCTIONS  1	(task info + list)
// #define configGENERATE_RUN_TIME_STATS         1	(task info + list + stats)

#include "my_rtt.h"
#include "cmsis_os.h"


#define MAX_MONITORED_TASKS    10

void FreeRTOS_PrintTaskInfo(void)
{
	TaskStatus_t taskStatus[MAX_MONITORED_TASKS];

	UBaseType_t taskCount;

	taskCount = uxTaskGetSystemState( taskStatus,	MAX_MONITORED_TASKS, NULL);

	if (taskCount == 0)
	{
		RTT_printf(0, "Task list unavailable or buffer too small\r\n");
		return;
	}

	RTT_printf(0, "\r\n");
	RTT_printf(0, "FreeRTOS Task Information\r\n");
	RTT_printf(0, "--------------------------\r\n");

	for (UBaseType_t i = 0; i < taskCount; i++)
	{
		RTT_printf(0, "Task: %s\r\n",
				taskStatus[i].pcTaskName);

		RTT_printf(0, "  Handle: %p\r\n",
				(void *)taskStatus[i].xHandle);

		RTT_printf(0, "  State: %d\r\n",
				(int)taskStatus[i].eCurrentState);

		RTT_printf(0, "  Priority: %lu\r\n",
				(unsigned long)taskStatus[i].uxCurrentPriority);

		RTT_printf(0, "  Stack base: %p\r\n",
				(void *)taskStatus[i].pxStackBase);

		RTT_printf(0, "  Stack remaining: %u bytes\r\n",
				(unsigned int)(
						taskStatus[i].usStackHighWaterMark *
						sizeof(StackType_t)));

		RTT_printf(0, "\r\n");
	}
}

void FreeRTOS_PrintTaskList(void)
{
	char buffer[512];

	vTaskList(buffer);

	RTT_printf(0, "%s\r\n", buffer);
}

void FreeRTOS_PrintRunTimeStats(void)
{
	char buffer[512];

	vTaskGetRunTimeStats(buffer);

	RTT_puts(0, "Name          Stat Prio Stack Num\r\n");
	RTT_printf(0, "%s\r\n", buffer);
}

void FreeRTOS_PrintFreeHeapSize(void)
{
	RTT_printf(0, "Free heap: %lu bytes\r\n", (unsigned long)xPortGetFreeHeapSize());
}
