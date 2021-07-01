/*
 * ConfigData.c 
 */

#include <stdio.h>
#include <string.h>
#include "common.h"
#include "s2e_ssl_board.h"
#include "ConfigData.h"
#include "timerHandler.h"
#include "flashHandler.h"
#include "storageHandler.h"
#include "deviceHandler.h"
#include "gpioHandler.h"
#include "uartHandler.h"
#include "wiz_debug.h"

#include "seg.h"
#include "segcp.h"

#include "mbedtls/ssl.h"
#include "memory_define.h"

static DevConfig dev_config;
extern IWDG_HandleTypeDef hiwdg;
uint8_t mac[] = {0x00, 0x08, 0xdc, 0xAA, 0xBB, 0xCC};

DevConfig* get_DevConfig_pointer(void)
{
    return &dev_config;
}

void set_DevConfig_to_factory_value(void)
{
    uint8_t i;

    dev_config.device_common.fw_ver[0] = MAJOR_VER;
    dev_config.device_common.fw_ver[1] = MINOR_VER;
    dev_config.device_common.fw_ver[2] = MAINTENANCE_VER;

    /* Product code */
    // WIZ550S2E  : 000
    // WIZ550web  : 120
    // W7500S2E   : 010 (temporary)
    // WIZ2000-MB : 201
    // WIZ510SSL : 301
    dev_config.device_common.device_type[0] = 0x03;
    dev_config.device_common.device_type[1] = 0x00;
    dev_config.device_common.device_type[2] = 0x01;

    memset(dev_config.device_common.device_name, 0x00, sizeof(DEVICE_ID_DEFAULT));
    memcpy(dev_config.device_common.device_name, DEVICE_ID_DEFAULT, sizeof(DEVICE_ID_DEFAULT));

    dev_config.device_common.device_mode = DEVICE_APP_MODE;    // Reserved field for App / Boot identification

    dev_config.config_common.app_protocol = 0;      // Reserved field for device support protocols
    dev_config.config_common.packet_size = sizeof(DevConfig);
    memset(dev_config.config_common.pw_search, 0x00, sizeof(dev_config.config_common.pw_search));

    dev_config.network_common.local_ip[0] = 192;
    dev_config.network_common.local_ip[1] = 168;
    dev_config.network_common.local_ip[2] = 11;
    dev_config.network_common.local_ip[3] = 2;

    dev_config.network_common.gateway[0] = 192;
    dev_config.network_common.gateway[1] = 168;
    dev_config.network_common.gateway[2] = 11;
    dev_config.network_common.gateway[3] = 1;

    dev_config.network_common.subnet[0] = 255;
    dev_config.network_common.subnet[1] = 255;
    dev_config.network_common.subnet[2] = 255;
    dev_config.network_common.subnet[3] = 0;

    for(i = 0; i < DEVICE_UART_CNT; i++)
    {
        dev_config.network_connection[i].working_mode = TCP_SERVER_MODE; //UDP_MODE; //TCP_MIXED_MODE;
        dev_config.network_connection[i].working_state = ST_OPEN;

        dev_config.network_connection[i].local_port = 5000+i;

        dev_config.network_connection[i].remote_port = 5000+i;
        dev_config.network_connection[i].remote_ip[0] = 192;
        dev_config.network_connection[i].remote_ip[1] = 168;
        dev_config.network_connection[i].remote_ip[2] = 11;
        dev_config.network_connection[i].remote_ip[3] = 3;

        dev_config.network_connection[i].fixed_local_port = DISABLE;
        dev_config.network_connection[i].dns_use = DISABLE;

        memset(dev_config.network_connection[i].dns_domain_name, 0x00, sizeof(dev_config.network_connection[i].dns_domain_name));
        memcpy(dev_config.network_connection[i].dns_domain_name, "192.168.11.3", 12);
        dev_config.serial_data_packing[i].packing_time = 0;

        dev_config.serial_data_packing[i].packing_size = 0;
        dev_config.serial_data_packing[i].packing_delimiter[0] = 0; // packing_delimiter used only one-byte (for WIZ107SR compatibility)
        dev_config.serial_data_packing[i].packing_delimiter[1] = 0;
        dev_config.serial_data_packing[i].packing_delimiter[2] = 0;
        dev_config.serial_data_packing[i].packing_delimiter[3] = 0;
        dev_config.serial_data_packing[i].packing_delimiter_length = 0;
        dev_config.serial_data_packing[i].packing_data_appendix = 0;

        dev_config.tcp_option[i].inactivity = 0;        // sec, default: NONE
        dev_config.tcp_option[i].reconnection = 3000;   // msec, default: 3 sec
        dev_config.tcp_option[i].keepalive_en = ENABLE;
        dev_config.tcp_option[i].keepalive_wait_time = 7000;
        dev_config.tcp_option[i].keepalive_retry_time = 5000;

        memset(dev_config.tcp_option[i].pw_connect, 0x00, sizeof(dev_config.tcp_option[i].pw_connect));
        dev_config.tcp_option[i].pw_connect_en = DISABLE;

        // Default Settings for Data UART: 115200-8-N-1, No flowctrl
        dev_config.serial_option[i].uart_interface = UART_IF_RS232;
        dev_config.serial_option[i].protocol = SEG_SERIAL_PROTOCOL_NONE;
        dev_config.serial_option[i].baud_rate = baud_115200;
        dev_config.serial_option[i].data_bits = word_len8;
        dev_config.serial_option[i].parity = parity_none;
        dev_config.serial_option[i].stop_bits = stop_bit1;
        dev_config.serial_option[i].flow_control = flow_none;

#ifdef __USE_DSR_DTR_DEFAULT__
        dev_config.serial_option[i].dtr_en = ENABLE;
        dev_config.serial_option[i].dsr_en = ENABLE;
#else
        dev_config.serial_option[i].dtr_en = DISABLE;
        dev_config.serial_option[i].dsr_en = DISABLE;
#endif
    }

    //dev_config.serial_info[0].serial_debug_en = DISABLE;
    dev_config.serial_common.serial_debug_en = ENABLE;
    dev_config.serial_common.uart_interface_cnt = DEVICE_UART_CNT;

    //memset(dev_config.options.pw_setting, 0x00, sizeof(dev_config.options.pw_setting));

    dev_config.network_option.dhcp_use = DISABLE; //hoon

    dev_config.network_option.dns_server_ip[0] = 8; // Default DNS server IP: Google Public DNS (8.8.8.8)
    dev_config.network_option.dns_server_ip[1] = 8;
    dev_config.network_option.dns_server_ip[2] = 8;
    dev_config.network_option.dns_server_ip[3] = 8;

    dev_config.network_option.tcp_rcr_val = 8; // Default RCR(TCP retransmission retry count) value: 8

    dev_config.serial_command.serial_command = ENABLE;
    dev_config.serial_command.serial_command_echo = DISABLE;
    dev_config.serial_command.serial_trigger[0] = 0x2b; // Default serial command mode trigger code: '+++' (0x2b, 0x2b, 0x2b)
    dev_config.serial_command.serial_trigger[1] = 0x2b;
    dev_config.serial_command.serial_trigger[2] = 0x2b;

#ifdef __USE_USERS_GPIO__
    dev_config.user_io_info.user_io_enable = USER_IO_A | USER_IO_B | USER_IO_C | USER_IO_D; // [Enabled] / Disabled
    dev_config.user_io_info.user_io_type = USER_IO_A; // Analog: USER_IO_A, / Digital: USER_IO_B, USER_IO_C, USER_IO_D
    dev_config.user_io_info.user_io_direction = USER_IO_C | USER_IO_D; // IN / IN / OUT / OUT
    dev_config.user_io_info.user_io_status = 0;
#else
    dev_config.user_io_info.user_io_enable = 0;
    dev_config.user_io_info.user_io_type = 0;
    dev_config.user_io_info.user_io_direction = 0;
    dev_config.user_io_info.user_io_status = 0;
#endif

    if ((dev_config.firmware_update.current_bank != APP_BANK0) && (dev_config.firmware_update.current_bank != APP_BANK1))
        dev_config.firmware_update.current_bank = APP_BANK0;

    // SSL Option
    dev_config.ssl_option.root_ca_option = MBEDTLS_SSL_VERIFY_NONE;
    dev_config.ssl_option.client_cert_enable = DISABLE;
    
    // MQTT Option

    memset(dev_config.mqtt_option.user_name, 0x00, sizeof(dev_config.mqtt_option.user_name));
    memset(dev_config.mqtt_option.password, 0x00, sizeof(dev_config.mqtt_option.password));
    memset(dev_config.mqtt_option.client_id, 0x00, sizeof(dev_config.mqtt_option.client_id));
    memset(dev_config.mqtt_option.pub_topic, 0x00, sizeof(dev_config.mqtt_option.pub_topic));
    memset(dev_config.mqtt_option.sub_topic_0, 0x00, sizeof(dev_config.mqtt_option.sub_topic_0));
    memset(dev_config.mqtt_option.sub_topic_1, 0x00, sizeof(dev_config.mqtt_option.sub_topic_1));
    memset(dev_config.mqtt_option.sub_topic_2, 0x00, sizeof(dev_config.mqtt_option.sub_topic_2));
    dev_config.mqtt_option.qos = QOS0;
    dev_config.mqtt_option.keepalive = 0;

    // fixed local port enable / disable

    dev_config.device_option.pw_setting_en = ENABLE;
    memset(dev_config.device_option.pw_setting, 0x00, sizeof(dev_config.device_option.pw_setting));
    memset(dev_config.device_option.device_group, 0x00, sizeof(dev_config.device_option.device_group));
    memset(dev_config.device_option.device_alias, 0x00, sizeof(dev_config.device_option.device_alias));

    memcpy(dev_config.device_option.pw_setting, DEVICE_SETTING_PASSWORD_DEFAULT, sizeof(DEVICE_SETTING_PASSWORD_DEFAULT));
    memcpy(dev_config.device_option.device_group, DEVICE_GROUP_DEFAULT, sizeof(DEVICE_GROUP_DEFAULT));

    sprintf((char *)dev_config.device_option.device_alias, "%s-%02X%02X%02X%02X%02X%02X",
                                                       dev_config.device_common.device_name,
                                                       dev_config.network_common.mac[0],
                                                       dev_config.network_common.mac[1],
                                                       dev_config.network_common.mac[2],
                                                       dev_config.network_common.mac[3],
                                                       dev_config.network_common.mac[4],
                                                       dev_config.network_common.mac[5]);

    dev_config.devConfigVer = DEV_CONFIG_VER;//DEV_CONFIG_VER;
}

