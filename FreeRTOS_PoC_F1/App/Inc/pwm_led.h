#ifndef _PWM_LED_H_
#define _PWM_LED_H_

#include "protocol.h"

// LED modes
#define LED_MODE_MANUAL	0
#define LED_MODE_AUTO		1

#define MAX_PWM_TIMER_COUNTER_PERIOD	999


TLV_STATUS PWM_LED_SetMode(uint8_t mode, uint16_t param);



#endif /* INC_PWM_LED_H_ */
