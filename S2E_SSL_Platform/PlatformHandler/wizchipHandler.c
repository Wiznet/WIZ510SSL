#include <stdio.h>
#include <string.h>
#include "stm32l5xx_hal.h"
#include "wizchip_conf.h"

#include "s2e_ssl_board.h"
#include "wizchipHandler.h"
#include "gpioHandler.h"
#include "timerHandler.h"

#include "ConfigData.h"
#include "storageHandler.h"

#include "wiz_debug.h"

SPI_HandleTypeDef hspi1; // SPI1: wizchip W5500

void WIZCHIP_Reset_Init(void);
void WIZCHIP_INT_Init(void);
static void wizchip_resetAssert(void);
static void wizchip_resetDeassert(void);

#if (_WIZCHIP_ == W5500)
    void WIZCHIP_SPI_Init(void);
    void WIZCHIP_SPI_CS_Init(void);
    static void  wizchip_spi_select(void);
    static void  wizchip_spi_deselect(void);
    static uint8_t wizchip_spi_rw(uint8_t byte);
    static uint8_t wizchip_spi_read(void);
    static void  wizchip_spi_write(uint8_t wb);

#elif (_WIZCHIP_ == W5100S) && (_WIZCHIP_IO_MODE_ == _WIZCHIP_IO_MODE_BUS_INDIR_)
    static void W5500_FSMC_Init(void);
    static void FSMCLowSpeed(void);
    static void FSMCHighSpeed(void);
    
#elif (_WIZCHIP_ == W5100S) && (_WIZCHIP_IO_MODE_ == _WIZCHIP_IO_MODE_SPI_)
    void WIZCHIP_SPI_Init(void);
    void WIZCHIP_SPI_CS_Init(void);
    static void  wizchip_spi_select(void);
    static void  wizchip_spi_deselect(void);
    static uint8_t wizchip_spi_rw(uint8_t byte);
    static uint8_t wizchip_spi_read(void);
    static void  wizchip_spi_write(uint8_t wb);
    
#endif


/*
void wizchip_spi_readburst(uint8_t* pBuf, uint16_t len);
void wizchip_spi_writeburst(uint8_t* pBuf, uint16_t len);
void stm32_wizchip_dma_transfer(uint8_t receive, const uint8_t *buff, uint16_t btr);
*/

void WIZnet_Chip_Init(void)
{
    ////////////////////////////////////////////////////
    // WIZnet Hardwired TCP/IP Chip Initialize
    ////////////////////////////////////////////////////
    
#if (_WIZCHIP_ == W5500)
    /* Set Network Configuration: HW Socket Tx/Rx buffer size */
    uint8_t W5500_SocketBuf[2][8] = {{2,2,2,2,2,2,2,2},{2,2,2,2,2,2,2,2}}; // WIZCHIP socket buffers size
    
    WIZCHIP_Reset_Init();
    WIZCHIP_INT_Init();
    
    printf(" W5500 SPI Init\r\n");
    W5500_SPI_Init();
    W5500_SPI_CS_Init();
    
    wizchip_spi_deselect();
    reg_wizchip_cs_cbfunc(wizchip_spi_select, wizchip_spi_deselect);
    reg_wizchip_spi_cbfunc(wizchip_spi_read, wizchip_spi_write);
    
#ifdef STM32_WIZCHIP_USE_DMA
    // todo: develop not yet
    reg_wizchip_spiburst_cbfunc(wizchip_spi_readburst, wizchip_spi_writeburst);
#endif
    
    WIZCHIP_HW_Reset();
    
    if(ctlwizchip(CW_INIT_WIZCHIP,(void*)W5500_SocketBuf) == -1)
    {
        printf(" W5500 Initialize failed\r\n");
    }
    else
    {
        printf(" W5500 Initialized\r\n");
    }
    
#elif (_WIZCHIP_ == W5100S) && (_WIZCHIP_IO_MODE_ == _WIZCHIP_IO_MODE_BUS_INDIR_)
    
    /* Set Network Configuration: HW Socket Tx/Rx buffer size */
    uint8_t W5100S_SocketBuf[2][4] = {{2,2,2,2},{2,2,2,2}}; // WIZCHIP socket buffers size
    
    WIZCHIP_Reset_Init();
    
    printf("W5100S FSMC Init\r\n");
    W5100S_FSMC_Init();
    
    WIZCHIP_HW_Reset();
    
    if(ctlwizchip(CW_INIT_WIZCHIP,(void*)W5100S_SocketBuf) == -1)
    {
        PRT_INFO("W5100S Initialize failed\r\n");
    }
    else
    {
        PRT_INFO("W5100S Initialized\r\n");
    }
#elif (_WIZCHIP_ == W5100S) && (_WIZCHIP_IO_MODE_ == _WIZCHIP_IO_MODE_SPI_)
    
    uint8_t W5100S_SocketBuf[2][4] = {{2,2,2,2},{2,2,2,2}}; // WIZCHIP socket buffers size

    WIZCHIP_Reset_Init();
    WIZCHIP_INT_Init();
    
    printf(" W5100S SPI Init\r\n");
    //W5500_SPI_Init();
    //WIZCHIP_SPI_CS_Init();
    
    wizchip_spi_deselect();
    reg_wizchip_cs_cbfunc(wizchip_spi_select, wizchip_spi_deselect);
    reg_wizchip_spi_cbfunc(wizchip_spi_read, wizchip_spi_write);
    
#ifdef STM32_WIZCHIP_USE_DMA
    // todo: develop not yet
    reg_wizchip_spiburst_cbfunc(wizchip_spi_readburst, wizchip_spi_writeburst);
#endif
    
    WIZCHIP_HW_Reset();
    
    if(ctlwizchip(CW_INIT_WIZCHIP,(void*)W5100S_SocketBuf) == -1)
    {
        PRT_INFO(" W5100S Initialize failed\r\n");
    }
    else
    {
        PRT_INFO(" W5100S Initialized\r\n");
    }

#endif
}

