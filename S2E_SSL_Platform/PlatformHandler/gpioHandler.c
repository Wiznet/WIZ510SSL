#include "stm32l5xx_hal.h"

#include "common.h"
#include "s2e_ssl_board.h"

#include "seg.h"
#include "ConfigData.h"
#include "wizchipHandler.h"
#include "gpioHandler.h"
#include "wiz_debug.h"

#ifdef _GPIO_DEBUG_
    #include <stdio.h>
#endif

#ifdef __USE_USERS_GPIO__
    const uint16_t USER_IO_PIN[USER_IOn] =     {USER_IO_A_PIN, USER_IO_B_PIN, USER_IO_C_PIN, USER_IO_D_PIN};
    GPIO_TypeDef* USER_IO_PORT[USER_IOn] =     {USER_IO_A_PORT, USER_IO_B_PORT, USER_IO_C_PORT, USER_IO_D_PORT};
    uint8_t     USER_IO_ADC_CH[USER_IOn] =     {USER_IO_A_ADC_CH, USER_IO_B_ADC_CH, USER_IO_C_ADC_CH, USER_IO_D_ADC_CH};

    uint8_t        USER_IO_SEL[USER_IOn] =     {USER_IO_A, USER_IO_B, USER_IO_C, USER_IO_D};
    const char*    USER_IO_STR[USER_IOn] =     {"a", "b", "c", "d"};
    #if (DEVICE_BOARD_NAME == WIZ750SR_1xx)
        const char*    USER_IO_PIN_STR[USER_IOn] = {"PC15\0", "PC14\0", "PC13\0", "PC12\0"}; // WIZ750SR_1xx
    #else
        const char*    USER_IO_PIN_STR[USER_IOn] = {"PC28\0", "PC27\0", "PC26\0", "PC25\0",}; // W(IZ750SR
    #endif
    const char*    USER_IO_TYPE_STR[] =        {"Digital", "Analog"};
    const char*    USER_IO_DIR_STR[] =         {"Input", "Output"};
#endif


/**
  * @brief  STM32 GPIO Initialize Function
  */
 
void GPIO_Configuration(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, uint16_t GPIO_Mode, uint16_t GPIO_Pull)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    
    /* Enable the GPIO Clock */
    if (GPIOx == GPIOA)
    {
    	__HAL_RCC_GPIOA_CLK_ENABLE();
    }
    else if (GPIOx == GPIOB)
    {
    	__HAL_RCC_GPIOB_CLK_ENABLE();
    }
    else if (GPIOx == GPIOC)
    {
    	__HAL_RCC_GPIOC_CLK_ENABLE();
    }
    else if (GPIOx == GPIOD)
    {
    	__HAL_RCC_GPIOD_CLK_ENABLE();
    }    
    else if (GPIOx == GPIOE)
    {
    	__HAL_RCC_GPIOE_CLK_ENABLE();
    }
    /*
    else if (GPIOx == GPIOF)
    {
    	__HAL_RCC_GPIOF_CLK_ENABLE();
    }
    else if (GPIOx == GPIOG)
    {
    	__HAL_RCC_GPIOG_CLK_ENABLE();
    }
    */
    else
    {
        if (GPIOx == GPIOH)
        {
        	__HAL_RCC_GPIOH_CLK_ENABLE();
        }
    }

	/*Configure GPIO pin Output Level */
    if(GPIO_Mode == GPIO_MODE_OUTPUT_PP)
    {
    	HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_RESET);
    }

	/*Configure GPIO pin */
	GPIO_InitStruct.Pin = GPIO_Pin;
	GPIO_InitStruct.Mode = GPIO_Mode;
	GPIO_InitStruct.Pull = GPIO_Pull;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

	HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}


void GPIO_Output_Set(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
    HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_SET);
}


void GPIO_Output_Reset(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
    HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_RESET);
}


void GPIO_Output_Toggle(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
    HAL_GPIO_TogglePin(GPIOx, GPIO_Pin);
}


uint8_t GPIO_Output_Read(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
    return HAL_GPIO_ReadPin(GPIOx, GPIO_Pin);
}


uint8_t GPIO_Input_Read(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
    return HAL_GPIO_ReadPin(GPIOx, GPIO_Pin);
}

/**
  * @brief  Device I/O Initialize Function
  */
