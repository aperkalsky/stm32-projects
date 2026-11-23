#ifndef INC_WEATHER_H_
#define INC_WEATHER_H_

#include <stdbool.h>

#define AHT20_I2C_ADDR	0x38
#define BMP280_I2C_ADDR	0x77

bool WEATHER_Init(void);	// Initialize both sensors
bool WEATHER_GetTemperatureAndHumidity(float *pTempr, float *pHum);	// Celsius and relative humidity (%)
bool WEATHER_GetPressure(float *pPres);	// hPa

#endif
