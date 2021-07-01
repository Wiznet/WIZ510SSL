/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "wiz_debug.h"
#include "flashHandler.h"
#include "ConfigData.h"
#include "deviceHandler.h"
#include "dhcp.h"
#include "uartHandler.h"
#include "seg.h"
#include "memory_define.h"
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
IWDG_HandleTypeDef hiwdg;

RNG_HandleTypeDef hrng;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
extern uint8_t flag_process_dhcp_success;
extern uint8_t g_rootca_buf[];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_RNG_Init(void);
static void MX_IWDG_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  DevConfig *dev_config = get_DevConfig_pointer();
  uint8_t serial_mode;
  uint32_t i;
  uint32_t updatetime = 0;
  uint32_t t1, t2;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  MX_TIM2_Init();
  MX_RNG_Init();
  //MX_IWDG_Init();
  /* USER CODE BEGIN 2 */
    
    HAL_TIM_Base_Start_IT(&htim2);
    /* WIZnet Hardwired TCP/IP chip Initialization */
    WIZnet_Chip_Init();
    
    /* WIZ550SSL Device Server: Board Initialization */
    s2e_ssl_Board_Init();
  
    //////////////////////////////////////////////////////////////////////////////////////////
    // WIZ550SSL Application Settings Initialize
    //////////////////////////////////////////////////////////////////////////////////////////
    
    /* Load the Configuration data */
    load_DevConfig_from_storage();

    /* Data0 UART Initialization */
    init_serial_settings(&huart1, DATA0_UART_PORTNUM);
    check_mac_address();

    DATA0_UART_Configuration();

    delay_ms(3000);
    while (check_phylink_status() == PHY_LINK_OFF);

    /* Data1 UART Initialization */
    //DATA1_UART_Configuration();
  
    if(dev_config->serial_common.serial_debug_en)
    {
        // Debug UART: Device information print out
        display_Dev_Info_header();
        display_Dev_Info_main();
    }
  
      ////////////////////////////////////////////////////////////////////////////////////////////////////
      // WIZ550SSL Application: Network Protocols Initialization
      //                      * DHCP / DNS / NTP client handler
      ////////////////////////////////////////////////////////////////////////////////////////////////////
  
      /* Network Configuration - DHCP client */
      // Initialize Network Information: DHCP or Static IP allocation
      Net_Conf();
      display_Net_Info();
      if(dev_config->network_option.dhcp_use)
      {
          if(process_dhcp() == DHCP_IP_LEASED) // DHCP success
          {
              flag_process_dhcp_success = ON;
          }
          else // DHCP failed
          {
              Net_Conf(); // Set default static IP settings
          }
      }

      // Debug UART: Network information print out (includes DHCP IP allocation result)
      if(dev_config->serial_common.serial_debug_en)
      {
          display_Net_Info();
          display_Dev_Info_dhcp();
      }
    
    /* DNS client */
    for(i = 0; i < DEVICE_UART_CNT; i++)
    {
        if(dev_config->network_connection[i].working_mode != TCP_SERVER_MODE)
        {
            if(dev_config->network_connection[i].dns_use)
            {
                if(i == 0) // Data UART0(channel 0)
                {
                    if(process_dns(i)) // DNS success
                    {
                        flag_process_dns_success[i] = ON;
                        PRT_INFO("flag_process_dns_success[0] == ON\r\n");
                    }
                }
                else // Data UART1(channel 1) or others
                {
                    if(memcmp(dev_config->network_connection[0].dns_domain_name,
                              dev_config->network_connection[i].dns_domain_name,
                              DNS_DOMAIN_SIZE) == 0) // Channels use the same destination (domain name)
                    {
                        memcpy(dev_config->network_connection[i].remote_ip,
                               dev_config->network_connection[0].remote_ip, 4);
                        flag_process_dns_success[i] = ON;
    
                    }
                    else if(process_dns(i)) // Channels use the different destination (domain name)
                    {
                        flag_process_dns_success[i] = ON;
                    }
                }
    
                // Debug UART: DNS results print out
                if(dev_config->serial_common.serial_debug_en)
                {
                    display_Dev_Info_dns(i);
                }
            }
        }
    }
  
    for(i = 0; i < DEVICE_UART_CNT; i++)
    {
        serial_mode = get_serial_communation_protocol(i);
    
        if(serial_mode == SEG_SERIAL_PROTOCOL_NONE)
        {
            if(dev_config->serial_common.serial_debug_en)
                PRT_INFO(" > Serial to Ethernet Gateway Mode\r\n");
        }
  
#ifdef __USE_MODBUS_PROTOCOL__
        else if(serial_mode == SEG_SERIAL_MODBUS_RTU)
        {
            eMBTCPInit(SOCK_DATA, i);
            eMBRTUInit(baud_table[dev_config->serial_option[i].baud_rate]); // Init ModbusRTU
            if(dev_config->serial_common.serial_debug_en)
                PRT_INFO(" > %s to %s Gateway Mode\r\n", STR_MODBUS_RTU, STR_MODBUS_TCP);
        }
        else if(serial_mode == SEG_SERIAL_MODBUS_ASCII)
        {
            eMBTCPInit(SOCK_DATA, i);
            if(dev_config->serial_common.serial_debug_en)
                PRT_INFO(" > %s to %s Gateway Mode\r\n", STR_MODBUS_ASCII, STR_MODBUS_TCP);
        }
#endif
    }