void Device_IO_Init(void)
{
    struct __serial_option *serial_option = (struct __serial_option *)&(get_DevConfig_pointer()->serial_option);
    
    // Status I/O - Shared pin init: Connection status pins or DTR/DSR pins
    //init_connection_status_io();


    // Set the DTR pin to high when the DTR signal enabled (== PHY link status disabled)
    if(serial_option->dtr_en == 1) set_flowcontrol_dtr_pin(ON);
    
    /* GPIOs Initialization */
    // Expansion GPIOs (4-pins, A to D)
    // Available to the ANALOG input pins or DIGITAL input/output pins
#ifdef __USE_USERS_GPIO__
    if(get_user_io_enabled(USER_IO_A)) init_user_io(USER_IO_A);
    if(get_user_io_enabled(USER_IO_B)) init_user_io(USER_IO_B);
    if(get_user_io_enabled(USER_IO_C)) init_user_io(USER_IO_C);
    if(get_user_io_enabled(USER_IO_D)) init_user_io(USER_IO_D);
#endif
    
    // ## debugging: io functions test
    /*
    get_user_io_val(USER_IO_A, &val);
    printf("USER_IO_A: [val] %d, [enable] %s, [type] %s, [direction] %s\r\n", val, get_user_io_enabled(USER_IO_A)?"enabled":"disabled", get_user_io_type(USER_IO_A)?"analog":"digital", get_user_io_direction(USER_IO_A)?"output":"input");
    get_user_io_val(USER_IO_B, &val);
    printf("USER_IO_B: [val] %d, [enable] %s, [type] %s, [direction] %s\r\n", val, get_user_io_enabled(USER_IO_B)?"enabled":"disabled", get_user_io_type(USER_IO_B)?"analog":"digital", get_user_io_direction(USER_IO_B)?"output":"input");
    get_user_io_val(USER_IO_C, &val);
    printf("USER_IO_C: [val] %d, [enable] %s, [type] %s, [direction] %s\r\n", val, get_user_io_enabled(USER_IO_C)?"enabled":"disabled", get_user_io_type(USER_IO_C)?"analog":"digital", get_user_io_direction(USER_IO_C)?"output":"input");
    get_user_io_val(USER_IO_D, &val);
    printf("USER_IO_D: [val] %d, [enable] %s, [type] %s, [direction] %s\r\n", val, get_user_io_enabled(USER_IO_D)?"enabled":"disabled", get_user_io_type(USER_IO_D)?"analog":"digital", get_user_io_direction(USER_IO_D)?"output":"input");
    */
}

#ifdef __USE_USERS_GPIO__

void init_user_io(uint8_t io_sel)
{
    /*
    struct __user_io_info *user_io_info = (struct __user_io_info *)&(get_DevConfig_pointer()->user_io_info);
    uint8_t idx = 0;
    
    uint8_t io_status = 0;
    
    if((user_io_info->user_io_enable & io_sel) == io_sel)
    {
        idx = get_user_io_bitorder(io_sel);
        
        if((user_io_info->user_io_type & io_sel) == io_sel) // IO_ANALOG_IN == 1 
        {
            if(USER_IO_ADC_CH[idx] != USER_IO_NO_ADC)
            {
                // Analog Input
                //ADC_Init();
                //GPIO_Configuration(USER_IO_PORT[idx], USER_IO_PIN[idx], GPIO_Mode_IN, USER_IO_AIN_PAD_AF);
            }
            //else // IO init falied
        }
        else
        {
            // Digital Input / Output
            if((user_io_info->user_io_direction & io_sel) == io_sel) // IO_OUTPUT == 1
            {
                //GPIO_Configuration(USER_IO_PORT[idx], USER_IO_PIN[idx], GPIO_Mode_OUT, USER_IO_DEFAULT_PAD_AF);
                io_status = ((user_io_info->user_io_status & (uint16_t)io_sel) == (uint16_t)io_sel)?IO_HIGH:IO_LOW;
                
                if(io_status == IO_HIGH) {
                    GPIO_SetBits(USER_IO_PORT[idx], USER_IO_PIN[idx]);
                } else {
                    GPIO_ResetBits(USER_IO_PORT[idx], USER_IO_PIN[idx]);
                }
            }
            else
            {
                //GPIO_Configuration(USER_IO_PORT[idx], USER_IO_PIN[idx], GPIO_Mode_IN, USER_IO_DEFAULT_PAD_AF);
                USER_IO_PORT[idx]->OUTENCLR = USER_IO_PIN[idx];
            }
        }
    }
    */
}