void load_DevConfig_from_storage(void)
{
    int ret = -1;

    read_storage(STORAGE_CONFIG, 0, &dev_config, sizeof(DevConfig));
    read_storage(STORAGE_MAC, 0, dev_config.network_common.mac, 6);
    //read_flash(FLASH_MAC_ADDR, &dev_config.network_common.mac, 6);

    PRT_INFO("MAC = %02X%02X%02X%02X%02X%02X\r\n", dev_config.network_common.mac[0], dev_config.network_common.mac[1], dev_config.network_common.mac[2], \
                                                   dev_config.network_common.mac[3], dev_config.network_common.mac[4], dev_config.network_common.mac[5]);

   PRT_INFO("dev_config.devConfigVer = %d, DEV_CONFIG_VER = %d\r\n", dev_config.devConfigVer, DEV_CONFIG_VER);
    if((dev_config.config_common.packet_size == 0x0000) ||
       (dev_config.config_common.packet_size == 0xFFFF) ||
       (dev_config.config_common.packet_size != sizeof(DevConfig)) ||
        dev_config.devConfigVer != DEV_CONFIG_VER)
    { 
        PRT_INFO(" Config Data size: %d / %d\r\n", dev_config.config_common.packet_size, sizeof(DevConfig));
        
        set_DevConfig_to_factory_value();

        erase_storage(STORAGE_CONFIG);
        write_storage(STORAGE_CONFIG, 0, (uint8_t *)&dev_config, sizeof(DevConfig));
        read_storage(STORAGE_CONFIG, 0, &dev_config, sizeof(DevConfig));

        PRT_INFO("After Config Data size: %d / %d\r\n", dev_config.config_common.packet_size, sizeof(DevConfig));
        device_reboot();
    }
    dev_config.firmware_update.current_bank = SECURE_Get_Running_Bank();
}


