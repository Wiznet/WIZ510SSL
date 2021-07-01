#include <stdio.h>
#include <string.h>

#include "stm32l5xx_hal.h"

#include "common.h"
//#include "configdata.h"

#include "s2e_ssl_board.h"
#include "uartHandler.h"
#include "gpioHandler.h"
#include "timerHandler.h"

#include "seg.h"

#ifdef __USE_MODBUS_PROTOCOL__
    #include "mbrtu.h"
    #include "mbtcp.h"
    #include "mbascii.h"
    #include "mbtimer.h"
#endif

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
void init_serial_settings(UART_HandleTypeDef *huart, uint8_t uartNum);

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;
//UART_HandleTypeDef huart2;
UART_HandleTypeDef huart6;

// UART Ring buffer declaration
BUFFER_DEFINITION(data0_rx, SEG_DATA_BUF_SIZE);

uint8_t flag_ringbuf_full = 0;

uint32_t baud_table[] = {300, 600, 1200, 1800, 2400, 4800, 9600, 14400, 19200, 28800, 38400, 57600, 115200, 230400, 460800};
uint8_t word_len_table[] = {7, 8, 9};
uint8_t * parity_table[] = {(uint8_t *)"N", (uint8_t *)"ODD", (uint8_t *)"EVEN"};
uint8_t stop_bit_table[] = {1, 2};
uint8_t * flow_ctrl_table[] = {(uint8_t *)"NONE", (uint8_t *)"XON/XOFF", (uint8_t *)"RTS/CTS", (uint8_t *)"RTS Only"};
uint8_t * uart_if_table[] = {(uint8_t *)UART_IF_STR_TTL, (uint8_t *)UART_IF_STR_RS232, (uint8_t *)UART_IF_STR_RS422, (uint8_t *)UART_IF_STR_RS485};
//uint8_t * uart_if_table[] = {(uint8_t *)UART_IF_STR_RS232_TTL, (uint8_t *)UART_IF_STR_RS422_485};

// 1-byte char from UART Rx/Tx for interrupt handler
static uint8_t uartRxByte;

// XON/XOFF Status; 
static uint8_t xonoff_status[DEVICE_UART_CNT] = {UART_XON, };

// RTS Status; __USE_GPIO_HARDWARE_FLOWCONTROL__ defined
#ifdef __USE_GPIO_HARDWARE_FLOWCONTROL__
    static uint8_t rts_status = UART_RTS_LOW;
#endif

// UART Interface selector; RS-422 or RS-485 use only
static uint8_t uart_if_mode[DEVICE_UART_CNT] = {UART_IF_RS422, };

/* Public functions ----------------------------------------------------------*/

////////////////////////////////////////////////////////////////////////////////
// Data UART Configuration
////////////////////////////////////////////////////////////////////////////////

void DEBUG_UART_Configuration(void)
{
#if 0
    /* USART2 init function */
    huart2.Instance = USART2;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;

    // Default settings: 115200-8-N-1 / None
    huart2.Init.BaudRate = DEBUG_UART_DEFAULT_BAUDRATE;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;

    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        _Error_Handler(__FILE__, __LINE__);
    }

    // function call w/ debug UART initialize
    setbuf(stdout, NULL); // for printf() function flush immediately
#endif
}

////////////////////////////////////////////////////////////////////////////////
// Data UART Configuration & IRQ handler
////////////////////////////////////////////////////////////////////////////////

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
#ifdef __USE_UART_TX_INTERRUPT__
    if(huart->Instance == DATA0_UART) // USART1
    {
        if(is_uart_buffer_empty(DATA0_UART_PORTNUM) == FALSE)
        {
            uartTxByte = BUFFER_OUT(data0_tx);
            BUFFER_OUT_MOVE(data0_tx, 1);

            // Write one byte to the transmit data register
            HAL_UART_Transmit_IT(&huart, &uartTxByte, 1);
        }
        //else
        //{
        //    HAL_NVIC_DisableIRQ(DATA0_UART_IRQn); // Disable the UART Transmit interrupt
        //}
    }
    else if(huart->Instance == DATA1_UART) // USART6
    {
        if(is_uart_buffer_empty(DATA1_UART_PORTNUM) == FALSE)
        {
            uartTxByte = BUFFER_OUT(data1_tx);
            BUFFER_OUT_MOVE(data1_tx, 1);

            // Write one byte to the transmit data register
            HAL_UART_Transmit_IT(&huart, &uartTxByte, 1);
        }
        //else
        //{
        //    HAL_NVIC_DisableIRQ(DATA1_UART_IRQn); // Disable the UART Transmit interrupt
        //}
    }
