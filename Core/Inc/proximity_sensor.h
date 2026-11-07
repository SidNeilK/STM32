/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef PROXIMITY_SENSOR_H
#define PROXIMITY_SENSOR_H

#include "stm32l4xx_hal.h"

#define PROXIMITY_SENSOR_DETECTION_LEVEL 500U

HAL_StatusTypeDef ProximitySensor_Init(void);
HAL_StatusTypeDef ProximitySensor_Read(uint16_t *value);

#endif /* PROXIMITY_SENSOR_H */
