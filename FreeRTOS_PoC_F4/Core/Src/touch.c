// Touch screen controller driver + task

#include "touch.h"
#include "main.h"
#include "SEGGER_RTT.h"

static osThreadId_t touchTaskHandle;

void Touch_Init(osThreadId_t taskHandle)
{
    touchTaskHandle = taskHandle;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (GPIO_Pin == T_PEN_Pin)
	{
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;

		vTaskNotifyGiveFromISR(touchTaskHandle,	&xHigherPriorityTaskWoken);

		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}

// bit-banging helper functions
// ----------------------------

static inline void TouchCsCelect(void)
{
  HAL_GPIO_WritePin(T_CS_GPIO_Port, T_CS_Pin, GPIO_PIN_RESET);	// active LOW
}

static inline void TouchCsDeselect(void)
{
  HAL_GPIO_WritePin(T_CS_GPIO_Port, T_CS_Pin, GPIO_PIN_SET);
}

static inline void Touch_SetOutputHigh(void)
{
  HAL_GPIO_WritePin(T_MOSI_GPIO_Port, T_MOSI_Pin, GPIO_PIN_SET);
}

static inline void Touch_SetOutputLow(void)
{
  HAL_GPIO_WritePin(T_MOSI_GPIO_Port, T_MOSI_Pin, GPIO_PIN_RESET);
}

static inline void Touch_SetClockHigh(void)
{
  HAL_GPIO_WritePin(T_SCK_GPIO_Port, T_SCK_Pin, GPIO_PIN_SET);
}

static inline void Touch_SetClockLow(void)
{
  HAL_GPIO_WritePin(T_SCK_GPIO_Port, T_SCK_Pin, GPIO_PIN_RESET);
}

static inline GPIO_PinState TouchReadInput(void)
{
	return HAL_GPIO_ReadPin(T_MISO_GPIO_Port, T_MISO_Pin);
}

// ----------------------------

void Touch_WriteByte(uint8_t data)
{
	for(uint8_t i = 0; i < 8; i++)
	{
		if(data & 0x80)
		{
			Touch_SetOutputHigh();
		}
		else
		{
			Touch_SetOutputLow();
		}

		data<<=1;
		Touch_SetClockLow();
		osDelay(DELAY_1_MS);
		Touch_SetClockHigh();
	}
}

uint16_t TouchReadCoordinate(uint8_t command)
{
	uint16_t buf = 0;

	// request conversion
	Touch_SetClockLow();
	Touch_SetOutputLow();
	TouchCsCelect();
	Touch_WriteByte(command);

	// allow time for conversion
	osDelay(DELAY_6_MS);
	Touch_SetClockLow();
	osDelay(DELAY_1_MS);
	Touch_SetClockHigh();
	osDelay(DELAY_1_MS);
	Touch_SetClockLow();

	// read data
	for(uint8_t count=0; count < 16; count++)
	{
		buf<<=1;
		Touch_SetClockLow();
		osDelay(DELAY_1_MS);
		Touch_SetClockHigh();

 		if(TouchReadInput() == GPIO_PIN_SET)
 			{
 				buf++;
 			}
	}
	TouchCsDeselect();

	// the response payload is 12 first bits only
	buf>>=4;

	return(buf);
}

void TouchTask_Run(void *argument)
{
	(void)argument;

	for (;;)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		/*
		 * ADS7843 drives T_PEN as BUSY during each conversion (high, then low).
		 * That falling edge would otherwise re-notify this task and retrigger
		 * SPI forever, including after the finger is lifted.
		 */
		HAL_NVIC_DisableIRQ(T_PEN_EXTI_IRQn);

		while (HAL_GPIO_ReadPin(T_PEN_GPIO_Port, T_PEN_Pin) == GPIO_PIN_RESET)
		{
			SEGGER_RTT_WriteString(0, "Screen touch\r\n");
			SEGGER_RTT_printf(0, "X = %d\r\n", TouchReadCoordinate(CMD_READ_X));
			SEGGER_RTT_printf(0, "Y = %d\r\n", TouchReadCoordinate(CMD_READ_Y));
		}

		__HAL_GPIO_EXTI_CLEAR_IT(T_PEN_Pin);
		HAL_NVIC_ClearPendingIRQ(T_PEN_EXTI_IRQn);
		(void)ulTaskNotifyTake(pdTRUE, 0);
		HAL_NVIC_EnableIRQ(T_PEN_EXTI_IRQn);
	}
}

