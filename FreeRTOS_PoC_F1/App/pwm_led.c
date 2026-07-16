// code to control the LED via PWM mode of the timer

// The design:
// ===========
//
// TIM3 works in PWM mode. Prescaler is set to 71 and the auto reload reg - to 999. It gives
// us the refresh rate of 1000 Hz and possibility to control duty cycle in 1000 steps.
//
// For breathing mode the TIM4 is added in counter mode, to generate interrupts for brihtness
// level change. It will be stopped in manual mode (default) and activated in automatic mode
//

#include "pwm_led.h"
#include "main.h"
#include "SEGGER_RTT.h"

extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;

// A lookup table for setting the brightness. The index is desired percentage of brightness
#define PWM_LED_TABLE_SIZE	101
#define PWM_LED_TABLE_INDEX_MAX	(PWM_LED_TABLE_SIZE - 1)
#define PWM_LED_TABLE_INDEX_MIN	0

static const uint16_t brightnessTable[PWM_LED_TABLE_SIZE] =
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

static PwmLedState_t _state;

// internal functions
// ------------------

// percents: requested percentage of brightness, 0 to 100
TLV_STATUS PWM_LED_SetBrightness(uint16_t percents)
{
	if(percents > sizeof(brightnessTable) - 1)
	{
		return TLV_STAT_INVALID_ARGUMENT;
	}

	__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, brightnessTable[percents]);

	return TLV_STAT_OK;
}

void PWM_LED_BreatheOn()
{
	HAL_TIM_Base_Start_IT(&htim4);
}

void PWM_LED_BreatheOff()
{
	HAL_TIM_Base_Stop_IT(&htim4);
}

// param comes in units of 100ms
TLV_STATUS PWM_LED_SetBreathePeriod(uint16_t param)
{
	// TODO
	return TLV_STAT_OK;
}

// external functions
// ------------------

void PWM_LED_Init()
{
	// init state machine
	_state.mode = LED_MODE_MANUAL;
	_state.lookupIndex = 0;
	_state.direction = LED_DIRECTION_UP;
	_state.divider = 9999;

	// start PWM timer
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
}

TLV_STATUS PWM_LED_SetMode(uint8_t mode, uint16_t param)
{
	if(mode == LED_MODE_MANUAL)
	{
		if(_state.mode == LED_MODE_AUTO)
		{
			PWM_LED_BreatheOff();
			_state.mode = LED_MODE_MANUAL;
		}

		return PWM_LED_SetBrightness(param);
	}

	if(mode == LED_MODE_AUTO)
	{
		PWM_LED_BreatheOff();

		TLV_STATUS stat = PWM_LED_SetBreathePeriod(param);

		if(stat == TLV_STAT_OK)
		{
			_state.mode = LED_MODE_AUTO;	// skip mode check to speed up things
			PWM_LED_BreatheOn();
		}

		return stat;
	}

	return TLV_STAT_INVALID_ARGUMENT;
}

// changes LED brightness according to the state machine
void PWM_LED_DoBreathe()
{
	// calc next lookup table index
	_state.lookupIndex += _state.direction;

	if (_state.lookupIndex == PWM_LED_TABLE_INDEX_MAX)
	{
		_state.direction = LED_DIRECTION_DOWN;
	}
	else if (_state.lookupIndex == PWM_LED_TABLE_INDEX_MIN)
	{
		_state.direction = LED_DIRECTION_UP;
	}

	// set brightness
//	SEGGER_RTT_printf(0, "i=%d d=%d b=%d\r\n", _state.lookupIndex, _state.direction, brightnessTable[_state.lookupIndex]);
	PWM_LED_SetBrightness(_state.lookupIndex);
}


