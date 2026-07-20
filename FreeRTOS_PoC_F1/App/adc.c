// A/D driver
#include "adc.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"

extern ADC_HandleTypeDef hadc1;

static TaskHandle_t adcWaitingTask = NULL;

static volatile uint32_t adcRawValue;
static volatile ADC_STATUS adcStatus;

int32_t ADC_ReadingToTemperature(uint32_t reading)
{
	uint32_t vsense_mV = reading * 3300UL / 4095UL;

	return (int32_t)((1430 - (int32_t)vsense_mV) * 1000) / 43 + 25;
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
	if (hadc != &hadc1)
	{
		return;
	}

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	adcRawValue = HAL_ADC_GetValue(hadc);

	HAL_ADC_Stop_IT(hadc);

	adcStatus = ADC_STATUS_OK;

	if (adcWaitingTask != NULL)
	{
		vTaskNotifyGiveFromISR(adcWaitingTask,
				&xHigherPriorityTaskWoken);

		adcWaitingTask = NULL;
	}

	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc)
{
	if (hadc != &hadc1)
	{
		return;
	}

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	adcStatus = ADC_STATUS_FAILURE;

	if (adcWaitingTask != NULL)
	{
		vTaskNotifyGiveFromISR(adcWaitingTask,
				&xHigherPriorityTaskWoken);

		adcWaitingTask = NULL;
	}

	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// works in blocking mode (CPU is blocked)
ADC_STATUS ADC_GetCpuTemperaturePolling(int32_t* pData)
{
	if(HAL_ADC_Start(&hadc1) != HAL_OK)
	{
		return ADC_STATUS_FAILURE;
	}

	if(HAL_ADC_PollForConversion(&hadc1, 100) != HAL_OK)
	{
		return ADC_STATUS_TIMEOUT;
	}

	uint32_t reading = HAL_ADC_GetValue(&hadc1);
	HAL_ADC_Stop(&hadc1);

	*pData = ADC_ReadingToTemperature(reading);

	return ADC_STATUS_OK;
}

// works in non-blocking mode (CPU is not blocked, only the calling task)
ADC_STATUS ADC_GetCpuTemperature(int32_t *pData)
{
	adcWaitingTask = xTaskGetCurrentTaskHandle();
	adcStatus = ADC_STATUS_FAILURE;

	if (HAL_ADC_Start_IT(&hadc1) != HAL_OK)
	{
		adcWaitingTask = NULL;
		return ADC_STATUS_FAILURE;
	}

	// Wait until interrupt wakes us
	if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100)) == 0)
	{
		HAL_ADC_Stop_IT(&hadc1);

		adcWaitingTask = NULL;
		return ADC_STATUS_TIMEOUT;
	}

	*pData = ADC_ReadingToTemperature(adcRawValue);

	return adcStatus;
}