void WIZCHIP_Reset_Init(void)
{
//    GPIO_Configuration(WIZCHIP_RSTn_PORT, WIZCHIP_RSTn_PIN, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL);
}

void WIZCHIP_INT_Init(void)
{
//    GPIO_Configuration(WIZCHIP_INTn_PORT, WIZCHIP_INTn_PIN, GPIO_MODE_INPUT, GPIO_NOPULL);
}

void WIZCHIP_HW_Reset(void)
{
    wizchip_resetAssert();
    delay_ms(20);
    wizchip_resetDeassert();
    delay_ms(150);
}

static void wizchip_resetAssert(void)
{
    HAL_GPIO_WritePin(WIZCHIP_RSTn_PORT, WIZCHIP_RSTn_PIN, GPIO_PIN_RESET);
}

static void wizchip_resetDeassert(void)
{
    HAL_GPIO_WritePin(WIZCHIP_RSTn_PORT, WIZCHIP_RSTn_PIN, GPIO_PIN_SET);
}


#if (_WIZCHIP_ == W5500)

void W5500_SPI_Init(void)
{
    /* SPI1 init function */
    /* SPI1 parameter configuration*/
      hspi1.Instance = SPI1;
      hspi1.Init.Mode = SPI_MODE_MASTER;
      hspi1.Init.Direction = SPI_DIRECTION_2LINES;
      hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
      hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
      hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
      hspi1.Init.NSS = SPI_NSS_SOFT;
      hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
      hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
      hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
      hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
      hspi1.Init.CRCPolynomial = 10;
      if (HAL_SPI_Init(&hspi1) != HAL_OK)
      {
        _Error_Handler(__FILE__, __LINE__);
      }
}

#endif

void WIZCHIP_SPI_DeInit(void)
{
    HAL_SPI_DeInit(&hspi1); // including HAL_SPI_MspDeInit()
}


void WIZCHIP_SPI_CS_Init(void)
{
    GPIO_Configuration(WIZCHIP_CS_PORT, WIZCHIP_CS_PIN, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL);
}


static uint8_t wizchip_spi_rw(uint8_t byte)
{
    uint8_t rtnByte;

    while (HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY);
    HAL_SPI_TransmitReceive(&hspi1, &byte, &rtnByte, 1, 10);

    return rtnByte;

#if 0
    /*!< Loop while DR register in not emplty */
    while (SPI_I2S_GetFlagStatus(W5500_SPI, SPI_I2S_FLAG_TXE) == RESET);
    /*!< Send byte through the SPI2 peripheral */
    SPI_I2S_SendData(W5500_SPI, byte);
    /*!< Wait to receive a byte */
    while (SPI_I2S_GetFlagStatus(W5500_SPI, SPI_I2S_FLAG_RXNE) == RESET);
    /*!< Return the byte read from the SPI bus */
    return SPI_I2S_ReceiveData(W5500_SPI);
#endif
}

static void  wizchip_spi_select(void)
{
    HAL_GPIO_WritePin(WIZCHIP_CS_PORT, WIZCHIP_CS_PIN, GPIO_PIN_RESET);
}