void save_DevConfig_to_storage(void)
{
    erase_storage(STORAGE_CONFIG);
#ifndef __USE_SAFE_SAVE__
    write_storage(STORAGE_CONFIG, 0, (uint8_t *)&dev_config, sizeof(DevConfig));
#else
    DevConfig dev_config_tmp;
    uint8_t update_success = SEGCP_DISABLE;
    uint8_t retry_cnt = 0;
    int ret;
    
    do {
            // ## 20180208 Added by Eric, Added the verify function to flash write (Device config-data)
            //read_storage(STORAGE_CONFIG, 0, &dev_config_tmp, sizeof(DevConfig));
            write_storage(STORAGE_CONFIG, 0, (uint8_t *)&dev_config, sizeof(DevConfig));
            read_storage(STORAGE_CONFIG, 0, &dev_config_tmp, sizeof(DevConfig));
            
#endif        
            if(memcmp(&dev_config, &dev_config_tmp, sizeof(DevConfig)) == 0) { // Config-data set is successfully updated.
                update_success = SEGCP_ENABLE;
                //if(dev_config.serial_info[0].serial_debug_en) {printf(" > DevConfig is successfully updated\r\n");}
            } else {
                retry_cnt++;
                if(dev_config.serial_common.serial_debug_en) {SECURE_debug(" > DevConfig update failed, Retry: %d\r\n", retry_cnt);}
            }
            
            if(retry_cnt >= MAX_SAVE_RETRY) {
                break;
            }
    } while(update_success != SEGCP_ENABLE);
}