uint8_t set_user_io_enable(uint8_t io_sel, uint8_t enable)
{
    struct __user_io_info *user_io_info = (struct __user_io_info *)&(get_DevConfig_pointer()->user_io_info);
    uint8_t ret = 1;
    
    
    if(enable == IO_ENABLE)
    {
        // IO enables
        user_io_info->user_io_enable |= io_sel;
    }
    else if(enable == IO_DISABLE)
    {
        // IO disables
        user_io_info->user_io_enable &= ~(io_sel);
    }
    else
        ret = 0;
    
    return ret;
}


uint8_t set_user_io_type(uint8_t io_sel, uint8_t type)
{
    struct __user_io_info *user_io_info = (struct __user_io_info *)&(get_DevConfig_pointer()->user_io_info);
    uint8_t ret = 1;
    uint8_t idx;
    
    if(type == IO_ANALOG_IN)
    {
        idx = get_user_io_bitorder(io_sel);
        
        if(USER_IO_ADC_CH[idx] != USER_IO_NO_ADC)
        {
            set_user_io_direction(io_sel, IO_INPUT); // Analog: input only
            user_io_info->user_io_type |= io_sel;
            init_user_io(io_sel); // IO reinitialize
        }
        else
        {
            // IO setting failed
            ret = 0;
        }
    }
    else if(type == IO_DIGITAL)
    {
        user_io_info->user_io_type &= ~(io_sel);
        init_user_io(io_sel); // IO reinitialize
    }
    else
        ret = 0;
    
    init_user_io(io_sel);
    
    return ret;
}


uint8_t set_user_io_direction(uint8_t io_sel, uint8_t dir)
{
    struct __user_io_info *user_io_info = (struct __user_io_info *)&(get_DevConfig_pointer()->user_io_info);
    uint8_t ret = 1;
    
    
    if(dir == IO_OUTPUT)
    {
        user_io_info->user_io_direction |= io_sel;
        init_user_io(io_sel); // IO reinitialize
    }
    else if(dir == IO_INPUT)
    {
        user_io_info->user_io_direction &= ~(io_sel);
        init_user_io(io_sel); // IO reinitialize
    }
    else
        ret = 0;
    
    init_user_io(io_sel);
    
    return ret;
}


uint8_t get_user_io_enabled(uint8_t io_sel)
{
    struct __user_io_info *user_io_info = (struct __user_io_info *)&(get_DevConfig_pointer()->user_io_info);
    uint8_t ret;
    
    if((user_io_info->user_io_enable & io_sel) == io_sel)
    {
        ret = IO_ENABLE;
    }
    else
    {
        ret = IO_DISABLE;
    }
    
    return ret;
}


uint8_t get_user_io_type(uint8_t io_sel)
{
    struct __user_io_info *user_io_info = (struct __user_io_info *)&(get_DevConfig_pointer()->user_io_info);
    uint8_t ret;
    
    if((user_io_info->user_io_type & io_sel) == io_sel)
    {
        ret = IO_ANALOG_IN;
    }
    else
    {
        ret = IO_DIGITAL;
    }
    
    return ret;
}


uint8_t get_user_io_direction(uint8_t io_sel)
{
    struct __user_io_info *user_io_info = (struct __user_io_info *)&(get_DevConfig_pointer()->user_io_info);
    uint8_t ret;
    
    if((user_io_info->user_io_direction & io_sel) == io_sel)
    {
        ret = IO_OUTPUT;
    }
    else
    {
        ret = IO_INPUT;
    }
    
    return ret;
}