static void  wizchip_spi_deselect(void)
{
    HAL_GPIO_WritePin(WIZCHIP_CS_PORT, WIZCHIP_CS_PIN, GPIO_PIN_SET);
}

static void  wizchip_spi_write(uint8_t wb)
{
    wizchip_spi_rw(wb);
}

static uint8_t wizchip_spi_read(void)
{
    return wizchip_spi_rw(0xFF);
}

#if (_WIZCHIP_ == W5500)
uint8_t isPHYLinkUp(void)
{
    return (getPHYCFGR() & PHYCFGR_LNK_ON);
}
#endif

#ifdef __USE_WIZCHIP_RECOVERY__

// Check the MAC address
uint8_t get_wizchip_working_status(uint8_t * mac)
{
    uint8_t ret = TRUE;
    uint8_t read_mac[6] = {0x00, };

    // 1st read
    getSHAR(read_mac);
    if(memcmp(read_mac, mac, 6) != 0)
    {
        // 2nd read
        getSHAR(read_mac);
        if(memcmp(read_mac, mac, 6) != 0)
        {
            ret = FALSE;
        }
    }
    return ret;
}

// Run in MAIN routine
uint8_t process_check_wizchip_status(void)
{
    DevConfig *dev_config = get_DevConfig_pointer();

    uint8_t status;
    uint32_t tick = HAL_GetTick();

    static uint32_t update_tick;

    if(tick >= update_tick)
    {
        status = get_wizchip_working_status(dev_config->network_common.mac);
        update_tick = tick + WIZCHIP_CHECK_PERIOD_MS;

        if(status == FALSE)
        {
            return TRUE; // Event occurred
        }
    }
    return FALSE;
}
#endif



#ifdef STM32_WIZCHIP_USE_DMA

void wizchip_spi_readburst(uint8_t* pBuf, uint16_t len)
{
    stm32_wizchip_dma_transfer(1, pBuf, len);  //FALSE(0) for buff->SPI, TRUE(1) for SPI->buff
}

void  wizchip_spi_writeburst(uint8_t* pBuf, uint16_t len)
{
    stm32_wizchip_dma_transfer(0, pBuf, len);  //FALSE(0) for buff->SPI, TRUE(1) for SPI->buff
}

/*-----------------------------------------------------------------------*/
/* Transmit/Receive Block using DMA                                      */
/*-----------------------------------------------------------------------*/
void stm32_wizchip_dma_transfer(
	uint8_t receive,			/* FALSE(0) for buff->SPI, TRUE(1) for SPI->buff      */
	const uint8_t *buff,		/* receive TRUE  : 512 byte data block to be transmitted
						   		receive FALSE : Data buffer to store received data    */
	uint16_t btr 				/* Byte count */
)
{
	DMA_InitTypeDef DMA_InitStructure;
	uint16_t rw_workbyte[] = { 0xffff };

	/* shared DMA configuration values */
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&(W5500_SPI->DR));
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_BufferSize = btr;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;

	DMA_DeInit(DMA_Channel_SPI_WIZCHIP_RX);
	DMA_DeInit(DMA_Channel_SPI_WIZCHIP_TX);

	if ( receive ) {

		/* DMA1 channel2 configuration SPI1 RX ---------------------------------------------*/
		/* DMA1 channel4 configuration SPI2 RX ---------------------------------------------*/
		DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)buff;
		DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
		DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
		DMA_Init(DMA_Channel_SPI_WIZCHIP_RX, &DMA_InitStructure);

		/* DMA1 channel3 configuration SPI1 TX ---------------------------------------------*/
		/* DMA1 channel5 configuration SPI2 TX ---------------------------------------------*/
		DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)rw_workbyte;
		DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
		DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Disable;
		DMA_Init(DMA_Channel_SPI_WIZCHIP_TX, &DMA_InitStructure);

	} else {

		/* DMA1 channel2 configuration SPI1 RX ---------------------------------------------*/
		/* DMA1 channel4 configuration SPI2 RX ---------------------------------------------*/
		DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)rw_workbyte;
		DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
		DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Disable;
		DMA_Init(DMA_Channel_SPI_WIZCHIP_RX, &DMA_InitStructure);

		/* DMA1 channel3 configuration SPI1 TX ---------------------------------------------*/
		/* DMA1 channel5 configuration SPI2 TX ---------------------------------------------*/
		DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)buff;
		DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
		DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
		DMA_Init(DMA_Channel_SPI_WIZCHIP_TX, &DMA_InitStructure);
	}

	/* Enable DMA RX Channel */
	DMA_Cmd(DMA_Channel_SPI_WIZCHIP_RX, ENABLE);
	/* Enable DMA TX Channel */
	DMA_Cmd(DMA_Channel_SPI_WIZCHIP_TX, ENABLE);

	/* Enable SPI TX/RX request */
	SPI_I2S_DMACmd(W5500_SPI, SPI_I2S_DMAReq_Rx | SPI_I2S_DMAReq_Tx, ENABLE);

	/* Wait until DMA1_Channel 3 Transfer Complete */
	/// not needed: while (DMA_GetFlagStatus(DMA_FLAG_SPI_WIZCHIP_TC_TX) == RESET) { ; }
	/* Wait until DMA1_Channel 2 Receive Complete */
	while (DMA_GetFlagStatus(DMA_FLAG_SPI_WIZCHIP_TC_RX) == RESET) { ; }
	// same w/o function-call:
	// while ( ( ( DMA1->ISR ) & DMA_FLAG_SPI_WIZCHIP_TC_RX ) == RESET ) { ; }

	/* Disable DMA RX Channel */
	DMA_Cmd(DMA_Channel_SPI_WIZCHIP_RX, DISABLE);
	/* Disable DMA TX Channel */
	DMA_Cmd(DMA_Channel_SPI_WIZCHIP_TX, DISABLE);

	/* Disable SPI RX/TX request */
	SPI_I2S_DMACmd(W5500_SPI, SPI_I2S_DMAReq_Rx | SPI_I2S_DMAReq_Tx, DISABLE);
}
#endif /* STM32_WIZCHIP_USE_DMA */

