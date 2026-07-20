#ifndef _ADC_H_
#define _ADC_H_

#include <stdint.h>

typedef uint8_t ADC_STATUS;

#define ADC_STATUS_OK				0
#define ADC_STATUS_FAILURE	1
#define ADC_STATUS_TIMEOUT	2

ADC_STATUS ADC_GetCpuTemperaturePolling(int32_t* pData);


#endif /* _ADC_H_ */
