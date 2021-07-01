#include "common.h"
#include "s2e_ssl_board.h"

#include "wizchip_conf.h"

#include "timerHandler.h"
#include "gpioHandler.h"
#include "uartHandler.h"

#ifdef __USE_EXT_EEPROM__
    #include "eepromHandler.h"
#endif

volatile uint16_t phylink_check_time_msec = 0;
uint8_t flag_check_phylink = 0;
uint8_t flag_hw_trig_enable = 0;

// LEDs on board
GPIO_TypeDef* GPIO_PORT[LEDn] = {LED_0_PORT, LED_1_PORT, LED_2_PORT};
const uint16_t GPIO_PIN[LEDn] = {LED_0_PIN, LED_1_PIN, LED_2_PIN};
uint8_t GPIO_INIT[LEDn] = {DISABLE, DISABLE, DISABLE};

// Serial interface mode selector 0/1
void init_serial_mode_select_pin(void);
uint8_t get_serial_mode_select_pin(uint8_t sel);

/* WIZ2000 Board Initialization */
void s2e_ssl_Board_Init(void)
{
#ifdef __USE_SERIAL_FLASH__
    /* On-board Serial Flash Initialize */
    SFlash_Init();
#endif

#ifdef __USE_HW_FACTORY_RESET__
    // Factory reset pin initialize
    init_factory_reset_pin();
#endif

#ifdef __USE_HW_APPBOOT_ENTRY__
    // AppBoot entry pin initialize
    init_appboot_entry_pin();
#endif

#ifdef __USE_UART_IF_SELECTOR__
    // UART interface selector pin initialize
    init_uart_if_sel_pin(); // UART interface selector: RS-232 / RS-422 / RS-485
#endif

    /* GPIOs Initialize */
    Device_IO_Init();

    /* HW_TRIG input pin - Check this Pin only once at boot (switch) */
    //init_hw_trig_pin();    
    //flag_hw_trig_enable = (get_hw_trig_pin()?0:1);

#ifdef __USE_EXT_EEPROM__
    init_eeprom();
#endif
    

    // STATUS #1 : PHY link status (LED_0)
    // STATUS #2 : TCP connection status (LED_1)
    LED_Init(LED_0);
    LED_Init(LED_1);
    //LED_Init(LED_2);
}

uint8_t get_phylink(void)
{
    return wizphy_getphylink();
}

// Hardware mode switch pin, active low
void init_hw_trig_pin(void)
{
    GPIO_Configuration(HW_TRIG_PORT, HW_TRIG_PIN, GPIO_MODE_INPUT, GPIO_NOPULL);
    //GPIO_Output_Set(HW_TRIG_PORT, HW_TRIG_PIN);
}


uint8_t get_hw_trig_pin(void)
{
    // HW_TRIG input; Active low
    uint8_t hw_trig, i;
    for(i = 0; i < 5; i++)
    {
        hw_trig = GPIO_Input_Read(HW_TRIG_PORT, HW_TRIG_PIN);
        if(hw_trig != 0) return 1; // High
        delay_ms(5);
    }
    return 0; // Low
}


void init_uart_if_sel_pin(void)
{
    init_serial_mode_select_pin();

    // for WIZ750SR series
    //GPIO_Configuration(UART_IF_SEL_PORT, UART_IF_SEL_PIN, GPIO_Mode_IN);
}


// Get serial mode select pin status
//           ----------------------------
//           | RS-232 | RS-422 | RS-485 |
// --------------------------------------
// MODE_SEL0 |    0   |    1   |    1   |
// MODE_SEL1 |    0   |    0   |    1   |
// --------------------------------------

