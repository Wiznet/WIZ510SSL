/*
*
@file   s2e_ssl_board.h
@brief
*/

#ifndef __S2E_SSL_BOARD_H_
#define __S2E_SSL_BOARD_H_

#include "stm32l5xx_hal.h"
#include "common.h"
#include "main.h"

////////////////////////////////
// Product Configurations     //
////////////////////////////////

/* Target Board Selector */
#define DEVICE_BOARD_NAME   S2E_SSL

// Get serial mode select pin status
//           ----------------------------
//           | RS-485 | RS-422 | RS-232 |
// --------------------------------------
// MODE_SEL0 |    1   |    1   |    0   |
// MODE_SEL1 |    1   |    0   |    0   |
// --------------------------------------

#ifdef DEVICE_BOARD_NAME
    #if (DEVICE_BOARD_NAME == S2E_SSL)
        #define __USE_DHCP_INFINITE_LOOP__          // When this option is enabled, if DHCP IP allocation failed, process_dhcp() function will try to DHCP steps again.
        #define __USE_DNS_INFINITE_LOOP__           // When this option is enabled, if DNS query failed, process_dns() function will try to DNS steps again.
        //#define __USE_HW_APPBOOT_ENTRY__            // Use App-boot entry pin
        #define __USE_HW_FACTORY_RESET__            // Use Factory reset pin
        //#define __USE_UART_IF_SELECTOR__            // Use Serial interface port selector pin
        //#define __USE_SUPERVISORY_IC__              // Use MCU supervisory reset IC for voltage detect and reset automatically
        #define __USE_SAFE_SAVE__                   // When this option is enabled, data verify is additionally performed in the flash save of config-data.
        //#define __USE_GPIO_HARDWARE_FLOWCONTROL__   // Serial H/W flow control enable: RTS signal is activated by serial buffer usage
        //#define __USE_MODBUS_PROTOCOL__             // Modbus converter function available: RTU to TCP / ASCII to TCP
        //#define __USE_MODBUS_MONITOR__              // Use Modbus data frame monitoring function
        #define __USE_WATCHDOG__                  // WDT timeout 32 Second
        //#define __USE_RTC__
        #define __USE_S2E_OVER_TLS__                // Use S2E TCP client over SSL/TLS mode
        //#define __USE_SERIAL_FLASH__
        //#define __USE_WIZCHIP_RECOVERY__            // When this option is enabled, periodically check the status of WIZCHIP and recover when an error occurs.
        //#define __USE_UART_485_422__
        #define DEVICE_ID_DEFAULT                   "WIZ510SSL"//"S2E_SSL-MB" // Device name
        #define DEVICE_CLOCK_SELECT                 CLOCK_SOURCE_EXTERNAL // or CLOCK_SOURCE_INTERNAL
        #define DEVICE_UART_CNT                     (1)
        #define DEVICE_SETTING_PASSWORD_DEFAULT     "00000000"
        #define DEVICE_GROUP_DEFAULT                "WORKGROUP" // Device group

    #endif
#else // Unknown Device
    #define DEVICE_BOARD_NAME           UNKNOWN_DEV
    #define DEVICE_ID_DEFAULT           "Unknown-Dev" // Device name
#endif


#define UART_MODE_SEL_TIME_MS           300 // ms
#define APPBOOT_ENTRY_TIME_MS           300
#define FACTORY_RESET_TIME_MS           5000

#define FACTORY_CHECK_PERIOD_MS         1000
#define WIZCHIP_CHECK_PERIOD_MS         5000

#define WIZCHIP_RECOVERY_TIMEOUT_MS     3000

#ifdef __USE_WATCHDOG__
    #define DEVICE_WATCHDOG_COUNTER_SEC     (5) // Watchdog(STM32 IWDG), Second unit
#endif