#ifdef __USE_WATCHDOG__
    MX_IWDG_Init();
#endif

    updatetime = millis();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      if (flag_check_phylink)
      {
        flag_check_phylink = 0;
        if (check_phylink_status() == PHY_LINK_OFF)
            device_reboot();
      }
  
      t1 = millis(); // Start time of main routine

      do_segcp();

      if((dev_config->serial_option[0].uart_interface == UART_IF_TTL) ||
         (dev_config->serial_option[0].uart_interface == UART_IF_RS232))
      {
          // UART Port #0: RS-232c
          do_seg(DATA0_UART_PORTNUM, SEG_DATA0_SOCK);
      }
      else
      {
          // UART Port #1: RS-422/485
          do_seg(DATA1_UART_PORTNUM, SEG_DATA0_SOCK);
      }

      if(flag_process_dhcp_success == ON) DHCP_run(); // DHCP client handler for IP renewal

#ifdef __USE_WIZCHIP_RECOVERY__
      if(process_check_wizchip_status())
      {
          if(dev_config->serial_common.serial_debug_en) printf(" > WIZCHIP RE-INIT\r\n");

          set_wizchip_recovery(WIZCHIP_RECOVERY_TIMEOUT_MS); // WIZCHIP Netinfo recovery with WIZCHIP H/W reset
      }
#endif

      // Optional Proto  col handlers
#ifdef __USE_NETBIOS__
      do_netbios();
#endif

      // ## debugging: Data echoback
      //loopback_tcps(7, g_recv_buf, 50000); // Loopback

      t2 = millis(); // End time of main routine

      // ## debugging: Timer
      if((millis() - updatetime) >= 1000)
      {
          //printf("%ldms\r\n", t2-t1);

          //printf("%d\r\n", wizdevice_gcp_get_status_update_counter());
          //printf_datetime_str(getNow(), 9);
          //printf("updatetime: %ld\r\n", updatetime);

          LED_Toggle(LED_0);
          LED_Toggle(LED_1);

          updatetime = millis();
      }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
      wdt_reset();
  }
  /* USER CODE END 3 */
}

/**
  * @brief IWDG Initialization Function
  * @param None
  * @retval None
  */
static void MX_IWDG_Init(void)
{

  /* USER CODE BEGIN IWDG_Init 0 */

  /* USER CODE END IWDG_Init 0 */

  /* USER CODE BEGIN IWDG_Init 1 */

  /* USER CODE END IWDG_Init 1 */
  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_128;
  hiwdg.Init.Window = 4095;
  hiwdg.Init.Reload = 4095;
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN IWDG_Init 2 */

  /* USER CODE END IWDG_Init 2 */

}

/**
  * @brief RNG Initialization Function
  * @param None
  * @retval None
  */
static void MX_RNG_Init(void)
{

  /* USER CODE BEGIN RNG_Init 0 */

  /* USER CODE END RNG_Init 0 */

  /* USER CODE BEGIN RNG_Init 1 */

  /* USER CODE END RNG_Init 1 */
  hrng.Instance = RNG;
  hrng.Init.ClockErrorDetection = RNG_CED_ENABLE;
  if (HAL_RNG_Init(&hrng) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RNG_Init 2 */

  /* USER CODE END RNG_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 100;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_RTS_CTS;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, W5100S_RST_Pin|DTR_OUT_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, STATUS_LED0_Pin|STATUS_LED1_Pin|STATUS_OUT_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : SPI1_CS_Pin */
  GPIO_InitStruct.Pin = SPI1_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SPI1_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : W5100S_INT_Pin DSR_IN_Pin */
  GPIO_InitStruct.Pin = W5100S_INT_Pin|DSR_IN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : W5100S_RST_Pin STATUS_LED0_Pin STATUS_LED1_Pin STATUS_OUT_Pin
                           DTR_OUT_Pin */
  GPIO_InitStruct.Pin = W5100S_RST_Pin|STATUS_LED0_Pin|STATUS_LED1_Pin|STATUS_OUT_Pin
                          |DTR_OUT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : FAC_RESET_Pin */
  GPIO_InitStruct.Pin = FAC_RESET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(FAC_RESET_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

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

#ifdef  USE_FULL_ASSERT
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
