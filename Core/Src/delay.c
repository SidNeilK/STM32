#include "delay.h"
#include "stm32l4xx_hal.h"
#include "tim.h"

void delay_init(void)
{
  (void)HAL_TIM_Base_Start(&htim2);
}

void delay_us(uint32_t microseconds)
{
  __HAL_TIM_SET_COUNTER(&htim2, 0U);
  while (__HAL_TIM_GET_COUNTER(&htim2) < microseconds)
  {
  }
}

void delay_ms(uint16_t milliseconds)
{
  HAL_Delay(milliseconds);
}
