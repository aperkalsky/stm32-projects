#ifndef _PWM_LED_H_
#define _PWM_LED_H_

#include "protocol.h"

// LED modes
#define LED_MODE_MANUAL	0
#define LED_MODE_AUTO		1

#define LED_DIRECTION_UP		(1)
#define LED_DIRECTION_DOWN	(-1)


#define MAX_PWM_TIMER_COUNTER_PERIOD	999

typedef struct
{
    uint8_t mode;						// manual/auto
    uint8_t lookupIndex;		// index in brightness lookup table
    int8_t direction;				// direction of lookupIndex change (up/down)
    uint16_t divider;				// TIM4 divider
    uint16_t stepPeriodMs;
} PwmLedState_t;

void PWM_LED_Init();
TLV_STATUS PWM_LED_SetMode(uint8_t mode, uint16_t param);
void PWM_LED_DoBreathe();

#endif /* INC_PWM_LED_H_ */