#if 1
    #define WIZCHIP_CS_PORT             SPI1_CS_GPIO_Port
    #define WIZCHIP_CS_PIN              SPI1_CS_Pin  // nSS

    // WIZCHIP Pins: commonly used
    #define WIZCHIP_RSTn_PORT           W5100S_RST_GPIO_Port        // GPIOC
    #define WIZCHIP_RSTn_PIN            W5100S_RST_Pin   // GPIO_PIN_7

    #define WIZCHIP_INTn_PORT           W5100S_INT_GPIO_Port        // GPIOB
    #define WIZCHIP_INTn_PIN            W5100S_INT_Pin   // GPIO_PIN_5


    #define DATA0_UART_PORTNUM          (0)
    #define DATA1_UART_PORTNUM          (1)
    #define DEBUG_UART_PORTNUM          (2)

    // [0] Data0 UART: USART1 (RS-232C)
    #define DATA0_UART                  USART1
    #define DATA0_UART_IRQn             USART1_IRQn
    #define DATA0_UART_PORT             GPIOA
    #define DATA0_UART_TX_PIN           GPIO_PIN_9
    #define DATA0_UART_RX_PIN           GPIO_PIN_10
    #define DATA0_UART_CTS_PIN          GPIO_PIN_11
    #define DATA0_UART_RTS_PIN          GPIO_PIN_12

#if (DEVICE_UART_CNT > 1)
    // [1] Data1 UART: USART2 (RS-422/485)
    #define DATA1_UART                  0 //USART1
    #define DATA1_UART_IRQn             0 //USART1_IRQn
    #define DATA1_UART_PORT             0 //GPIOA
    #define DATA1_UART_TX_PIN           0 //GPIO_PIN_2
    #define DATA1_UART_RX_PIN           0 //GPIO_PIN_3
    #define DATA1_UART_RTS_PIN          0 //GPIO_PIN_1 // 485_SEL
#endif

    // [2] Debug UART: LPUART1 (TTL, Debug)
    #define DEBUG_UART                  USART2
//    #define DEBUG_UART_PORT             USART2_TX_GPIO_Port
//    #define DEBUG_UART_TX_PIN           USART2_TX_Pin
//    #define DEBUG_UART_RX_PIN           LPUART1_RX__ST_LINK_VCP_TX__Pin

    // Connection status indicator pins
    // Direction: Output
    #define STATUS_PHYLINK_PORT         STATUS_LED0_GPIO_Port //PHYLINK_GPIO_Port
    #define STATUS_PHYLINK_PIN          STATUS_LED0_Pin //PHYLINK_Pin

    #define STATUS_TCPCONNECT_PORT      STATUS_LED1_GPIO_Port //TCPCONNECT_GPIO_Port
    #define STATUS_TCPCONNECT_PIN       STATUS_LED1_Pin //TCPCONNECT_Pin

    #define DTR_PIN                     DTR_OUT_Pin
    #define DTR_PORT                    DTR_OUT_GPIO_Port

    #define DSR_PIN                     DSR_IN_Pin
    #define DSR_PORT                    DSR_IN_GPIO_Port

#ifdef __USE_HW_FACTORY_RESET__
    #define FAC_RSTn_PIN                FAC_RESET_Pin
    #define FAC_RSTn_PORT               FAC_RESET_GPIO_Port
#endif

        // HW_TRIG - Command mode switch enable pin
        // Direction: Input (Shared pin with TCP connection status pin)
    #define HW_TRIG_PIN                 0 //STATUS_TCPCONNECT_PIN
    #define HW_TRIG_PORT                0 //STATUS_TCPCONNECT_PORT

        // [RGB LED]    R: PC_0x, G: PC_0x, B: PC_0x
        // [LED 0/1/2]  0: PC_0x, 1: PC_0x, 2: PC_0x
    #define LED_0_PIN                   GPIO_PIN_11
    #define LED_0_PORT                  GPIOC

    #define LED_1_PIN                   GPIO_PIN_12
    #define LED_1_PORT                  GPIOC

    #define LED_2_PIN                   GPIO_PIN_6
    #define LED_2_PORT                  GPIOA

    // LED
    #define LEDn        3
    typedef enum
    {
      LED_0 = 0,
      LED_1 = 1,
      LED_2 = 2
    } Led_TypeDef;