uint8_t get_uart_if_sel_pin(uint32_t time_ms)
{
// Status of UART interface selector pin input; [1] RS-232, [2] RS-422 [3] RS-485 mode
#ifdef __USE_UART_IF_SELECTOR__

    uint8_t ret = UART_IF_DEFAULT;
    uint8_t mode_sel_0 = IO_HIGH;
    uint8_t mode_sel_1 = IO_HIGH;

    uint32_t tick = HAL_GetTick();

    while((HAL_GetTick() - tick) < time_ms)
    {
        mode_sel_0 = get_serial_mode_select_pin(0); // MOD_SEL0_PIN state
        mode_sel_1 = get_serial_mode_select_pin(1); // MOD_SEL1_PIN state
    }

    if((mode_sel_0 == IO_HIGH) && (mode_sel_1 == IO_HIGH))
    {
        ret = UART_IF_RS485;
    }
    else if((mode_sel_0 == IO_HIGH) && (mode_sel_1 == IO_LOW))
    {
        ret = UART_IF_RS422;
    }
    else if((mode_sel_0 == IO_LOW) && (mode_sel_1 == IO_LOW))
    {
        ret = UART_IF_RS232;
    }
    else
    {
        // todo: Add another feature
        // (mode_sel_0 == IO_LOW) && (mode_sel_1 == IO_HIGH)
        ret = UART_IF_RS422;
    }
    return ret;

    // for WIZ750SR series
    // Status of UART interface selector pin input; [0] RS-232/TTL mode, [1] RS-422/485 mode
    //return GPIO_ReadInputDataBit(UART_IF_SEL_PORT, UART_IF_SEL_PIN);
#else
    return UART_IF_DEFAULT;
#endif
}

#ifdef __USE_BOOT_ENTRY__
// Application boot mode entry pin, active low
void init_boot_entry_pin(void)
{
    GPIO_Configuration(BOOT_ENTRY_PORT, BOOT_ENTRY_PIN, GPIO_Mode_IN);
    GPIO_SetBits(BOOT_ENTRY_PORT, BOOT_ENTRY_PIN); // set to high
}

uint8_t get_boot_entry_pin(void)
{
    // Get the status of application boot mode entry pin, active low
    return GPIO_ReadInputDataBit(BOOT_ENTRY_PORT, BOOT_ENTRY_PIN);
}
#endif


// TCP connection status pin
void init_tcpconnection_status_pin(void)
{
    GPIO_Configuration(STATUS_TCPCONNECT_PORT, STATUS_TCPCONNECT_PIN, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL);

    // Pin initial state; High
    GPIO_Output_Set(STATUS_TCPCONNECT_PORT, STATUS_TCPCONNECT_PIN);
}

#if 0	//hoon
// Init Serial interface select pins
// input pins
void init_serial_mode_select_pin(void)
{
    GPIO_Configuration(MODE_SEL_PORT, (MODE_SEL0_PIN | MODE_SEL1_PIN), GPIO_MODE_INPUT, GPIO_NOPULL);
}


uint8_t get_serial_mode_select_pin(uint8_t sel)
{
    uint8_t status = IO_LOW;

    if(sel == 0) // Serial mode select pin 0
    {
        status = GPIO_Input_Read(MODE_SEL_PORT, MODE_SEL0_PIN)? IO_HIGH : IO_LOW;
    }
    else if(sel == 1) // Serial mode select pin 1
    {
        status = GPIO_Input_Read(MODE_SEL_PORT, MODE_SEL1_PIN)? IO_HIGH : IO_LOW;
    }

    return status;
}
#endif

#ifdef __USE_HW_FACTORY_RESET__
void init_factory_reset_pin(void)
{
    //GPIO_Configuration(FAC_RSTn_PORT, FAC_RSTn_PIN, GPIO_MODE_INPUT, GPIO_PULLUP);
    NVIC_EnableIRQ(FAC_RESET_EXTI_IRQn);
}

uint8_t get_factory_reset_pin(void)
{
    return GPIO_Input_Read(FAC_RSTn_PORT, FAC_RSTn_PIN);
}

