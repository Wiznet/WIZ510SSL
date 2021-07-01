#ifndef SEG_H_
#define SEG_H_

#include <stdint.h>
#include "common.h"
#include "s2e_ssl_board.h"
#include "MQTTClient.h"
//#define _SEG_DEBUG_

///////////////////////////////////////////////////////////////////////////////////////////////////////

#define SEG_DATA0_UART      DATA0_UART_PORTNUM // Data UART selector
#define SEG_DATA1_UART      DATA1_UART_PORTNUM
#define SEG_DEBUG_UART      DEBUG_UART_PORTNUM // Debug UART

//#define SEG_DATA_BUF_SIZE 2048 // UART Ring buffer size
#define SEG_DATA_BUF_SIZE   4096 // UART Ring buffer size

///////////////////////////////////////////////////////////////////////////////////////////////////////

#define DEFAULT_MODESWITCH_INTER_GAP        500 // 500ms (0.5sec)

//#define MIXED_CLIENT_INFINITY_CONNECT
#ifndef MIXED_CLIENT_INFINITY_CONNECT
    #define MIXED_CLIENT_LIMITED_CONNECT    //  TCP_MIXED_MODE: TCP CLIENT - limited count of connection retries
    #define MAX_RECONNECTION_COUNT          10
#endif

#define MAX_CONNECTION_AUTH_TIME            5000 // 5000ms (5sec)


#ifdef __USE_S2E_OVER_TLS__
    #define S2E_OVER_TLS_CONNECT_TIME_MS    10000
#endif
///////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef DATA_BUF_SIZE
    #define DATA_BUF_SIZE           2048
#endif

#define SEG_DISABLE                 0
#define SEG_ENABLE                  1

#define SEG_KILOBYTE                   (1024)
#define SEG_MEGABYTE                   (SEG_KILOBYTE * 1024)

extern uint8_t opmode;
extern uint8_t flag_s2e_application_running;
extern uint8_t flag_process_dhcp_success;
extern uint8_t flag_process_dns_success[];
extern char * str_working[];

// todo:
/*
typedef struct __DevChannel {
    uint8_t id;
    uint8_t uartNum;
    uint8_t sockNum;
} __attribute__((packed)) DevChannel;

typedef struct __seg_opts {
    // flags and timer variables
    // ...
} __attribute__((packed)) seg_opts;
*/


typedef enum{SEG_UART_RX, SEG_UART_TX, SEG_ETHER_RX, SEG_ETHER_TX, SEG_ALL} teDATADIR;
typedef enum{
    SEG_DEBUG_DISABLED = 0,
    SEG_DEBUG_ENABLED  = 1,
    SEG_DEBUG_S2E      = 2,
    SEG_DEBUG_E2S      = 3,
    SEG_DEBUG_ALL      = 4
} teDEBUGTYPE;

enum{
    SEG_SERIAL_PROTOCOL_NONE = 0,
    SEG_SERIAL_MODBUS_RTU    = 1,
    SEG_SERIAL_MODBUS_ASCII  = 2
};

/* Remote monitor option */
enum {
    SEG_REMOTE_MONITOR_NONE   = 0,
    SEG_REMOTE_MONITOR_S2E    = 1,
    SEG_REMOTE_MONITOR_E2S    = 2,
    SEG_REMOTE_MONITOR_ALL    = 3,
};

/* Auto message - The first data packet from device */
// 0: No massage (default)
// 1: Send device type(name) when TCP connected
// 2: Send device MAC address when TCP connected
// 3: Send device IP address when TCP connected
// 4: Send device ID(device name + MAC) when TCP connected
// 5: Send device alias when TCP connected
// 6: Send device group when TCP connected

enum {
    SEG_LINK_MSG_NONE       = 0,
    SEG_LINK_MSG_DEVNAME    = 1,
    SEG_LINK_MSG_MAC        = 2,
    SEG_LINK_MSG_IP         = 3,
    SEG_LINK_MSG_DEVID      = 4,
    SEG_LINK_MSG_DEVALIAS   = 5,
    SEG_LINK_MSG_DEVGROUP   = 6,
};


// Serial to Ethernet function handler; call by main loop
void do_seg(uint8_t uartNum, uint8_t sock);

// Timer for S2E core operations
void seg_timer_sec(void);
void seg_timer_msec(void);

void init_trigger_modeswitch(uint8_t channel, uint8_t uartNum, uint8_t mode);

void set_device_status(uint8_t channel, teDEVSTATUS status);
void set_device_status_all(teDEVSTATUS status);

uint8_t get_seg_channel(uint8_t uartNum);
uint8_t get_device_status(uint8_t channel);
uint8_t get_serial_communation_protocol(uint8_t channel);

uint8_t process_socket_termination(uint8_t sock, uint32_t timeout);

// Send Keep-alive packet manually (once)
void send_keepalive_packet_manual(uint8_t sock);

//These functions must be located in UART Rx IRQ Handler.
uint8_t check_serial_store_permitted(uint8_t channel, uint8_t ch);
uint8_t check_modeswitch_trigger(uint8_t ch);	        // Serial command mode switch trigger code (3-bytes) checker
void init_time_delimiter_timer(uint8_t channel); 		// Serial data packing option [Time]: Timer enable function for Time delimiter

// Send Auto-message function
void send_sid(uint8_t sock, uint8_t link_message, uint8_t uart_channel);

// Serial debug messages for verifying data transfer
uint16_t debugSerial_dataTransfer(uint8_t * buf, uint16_t size, teDEBUGTYPE type);

// UART tx/rx and Ethernet tx/rx data transfer bytes counter
void add_data_transfer_bytecount(uint8_t channel, teDATADIR dir, uint16_t len);

// MQTT sub handler
void mqtt_subscribeMessageHandler(MessageData* md);

int wizchip_mqtt_publish(MQTTClient *mqtt_c, uint8_t *pub_topic, uint8_t qos, uint8_t *pub_data, uint32_t pub_data_len);


// UART tx/rx and Ethernet tx/rx data transfer bytes counter
void clear_data_transfer_bytecount(uint8_t channel, teDATADIR dir);
void clear_data_transfer_megacount(uint8_t channel, teDATADIR dir);
uint32_t get_data_transfer_bytecount(uint8_t channel, teDATADIR dir);
uint32_t get_data_transfer_megacount(uint8_t channel, teDATADIR dir);

#endif /* SEG_H_ */

