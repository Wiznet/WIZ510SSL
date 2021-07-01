/**
  ******************************************************************************
  * @file    Secure/Src/secure_nsc.c
  * @author  MCD Application Team
  * @brief   This file contains the non-secure callable APIs (secure world)
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE BEGIN Non_Secure_CallLib */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "secure_nsc.h"
#include <stdio.h>
#include <stdarg.h>
#include "secureStorageHandler.h"

/** @addtogroup STM32L5xx_HAL_Examples
  * @{
  */

/** @addtogroup Templates
  * @{
  */

/* Global variables ----------------------------------------------------------*/
void *pSecureFaultCallback = NULL;   /* Pointer to secure fault callback in Non-secure */
void *pSecureErrorCallback = NULL;   /* Pointer to secure error callback in Non-secure */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Secure registration of non-secure callback.
  * @param  CallbackId  callback identifier
  * @param  func        pointer to non-secure function
  * @retval None
  */
CMSE_NS_ENTRY void SECURE_RegisterCallback(SECURE_CallbackIDTypeDef CallbackId, void *func)
{
  if(func != NULL)
  {
    switch(CallbackId)
    {
      case SECURE_FAULT_CB_ID:           /* SecureFault Interrupt occurred */
        pSecureFaultCallback = func;
        break;
      case GTZC_ERROR_CB_ID:             /* GTZC Interrupt occurred */
        pSecureErrorCallback = func;
        break;
      default:
        /* unknown */
        break;
    }
  }
}

//CMSE_NS_ENTRY void SECURE_printf(const char* format, ...)
#if 0
CMSE_NS_CALL void SECURE_printf(const char* format, ...)
{
	char va_buf[1024] = {0, };
	va_list ap;
	va_start(ap, format);
	//vprintf(format, ap);
	//printf(format, ##__VA_ARGS__);

	vsprintf(va_buf, format, ap);
	printf(va_buf);
	va_end(ap);
}
#else
CMSE_NS_ENTRY void SECURE_printf(const char* buf)
{
	//extern UART_HandleTypeDef huart2;
	printf("%s", buf);

	//HAL_UART_Transmit(&huart2, "Secure_printf\r\n", 15, 100);
}

CMSE_NS_ENTRY int SECURE_FLASH_WRITE(teDATASTORAGE stype, void *data, uint16_t size)
{
    secure_erase_storage(stype);
	return secure_write_storage(stype, data, size);
}

CMSE_NS_ENTRY int SECURE_FLASH_READ(teDATASTORAGE stype, void *data, uint16_t size)
{
	return secure_read_storage(stype, data, size);
}

CMSE_NS_ENTRY int SECURE_FLASH_ERASE(teDATASTORAGE stype)
{
	return secure_erase_storage(stype);
}

extern uint32_t VTOR_TABLE_NS_START_ADDR;
CMSE_NS_ENTRY uint8_t SECURE_Get_Running_Bank(void)
{
    if (VTOR_TABLE_NS_START_ADDR == FLASH_START_ADDR_BANK1)
        return 1;
    else
        return 0;
}


#endif

/**
  * @}
  */

/**
  * @}
  */
/* USER CODE END Non_Secure_CallLib */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
