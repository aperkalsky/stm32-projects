#ifndef _ADC_H_
#define _ADC_H_

#include <stdint.h>

typedef enum
{
	ADC_STATUS_OK = 0,
	ADC_STATUS_FAILURE,
	ADC_STATUS_TIMEOUT
}AdcStatus_t;

AdcStatus_t ADC_GetCpuTemperaturePolling(int32_t* pData);
AdcStatus_t ADC_GetCpuTemperature(int32_t* pData);


#endif /* _ADC_H_ */
