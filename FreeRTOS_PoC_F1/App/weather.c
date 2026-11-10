/*
 * weather.c
 *
 * Weather sensor drivers for the AHT20 and BMP280. Ported from https://github.com/peff74/ESP_AHT20_BMP280
 */

#include "main.h"
#include "weather.h"

#include <stddef.h>
#include <stdint.h>

extern I2C_HandleTypeDef hi2c1;

#define WEATHER_I2C_TIMEOUT_MS 100U
#define AHT20_MEASUREMENT_TIMEOUT_MS 200U
#define BMP280_CALIBRATION_LENGTH 24U

#define AHT20_CMD_INITIALIZE 0xBEU
#define AHT20_CMD_MEASURE 0xACU
#define AHT20_STATUS_BUSY 0x80U
#define AHT20_STATUS_CALIBRATED 0x08U

#define BMP280_REG_CALIBRATION 0x88U
#define BMP280_REG_ID 0xD0U
#define BMP280_REG_RESET 0xE0U
#define BMP280_REG_STATUS 0xF3U
#define BMP280_REG_CONTROL 0xF4U
#define BMP280_REG_CONFIG 0xF5U
#define BMP280_REG_PRESSURE 0xF7U
#define BMP280_CHIP_ID 0x58U

typedef struct
{
	uint16_t dig_T1;
	int16_t dig_T2;
	int16_t dig_T3;
	uint16_t dig_P1;
	int16_t dig_P2;
	int16_t dig_P3;
	int16_t dig_P4;
	int16_t dig_P5;
	int16_t dig_P6;
	int16_t dig_P7;
	int16_t dig_P8;
	int16_t dig_P9;
} BMP280_Calibration;

static BMP280_Calibration bmp280_calibration;
static bool sensors_initialized;

static HAL_StatusTypeDef read_registers(uint8_t address, uint8_t reg,
		uint8_t *data, uint16_t length)
{
	return HAL_I2C_Mem_Read(&hi2c1, (uint16_t)(address << 1), reg,
			I2C_MEMADD_SIZE_8BIT, data, length, WEATHER_I2C_TIMEOUT_MS);
}

static HAL_StatusTypeDef write_registers(uint8_t address, uint8_t reg,
		const uint8_t *data, uint16_t length)
{
	return HAL_I2C_Mem_Write(&hi2c1, (uint16_t)(address << 1), reg,
			I2C_MEMADD_SIZE_8BIT, (uint8_t *)data, length,
			WEATHER_I2C_TIMEOUT_MS);
}

static HAL_StatusTypeDef aht20_receive(uint8_t *data, uint16_t length)
{
	return HAL_I2C_Master_Receive(&hi2c1, (uint16_t)(AHT20_I2C_ADDR << 1),
			data, length, WEATHER_I2C_TIMEOUT_MS);
}

static HAL_StatusTypeDef aht20_transmit(const uint8_t *data, uint16_t length)
{
	return HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)(AHT20_I2C_ADDR << 1),
			(uint8_t *)data, length, WEATHER_I2C_TIMEOUT_MS);
}