#if (_WIZCHIP_ == W5100S) && (_WIZCHIP_IO_MODE_ == _WIZCHIP_IO_MODE_BUS_INDIR_)

static void W5100S_FSMC_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    //RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE | RCC_APB2Periph_GPIOF | RCC_APB2Periph_GPIOG, ENABLE);
    
    /* GPIO for W5500 initialize */
    RCC_APB2PeriphClockCmd(WIZCHIP_GPIO_RCC, ENABLE);

    /* Enable FSMC clock */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, ENABLE);
    
    //DATA |PD14|PD15|PD0|PD1|PE7|PE8|PE9|PE10|
    
    /* GPIOD configuration for FSMC data D0 ~ D3 and */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = (GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_14 | GPIO_Pin_15);
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    
    /* GPIOE configuration for FSMC data D4 ~ D7 */
    GPIO_InitStructure.GPIO_Pin = (GPIO_Pin_7 | GPIO_Pin_8  | GPIO_Pin_9  | GPIO_Pin_10);
    GPIO_Init(GPIOE, &GPIO_InitStructure);

    /* GPIOB configuration for FSMC NADV */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* NOE and NWE configuration */
    GPIO_InitStructure.GPIO_Pin = (GPIO_Pin_4 | GPIO_Pin_5);
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    /* NE1 configuration */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    
    //GPIO_WriteBit(GPIOD, GPIO_Pin_7, Bit_RESET);
    
    /* NBL0, NBL1 configuration */
    // GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
    // GPIO_Init(GPIOE, &GPIO_InitStructure);

    FSMCHighSpeed();
    //FSMCLowSpeed();
}

static void FSMCLowSpeed(void)
{
    FSMC_NORSRAMInitTypeDef  FSMC_NORSRAMInitStructure;
    FSMC_NORSRAMTimingInitTypeDef  p;

    FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM1, DISABLE);

    p.FSMC_AddressSetupTime = 0x0a;
    p.FSMC_AddressHoldTime = 00;
    p.FSMC_DataSetupTime = 0x1a;
    p.FSMC_BusTurnAroundDuration = 5;
    p.FSMC_CLKDivision = 0x00;
    p.FSMC_DataLatency = 0;
    p.FSMC_AccessMode = FSMC_AccessMode_B;

    FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM1;
    FSMC_NORSRAMInitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Enable;
    FSMC_NORSRAMInitStructure.FSMC_MemoryType = FSMC_MemoryType_NOR;
    FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_8b;
    FSMC_NORSRAMInitStructure.FSMC_BurstAccessMode = FSMC_BurstAccessMode_Disable;
    FSMC_NORSRAMInitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;
    FSMC_NORSRAMInitStructure.FSMC_WrapMode = FSMC_WrapMode_Disable;
    FSMC_NORSRAMInitStructure.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;
    FSMC_NORSRAMInitStructure.FSMC_WriteOperation = FSMC_WriteOperation_Enable;
    FSMC_NORSRAMInitStructure.FSMC_WaitSignal = FSMC_WaitSignal_Disable;
    FSMC_NORSRAMInitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Disable;
    FSMC_NORSRAMInitStructure.FSMC_WriteBurst = FSMC_WriteBurst_Disable;
    FSMC_NORSRAMInitStructure.FSMC_ReadWriteTimingStruct = &p;
    FSMC_NORSRAMInitStructure.FSMC_WriteTimingStruct = &p;
    FSMC_NORSRAMInitStructure.FSMC_AsynchronousWait = FSMC_AsynchronousWait_Disable;

    FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure);

    /*!< Enable FSMC Bank1_SRAM1 Bank */
    FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM1, ENABLE);
}

