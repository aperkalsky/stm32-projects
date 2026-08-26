// Touch screen controller driver + task

#include "touch.h"
#include "main.h"
#include "SEGGER_RTT.h"
#include <stdint.h>
#include "sampleimage.h"
#include "semphr.h"
#include "ili9341.h"
#include "sampleImage.h"


static osThreadId_t touchTaskHandle;
static osThreadId_t drawTaskHandle;
static osMutexId_t imgPosMutexHandle;
static ScreenPosDef imgPosition;			// position to output an image on screen, mutex protected
static volatile uint32_t drawDelayStartTime = 0;

void Touch_Init(osThreadId_t hTouch, osThreadId_t hDraw, osMutexId_t hMutex)
{
	touchTaskHandle = hTouch;
	drawTaskHandle = hDraw;
	imgPosMutexHandle = hMutex;

	imgPosition.x = imgPosition.y = 0;
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

bool TouchConvertRawToScreenPos(uint16_t rawX, uint16_t rawY, ScreenPosDef *pPos)
{
	int32_t x;
	int32_t y;

	if (pPos == NULL)
	{
		return false;
	}

	/*
	 * Calibration:
	 *
	 * X = 255.169000 - 0.0642941 * rawX
	 * Y = -18.023900 + 0.08982995 * rawY
	 *
	 * Scale = 1,000,000
	 */
	x = 255169000L - 64294L * (int32_t)rawX;
	y = -18023900L + 89830L * (int32_t)rawY;

	x /= 1000000L;
	y /= 1000000L;

	/*
	 * Screen resolution:
	 * X = 0..239
	 * Y = 0..319
	 */
	if ((x < 0) || (x >= 240) || (y < 0) || (y >= 320))
	{
		return false;
	}

	pPos->x = (uint16_t)x;
	pPos->y = (uint16_t)y;

	return true;
}

void getRandomPosition(ScreenPosDef* pPos)
{
	// for meantime - return the initial pos
	pPos->x = 0;
	pPos->y = 0;
}

uint16_t getRandomDelayMs()
{
	// for meantime return fixed value
	return 1000;
}

bool isHit(ScreenPosDef* pTouchLoc)
{
	if (xSemaphoreTake(imgPosMutexHandle, pdMS_TO_TICKS(100)) == pdTRUE)
	{
		// TODO: add comparison code
		xSemaphoreGive(imgPosMutexHandle);
		return true;
	}
	else
	{
		return false;
	}
}

void TouchTask_Run(void *argument)
{
	(void)argument;
	uint16_t rawX, rawY;
	ScreenPosDef touchLocation;
	bool coordinatesRead;

	while(1)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		/*
		 * ADS7843 drives T_PEN as BUSY during each conversion (high, then low).
		 * That falling edge would otherwise re-notify this task and retrigger
		 * SPI forever, including after the finger is lifted.
		 */
		HAL_NVIC_DisableIRQ(T_PEN_EXTI_IRQn);

		coordinatesRead = false;

		while (HAL_GPIO_ReadPin(T_PEN_GPIO_Port, T_PEN_Pin) == GPIO_PIN_RESET)
		{
			//			SEGGER_RTT_WriteString(0, "Screen touch\r\n");
			//			SEGGER_RTT_printf(0, "X = %d\r\n", TouchReadCoordinate(CMD_READ_X));
			//			SEGGER_RTT_printf(0, "Y = %d\r\n", TouchReadCoordinate(CMD_READ_Y));
			if(!coordinatesRead)
			{
				rawX = TouchReadCoordinate(CMD_READ_X);
				rawY = TouchReadCoordinate(CMD_READ_Y);
				SEGGER_RTT_printf(0, "x:%d y:%d\r\n", rawX, rawY);

				if(TouchConvertRawToScreenPos(rawX, rawY, &touchLocation))
				{
					coordinatesRead = true;
					SEGGER_RTT_printf(0, "-> x:%d y:%d\r\n", touchLocation.x, touchLocation.y);

					if(isHit(&touchLocation))
					{
						// instruct draw task to wait with moving
						drawDelayStartTime = osKernelGetTickCount();

						// display a new image
						lcdDrawImage(imgPosition.x, imgPosition.y, &flySmashed);
					}
				}
			}
			else
			{
				// just wait for screen release
				osDelay(DELAY_100_MS);
			}
		}

		__HAL_GPIO_EXTI_CLEAR_IT(T_PEN_Pin);
		HAL_NVIC_ClearPendingIRQ(T_PEN_EXTI_IRQn);
		(void)ulTaskNotifyTake(pdTRUE, 0);
		HAL_NVIC_EnableIRQ(T_PEN_EXTI_IRQn);
	}
}

void DrawTask_Run(void *argument)
{
	(void)argument;
	ScreenPosDef newPosition;

	while(1)
	{
		if ((osKernelGetTickCount() - drawDelayStartTime) >= pdMS_TO_TICKS(10000))
		{
			// we can run
			getRandomPosition(&newPosition);

			if (xSemaphoreTake(imgPosMutexHandle, pdMS_TO_TICKS(100)) == pdTRUE)
			{
				imgPosition.x = newPosition.x;
				imgPosition.y = newPosition.y;

				xSemaphoreGive(imgPosMutexHandle);

				lcdDrawImage(newPosition.x, newPosition.y, &flyAlive);

				SEGGER_RTT_WriteString(0, "Pos changed\r\n");
			}
			else
			{
				// for some reason the resource is locked. Do not change the position
				SEGGER_RTT_WriteString(0, "Cannot take mutex\r\n");
			}

			vTaskDelay(pdMS_TO_TICKS(getRandomDelayMs()));
		}
		else
		{
			// continue sleeping
			vTaskDelay(pdMS_TO_TICKS(500));
		}
	}
}
