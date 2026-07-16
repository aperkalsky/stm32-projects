// code to control the LED via PWM mode of the timer

#include "pwm_led.h"
#include "main.h"

extern TIM_HandleTypeDef htim3;

// A lookup table for setting the brightness. The index is desired percentage of brightness
static const uint16_t brightnessTable[101] =
{
    999, 999, 998, 997, 996, 994, 992, 990, 987, 984,
    980, 976, 972, 967, 962, 956, 950, 944, 937, 930,
    922, 914, 906, 897, 888, 879, 869, 859, 848, 837,
    826, 815, 803, 791, 779, 766, 753, 740, 727, 713,
    699, 685, 671, 656, 642, 627, 611, 596, 581, 565,
    549, 534, 518, 502, 486, 470, 454, 438, 422, 406,
    390, 374, 358, 342, 327, 311, 295, 280, 264, 249,
    234, 219, 204, 190, 176, 162, 148, 135, 122, 109,
     97,  85,  74,  63,  53,  43,  34,  26,  19,  13,
      8,   4,   2,   1,   0,   0,   0,   0,   0,   0,
      0
};

// percents - requested percentage of brightness, 0 to 100
TLV_STATUS PWM_LED_SetBrightness(uint16_t percents)
{
	if(percents > sizeof(brightnessTable) - 1)
	{
		return TLV_STAT_INVALID_ARGUMENT;
	}

	__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, brightnessTable[percents]);

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