#endif
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    //uartRxByte: // 1-byte character variable for UART Interrupt request handler

    uint8_t channel, serial_protocol;

    if(huart->Instance == DATA0_UART) // USART1
    {
        channel = get_seg_channel(DATA0_UART_PORTNUM);
        serial_protocol = get_serial_communation_protocol(channel);

#ifdef _UART_DEBUG_
        uart_putc(DEBUG_UART_PORTNUM, uartRxByte);// ## UART echo; for debugging
#endif

        if(!(check_modeswitch_trigger(uartRxByte))) // ret: [0] data / [!0] trigger code
        {
            if(serial_protocol == SEG_SERIAL_PROTOCOL_NONE) // Serial to Ethernet Gateway mode
            {
                if(is_uart_buffer_full(DATA0_UART_PORTNUM) == FALSE)
                {
                    if(check_serial_store_permitted(channel, uartRxByte)) // ret: [0] not permitted / [1] permitted
                    {
                        put_byte_to_uart_buffer(DATA0_UART_PORTNUM, uartRxByte);
                    }
                }
                else
                {
                    // buffer full
                    flag_ringbuf_full = ON;
                }
                init_time_delimiter_timer(channel);
            }
#ifdef __USE_MODBUS_PROTOCOL__
            else if(serial_protocol == SEG_SERIAL_MODBUS_RTU)   // ModbusRTU to ModbusTCP Gateway mode
            {
                RTU_Uart_RX(uartRxByte);
            }
            else if(serial_protocol == SEG_SERIAL_MODBUS_ASCII) // ModbusASCII to ModbusTCP Gateway mode
            {
                ASCII_Uart_RX(uartRxByte);
            }
#endif
        }
        //__HAL_UART_FLUSH_DRREGISTER(huart);
        get_byte_from_uart_it(DATA0_UART_PORTNUM);
    }
#if (DEVICE_UART_CNT > 1)
    else if(huart->Instance == DATA1_UART) // USART6
    {
        channel = get_seg_channel(DATA1_UART_PORTNUM);
        serial_protocol = get_serial_communation_protocol(channel);

#ifdef _UART_DEBUG_
        uart_putc(DEBUG_UART_PORTNUM, uartRxByte);// ## UART echo; for debugging
#endif

        if(!(check_modeswitch_trigger(uartRxByte))) // ret: [0] data / [!0] trigger code
        {
            if(serial_protocol == SEG_SERIAL_PROTOCOL_NONE) // Serial to Ethernet Gateway mode
            {
                if(is_uart_buffer_full(DATA1_UART_PORTNUM) == FALSE)
                {
                    if(check_serial_store_permitted(channel, uartRxByte)) // ret: [0] not permitted / [1] permitted
                    {
                        put_byte_to_uart_buffer(DATA1_UART_PORTNUM, uartRxByte);
                    }
                }
                else
                {
                    // buffer full
                    flag_ringbuf_full = ON;
                }
                init_time_delimiter_timer(channel);
            }
#ifdef __USE_MODBUS_PROTOCOL__
            else if(serial_protocol == SEG_SERIAL_MODBUS_RTU)   // ModbusRTU to ModbusTCP Gateway mode
            {
                RTU_Uart_RX(uartRxByte);
            }
            else if(serial_protocol == SEG_SERIAL_MODBUS_ASCII) // ModbusASCII to ModbusTCP Gateway mode
            {
                ASCII_Uart_RX(uartRxByte);
            }
#endif
        }

        get_byte_from_uart_it(DATA1_UART_PORTNUM);
    }