// Run in MAIN routine
uint8_t process_check_factory_reset(uint32_t time_ms)
{
    uint8_t status;
    uint32_t tick = HAL_GetTick();

    static uint8_t  is_button_pressed;
    static uint32_t update_tick;
    static uint32_t signal_holding_start_time_ms;

    if(tick >= update_tick)
    {
        status = get_factory_reset_pin();
        update_tick = tick + FACTORY_CHECK_PERIOD_MS;

        if(status == IO_LOW)
        {
            if(!is_button_pressed)
            {
                is_button_pressed = TRUE;
                signal_holding_start_time_ms = tick;
            }

            if((tick - signal_holding_start_time_ms) > time_ms) // Factory reset
            {
                is_button_pressed = FALSE;
                signal_holding_start_time_ms = 0;

                return TRUE;
            }
        }
        else
        {
            is_button_pressed = FALSE;
            signal_holding_start_time_ms = 0;
        }
    }

    return FALSE;
}
#endif


#ifdef __USE_HW_APPBOOT_ENTRY__
void init_appboot_entry_pin(void)
{
    GPIO_Configuration(APPBOOT_ENTRYn_PORT, APPBOOT_ENTRYn_PIN, GPIO_MODE_INPUT, GPIO_PULLUP);
}

// AppBoot mode only
uint8_t get_appboot_entry_pin(uint32_t time_ms) // APPBOOT_ENTRY_TIME_MS
{
    uint8_t status = IO_HIGH; // AppBoot entry: Active low
    uint32_t tick = HAL_GetTick();
    while((HAL_GetTick() - tick) < time_ms)
    {
        status = GPIO_Input_Read(APPBOOT_ENTRYn_PORT, APPBOOT_ENTRYn_PIN);
        if(status == IO_HIGH)
            return status;
    }
    return IO_LOW;
    //return GPIO_Input_Read(APPBOOT_PORT, APPBOOT_PIN);
}
#endif

#ifdef __USE_SUPERVISORY_IC__
void init_supervisory_ic_clockout_pin(void)
{
    GPIO_Configuration(STATUS_TCPCONNECT_PORT, STATUS_TCPCONNECT_PIN, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL);
}

void Supervisory_IC_Init(void)
{
    init_supervisory_ic_clockout_pin();


}


#endif

/**
  * @brief  Configures LED GPIO.
  * @param  Led: Specifies the Led to be configured.
  *   This parameter can be one of following parameters:
  *     @arg LED1
  *     @arg LED2
  * @retval None
  */
void LED_Init(Led_TypeDef Led)
{
    GPIO_Configuration(GPIO_PORT[Led], GPIO_PIN[Led], GPIO_MODE_OUTPUT_PP, GPIO_NOPULL);
    HAL_GPIO_WritePin(GPIO_PORT[Led], GPIO_PIN[Led], GPIO_PIN_SET); // LED Off
    
    GPIO_INIT[Led] = ENABLE; // init state
}

/**
  * @brief  Turns selected LED On.
  * @param  Led: Specifies the Led to be set on.
  *   This parameter can be one of following parameters:
  *     @arg LED1
  *     @arg LED2
  * @retval None
  */
void LED_On(Led_TypeDef Led)
{
  if(GPIO_INIT[Led] != ENABLE) return;
  HAL_GPIO_WritePin(GPIO_PORT[Led], GPIO_PIN[Led], GPIO_PIN_RESET);
}

/**
  * @brief  Turns selected LED Off.
  * @param  Led: Specifies the Led to be set off.
  *   This parameter can be one of following parameters:
  *     @arg LED1
  *     @arg LED2
  * @retval None
  */
void LED_Off(Led_TypeDef Led)
{
  if(GPIO_INIT[Led] != ENABLE) return;
  HAL_GPIO_WritePin(GPIO_PORT[Led], GPIO_PIN[Led], GPIO_PIN_SET);
}

/**
  * @brief  Toggles the selected LED.
  * @param  Led: Specifies the Led to be toggled.
  *   This parameter can be one of following parameters:
  *     @arg LED1
  *     @arg LED2
  * @retval None
  */
void LED_Toggle(Led_TypeDef Led)
{
    if(GPIO_INIT[Led] != ENABLE) return;
    GPIO_PORT[Led]->ODR ^= GPIO_PIN[Led];
}

uint8_t get_LED_Status(Led_TypeDef Led)
{
    if(GPIO_INIT[Led] != ENABLE) return 0;
    return HAL_GPIO_ReadPin(GPIO_PORT[Led], GPIO_PIN[Led]);
}
