#ifndef INC_TOUCH_H_
#define INC_TOUCH_H_

#include "cmsis_os.h"

#define DELAY_1_MS (1)
#define DELAY_6_MS (6)

#define CMD_READ_X 0xD0
#define CMD_READ_Y 0x90

void Touch_Init(osThreadId_t taskHandle);
void TouchTask_Run(void *argument);

#endif
