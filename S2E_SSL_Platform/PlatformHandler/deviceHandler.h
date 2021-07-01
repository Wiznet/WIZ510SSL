#ifndef DEVICEHANDLER_H_
#define DEVICEHANDLER_H_

#include <stdint.h>
//#include "s2e_ssl_board.h"
#include "flashHandler.h"
#include "storageHandler.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>



/* Debug message enable */
//#define _FWUP_DEBUG_

/* Application Port */
#define DEVICE_SEGCP_PORT           50001   // Search / Setting Port (UDP Broadcast / TCP unicast)
#define DEVICE_FWUP_PORT            50002   // Firmware Update Port
#define DEVICE_DDNS_PORT            3030    // Not used

// HTTP Response: Status code
#define STATUS_HTTP_OK              200

#define NTP_SERVER_LIST
#ifdef NTP_SERVER_LIST
    #define WIZ2000_NTP_SERVER_CNT          NTP_SERVER_DOMAIN_CNT
#else
    #define NTP_SERVER_LIST_DEFAULT         {"time1.google.com", "time2.google.com", "time3.google.com", "kr.pool.ntp.org", "time.nuri.net"}
    #define NTP_SERVER_CNT_DEFAULT          5
#endif
#define NTP_TIMEOUT                     (1000)  // unit: ms
#define NTP_UPDATE_PERIODIC_MS          (1000 * 3600 * 24)  // 24 hour
#define NTP_UPDATE_RETRY_INTERVAL_MS    (1000 * 30)         // 30 sec

/* Firmware update */
//#define DEVICE_FWUP_SIZE            DEVICE_APP_SIZE
#define DEVICE_FWUP_TIMEOUT         5000 // 5 secs.

// Return values for firmware update
#define DEVICE_FWUP_RET_SUCCESS     0x80
#define DEVICE_FWUP_RET_FAILED      0x40
#define DEVICE_FWUP_RET_PROGRESS    0x20
#define DEVICE_FWUP_RET_NONE        0x00


void device_set_factory_default(void);
void device_socket_termination(void);
void device_reboot(void);

uint8_t device_firmware_update(teDATASTORAGE stype); // Firmware update by Configuration tool / Flash to Flash
uint8_t device_appboot_update(void);
uint8_t device_bank_update(void);

// function for timer
void device_timer_msec(void);

void display_Dev_Info_main(void);
void display_Dev_Info_dhcp(void);
void display_Dev_Info_dns(uint8_t idx);

int8_t process_dhcp(void);
int8_t process_dns(uint8_t channel);
int8_t get_ipaddr_from_dns(uint8_t * domain, uint8_t * ip_from_dns, uint32_t timeout);
time_t get_ntp_time(uint8_t * NTPServer, uint32_t timeout);
//int8_t process_ntp(void);

#ifdef __USE_WATCHDOG__
void wdt_reset(void);
#endif



#endif /* DEVICEHANDLER_H_ */