void get_DevConfig_value(void *dest, const void *src, uint16_t size)
{
    memcpy(dest, src, size);
}

void set_DevConfig_value(void *dest, const void *value, const uint16_t size)
{
    memcpy(dest, value, size);
}

void set_DevConfig(wiz_NetInfo *net)
{
    set_DevConfig_value(dev_config.network_common.mac, net->mac, sizeof(dev_config.network_common.mac));
    set_DevConfig_value(dev_config.network_common.local_ip, net->ip, sizeof(dev_config.network_common.local_ip));
    set_DevConfig_value(dev_config.network_common.gateway, net->gw, sizeof(dev_config.network_common.gateway));
    set_DevConfig_value(dev_config.network_common.subnet, net->sn, sizeof(dev_config.network_common.subnet));
    set_DevConfig_value(dev_config.network_option.dns_server_ip, net->dns, sizeof(dev_config.network_option.dns_server_ip));
    if(net->dhcp == NETINFO_STATIC)
        dev_config.network_option.dhcp_use = DISABLE;
    else
        dev_config.network_option.dhcp_use = ENABLE;
}

void get_DevConfig(wiz_NetInfo *net)
{
    get_DevConfig_value(net->mac, dev_config.network_common.mac, sizeof(net->mac));
    get_DevConfig_value(net->ip, dev_config.network_common.local_ip, sizeof(net->ip));
    get_DevConfig_value(net->gw, dev_config.network_common.gateway, sizeof(net->gw));
    get_DevConfig_value(net->sn, dev_config.network_common.subnet, sizeof(net->sn));
    get_DevConfig_value(net->dns, dev_config.network_option.dns_server_ip, sizeof(net->dns));
    if(dev_config.network_option.dhcp_use)
        net->dhcp = NETINFO_DHCP;
    else
        net->dhcp = NETINFO_STATIC;
}

