#include "proximity_sensor.h"
#include "i2c.h"

#define VCNL4040_I2C_ADDRESS (0x60U << 1)
#define VCNL4040_PS_CONF1_2 0x03U
#define VCNL4040_PS_DATA     0x08U

static HAL_StatusTypeDef VCNL4040_Write16(uint8_t reg, uint16_t value)
{
  uint8_t data[2] = {(uint8_t)(value & 0xFFU), (uint8_t)(value >> 8)};

  return HAL_I2C_Mem_Write(&hi2c1, VCNL4040_I2C_ADDRESS, reg,
                           I2C_MEMADD_SIZE_8BIT, data, sizeof(data), 100U);
}

static HAL_StatusTypeDef VCNL4040_Read16(uint8_t reg, uint16_t *value)
{
  uint8_t data[2];
  HAL_StatusTypeDef status;

  status = HAL_I2C_Mem_Read(&hi2c1, VCNL4040_I2C_ADDRESS, reg,
                            I2C_MEMADD_SIZE_8BIT, data, sizeof(data), 100U);
  if (status == HAL_OK)
  {
    *value = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
  }

  return status;
}

HAL_StatusTypeDef ProximitySensor_Init(void)
{
  if (HAL_I2C_IsDeviceReady(&hi2c1, VCNL4040_I2C_ADDRESS, 3U, 100U) != HAL_OK)
  {
    return HAL_ERROR;
  }

  /* Enable proximity conversion; the VCNL4040 registers use little-endian data. */
  if (VCNL4040_Write16(VCNL4040_PS_CONF1_2, 0x0800U) != HAL_OK)
  {
    return HAL_ERROR;
  }

  HAL_Delay(10U);
  return HAL_OK;
}

HAL_StatusTypeDef ProximitySensor_Read(uint16_t *value)
{
  if (value == NULL)
  {
    return HAL_ERROR;
  }

  return VCNL4040_Read16(VCNL4040_PS_DATA, value);
}