// Analog input: 	Returns ADC value (12-bit resolution)
// Digital in/out: 	Returns I/O status to match the bit ordering
uint8_t get_user_io_val(uint16_t io_sel, uint16_t * val)
{
    struct __user_io_info *user_io_info = (struct __user_io_info *)&(get_DevConfig_pointer()->user_io_info);
    
    uint8_t ret = 0; // I/O Read failed
    uint8_t idx = 0;
    uint8_t status = 0;
    
    *val = 0;
    
    if((user_io_info->user_io_enable & io_sel) == io_sel) // Enable
    {
        idx = get_user_io_bitorder(io_sel);
        
        if((user_io_info->user_io_type & io_sel) == io_sel) // IO_ANALOG == 1
        {
            if(USER_IO_ADC_CH[idx] != USER_IO_NO_ADC)
            {
                // Analog Input: value
                *val = read_ADC((ADC_TypeDef*)USER_IO_ADC_CH[idx]);
            }
            else
            {
                // IO value get failed
                *val = 0;
            }
        }
        else // IO_DIGITAL == 0
        {
            // Digital Input / Output
            if((user_io_info->user_io_direction & io_sel) == io_sel) // IO_OUTPUT == 1
            {
                // Digital Output: status
                status = (uint16_t)GPIO_ReadOutputDataBit(USER_IO_PORT[idx], USER_IO_PIN[idx]);
            }
            else // IO_INPUT == 0
            {
                // Digital Input: status
                status = (uint16_t)GPIO_ReadInputDataBit(USER_IO_PORT[idx], USER_IO_PIN[idx]);
            }
            
            //*val |= (status << i);
            *val = status;
        }
        
        ret = 1; // I/O Read success
    }
    
    return ret;
}


uint8_t set_user_io_val(uint16_t io_sel, uint16_t * val)
{
    struct __user_io_info *user_io_info = (struct __user_io_info *)&(get_DevConfig_pointer()->user_io_info);
    uint8_t ret = 0; // I/O Read failed
    uint8_t idx = 0;
    
    if((user_io_info->user_io_enable & io_sel) == io_sel) // Enable
    {
        // Digital output only (type == 0 && direction == 1)
        if(((user_io_info->user_io_type & io_sel) == IO_DIGITAL) && ((user_io_info->user_io_direction & io_sel) == io_sel))
        {
            idx = get_user_io_bitorder(io_sel);
            
            if(*val == 0)
            {
                user_io_info->user_io_status &= ~(io_sel);
                GPIO_ResetBits(USER_IO_PORT[idx], USER_IO_PIN[idx]);
            }
            else if(*val == 1)
            {
                user_io_info->user_io_status |= io_sel;
                GPIO_SetBits(USER_IO_PORT[idx], USER_IO_PIN[idx]);
            }
            
            ret = 1;
        }
    }
    
    return ret;
}


uint8_t get_user_io_bitorder(uint16_t io_sel)
{
    uint8_t i;
    uint8_t ret = 0;
    
    for(i = 0; i < USER_IOn; i++)
    {
        if((io_sel >> i) == 1)
        {
            ret = i;
            break;
        }
    }
        
    return ret;
}

// Read ADC val: 12-bit ADC resolution
uint16_t read_ADC(ADC_TypeDef* ch)
{
    ADC_ChannelSelect(ch);				///< Select ADC channel to CH0
    ADC_Start();						///< Start ADC
    while(ADC_IsEOC());					///< Wait until End of Conversion
    
    return ((uint16_t)ADC_ReadData());	///< read ADC Data
}

#endif

void init_connection_status_io(void)
{
    /* ////////////////////////////////////////////////////////////////////////////////
    struct __serial_option *serial_option = (struct __serial_option *)&(get_DevConfig_pointer()->serial_option);

    if(serial->dtr_en == 0)
        init_phylink_status_pin();
    else
        init_flowcontrol_dtr_pin();

    if(serial->dsr_en == 0)
        init_tcpconnection_status_pin();
    else
        init_flowcontrol_dsr_pin();
    */
}

// This function is intended only for output connection status pins; PHYlink, TCPconnection
void set_connection_status_io(uint16_t pin, uint8_t channel, uint8_t set)
{
    struct __serial_option *serial_option = (struct __serial_option *)&(get_DevConfig_pointer()->serial_option);

    if(channel >= DEVICE_UART_CNT) return;

    if(pin == STATUS_PHYLINK_PIN)
    {
        PRT_INFO("pin = PHY, set = %d\r\n", set);
        if(set == ON)
        {
            GPIO_Output_Reset(STATUS_PHYLINK_PORT, STATUS_PHYLINK_PIN);
            //LED_On(LED_0);
        }
        else // OFF
        {
            GPIO_Output_Set(STATUS_PHYLINK_PORT, STATUS_PHYLINK_PIN);
            //LED_Off(LED_0);
        }
    }
    else if(pin == STATUS_TCPCONNECT_PIN)
    {
        PRT_INFO("pin = TCP, set = %d\r\n", set);
        if(set == ON)
        {
            GPIO_Output_Reset(STATUS_TCPCONNECT_PORT, STATUS_TCPCONNECT_PIN);
            //LED_On(LED_1);
        }
        else // OFF
        {
            
            GPIO_Output_Set(STATUS_TCPCONNECT_PORT, STATUS_TCPCONNECT_PIN);
            //LED_Off(LED_1);
        }
    }
    
}

