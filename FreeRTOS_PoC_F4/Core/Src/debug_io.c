#include "debug_io.h"
#include "main.h"

void DBG0_High()
{
  HAL_GPIO_WritePin(DBG0_GPIO_Port, DBG0_Pin, GPIO_PIN_SET);
}

void DBG0_Low()
{
  HAL_GPIO_WritePin(DBG0_GPIO_Port, DBG0_Pin, GPIO_PIN_RESET);
}

void DBG1_High()
{
  HAL_GPIO_WritePin(DBG1_GPIO_Port, DBG1_Pin, GPIO_PIN_SET);
}

void DBG1_Low()
{
  HAL_GPIO_WritePin(DBG1_GPIO_Port, DBG1_Pin, GPIO_PIN_RESET);
}

void DBG2_High()
{
  HAL_GPIO_WritePin(DBG2_GPIO_Port, DBG2_Pin, GPIO_PIN_SET);
}

void DBG2_Low()
{
  HAL_GPIO_WritePin(DBG2_GPIO_Port, DBG2_Pin, GPIO_PIN_RESET);
}

void DBG3_High()
{
  HAL_GPIO_WritePin(DBG3_GPIO_Port, DBG3_Pin, GPIO_PIN_SET);
}

void DBG3_Low()
{
  HAL_GPIO_WritePin(DBG3_GPIO_Port, DBG3_Pin, GPIO_PIN_RESET);
}
