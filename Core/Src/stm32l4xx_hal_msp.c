/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32l4xx_hal_msp.c
  * @brief   Global MSP initialization.
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"

void HAL_MspInit(void)
{
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();
}
