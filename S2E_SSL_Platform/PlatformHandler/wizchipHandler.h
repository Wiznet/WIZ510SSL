/*
*
@file   wiz2000_board.h
@brief
*/

#ifndef __WIZCHIPHANDLER_H_
#define __WIZCHIPHANDLER_H_

#include "common.h"

//#define STM32_WIZCHIP_USE_DMA

#ifdef STM32_WIZCHIP_USE_DMA
    #define DMA_Channel_SPI_WIZCHIP_RX    DMA1_Channel4
    #define DMA_Channel_SPI_WIZCHIP_TX    DMA1_Channel5
    #define DMA_FLAG_SPI_WIZCHIP_TC_RX    DMA1_FLAG_TC4
    #define DMA_FLAG_SPI_WIZCHIP_TC_TX    DMA1_FLAG_TC5    
#endif


/* This function only supports W5500 SPI mode and W5100S Indirect BUS mode(FSMC) */
void WIZnet_Chip_Init(void);
void WIZCHIP_HW_Reset(void);

uint8_t isPHYLinkUp(void);
uint8_t get_wizchip_working_status(uint8_t * mac);
uint8_t process_check_wizchip_status(void); // for Main routine

#endif
