#include <sys/time.h>
#include "stm32l5xx_hal.h"

#include "common.h"
#include "s2e_ssl_board.h"

#include "timerHandler.h"
#include "deviceHandler.h"
#include "gpioHandler.h"
//#include "mqtt_interface.h"

#include "seg.h"
#include "segcp.h"

#include "dhcp.h"
#include "dns.h"

void TIM2_Configuration(void);

volatile uint32_t delaytime_msec = 0;

static volatile uint16_t msec_cnt = 0;
static volatile uint8_t  sec_cnt = 0;
static volatile uint8_t  min_cnt = 0;
static volatile uint8_t  hour_cnt = 0;
static volatile uint16_t day_cnt = 0;

static volatile time_t currenttime_sec = 0;
static volatile time_t devtime_sec = 0;
static volatile time_t devtime_msec = 0;

extern uint8_t factory_flag;
static uint32_t factory_time;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;

extern uint8_t flag_s2e_application_running;

void Timer_Configuration(void)
{
    TIM2_Configuration(); // Timer 2

    /* Starts the TIM Base generation in interrupt mode */
    HAL_TIM_Base_Start_IT(&htim2);
}


void TIM1_Configuration( uint16_t usTim1Timerout50us )
{
    /*
    TIM_MasterConfigTypeDef sMasterConfig;

    htim3.Instance = TIM1;
    htim3.Init.Prescaler = ( HAL_RCC_GetHCLKFreq() / 100000000) - 1; // 0.01us
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = ;

    if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
    {
        _Error_Handler(__FILE__, __LINE__);
    }

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
    {
        _Error_Handler(__FILE__, __LINE__);
    }
    */
}

/* TIM2 init function */
void TIM2_Configuration(void)
{
    /* Initialize TIMx peripheral as follow:
    + Period = [(TIM11CLK/1000) - 1]. to have a (1/1000) s time base.
    + Prescaler = (uwTimclock/1000000 - 1) to have a 1MHz counter clock.
    + ClockDivision = 0
    + Counter direction = Up
    */

    TIM_ClockConfigTypeDef sClockSourceConfig;
    TIM_MasterConfigTypeDef sMasterConfig;

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = (SystemCoreClock / 1000000) - 1;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 1000 - 1;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
    {
    _Error_Handler(__FILE__, __LINE__);
    }

    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
    {
    _Error_Handler(__FILE__, __LINE__);
    }

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
    {
    _Error_Handler(__FILE__, __LINE__);
    }
}


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM2)
    {
        delaytime_msec++;
        msec_cnt++; // millisecond counter

        devtime_msec++;

        seg_timer_msec();		        // [msec] time counter for SEG (S2E)
        segcp_timer_msec();		        // [msec] time counter for SEGCP (Config)
        //device_timer_msec();	        // [msec] time counter for DeviceHandler (fw update)
        MilliTimer_Handler();           // [msec] time counter for MQTT client

//        if(flag_s2e_application_running)
        {
            gpio_handler_timer_msec();
        }

        /* Second Process */
        if(msec_cnt >= 1000 - 1)
        {
            msec_cnt = 0;
            sec_cnt++;                  // second counter

            seg_timer_sec();            // [sec] time counter for SEG (S2E)

//            wizdevice_gcp_timer_sec();  // [sec] time counter for WIZnet cloud send status

            DHCP_time_handler();	    // [sec] time counter for DHCP timeout
            DNS_time_handler();		    // [sec] time counter for DNS timeout

            devtime_sec++;              // device time counter,
            currenttime_sec++;          // Can be updated this counter value by time protocol like NTP.
        }

        /* Minute Process */
        if(sec_cnt >= 60)
        {
            sec_cnt = 0;
            min_cnt++;                  // minute counter
        }

        /* Hour Process */
        if(min_cnt >= 60)
        {
            min_cnt = 0;
            hour_cnt++;                 // hour counter
        }

        /* Day Process */
        if(hour_cnt >= 24)
        {
            hour_cnt = 0;
            day_cnt++;                  // day counter
        }

#ifdef __USE_HW_FACTORY_RESET__
    	/* Factory Reset Process */
    	if(factory_flag) {
    		factory_time++;
            if (GPIO_Input_Read(FAC_RSTn_PORT, FAC_RSTn_PIN))
            {
                factory_flag = 0;
                factory_time = 0;
            }
            else if (factory_time >= FACTORY_RESET_TIME_MS) 
            {
    			/* Factory Reset */
    			device_set_factory_default();
    			NVIC_SystemReset();
    		}
    	}
#endif        
    }
}

void delay_ms(uint32_t ms)
{
    uint32_t wakeuptime_msec = delaytime_msec + ms;
    while(wakeuptime_msec > delaytime_msec){}
}

time_t getDevtime(void)
{
    return devtime_sec;
}

void setDevtime(time_t timeval_sec)
{
    devtime_sec = timeval_sec;
}

time_t millis(void)
{
    return devtime_msec;
}

time_t getNow(void)
{
    return currenttime_sec;
}

void setNow(time_t timeval_sec)
{
    currenttime_sec = timeval_sec;
}

uint32_t getDeviceUptime_day(void)
{
    return day_cnt;
}

uint8_t getDeviceUptime_hour(void)
{
    return hour_cnt;
}

uint8_t getDeviceUptime_min(void)
{
    return min_cnt;
}

uint8_t getDeviceUptime_sec(void)
{
    return sec_cnt;
}

uint16_t getDeviceUptime_msec(void)
{
    return msec_cnt;
}