#endif
}

#if 0
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    //struct __serial_info *serial = (struct __serial_info *)get_DevConfig_pointer()->serial_info;
    //uint8_t ch; // 1-byte character variable for UART Interrupt request handler

    if(huart->Instance == DATA0_UART) // USART1
    {
        if(is_uart_buffer_full(DATA0_UART_PORTNUM) == FALSE)
        {
            //ch = get_byte_from_uart(DATA0_UART_PORTNUM);

#ifdef _UART_DEBUG_
            uart_putc(DEBUG_UART_PORTNUM, uartRxByte);// ## UART echo; for debugging
#endif
            if(!(check_modeswitch_trigger(uartRxByte))) // ret: [0] data / [!0] trigger code
            {
                if(check_serial_store_permitted(channel, uartRxByte)) // ret: [0] not permitted / [1] permitted
                {
                    put_byte_to_uart_buffer(DATA0_UART_PORTNUM, uartRxByte);
                }
            }
        }
        else
        {
            get_byte_from_uart(DATA0_UART_PORTNUM); // buffer full: receive and discard
            flag_ringbuf_full = ON;
        }
        init_time_delimiter_timer();

        //__HAL_UART_FLUSH_DRREGISTER(huart);
        get_byte_from_uart_it(DATA0_UART_PORTNUM);
    }
    else if(huart->Instance == DATA1_UART) // USART6
    {
        if(is_uart_buffer_full(DATA1_UART_PORTNUM) == FALSE)
        {
            //ch = get_byte_from_uart(DATA0_UART_PORTNUM);

#ifdef _UART_DEBUG_
            uart_putc(DEBUG_UART_PORTNUM, uartRxByte);// ## UART echo; for debugging
#endif
            if(!(check_modeswitch_trigger(uartRxByte))) // ret: [0] data / [!0] trigger code
            {
                if(check_serial_store_permitted(channel, uartRxByte)) // ret: [0] not permitted / [1] permitted
                {
                    put_byte_to_uart_buffer(DATA1_UART_PORTNUM, uartRxByte);
                }
            }
        }
        else
        {
            get_byte_from_uart(DATA1_UART_PORTNUM); // buffer full: receive and discard
            flag_ringbuf_full = ON;
        }

        init_time_delimiter_timer();

        //__HAL_UART_FLUSH_DRREGISTER(huart);
        get_byte_from_uart_it(DATA1_UART_PORTNUM);
    }
}
#endif

void DATA0_UART_Configuration(void)
{
#if 0
    /* USART1 init function */
    huart1.Instance = USART1;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;

    //huart1.Init.BaudRate = 115200;
    //huart1.Init.WordLength = UART_WORDLENGTH_8B;
    //huart1.Init.StopBits = UART_STOPBITS_1;
    //huart1.Init.Parity = UART_PARITY_NONE;
    //huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;

    init_serial_settings(&huart1, DATA0_UART_PORTNUM);

    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        _Error_Handler(__FILE__, __LINE__);
    }
#endif

    get_seg_channel(DATA0_UART_PORTNUM);

    HAL_NVIC_SetPriority(DATA0_UART_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DATA0_UART_IRQn);

    get_byte_from_uart_it(DATA0_UART_PORTNUM);
}

