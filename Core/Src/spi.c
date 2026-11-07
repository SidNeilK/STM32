/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    spi.c
  * @brief   This file provides code for the configuration
  *          of the SPI instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "spi.h"

/* USER CODE BEGIN 0 */
#include "ArduCAM.h"
#include "usart.h"

/* USER CODE END 0 */

SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_rx;
DMA_HandleTypeDef hdma_spi1_tx;

/* SPI1 init function */
void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

void HAL_SPI_MspInit(SPI_HandleTypeDef* spiHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(spiHandle->Instance==SPI1)
  {
  /* USER CODE BEGIN SPI1_MspInit 0 */

  /* USER CODE END SPI1_MspInit 0 */
    /* SPI1 clock enable */
    __HAL_RCC_SPI1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**SPI1 GPIO Configuration
    PA1     ------> SPI1_SCK
    PA6     ------> SPI1_MISO
    PA7     ------> SPI1_MOSI
    */
    GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* SPI1 DMA Init */
    /* SPI1_RX Init */
    hdma_spi1_rx.Instance = DMA1_Channel2;
    hdma_spi1_rx.Init.Request = DMA_REQUEST_1;
    hdma_spi1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_spi1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_spi1_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_spi1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_spi1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_spi1_rx.Init.Mode = DMA_NORMAL;
    hdma_spi1_rx.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&hdma_spi1_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(spiHandle,hdmarx,hdma_spi1_rx);

    /* SPI1_TX Init */
    hdma_spi1_tx.Instance = DMA1_Channel3;
    hdma_spi1_tx.Init.Request = DMA_REQUEST_1;
    hdma_spi1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_spi1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_spi1_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_spi1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_spi1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_spi1_tx.Init.Mode = DMA_NORMAL;
    hdma_spi1_tx.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&hdma_spi1_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(spiHandle,hdmatx,hdma_spi1_tx);

  /* USER CODE BEGIN SPI1_MspInit 1 */

  /* USER CODE END SPI1_MspInit 1 */
  }
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef* spiHandle)
{

  if(spiHandle->Instance==SPI1)
  {
  /* USER CODE BEGIN SPI1_MspDeInit 0 */

  /* USER CODE END SPI1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_SPI1_CLK_DISABLE();

    /**SPI1 GPIO Configuration
    PA1     ------> SPI1_SCK
    PA6     ------> SPI1_MISO
    PA7     ------> SPI1_MOSI
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_1|GPIO_PIN_6|GPIO_PIN_7);

    /* SPI1 DMA DeInit */
    HAL_DMA_DeInit(spiHandle->hdmarx);
    HAL_DMA_DeInit(spiHandle->hdmatx);
  /* USER CODE BEGIN SPI1_MspDeInit 1 */

  /* USER CODE END SPI1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

uint8_t *picbuf = NULL;
volatile bool receive_OK = false;
volatile bool send_OK = true;
volatile uint32_t sendlen = 0U;
volatile uint32_t jpeglen = 0U;
static uint8_t buf1[BUFFER_MAX_SIZE];
static uint8_t spiDummyTx[BUFFER_MAX_SIZE];

uint8_t SPI1_ReadWriteByte(uint8_t txData)
{
  uint8_t rxData = 0U;

  if (HAL_SPI_TransmitReceive(&hspi1, &txData, &rxData, 1U,
                              HAL_MAX_DELAY) != HAL_OK)
  {
    return 0U;
  }

  return rxData;
}

void DMA1_RX(uint8_t *buffer, uint32_t length)
{
  if (buffer == NULL || length == 0U)
  {
    return;
  }

  CS_LOW();
  set_fifo_burst();
  if (HAL_SPI_TransmitReceive_DMA(&hspi1, spiDummyTx, buffer, length) != HAL_OK)
  {
    CS_HIGH();
  }
}

void DMA1_SendtoUart(uint8_t *buffer, uint32_t length)
{
  if (buffer != NULL && length > 0U)
  {
    (void)HAL_UART_Transmit_DMA(&huart4, buffer, length);
  }
}

bool SendImageToESP(const uint8_t *buffer, uint32_t length)
{
  static const uint8_t imageHeader[3] = {'I', 'M', 'G'};

  if (buffer == NULL || length == 0U)
  {
    return false;
  }

  if (HAL_UART_Transmit(&huart4, (uint8_t *)imageHeader,
                        sizeof(imageHeader), HAL_MAX_DELAY) != HAL_OK)
  {
    return false;
  }

  return HAL_UART_Transmit(&huart4, (uint8_t *)buffer, length,
                           HAL_MAX_DELAY) == HAL_OK;
}

void SendBufferAsHex(uint8_t *buffer, uint32_t length)
{
  static const char hexDigits[] = "0123456789ABCDEF";
  uint8_t output[3] = {0U, 0U, ' '};
  uint8_t newline = '\n';

  for (uint32_t i = 0U; i < length; i++)
  {
    output[0] = (uint8_t)hexDigits[buffer[i] >> 4];
    output[1] = (uint8_t)hexDigits[buffer[i] & 0x0FU];
    (void)HAL_UART_Transmit(&huart4, output, sizeof(output), HAL_MAX_DELAY);
  }
  (void)HAL_UART_Transmit(&huart4, &newline, 1U, HAL_MAX_DELAY);
}

void SingleCapTransfer(void)
{
  uint32_t length;

  flush_fifo();
  clear_fifo_flag();
  start_capture();
  while (get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK) == 0U)
  {
  }

  length = read_fifo_length();
  /*
   * The first byte returned after BURST_FIFO_READ is a dummy byte.
   * Reserve one buffer byte for it so the actual FIFO data starts at buf1[1].
   */
  sendlen = length < (BUFFER_MAX_SIZE - 1U)
              ? length : (BUFFER_MAX_SIZE - 1U);
  picbuf = &buf1[1];
  receive_OK = false;
  jpeglen = 0U;
  DMA1_RX(picbuf, sendlen + 1U);
}

static void CameraDmaComplete(SPI_HandleTypeDef *spiHandle)
{
  if (spiHandle == &hspi1)
  {
    CS_HIGH();
    receive_OK = true;
    for (uint32_t i = sendlen + 1U; i >= 3U; i--)
    {
      uint32_t index = i - 2U;
      if (buf1[index] == 0xFFU && buf1[index + 1U] == 0xD9U)
      {
        jpeglen = i - 1U;
        break;
      }
    }
    if (jpeglen == 0U)
    {
      jpeglen = sendlen;
    }
  }
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *spiHandle)
{
  CameraDmaComplete(spiHandle);
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *spiHandle)
{
  CameraDmaComplete(spiHandle);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *uartHandle)
{
  if (uartHandle == &huart4)
  {
    send_OK = true;
  }
}

/* USER CODE END 1 */