void display_Net_Info(void)
{
    DevConfig *dev_config = get_DevConfig_pointer();
    wiz_NetInfo gWIZNETINFO;

    uint8_t i;

    ctlnetwork(CN_GET_NETINFO, (void*) &gWIZNETINFO);
    PRT_INFO(" # MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n", gWIZNETINFO.mac[0], gWIZNETINFO.mac[1], gWIZNETINFO.mac[2], gWIZNETINFO.mac[3], gWIZNETINFO.mac[4], gWIZNETINFO.mac[5]);
    PRT_INFO(" # IP : %d.%d.%d.%d / Port : \r\n", gWIZNETINFO.ip[0], gWIZNETINFO.ip[1], gWIZNETINFO.ip[2], gWIZNETINFO.ip[3]);
    for(i = 0; i < DEVICE_UART_CNT; i++)
        PRT_INFO("[Ch%d] %d ", i, dev_config->network_connection[i].local_port);
    PRT_INFO("\r\n");
    PRT_INFO(" # GW : %d.%d.%d.%d\r\n", gWIZNETINFO.gw[0], gWIZNETINFO.gw[1], gWIZNETINFO.gw[2], gWIZNETINFO.gw[3]);
    PRT_INFO(" # SN : %d.%d.%d.%d\r\n", gWIZNETINFO.sn[0], gWIZNETINFO.sn[1], gWIZNETINFO.sn[2], gWIZNETINFO.sn[3]);
    PRT_INFO(" # DNS: %d.%d.%d.%d\r\n", gWIZNETINFO.dns[0], gWIZNETINFO.dns[1], gWIZNETINFO.dns[2], gWIZNETINFO.dns[3]);
    
    for(i = 0; i < DEVICE_UART_CNT; i++)
    {
        if(dev_config->network_connection[i].working_mode != TCP_SERVER_MODE)
        {
            if(dev_config->network_connection[i].dns_use == SEGCP_ENABLE)
            {
                PRT_INFO(" # Destination Domain: %s / Port: %d\r\n",
                        dev_config->network_connection[i].dns_domain_name,
                        dev_config->network_connection[i].remote_port);
            }
            else
            {
                PRT_INFO(" # Destination IP: %d.%d.%d.%d / Port: %d\r\n",
                        dev_config->network_connection[i].remote_ip[0],
                        dev_config->network_connection[i].remote_ip[1],
                        dev_config->network_connection[i].remote_ip[2],
                        dev_config->network_connection[i].remote_ip[3],
                        dev_config->network_connection[i].remote_port);

                if(dev_config->network_connection[i].working_mode == UDP_MODE)
                {
                    if((dev_config->network_connection[i].remote_ip[0] == 0) &&
                       (dev_config->network_connection[i].remote_ip[1] == 0) &&
                       (dev_config->network_connection[i].remote_ip[2] == 0) &&
                       (dev_config->network_connection[i].remote_ip[3] == 0))
                    {
                        PRT_INFO(" ## UDP 1:N Mode\r\n");
                    }
                    else
                    {
                        PRT_INFO(" ## UDP 1:1 Mode\r\n");
                    }
                }
            }
        }
    }
    printf("\r\n");
}

void Mac_Conf(void)
{
    DevConfig *dev_config = get_DevConfig_pointer();
    setSHAR(dev_config->network_common.mac);
}

