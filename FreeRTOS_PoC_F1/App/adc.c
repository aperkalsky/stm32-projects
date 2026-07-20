// A/D driver
#include "adc.h"
#include "main.h"

extern ADC_HandleTypeDef hadc1;

int32_t ADC_ReadingToTemperature(uint32_t reading)
{
	uint32_t vsense_mV = reading * 3300UL / 4095UL;

	return (int32_t)((1430 - (int32_t)vsense_mV) * 1000) / 43 + 25;
}

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