#else
////////////////////////////////////
// Pin & IRQ Handler definitions  //
////////////////////////////////////

#if (DEVICE_BOARD_NAME == S2E_SSL)

    #define DATA0_UART_PORTNUM          (0)
    #define DATA1_UART_PORTNUM          (1)
    #define DEBUG_UART_PORTNUM          (2)

    // [0] Data0 UART: USART1 (RS-232C)
    #define DATA0_UART                  USART1
    #define DATA0_UART_IRQn             USART1_IRQn
    #define DATA0_UART_PORT             GPIOA
    #define DATA0_UART_TX_PIN           GPIO_PIN_9
    #define DATA0_UART_RX_PIN           GPIO_PIN_10
    #define DATA0_UART_CTS_PIN          GPIO_PIN_11
    #define DATA0_UART_RTS_PIN          GPIO_PIN_12

    // [1] Data1 UART: USART6 (RS-422/485)
    #define DATA1_UART                  USART6
    #define DATA1_UART_IRQn             USART6_IRQn
    #define DATA1_UART_PORT             GPIOC
    #define DATA1_UART_TX_PIN           GPIO_PIN_6
    #define DATA1_UART_RX_PIN           GPIO_PIN_7
    #define DATA1_UART_RTS_PIN          GPIO_PIN_8 // 485_SEL

    // [2] Debug UART: USART2 (TTL, Debug)
    #define DEBUG_UART                  USART2
    #define DEBUG_UART_PORT             GPIOA
    #define DEBUG_UART_TX_PIN           GPIO_PIN_2
    #define DEBUG_UART_RX_PIN           GPIO_PIN_3

    // W5500 SPI: SPI1 (Arduino Ethernet shield-like pin placement w/o RST/INT pin)
    #define W5500_SPI                   SPI1
    #define W5500_SPI_PORT              GPIOA
    #define W5500_SPI_SCK_PIN           GPIO_PIN_5
    #define W5500_SPI_MISO_PIN          GPIO_PIN_6
    #define W5500_SPI_MOSI_PIN          GPIO_PIN_7
    
    //for WIZ2000
    #define W5500_CS_PORT               GPIOA
    #define W5500_CS_PIN                GPIO_PIN_4  // nSS

    // WIZCHIP Pins: commonly used
    #define WIZCHIP_RSTn_PORT           GPIOB       // GPIOC
    #define WIZCHIP_RSTn_PIN            GPIO_PIN_0  // GPIO_PIN_7

    #define WIZCHIP_INTn_PORT           GPIOB       // GPIOB
    #define WIZCHIP_INTn_PIN            GPIO_PIN_1  // GPIO_PIN_5

/*
    // for W5500 Ethernet Shield
    #define W5500_CS_PORT               GPIOB
    #define W5500_CS_PIN                GPIO_PIN_6  // nSS

    // WIZCHIP Pins: commonly used
    #define WIZCHIP_RSTn_PORT           GPIOC
    #define WIZCHIP_RSTn_PIN            GPIO_PIN_7

    #define WIZCHIP_INTn_PORT           GPIOB
    #define WIZCHIP_INTn_PIN            GPIO_PIN_5
*/

    // External Serial Flash Memory: SPI2
    #define SFLASH_SPI                  SPI2
    #define SFLASH_SPI_PORT             GPIOB
    #define SFLASH_SPI_SCK_PIN          GPIO_PIN_13
    #define SFLASH_SPI_MISO_PIN         GPIO_PIN_14
    #define SFLASH_SPI_MOSI_PIN         GPIO_PIN_15

    #define SFLASH_CS_PIN               GPIO_PIN_12   // nSS
    #define SFLASH_CS_PORT              GPIOB

    // Serial Mode Selector
    #define MODE_SEL_PORT               GPIOB
    #define MODE_SEL0_PORT              GPIOB
    #define MODE_SEL0_PIN               GPIO_PIN_7
    #define MODE_SEL1_PORT              GPIOB
    #define MODE_SEL1_PIN               GPIO_PIN_8

    // Connection status indicator pins
    // Direction: Output
    #define STATUS_PHYLINK_PORT         GPIOC
    #define STATUS_PHYLINK_PIN          GPIO_PIN_11

    #define STATUS_TCPCONNECT_PORT      GPIOC
    #define STATUS_TCPCONNECT_PIN       GPIO_PIN_12