void Net_Conf(void)
{
    DevConfig *dev_config = get_DevConfig_pointer();
    wiz_NetInfo gWIZNETINFO;

    /* wizchip netconf */
    get_DevConfig_value(gWIZNETINFO.mac, dev_config->network_common.mac, sizeof(gWIZNETINFO.mac[0]) * 6);
    get_DevConfig_value(gWIZNETINFO.ip, dev_config->network_common.local_ip, sizeof(gWIZNETINFO.ip[0]) * 4);
    get_DevConfig_value(gWIZNETINFO.gw, dev_config->network_common.gateway, sizeof(gWIZNETINFO.gw[0]) * 4);
    get_DevConfig_value(gWIZNETINFO.sn, dev_config->network_common.subnet, sizeof(gWIZNETINFO.sn[0]) * 4);
    get_DevConfig_value(gWIZNETINFO.dns, dev_config->network_option.dns_server_ip, sizeof(gWIZNETINFO.dns));
    if(dev_config->network_option.dhcp_use)
        gWIZNETINFO.dhcp = NETINFO_DHCP;
    else
        gWIZNETINFO.dhcp = NETINFO_STATIC;

    ctlnetwork(CN_SET_NETINFO, (void*) &gWIZNETINFO);
}

void set_dhcp_mode(void)
{
    DevConfig *dev_config = get_DevConfig_pointer();
    dev_config->network_option.dhcp_use = 1;
}

void set_static_mode(void)
{
    DevConfig *dev_config = get_DevConfig_pointer();
    dev_config->network_option.dhcp_use = 0;
}

void set_mac(uint8_t *mac)
{
    DevConfig *dev_config = get_DevConfig_pointer();
    memcpy(dev_config->network_common.mac, mac, sizeof(dev_config->network_common.mac));
}
//#if 0//Viktor MAC address save

void check_mac_address()
{
	//DevConfig *dev_config = get_DevConfig_pointer();
	int ret;
  	uint8_t buf[12], vt, temp;
  	uint32_t vi, vj;
    uint8_t temp_buf[] = "INPUT MAC ? ";
    
  	if (dev_config.network_common.mac[0] != 0x00 || dev_config.network_common.mac[1] != 0x08 || dev_config.network_common.mac[2] != 0xDC)
  	{
  		PRT_INFO("%s\r\n", temp_buf);
        HAL_UART_Transmit(&huart1, temp_buf, strlen(temp_buf), 100);
  		while(1){
  			HAL_UART_Receive(&huart1, &vt ,1,100);
  			if(vt == 'S') {
  				temp = 'R';
  				HAL_UART_Transmit(&huart1,&temp,1, 100);
  				break;
  			}
  		}
  		for(vi = 0; vi < 12; vi++){
  			HAL_UART_Receive(&huart1,&buf[vi],1,100);
  		    HAL_UART_Transmit(&huart1,&buf[vi],1,100);
  		}
		HAL_UART_Transmit(&huart1,"\r\n",2,100);
  		for(vi = 0, vj = 0 ; vi < 6 ; vi++, vj += 2){
  			dev_config.network_common.mac[vi] = get_hex(buf[vj], buf[vj+1]);
  			mac[vi] = get_hex(buf[vj], buf[vj+1]);
  		}
//INPUT address before testing
//Be careful with OTP base
// write_flash can be replaced with write_storage
  		ret = erase_flash_page(FLASH_MAC_ADDR);
  		PRT_INFO("erase_flash_page ret = %d\r\n", ret);

  		ret = write_flash(FLASH_MAC_ADDR, mac, 6);
  		PRT_INFO("write_flash ret = %d\r\n", ret);
  		
        device_reboot();
  	}

}

uint8_t get_hex(uint8_t b0, uint8_t b1)
{
  uint8_t buf[2];
  buf[0]   = b0;
  buf[1]   = b1;
  buf[0]   = atonum(buf[0]);
  buf[0] <<= 4;
  buf[0]  += atonum(buf[1]);
  return(buf[0]);
}

char atonum(char ch)
{
  ch -= '0';
  if (ch > 9) ch -= 7;
  if (ch > 15) ch -= 0x20;
  return(ch);
}

