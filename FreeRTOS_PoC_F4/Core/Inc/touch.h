#ifndef INC_TOUCH_H_
#define INC_TOUCH_H_

#include "cmsis_os.h"

void Touch_Init(osThreadId_t taskHandle);
void TouchTask_Run(void *argument);

#endif