static uint16_t read_u16_le(const uint8_t *data)
{
	return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static int16_t read_i16_le(const uint8_t *data)
{
	return (int16_t)read_u16_le(data);
}

static bool initialize_aht20(void)
{
	uint8_t status;
	const uint8_t initialize_command[] = { AHT20_CMD_INITIALIZE, 0x08U, 0x00U };

	HAL_Delay(40U);

	if (aht20_receive(&status, 1U) != HAL_OK)
	{
		return false;
	}

	if ((status & AHT20_STATUS_CALIBRATED) == 0U)
	{
		if (aht20_transmit(initialize_command, sizeof(initialize_command)) != HAL_OK)
		{
			return false;
		}

		HAL_Delay(10U);
		if (aht20_receive(&status, 1U) != HAL_OK
				|| (status & AHT20_STATUS_CALIBRATED) == 0U)
		{
			return false;
		}
	}

	return true;
}

static bool initialize_bmp280(void)
{
	uint8_t value;
	uint8_t calibration_data[BMP280_CALIBRATION_LENGTH];
	uint32_t start_time;

	if (read_registers(BMP280_I2C_ADDR, BMP280_REG_ID, &value, 1U) != HAL_OK
			|| value != BMP280_CHIP_ID)
	{
		return false;
	}

	value = 0xB6U;
	if (write_registers(BMP280_I2C_ADDR, BMP280_REG_RESET, &value, 1U) != HAL_OK)
	{
		return false;
	}

	start_time = HAL_GetTick();
	do
	{
		if (read_registers(BMP280_I2C_ADDR, BMP280_REG_STATUS, &value, 1U) != HAL_OK)
		{
			return false;
		}
		if ((value & 0x01U) == 0U)
		{
			break;
		}
		HAL_Delay(1U);
	} while ((uint32_t)(HAL_GetTick() - start_time) < WEATHER_I2C_TIMEOUT_MS);

	if ((value & 0x01U) != 0U
			|| read_registers(BMP280_I2C_ADDR, BMP280_REG_CALIBRATION,
					calibration_data, sizeof(calibration_data)) != HAL_OK)
	{
		return false;
	}

	bmp280_calibration.dig_T1 = read_u16_le(&calibration_data[0]);
	bmp280_calibration.dig_T2 = read_i16_le(&calibration_data[2]);
	bmp280_calibration.dig_T3 = read_i16_le(&calibration_data[4]);
	bmp280_calibration.dig_P1 = read_u16_le(&calibration_data[6]);
	bmp280_calibration.dig_P2 = read_i16_le(&calibration_data[8]);
	bmp280_calibration.dig_P3 = read_i16_le(&calibration_data[10]);
	bmp280_calibration.dig_P4 = read_i16_le(&calibration_data[12]);
	bmp280_calibration.dig_P5 = read_i16_le(&calibration_data[14]);
	bmp280_calibration.dig_P6 = read_i16_le(&calibration_data[16]);
	bmp280_calibration.dig_P7 = read_i16_le(&calibration_data[18]);
	bmp280_calibration.dig_P8 = read_i16_le(&calibration_data[20]);
	bmp280_calibration.dig_P9 = read_i16_le(&calibration_data[22]);

	if (bmp280_calibration.dig_P1 == 0U)
	{
		return false;
	}

	/* Configure in sleep mode, then enable temperature x1 / pressure x4. */
	value = 0x00U;
	if (write_registers(BMP280_I2C_ADDR, BMP280_REG_CONTROL, &value, 1U) != HAL_OK)
	{
		return false;
	}

	value = 0xD0U;
	if (write_registers(BMP280_I2C_ADDR, BMP280_REG_CONFIG, &value, 1U) != HAL_OK)
	{
		return false;
	}

	value = 0x2FU;
	if (write_registers(BMP280_I2C_ADDR, BMP280_REG_CONTROL, &value, 1U) != HAL_OK)
	{
		return false;
	}

	HAL_Delay(100U);
	return true;
}

static uint8_t aht20_crc8(const uint8_t *data, uint8_t length)
{
	uint8_t crc = 0xFFU;

	for (uint8_t i = 0U; i < length; ++i)
	{
		crc ^= data[i];
		for (uint8_t bit = 0U; bit < 8U; ++bit)
		{
			crc = (crc & 0x80U) != 0U
					? (uint8_t)((crc << 1) ^ 0x31U)
					: (uint8_t)(crc << 1);
		}
	}

	return crc;
}

bool WEATHER_Init(void)
{
	sensors_initialized = false;

	if (!initialize_aht20() || !initialize_bmp280())
	{
		return false;
	}

	sensors_initialized = true;
	return true;
}

bool WEATHER_GetTemperatureAndHumidity(float *pTempr, float *pHum)
{
	uint8_t command[] = { AHT20_CMD_MEASURE, 0x33U, 0x00U };
	uint8_t status;
	uint8_t data[7];
	uint32_t start_time;
	uint32_t raw_humidity;
	uint32_t raw_temperature;

	if (!sensors_initialized || pTempr == NULL || pHum == NULL)
	{
		return false;
	}

	if (aht20_transmit(command, sizeof(command)) != HAL_OK)
	{
		return false;
	}

	start_time = HAL_GetTick();
	do
	{
		HAL_Delay(5U);
		if (aht20_receive(&status, 1U) != HAL_OK)
		{
			return false;
		}
		if ((status & AHT20_STATUS_BUSY) == 0U)
		{
			break;
		}
	} while ((uint32_t)(HAL_GetTick() - start_time) < AHT20_MEASUREMENT_TIMEOUT_MS);

	if ((status & AHT20_STATUS_BUSY) != 0U
			|| aht20_receive(data, sizeof(data)) != HAL_OK
			|| (data[0] & AHT20_STATUS_BUSY) != 0U
			|| aht20_crc8(data, 6U) != data[6])
	{
		return false;
	}

	raw_humidity = ((uint32_t)data[1] << 12)
			| ((uint32_t)data[2] << 4)
			| ((uint32_t)data[3] >> 4);
	raw_temperature = (((uint32_t)data[3] & 0x0FU) << 16)
			| ((uint32_t)data[4] << 8)
			| data[5];

	*pHum = ((float)raw_humidity * 100.0f) / 1048576.0f;
	*pTempr = ((float)raw_temperature * 200.0f) / 1048576.0f - 50.0f;
	return true;
}

bool WEATHER_GetPressure(float *pPres)
{
	uint8_t data[6];
	int32_t adc_temperature;
	int32_t adc_pressure;
	int32_t var1_temperature;
	int32_t var2_temperature;
	int32_t t_fine;
	int64_t var1;
	int64_t var2;
	int64_t pressure;

	if (!sensors_initialized || pPres == NULL
			|| read_registers(BMP280_I2C_ADDR, BMP280_REG_PRESSURE,
					data, sizeof(data)) != HAL_OK)
	{
		return false;
	}

	adc_pressure = ((int32_t)data[0] << 16)
			| ((int32_t)data[1] << 8) | data[2];
	adc_pressure >>= 4;
	adc_temperature = ((int32_t)data[3] << 16)
			| ((int32_t)data[4] << 8) | data[5];
	adc_temperature >>= 4;

	var1_temperature = ((((adc_temperature >> 3)
			- ((int32_t)bmp280_calibration.dig_T1 << 1)))
			* (int32_t)bmp280_calibration.dig_T2) >> 11;
	var2_temperature = (((((adc_temperature >> 4)
			- (int32_t)bmp280_calibration.dig_T1)
			* ((adc_temperature >> 4) - (int32_t)bmp280_calibration.dig_T1))
			>> 12) * (int32_t)bmp280_calibration.dig_T3) >> 14;
	t_fine = var1_temperature + var2_temperature;

	var1 = ((int64_t)t_fine) - 128000;
	var2 = var1 * var1 * (int64_t)bmp280_calibration.dig_P6;
	var2 += (var1 * (int64_t)bmp280_calibration.dig_P5) * 131072;
	var2 += (int64_t)bmp280_calibration.dig_P4 * 34359738368LL;
	var1 = ((var1 * var1 * (int64_t)bmp280_calibration.dig_P3) >> 8)
			+ ((var1 * (int64_t)bmp280_calibration.dig_P2) * 4096);
	var1 = (((((int64_t)1) << 47) + var1)
			* (int64_t)bmp280_calibration.dig_P1) >> 33;

	if (var1 == 0)
	{
		return false;
	}

	pressure = 1048576 - adc_pressure;
	pressure = (((pressure << 31) - var2) * 3125) / var1;
	var1 = ((int64_t)bmp280_calibration.dig_P9
			* (pressure >> 13) * (pressure >> 13)) >> 25;
	var2 = ((int64_t)bmp280_calibration.dig_P8 * pressure) >> 19;
	pressure = ((pressure + var1 + var2) >> 8)
			+ ((int64_t)bmp280_calibration.dig_P7 * 16);

	*pPres = ((float)pressure / 256.0f) / 100.0f;
	return true;
}