#if (DEVICE_UART_CNT > 1)
void DATA1_UART_Configuration(void)
{
    /* USART6 init function */

    // set default
    huart6.Instance = USART2;
    huart6.Init.Mode = UART_MODE_TX_RX;
    huart6.Init.OverSampling = UART_OVERSAMPLING_16;

    //huart6.Init.BaudRate = 115200;
    //huart6.Init.WordLength = UART_WORDLENGTH_8B;
    //huart6.Init.StopBits = UART_STOPBITS_1;
    //huart6.Init.Parity = UART_PARITY_NONE;
    //huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;

    init_serial_settings(&huart6, DATA1_UART_PORTNUM);

    if (HAL_UART_Init(&huart6) != HAL_OK)
    {
        _Error_Handler(__FILE__, __LINE__);
    }

    HAL_NVIC_SetPriority(DATA1_UART_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DATA1_UART_IRQn);

    get_byte_from_uart_it(DATA1_UART_PORTNUM);
}
#endif

void init_serial_settings(UART_HandleTypeDef *huart, uint8_t uartNum)
{
    struct __serial_option *serial_option = (struct __serial_option *)(get_DevConfig_pointer()->serial_option);
    uint8_t valid_arg = 0;

    uint8_t channel = get_seg_channel(uartNum);

    HAL_UART_DeInit(huart);


    /* Set Baud Rate */
    if(serial_option[channel].baud_rate < (sizeof(baud_table) / sizeof(baud_table[0])))
    {
        huart->Init.BaudRate = baud_table[serial_option->baud_rate];
        valid_arg = 1;
    }
    
    if(!valid_arg)
        huart->Init.BaudRate = baud_table[baud_115200]; // Baud rate default: 115200bps

    /* Set Data Bits */
    switch(serial_option[channel].data_bits) {
        case word_len8:
            huart->Init.WordLength = UART_WORDLENGTH_8B;
            break;
        case word_len7:
            huart->Init.WordLength = UART_WORDLENGTH_8B; // + (ch & 0x7F)
            break;
        case word_len9:
            huart->Init.WordLength = UART_WORDLENGTH_9B; // ## todo
            break;
        default:
            huart->Init.WordLength = UART_WORDLENGTH_8B;
            serial_option[channel].data_bits = word_len8;
            break;
    }

    /* Set Stop Bits */
    switch(serial_option[channel].stop_bits) {
        case stop_bit1:
            huart->Init.StopBits = UART_STOPBITS_1;
            break;
        case stop_bit2:
            huart->Init.StopBits = UART_STOPBITS_2;
            break;
        default:
            huart->Init.StopBits = UART_STOPBITS_1;
            serial_option->stop_bits = stop_bit1;
            break;
    }

    /* Set Parity Bits */
    switch(serial_option[channel].parity) {
        case parity_none:
            huart->Init.Parity = UART_PARITY_NONE;
            break;
        case parity_odd:
            huart->Init.Parity = UART_PARITY_ODD;
            break;
        case parity_even:
            huart->Init.Parity = UART_PARITY_EVEN;
            break;
        default:
            huart->Init.Parity = UART_PARITY_NONE;
            serial_option[channel].parity = parity_none;
            break;
    }
    
    /* Flow Control */
    if((serial_option[channel].uart_interface == UART_IF_TTL) || (serial_option[channel].uart_interface == UART_IF_RS232))
    {
        // RS232 Hardware Flow Control
        //7     RTS     Request To Send     Output
        //8     CTS     Clear To Send       Input
        switch(serial_option->flow_control) {
            case flow_none:
                huart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
                break;
            case flow_rts_cts:
#ifdef __USE_GPIO_HARDWARE_FLOWCONTROL__
                huart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
                set_uart_rts_pin_low(uartNum);
#else
                huart->Init.HwFlowCtl = UART_HWCONTROL_RTS_CTS;
#endif
                break;
            case flow_xon_xoff:
                huart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
                break;
            default:
                huart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
                serial_option[channel].flow_control = flow_none;
                break;
        }
    }

#ifdef __USE_UART_485_422__
    else // UART_IF_RS422 || UART_IF_RS485
    {
        huart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
        
        // GPIO configuration (RTS pin -> GPIO: 485SEL)
        if((serial_option[channel].flow_control != flow_rtsonly) && (serial_option[channel].flow_control != flow_reverserts))
        {
            uart_if_mode[channel] = get_uart_rs485_sel(uartNum);
        }
        else
        {
            if(serial_option[channel].flow_control == flow_rtsonly)
            {
                uart_if_mode[channel] = UART_IF_RS485;
            }else
            {
                uart_if_mode[channel] = UART_IF_RS485_REVERSE;
            }
        }

        uart_rs485_rs422_init(uartNum);
    }
#endif
  if (HAL_UART_Init(huart) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(huart, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(huart, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(huart) != HAL_OK)
  {
    Error_Handler();
  }
}


void check_uart_flow_control(uint8_t uartNum, uint8_t flow_ctrl)
{
    uint8_t channel = get_seg_channel(uartNum);

    if(flow_ctrl == flow_xon_xoff)
    {
        if((xonoff_status[channel] == UART_XON) && (get_uart_buffer_usedsize(uartNum) > UART_OFF_THRESHOLD)) // Send the transmit stop command to peer - go XOFF
        {
            uart_putc(uartNum, UART_XOFF);
            xonoff_status[channel] = UART_XOFF;
#ifdef _UART_DEBUG_
            printf(" >> SEND XOFF [%d / %d]\r\n", get_uart_buffer_usedsize(uartNum), SEG_DATA_BUF_SIZE);
#endif
        }
        else if((xonoff_status[channel] == UART_XOFF) && (get_uart_buffer_usedsize(uartNum) < UART_ON_THRESHOLD)) // Send the transmit start command to peer. -go XON
        {
            uart_putc(uartNum, UART_XON);
            xonoff_status[channel] = UART_XON;
#ifdef _UART_DEBUG_
            printf(" >> SEND XON [%d / %d]\r\n", get_uart_buffer_usedsize(uartNum), SEG_DATA_BUF_SIZE);
#endif
        }
    }
#ifdef __USE_GPIO_HARDWARE_FLOWCONTROL__
    else if(flow_ctrl == flow_rts_cts) // RTS pin control
    {
        // Buffer full occurred
        if((rts_status == UART_RTS_LOW) && (get_uart_buffer_usedsize(uartNum) > UART_OFF_THRESHOLD))
        {
            set_uart_rts_pin_high(uartNum);
            rts_status = UART_RTS_HIGH;
#ifdef _UART_DEBUG_
            printf(" >> UART_RTS_HIGH [%d / %d]\r\n", get_uart_buffer_usedsize(uartNum), SEG_DATA_BUF_SIZE);
#endif
        }
        
        // Clear the buffer full event
        if((rts_status == UART_RTS_HIGH) && (get_uart_buffer_usedsize(uartNum) <= UART_OFF_THRESHOLD))
        {
            set_uart_rts_pin_low(uartNum);
            rts_status = UART_RTS_LOW;
#ifdef _UART_DEBUG_
            printf(" >> UART_RTS_LOW [%d / %d]\r\n", get_uart_buffer_usedsize(uartNum), SEG_DATA_BUF_SIZE);
#endif
        }
    }
#endif
}


int32_t uart_putc(uint8_t uartNum, uint16_t ch)
{
    struct __serial_option *serial_option = (struct __serial_option *)&(get_DevConfig_pointer()->serial_option);
    uint8_t c[1];

    uint8_t channel = get_seg_channel(uartNum);

    if(serial_option[channel].data_bits == word_len8)
    {
        c[0] = ch & 0x00FF;
    }
    else if (serial_option[channel].data_bits == word_len7)
    {
        c[0] = ch & 0x007F; // word_len7
    }

    if(uartNum == DATA0_UART_PORTNUM)
    {
        HAL_UART_Transmit(&huart1, &c[0], 1, 1000);
    }
    else if(uartNum == DATA1_UART_PORTNUM)
    {
        HAL_UART_Transmit(&huart6, &c[0], 1, 1000);
    }
#if 0
    else if(uartNum == DEBUG_UART_PORTNUM)
    {
        HAL_UART_Transmit(&huart2, &c[0], 1, 1000);
    }
#endif
    else
    {
        return RET_NOK;
    }

    return RET_OK;
}

int32_t uart_puts(uint8_t uartNum, uint8_t* buf, uint16_t bytes)
{
    if(uartNum == DATA0_UART_PORTNUM)
    {
        HAL_UART_Transmit(&huart1, (uint8_t *)buf, bytes, 1000);
    }
    else if(uartNum == DATA1_UART_PORTNUM)
    {
        HAL_UART_Transmit(&huart6, (uint8_t *)buf, bytes, 1000);
    }

#if 0
    else if(uartNum == DEBUG_UART_PORTNUM)
    {
        HAL_UART_Transmit(&huart2, (uint8_t *)&buf, bytes, 1000);
    }
#endif
    else
        return RET_NOK;

    return bytes;
}

int32_t uart_getc(uint8_t uartNum)
{
    int32_t ch;

    if(uartNum == DATA0_UART_PORTNUM)
    {
        while(IS_BUFFER_EMPTY(data0_rx));
        ch = (int32_t)BUFFER_OUT(data0_rx);
        BUFFER_OUT_MOVE(data0_rx, 1);
    }
    else
        return RET_NOK;

    return ch;
}

int32_t uart_getc_nonblk(uint8_t uartNum)
{
    int32_t ch;

    if(uartNum == DATA0_UART_PORTNUM)
    {
        if(IS_BUFFER_EMPTY(data0_rx)) return RET_NOK;
        ch = (int32_t)BUFFER_OUT(data0_rx);
        BUFFER_OUT_MOVE(data0_rx, 1);
    }
    else
        return RET_NOK;

    return ch;
}

int32_t uart_gets(uint8_t uartNum, uint8_t* buf, uint16_t bytes)
{
    uint16_t lentot = 0, len1st = 0;

    if(uartNum == DATA0_UART_PORTNUM)
    {
        lentot = bytes = MIN(BUFFER_USED_SIZE(data0_rx), bytes);
        if(IS_BUFFER_OUT_SEPARATED(data0_rx) && (len1st = BUFFER_OUT_1ST_SIZE(data0_rx)) < bytes) {
            memcpy(buf, &BUFFER_OUT(data0_rx), len1st);
            BUFFER_OUT_MOVE(data0_rx, len1st);
            bytes -= len1st;
        }
        memcpy(buf+len1st, &BUFFER_OUT(data0_rx), bytes);
        BUFFER_OUT_MOVE(data0_rx, bytes);
    }
    else
        return RET_NOK;

    return lentot;
}

void uart_rx_flush(uint8_t uartNum)
{
    if(uartNum == DATA0_UART_PORTNUM)
    {
        BUFFER_CLEAR(data0_rx);
    }
}

uint8_t get_byte_from_uart(uint8_t uartNum)
{
    uint8_t ch;

    if(uartNum == DATA0_UART_PORTNUM)
    {
        HAL_UART_Receive_IT(&huart1, &ch, 1);
    }
    else if(uartNum == DATA1_UART_PORTNUM)
    {
        HAL_UART_Receive_IT(&huart6, &ch, 1);
    }

    return ch;
}


void get_byte_from_uart_it(uint8_t uartNum)
{
    if(uartNum == DATA0_UART_PORTNUM)
    {
        HAL_UART_Receive_IT(&huart1, &uartRxByte, 1);
    }
    else if(uartNum == DATA1_UART_PORTNUM)
    {
        HAL_UART_Receive_IT(&huart6, &uartRxByte, 1);
    }
}


void put_byte_to_uart_buffer(uint8_t uartNum, uint8_t ch)
{
    if(uartNum == DATA0_UART_PORTNUM)
    {
        BUFFER_IN(data0_rx) = ch;
        BUFFER_IN_MOVE(data0_rx, 1);

    }
}


uint16_t get_uart_buffer_usedsize(uint8_t uartNum)
{
    uint16_t len = 0;

    if(uartNum == DATA0_UART_PORTNUM)
    {
        len = BUFFER_USED_SIZE(data0_rx);
    }

    return len;
}

uint16_t get_uart_buffer_freesize(uint8_t uartNum)
{
    uint16_t len = 0;

    if(uartNum == DATA0_UART_PORTNUM)
    {
        len = BUFFER_FREE_SIZE(data0_rx);
    }
    return len;
}

int8_t is_uart_buffer_empty(uint8_t uartNum)
{
    int8_t ret = 0;

    if(uartNum == DATA0_UART_PORTNUM)
    {
        ret = IS_BUFFER_EMPTY(data0_rx);
    }
    return ret;
}

int8_t is_uart_buffer_full(uint8_t uartNum)
{
    int8_t ret = 0;

    if(uartNum == DATA0_UART_PORTNUM)
    {
        ret = IS_BUFFER_FULL(data0_rx);
    }
    return ret;
}

#ifdef __USE_UART_485_422__
uint8_t get_uart_rs485_sel(uint8_t uartNum)
{
    struct __serial_option *serial_option = (struct __serial_option *)&(get_DevConfig_pointer()->serial_option);

    uint8_t channel = 0;

    // WIZ2000 series
    if(uartNum == DATA0_UART_PORTNUM) // UART0
    {
        channel = get_seg_channel(DATA0_UART_PORTNUM);
    }
    else if(uartNum == DATA1_UART_PORTNUM) // UART1
    {
        channel = get_seg_channel(DATA0_UART_PORTNUM);
    }

    return serial_option[channel].uart_interface;

#if 0
    // WIZ750SR
    if(uartNum == DATA0_UART_PORTNUM) // UART0
    {
        GPIO_Configuration(DATA0_UART_PORT, DATA0_UART_RTS_PIN, GPIO_MODE_INPUT, GPIO_NOPULL); // UART0 RTS pin: GPIO / Input
        HAL_GPIO_WritePin(DATA0_UART_PORT, DATA0_UART_RTS_PIN, GPIO_PIN_SET);
        
        if(GPIO_Input_Read(DATA0_UART_PORT, DATA0_UART_RTS_PIN) == IO_LOW)
        {
            uart_if_mode = UART_IF_RS422;
        }
        else
        {
            uart_if_mode = UART_IF_RS485;
        }
    }
    else if(uartNum == DATA1_UART_PORTNUM) // UART1
    {
        GPIO_Configuration(DATA1_UART_PORT, DATA1_UART_RTS_PIN, GPIO_MODE_INPUT, GPIO_NOPULL); // UART0 RTS pin: GPIO / Input
        HAL_GPIO_WritePin(DATA1_UART_PORT, DATA1_UART_RTS_PIN, GPIO_PIN_SET);

        if(GPIO_Input_Read(DATA1_UART_PORT, DATA1_UART_RTS_PIN) == IO_LOW)
        {
            uart_if_mode = UART_IF_RS422;
        }
        else
        {
            uart_if_mode = UART_IF_RS485;
        }
    }
#endif
}


void uart_rs485_rs422_init(uint8_t uartNum)
{
    if(uartNum == DATA0_UART_PORTNUM) // Data UART0
    {
        GPIO_Configuration(DATA0_UART_PORT, DATA0_UART_RTS_PIN, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL); // UART0 RTS pin: GPIO / Output
    }
    else if(uartNum == DATA1_UART_PORTNUM) // Data UART1
    {
        GPIO_Configuration(DATA1_UART_PORT, DATA1_UART_RTS_PIN, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL); // UART1 RTS pin: GPIO / Output
    }
}


void uart_rs485_enable(uint8_t uartNum)
{
    uint8_t channel = get_seg_channel(uartNum);

    if(uart_if_mode[channel] == UART_IF_RS485)
    {
        // RTS pin -> High
        if(uartNum == DATA0_UART_PORTNUM) // Data UART0
        {
            GPIO_Output_Set(DATA0_UART_PORT, DATA0_UART_RTS_PIN);
        }
        else if(uartNum == DATA1_UART_PORTNUM) // Data UART1
        {
            GPIO_Output_Set(DATA1_UART_PORT, DATA1_UART_RTS_PIN);
        }
        delay_ms(1);
    }
    else if(uart_if_mode[channel] == UART_IF_RS485_REVERSE)
    {
        // RTS pin -> High
        if(uartNum == DATA0_UART_PORTNUM) // Data UART0
        {
            GPIO_Output_Reset(DATA0_UART_PORT, DATA0_UART_RTS_PIN);
        }
        else if(uartNum == DATA1_UART_PORTNUM) // Data UART0
        {
            GPIO_Output_Reset(DATA1_UART_PORT, DATA1_UART_RTS_PIN);
        }
        delay_ms(1);
    }
    
    //UART_IF_RS422: None
}


void uart_rs485_disable(uint8_t uartNum)
{
    uint8_t channel = get_seg_channel(uartNum);

    if(uart_if_mode[channel] == UART_IF_RS485)
    {
        // RTS pin -> Low
        if(uartNum == DATA0_UART_PORTNUM) // UART0
        {
            GPIO_Output_Reset(DATA0_UART_PORT, DATA0_UART_RTS_PIN);
        }
        else if(uartNum == DATA1_UART_PORTNUM) // UART0
        {
            GPIO_Output_Reset(DATA1_UART_PORT, DATA1_UART_RTS_PIN);
        }
        delay_ms(1);
    }
    else if(uart_if_mode[channel] == UART_IF_RS485_REVERSE)
    {
        // RTS pin -> High
        if(uartNum == DATA0_UART_PORTNUM) // UART0
        {
            GPIO_Output_Set(DATA0_UART_PORT, DATA0_UART_RTS_PIN);
        }
        else if(uartNum == DATA1_UART_PORTNUM) // UART0
        {
            GPIO_Output_Set(DATA1_UART_PORT, DATA1_UART_RTS_PIN);
        }
        delay_ms(1);
    }
    
    //UART_IF_RS422: None
}
#endif

#ifdef __USE_GPIO_HARDWARE_FLOWCONTROL__
    
uint8_t get_uart_cts_pin(uint8_t uartNum)
{
    uint8_t cts_pin = UART_CTS_HIGH;

#ifdef _UART_DEBUG_
    static uint8_t prev_cts_pin;
#endif
    
    if(uartNum == DATA0_UART_PORTNUM) // UART0
    {
        cts_pin = GPIO_Input_Read(DATA0_UART_PORT, DATA0_UART_CTS_PIN);
    }
    /*
     * DATA1_UART does not support CTS pin
     */
    else if(uartNum == DATA1_UART_PORTNUM) // UART1
    {
        cts_pin = UART_CTS_LOW;
    }

#ifdef _UART_DEBUG_
    if(cts_pin != prev_cts_pin)
    {
        printf(" >> UART_CTS_%s\r\n", cts_pin?"HIGH":"LOW");
        prev_cts_pin = cts_pin;
    }
#endif
    
    return cts_pin;
}

void set_uart_rts_pin_high(uint8_t uartNum)
{
    if(uartNum == DATA0_UART_PORTNUM) // UART0
    {
        GPIO_Output_Set(DATA0_UART_PORT, DATA0_UART_RTS_PIN);
    }
    else if(uartNum == DATA1_UART_PORTNUM) // UART1
    {
        GPIO_Output_Set(DATA1_UART_PORT, DATA1_UART_RTS_PIN);
    }
}

void set_uart_rts_pin_low(uint8_t uartNum)
{
    if(uartNum == DATA0_UART_PORTNUM) // UART0
    {
        GPIO_Output_Reset(DATA0_UART_PORT, DATA0_UART_RTS_PIN);
    }
    else if(uartNum == DATA1_UART_PORTNUM) // UART1
    {
        GPIO_Output_Reset(DATA1_UART_PORT, DATA1_UART_RTS_PIN);
    }
}

#endif