static void FSMCHighSpeed(void)
{
    FSMC_NORSRAMInitTypeDef  FSMC_NORSRAMInitStructure;
    FSMC_NORSRAMTimingInitTypeDef  p;

    FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM1, DISABLE);

    p.FSMC_AddressSetupTime = 0x03;
    p.FSMC_AddressHoldTime = 0x01;
    p.FSMC_DataSetupTime = 0x08;
    p.FSMC_BusTurnAroundDuration = 0;
    p.FSMC_CLKDivision = 0x00;
    p.FSMC_DataLatency = 0;
    p.FSMC_AccessMode = FSMC_AccessMode_B;
    
    
    FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM1;
    FSMC_NORSRAMInitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Enable;
    FSMC_NORSRAMInitStructure.FSMC_MemoryType = FSMC_MemoryType_NOR; 
    FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_8b;
    FSMC_NORSRAMInitStructure.FSMC_BurstAccessMode = FSMC_BurstAccessMode_Disable;
    FSMC_NORSRAMInitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;
    FSMC_NORSRAMInitStructure.FSMC_WrapMode = FSMC_WrapMode_Disable;
    FSMC_NORSRAMInitStructure.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;
    FSMC_NORSRAMInitStructure.FSMC_WriteOperation = FSMC_WriteOperation_Enable;
    FSMC_NORSRAMInitStructure.FSMC_WaitSignal = FSMC_WaitSignal_Disable;
    FSMC_NORSRAMInitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Disable;
    FSMC_NORSRAMInitStructure.FSMC_WriteBurst = FSMC_WriteBurst_Disable;
    FSMC_NORSRAMInitStructure.FSMC_ReadWriteTimingStruct = &p;
    FSMC_NORSRAMInitStructure.FSMC_WriteTimingStruct = &p;
    FSMC_NORSRAMInitStructure.FSMC_AsynchronousWait = FSMC_AsynchronousWait_Disable;
    
    
    /*
    p.FSMC_AddressSetupTime = 0x5;
    //p.FSMC_AddressSetupTime = 0xc;
    p.FSMC_AddressHoldTime = 0x0;
    p.FSMC_DataSetupTime = 0x0a;
    //p.FSMC_DataSetupTime = 0x20;
    p.FSMC_BusTurnAroundDuration = 0x0;
    p.FSMC_CLKDivision = 0x0;
    p.FSMC_DataLatency = 0x0;
    p.FSMC_AccessMode = FSMC_AccessMode_A;

    FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM1;
    FSMC_NORSRAMInitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Disable;
    FSMC_NORSRAMInitStructure.FSMC_MemoryType = FSMC_MemoryType_SRAM;
    FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_8b;
    FSMC_NORSRAMInitStructure.FSMC_BurstAccessMode = FSMC_BurstAccessMode_Disable;
    FSMC_NORSRAMInitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;
    FSMC_NORSRAMInitStructure.FSMC_WrapMode = FSMC_WrapMode_Disable;
    FSMC_NORSRAMInitStructure.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;
    FSMC_NORSRAMInitStructure.FSMC_WriteOperation = FSMC_WriteOperation_Enable;
    FSMC_NORSRAMInitStructure.FSMC_WaitSignal = FSMC_WaitSignal_Disable;
    FSMC_NORSRAMInitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Enable;
    FSMC_NORSRAMInitStructure.FSMC_WriteBurst = FSMC_WriteBurst_Disable;
    FSMC_NORSRAMInitStructure.FSMC_ReadWriteTimingStruct = &p;
    FSMC_NORSRAMInitStructure.FSMC_WriteTimingStruct = &p;
    FSMC_NORSRAMInitStructure.FSMC_AsynchronousWait = FSMC_AsynchronousWait_Disable;
    */
   
    
    FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure);

    /*!< Enable FSMC Bank1_SRAM1 Bank */
    FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM1, ENABLE);
}

#endif
