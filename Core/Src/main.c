/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "dma.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_cdc_if.h"
#include "proximity_sensor.h"
#include "ArduCAM.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include <string.h>
#include <stdio.h>

extern USBD_HandleTypeDef hUsbDeviceFS;
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
extern volatile uint8_t usbCommandReady;
extern char usbCommand[64];

static uint8_t vcnl4040Ready = 0U;
static uint8_t proximityDisplayEnabled = 0U;
static uint8_t proximityMonitorEnabled = 0U;
static uint32_t lastProximityReportTick = 0U;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void USB_Send(const char *text);
static uint8_t USB_SendBuffer(const uint8_t *buffer, uint16_t length);
static void ProcessUsbCommand(void);
static void UpdateProximityLed(void);
static void ShowCommandOnOled(const char *command);
static void ShowCommandPromptOnOled(void);
static void ShowHelpOnOled(void);
static void ShowProximityOnOled(uint8_t detected, uint16_t proximity);
static void CaptureAndSendSnapshot(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static void USB_Send(const char *text)
{
  static uint8_t txBuffer[APP_TX_DATA_SIZE];
  size_t length = strlen(text);

  if (length >= sizeof(txBuffer))
  {
    length = sizeof(txBuffer) - 1U;
  }
  memcpy(txBuffer, text, length);
  txBuffer[length] = '\0';

  for (uint32_t attempt = 0U; attempt < 10U; attempt++)
  {
    if (CDC_Transmit_FS(txBuffer, (uint16_t)length) == USBD_OK)
    {
      return;
    }
    HAL_Delay(2U);
  }
}

static uint8_t USB_SendBuffer(const uint8_t *buffer, uint16_t length)
{
  uint32_t startTick = HAL_GetTick();
  USBD_CDC_HandleTypeDef *cdcHandle =
    (USBD_CDC_HandleTypeDef *)hUsbDeviceFS.pClassData;

  while (CDC_Transmit_FS((uint8_t *)buffer, length) != USBD_OK)
  {
    if ((HAL_GetTick() - startTick) >= 1000U)
    {
      return 0U;
    }
    HAL_Delay(1U);
  }

  while (cdcHandle != NULL && cdcHandle->TxState != 0U)
  {
    if ((HAL_GetTick() - startTick) >= 2000U)
    {
      return 0U;
    }
    HAL_Delay(1U);
  }

  return 1U;
}

static uint8_t USB_SendSnapshotHex(const uint8_t *buffer, uint32_t length)
{
  static const char hexDigits[] = "0123456789ABCDEF";
  uint8_t output[192];
  uint32_t index = 0U;

  while (index < length)
  {
    uint16_t outputLength = 0U;

    while (index < length && outputLength + 2U <= sizeof(output))
    {
      uint8_t value = buffer[index++];

      output[outputLength++] = (uint8_t)hexDigits[value >> 4];
      output[outputLength++] = (uint8_t)hexDigits[value & 0x0FU];
    }

    if (USB_SendBuffer(output, outputLength) == 0U)
    {
      return 0U;
    }
  }

  return 1U;
}

static void CaptureAndSendSnapshot(void)
{
  uint32_t jpegEnd;

  proximityMonitorEnabled = 0U;
  proximityDisplayEnabled = 0U;
  ShowCommandOnOled("SNAP");

  SingleCapTransfer();
  while (receive_OK == false)
  {
    HAL_Delay(1U);
  }

  jpegEnd = jpeglen;
  if (jpegEnd < 4U ||
      picbuf[0U] != 0xFFU ||
      picbuf[1U] != 0xD8U ||
      picbuf[jpegEnd - 2U] != 0xFFU ||
      picbuf[jpegEnd - 1U] != 0xD9U)
  {
    USB_Send("\r\nInvalid JPEG markers. Expected FF D8 ... FF D9\r\n");
    receive_OK = false;
    return;
  }

  if (!SendImageToESP(picbuf, jpegEnd))
  {
    USB_Send("\r\nUART4 image transmission failed\r\n");
    receive_OK = false;
    return;
  }

  receive_OK = false;
  if (USB_SendSnapshotHex(picbuf, jpegEnd) == 0U)
  {
    USB_Send("\r\nUSB hex transfer timeout\r\n");
    return;
  }
  USB_Send("\r\nJPEG sent to ESP32: ");
  {
    char response[32];

    (void)snprintf(response, sizeof(response), "%lu bytes\r\n",
                   (unsigned long)jpegEnd);
    USB_Send(response);
  }
}

static void ShowCommandOnOled(const char *command)
{
  char line[32];
  size_t length = 0U;

  ssd1306_Fill(Black);
  ssd1306_SetCursor(0U, 0U);
  ssd1306_WriteString("CMD:", Font_7x10, White);

  if (command != NULL)
  {
    length = strlen(command);
    if (length > (sizeof(line) - 1U))
    {
      length = sizeof(line) - 1U;
    }

    memcpy(line, command, length);
    line[length] = '\0';
    ssd1306_SetCursor(0U, 16U);
    ssd1306_WriteString(line, Font_7x10, White);
  }

  ssd1306_UpdateScreen();
}

static void ShowCommandPromptOnOled(void)
{
  ssd1306_Fill(Black);
  ssd1306_SetCursor(0U, 0U);
  ssd1306_WriteString("TYPE COMMAND", Font_7x10, White);
  ssd1306_SetCursor(0U, 16U);
  ssd1306_WriteString("help", Font_7x10, White);
  ssd1306_UpdateScreen();
}

static void ShowProximityOnOled(uint8_t detected, uint16_t proximity)
{
  char valueLine[24];

  ssd1306_Fill(Black);
  ssd1306_SetCursor(0U, 0U);
  ssd1306_WriteString(detected != 0U ? "HAND DETECTED" : "NO HAND", Font_7x10, White);
  (void)snprintf(valueLine, sizeof(valueLine), "VALUE: %u", proximity);
  ssd1306_SetCursor(0U, 16U);
  ssd1306_WriteString(valueLine, Font_7x10, White);
  ssd1306_UpdateScreen();
}

static void ShowHelpOnOled(void)
{
  ssd1306_Fill(Black);
  ssd1306_SetCursor(0U, 0U);
  ssd1306_WriteString("COMMANDS:", Font_7x10, White);
  ssd1306_SetCursor(0U, 16U);
  ssd1306_WriteString("prox = sensor", Font_7x10, White);
  ssd1306_SetCursor(0U, 28U);
  ssd1306_WriteString("snap = camera", Font_7x10, White);
  ssd1306_UpdateScreen();
}

static void ProcessUsbCommand(void)
{
  char command[sizeof(usbCommand)];
  char response[96];
  uint16_t proximity;

  if (usbCommandReady == 0U)
  {
    return;
  }

  __disable_irq();
  memcpy(command, usbCommand, sizeof(command));
  usbCommandReady = 0U;
  __enable_irq();

  if (strcmp(command, "help") == 0)
  {
    proximityDisplayEnabled = 0U;
    proximityMonitorEnabled = 0U;
    ShowHelpOnOled();
    USB_Send("\r\nCommands:\r\n  help - show this message\r\n  prox - read VCNL4040 proximity\r\n  snap - capture one JPEG and send it to ESP32\r\n\r\n");
  }
  else if (strcmp(command, "prox") == 0)
  {
    if (vcnl4040Ready == 0U ||
        ProximitySensor_Read(&proximity) != HAL_OK)
    {
      HAL_GPIO_WritePin(LD4_GPIO_Port, LD4_Pin, GPIO_PIN_RESET);
      USB_Send("\r\nVCNL4040 read error\r\n");
      return;
    }

    proximityDisplayEnabled = 1U;
    proximityMonitorEnabled = 1U;
    lastProximityReportTick = HAL_GetTick();
    ShowProximityOnOled(
      proximity >= PROXIMITY_SENSOR_DETECTION_LEVEL ? 1U : 0U,
      proximity);
    HAL_GPIO_WritePin(LD4_GPIO_Port, LD4_Pin,
                      proximity >= PROXIMITY_SENSOR_DETECTION_LEVEL
                        ? GPIO_PIN_SET : GPIO_PIN_RESET);
    (void)snprintf(response, sizeof(response),
                   "\r\nProximity: 0x%04X (%u)\r\n",
                   proximity, proximity);
    USB_Send(response);
  }
  else if (strcmp(command, "snap") == 0)
  {
    CaptureAndSendSnapshot();
  }
  else
  {
    ShowCommandOnOled(command);
    USB_Send("\r\nUnknown command. Type help.\r\n");
  }
}

static void UpdateProximityLed(void)
{
  uint16_t proximity;
  static uint8_t previousDetected = 0xFFU;
  static uint32_t lastProximityDisplayTick = 0U;
  uint8_t detected = 0U;
  GPIO_PinState ledState = GPIO_PIN_RESET;

  if (vcnl4040Ready != 0U &&
      ProximitySensor_Read(&proximity) == HAL_OK)
  {
    detected = proximity >= PROXIMITY_SENSOR_DETECTION_LEVEL ? 1U : 0U;
    ledState = detected != 0U ? GPIO_PIN_SET : GPIO_PIN_RESET;

    if (proximityDisplayEnabled != 0U &&
        (detected != previousDetected ||
         (HAL_GetTick() - lastProximityDisplayTick) >= 500U))
    {
      ShowProximityOnOled(detected, proximity);
      previousDetected = detected;
      lastProximityDisplayTick = HAL_GetTick();
    }

    if (proximityMonitorEnabled != 0U &&
        (HAL_GetTick() - lastProximityReportTick) >= 500U)
    {
      char response[64];

      (void)snprintf(response, sizeof(response),
                     "Proximity: 0x%04X (%u)\r\n",
                     proximity, proximity);
      USB_Send(response);
      lastProximityReportTick = HAL_GetTick();
    }
  }

  HAL_GPIO_WritePin(LD4_GPIO_Port, LD4_Pin, ledState);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_TIM2_Init();
  MX_UART4_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */
  vcnl4040Ready = (ProximitySensor_Init() == HAL_OK) ? 1U : 0U;
  ssd1306_Init();
  ssd1306_Fill(Black);
  ssd1306_SetCursor(0U, 0U);
  ssd1306_WriteString("OLED READY", Font_7x10, White);
  ssd1306_SetCursor(0U, 16U);
  ssd1306_WriteString("PROXIMITY ACTIVE", Font_7x10, White);
  ssd1306_UpdateScreen();
  ShowCommandPromptOnOled();
  delay_init();
  ArduCAM_CS_init();
  set_format(JPEG);
  ArduCAM_Init(OV2640);
  OV2640_set_JPEG_size(OV2640_160x120);
  USBD_CDC_ReceivePacket(&hUsbDeviceFS);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    ProcessUsbCommand();
    UpdateProximityLed();
    HAL_Delay(100U);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSE
                              |RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = 64;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable MSI Auto calibration
  */
  HAL_RCCEx_EnableMSIPLLMode();
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