#ifdef __USE_HW_FACTORY_RESET__
    #define FAC_RSTn_PORT               GPIOB
    #define FAC_RSTn_PIN                GPIO_PIN_5
#endif


#ifdef __USE_HW_APPBOOT_ENTRY__
    #define APPBOOT_ENTRYn_PORT         GPIOB
    #define APPBOOT_ENTRYn_PIN          GPIO_PIN_6
#endif


#ifdef __USE_UART_IF_SELECTOR__
    //#define UART_IF_SEL_PIN         GPIO_PIN_6
    //#define UART_IF_SEL_PORT        GPIOC
#endif


    // DTR / DSR - Handshaking signals, Shared with PHYLINK_PIN and TCPCONNECT_PIN (selectable)
    // > DTR - Data Terminal Ready, Direction: Output (= PHYLINK_PIN)
    // 		>> This signal pin asserted when the device could be possible to transmit the UART inputs
    // 		>> [O], After boot and initialize
    // 		>> [X], nope (E.g., TCP connected (Server / client mode) or TCP mixed mode or UDP mode)
    // > DSR - Data Set Ready, Direction: Input (= TCPCONNECT_PIN)
    // 		>> [O] Ethet_to_UART() function control

    #define DTR_PIN                     STATUS_PHYLINK_PIN
    #define DTR_PORT                    STATUS_PHYLINK_PORT

    #define DSR_PIN                     STATUS_TCPCONNECT_PIN
    #define DSR_PORT                    STATUS_TCPCONNECT_PORT

    // HW_TRIG - Command mode switch enable pin
    // Direction: Input (Shared pin with TCP connection status pin)
    #define HW_TRIG_PIN                 STATUS_TCPCONNECT_PIN
    #define HW_TRIG_PORT                STATUS_TCPCONNECT_PORT

    // [RGB LED]    R: PC_0x, G: PC_0x, B: PC_0x
    // [LED 0/1/2]  0: PC_0x, 1: PC_0x, 2: PC_0x
    #define LED_0_PIN                   GPIO_PIN_11
    #define LED_0_PORT                  GPIOC

    #define LED_1_PIN                   GPIO_PIN_12
    #define LED_1_PORT                  GPIOC

    #define LED_2_PIN                   GPIO_PIN_6
    #define LED_2_PORT                  GPIOA



    // LED
    #define LEDn        3
    typedef enum
    {
      LED_0 = 0,
      LED_1 = 1,
      LED_2 = 2
    } Led_TypeDef;
#endif
#endif


/* PHY Link check interval */
#define PHYLINK_CHECK_CYCLE_MSEC	1000 // 1000ms
    
    
extern volatile uint16_t phylink_check_time_msec;
extern uint8_t flag_check_phylink;
extern uint8_t flag_hw_trig_enable;

void s2e_ssl_Board_Init(void);

uint8_t get_phylink(void);

void init_hw_trig_pin(void);
uint8_t get_hw_trig_pin(void);

void init_uart_if_sel_pin(void);
uint8_t get_uart_if_sel_pin(uint32_t time_ms);

#ifdef __USE_HW_FACTORY_RESET__
    // Factory reset
    void init_factory_reset_pin(void);
    uint8_t get_factory_reset_pin(void);
    uint8_t process_check_factory_reset(uint32_t time_ms);
#endif

#ifdef __USE_HW_APPBOOT_ENTRY__
    // AppBoot entry
    void init_appboot_entry_pin(void);
    uint8_t get_appboot_entry_pin(uint32_t time_ms);
#endif


void LED_Init(Led_TypeDef Led);
void LED_On(Led_TypeDef Led);
void LED_Off(Led_TypeDef Led);
void LED_Toggle(Led_TypeDef Led);
uint8_t get_LED_Status(Led_TypeDef Led);

#endif