uint8_t get_connection_status_io(uint16_t pin, uint8_t channel)
{
    uint8_t status = IO_LOW;

    if(pin == STATUS_PHYLINK_PIN)
    {
        if(get_phylink() == PHY_LINK_ON)
            status = IO_HIGH;
    }
    else if(pin == STATUS_TCPCONNECT_PIN)
    {
        if(get_device_status(channel) == ST_CONNECT)
            status = IO_HIGH;
    }

#if 0
    struct __serial_option *serial_option = (struct __serial_option *)&(get_DevConfig_pointer()->serial_option);
    uint8_t status = IO_LOW;
    
    if(pin == STATUS_PHYLINK_PIN)
    {
            if(serial_option->dtr_en == 0)
                status = GPIO_Input_Read(STATUS_PHYLINK_PORT, STATUS_PHYLINK_PIN);
            else
                status = GPIO_Input_Read(DTR_PORT, DTR_PIN);
    }
    else if(pin == STATUS_TCPCONNECT_PIN)
    {
            if(serial_option->dsr_en == 0)
                status = GPIO_Input_Read(STATUS_TCPCONNECT_PORT, STATUS_TCPCONNECT_PIN);
            else 
                status = GPIO_Input_Read(DSR_PORT, DSR_PIN);
    }
#endif
    
    return status;
}

// PHY link status pin
void init_phylink_status_pin(void)
{
    GPIO_Configuration(STATUS_PHYLINK_PORT, STATUS_PHYLINK_PIN, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL);
    
    // Pin initial state; High
    GPIO_Output_Set(STATUS_PHYLINK_PORT, STATUS_PHYLINK_PIN);
}





// DTR pin
// output (shared pin with PHY link status)
void init_flowcontrol_dtr_pin(void)
{
    /* ////////////////////////////////////////////////////////////////////////////////
     *  WIZ2000 does not support DTR/DSR signals
     * ////////////////////////////////////////////////////////////////////////////////
     *
    GPIO_Configuration(DTR_PORT, DTR_PIN, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL);

    // Pin initial state; Low
    GPIO_Output_Reset(DTR_PORT, DTR_PIN);
     */
}

void set_flowcontrol_dtr_pin(uint8_t set)
{
    if(set == ON)
        GPIO_Output_Set(DTR_PORT, DTR_PIN);
    else
        GPIO_Output_Reset(DTR_PORT, DTR_PIN);
}


// DSR pin
// input, active high (shared pin with TCP connection status)
void init_flowcontrol_dsr_pin(void)
{
    /* ////////////////////////////////////////////////////////////////////////////////
     *  WIZ2000 does not support DTR/DSR signals
     * ////////////////////////////////////////////////////////////////////////////////
     *
    GPIO_Configuration(DSR_PORT, DSR_PIN, GPIO_MODE_INPUT, GPIO_NOPULL);
     */
}


uint8_t get_flowcontrol_dsr_pin(void)
{
    return GPIO_Input_Read(DSR_PORT, DSR_PIN);
}


// Check the PHY link status
uint8_t check_phylink_status(void)
{
    static uint8_t prev_link_status;
    uint8_t link_status;

    link_status = get_phylink();

    //PRT_INFO("link_status = %d\r\n", link_status);
    
    if(prev_link_status != link_status)
    {
        if(link_status == PHY_LINK_ON)
            set_connection_status_io(STATUS_PHYLINK_PIN, 0, ON); 	// PHY Link up
        else
            set_connection_status_io(STATUS_PHYLINK_PIN, 0, OFF); 	// PHY Link down
        
        prev_link_status = link_status;
    }
    return link_status;
}

// This function have to call every 1 millisecond by Timer IRQ handler routine.
void gpio_handler_timer_msec(void)
{
    // PHY link check
    if(++phylink_check_time_msec >= PHYLINK_CHECK_CYCLE_MSEC)
    {
        phylink_check_time_msec = 0;
        //check_phylink_status();
        
        flag_check_phylink = 1;
    }
}

uint8_t factory_flag;
void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
    factory_flag = 1;
}

