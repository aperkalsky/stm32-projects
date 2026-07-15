// code to control the LED via PWM mode of the timer

#include "pwm_led.h"
#include "main.h"
//#include "stm32f1xx_hal_tim.h"

extern TIM_HandleTypeDef htim3;

TLV_STATUS PWM_LED_SetBrightness(uint16_t param)
{
	if(param > MAX_PWM_TIMER_COUNTER_PERIOD)
	{
		return TLV_STAT_INVALID_ARGUMENT;
	}

	__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, param);

	return TLV_STAT_OK;
}

// param comes in units of 100ms
TLV_STATUS PWM_LED_SetBreathePeriod(uint16_t param)
{
	return TLV_STAT_NOT_IMPLEMENTED;
}

// external functions
// ------------------

TLV_STATUS PWM_LED_SetMode(uint8_t mode, uint16_t param)
{
	if(mode == LED_MODE_MANUAL)
	{
		return PWM_LED_SetBrightness(param);
	}

	if(mode == LED_MODE_AUTO)
	{
		return PWM_LED_SetBreathePeriod(param);
	}

	return TLV_STAT_INVALID_ARGUMENT;
}
