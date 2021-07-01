#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "s2e_ssl_board.h"
#include "socket.h"

#include "seg.h"
#include "segcp.h"
#include "timerHandler.h"
#include "uartHandler.h"
#include "gpioHandler.h"
#include "deviceHandler.h"

// Modbus
//#include "mb.h"
//#include "mbrtu.h"
//#include "mbascii.h"
//#include "mbtimer.h"

#include "wiz_debug.h"

// TLS support
#ifdef __USE_S2E_OVER_TLS__
    #include "SSLInterface.h"
    wiz_tls_context s2e_tlsContext;
    static int wiz_tls_init_state;
#endif

#include "mqtt_interface.h"

/* Private define ------------------------------------------------------------*/
// Ring Buffer
BUFFER_DECLARATION(data0_rx);

/* Private variables ---------------------------------------------------------*/
uint8_t flag_s2e_application_running = SEG_DISABLE;

uint8_t opmode = DEVICE_GW_MODE;
static uint8_t sw_modeswitch_at_mode_on = SEG_DISABLE;

// static variables for function: check_modeswitch_trigger()
static uint8_t triggercode_idx;
static uint8_t ch_tmp[3];

// Gateway mode <-> command mode switch gap time
uint8_t enable_modeswitch_timer = SEG_DISABLE;
volatile uint16_t modeswitch_time = 0;
volatile uint16_t modeswitch_gap_time = DEFAULT_MODESWITCH_INTER_GAP;

static uint8_t mixed_state[DEVICE_UART_CNT] = {MIXED_SERVER, };
static uint16_t client_any_port[DEVICE_UART_CNT] = {0, };

// Timer Enable flags / Time
uint8_t enable_inactivity_timer[DEVICE_UART_CNT] = {SEG_DISABLE, };
volatile uint16_t inactivity_time[DEVICE_UART_CNT] = {0, };

uint8_t enable_keepalive_timer[DEVICE_UART_CNT] = {SEG_DISABLE, };
volatile uint16_t keepalive_time[DEVICE_UART_CNT] = {0, };

uint8_t enable_reconnection_timer[DEVICE_UART_CNT] = {SEG_DISABLE, };
volatile uint16_t reconnection_time[DEVICE_UART_CNT] = {0, };

uint8_t enable_serial_input_timer[DEVICE_UART_CNT] = {SEG_DISABLE, };
volatile uint16_t serial_input_time[DEVICE_UART_CNT] = {0, };
uint8_t flag_serial_input_time_elapse[DEVICE_UART_CNT] = {SEG_DISABLE, }; // for Time delimiter

// added for auth timeout
uint8_t enable_connection_auth_timer[DEVICE_UART_CNT] = {SEG_DISABLE, };
volatile uint16_t connection_auth_time[DEVICE_UART_CNT] = {0, };

// flags
uint8_t flag_connect_pw_auth[DEVICE_UART_CNT] = {SEG_DISABLE, }; // TCP_SERVER_MODE only
uint8_t flag_sent_keepalive[DEVICE_UART_CNT] = {SEG_DISABLE, };
uint8_t flag_sent_first_keepalive[DEVICE_UART_CNT] = {SEG_DISABLE, };

// User's buffer / size idx
extern uint8_t g_send_buf[DATA_BUF_SIZE];
extern uint8_t g_recv_buf[DATA_BUF_SIZE];
extern uint8_t g_send_mqtt_buf[DATA_BUF_SIZE];

uint16_t u2e_size[DEVICE_UART_CNT] = {0, };
uint16_t e2u_size[DEVICE_UART_CNT] = {0, };

// S2E Data byte count variables
volatile uint32_t seg_byte_cnt[DEVICE_UART_CNT][4] = {{0, }, };
volatile uint32_t seg_mega_cnt[DEVICE_UART_CNT][4] = {{0, }, };

// UDP: Peer netinfo
uint8_t peerip[DEVICE_UART_CNT][4] = {{0, }, };
uint8_t peerip_tmp[DEVICE_UART_CNT][4] = {{0xff, }, };
uint16_t peerport[DEVICE_UART_CNT] = {0, };

// XON/XOFF (Software flow control) flag, Serial data can be transmitted to peer when XON enabled.
uint8_t isXON[DEVICE_UART_CNT] = {SEG_ENABLE, };

char * str_working[] = {"TCP_CLIENT_MODE", "TCP_SERVER_MODE", "TCP_MIXED_MODE", "UDP_MODE", "SSL_TCP_CLIENT_MODE", "MQTT_CLIENT_MODE", "MQTTS_CLIENT_MODE"};

uint8_t flag_process_dhcp_success = OFF;
uint8_t flag_process_dns_success[DEVICE_UART_CNT] = {OFF, };

uint8_t isSocketOpen_TCPclient[DEVICE_UART_CNT] = {OFF, };

// ## timeflag for debugging
uint8_t tmp_timeflag_for_debug = 0;

extern Network mqtt_n;
extern MQTTClient mqtt_c;
extern MQTTPacket_connectData mqtt_data;
//extern uint8_t g_rootca_buf[];

/* Private functions prototypes ----------------------------------------------*/
void proc_SEG_tcp_client(uint8_t uartNum, uint8_t sock);
void proc_SEG_tcp_server(uint8_t uartNum, uint8_t sock);
void proc_SEG_tcp_mixed(uint8_t uartNum, uint8_t sock);
void proc_SEG_udp(uint8_t uartNum, uint8_t sock);

#ifdef __USE_S2E_OVER_TLS__
    void proc_SEG_tcp_client_over_tls(uint8_t uartNum, uint8_t sock);
    int get_wiz_tls_init_state(void);
    void set_wiz_tls_init_state(int state);
#endif

void uart_to_ether(uint8_t uartNum, uint8_t sock);
void ether_to_uart(uint8_t uartNum, uint8_t sock);
uint16_t get_serial_data(uint8_t uartNum);
void restore_serial_data(uint8_t idx);

uint8_t check_connect_pw_auth(uint8_t channel, uint8_t * buf, uint16_t len);
uint8_t check_tcp_connect_exception(uint8_t channel);
void reset_SEG_timeflags(uint8_t channel);

uint16_t get_tcp_any_port(uint8_t channel);

/* Public & Private functions ------------------------------------------------*/

// Channel for UART port mapping to devConfig structure
uint8_t get_seg_channel(uint8_t uartNum)
{
    uint8_t channel = uartNum;

    if(channel >= DEVICE_UART_CNT)
    {
        channel = 0;
    }

    return channel;
}

void do_seg(uint8_t uartNum, uint8_t sock)
{

    struct __network_connection *network_connection = (struct __network_connection *)get_DevConfig_pointer()->network_connection;
    struct __serial_option *serial_option = (struct __serial_option *)get_DevConfig_pointer()->serial_option;
    struct __firmware_update *firmware_update = (struct __firmware_update *)&(get_DevConfig_pointer()->firmware_update);
    
    struct __device_option *device_option = (struct __device_option *)&(get_DevConfig_pointer()->device_option);

    uint8_t channel = get_seg_channel(uartNum);

#ifdef _SEG_DEBUG_
    if(tmp_timeflag_for_debug) // every 1 sec
    {
        //if(opmode == DEVICE_GW_MODE) 	printf("working mode: %s, mixed: %s\r\n", str_working[net->working_mode], (net->working_mode == 2)?(mixed_state ? "CLIENT":"SERVER"):("NONE"));
        //else 							printf("opmode: DEVICE_AT_MODE\r\n");
        //printf("UART2Ether - ringbuf: %d, rd: %d, wr: %d, u2e_size: %d\r\n", BUFFER_USED_SIZE(data0_rx), data0_rx_rd, data0_rx_wr, u2e_size);
        //printf("UART2Ether - ringbuf: %d, rd: %d, wr: %d, u2e_size: %d\r\n", BUFFER_USED_SIZE(data1_rx), data1_rx_rd, data1_rx_wr, u2e_size);
        //printf("UART2Ether - ringbuf: %d, rd: %d, wr: %d, u2e_size: %d peer xon/off: %s\r\n", BUFFER_USED_SIZE(data0_rx), data0_rx_rd, data0_rx_wr, u2e_size, isXON?"XON":"XOFF");
        //printf("modeswitch_time [%d] : modeswitch_gap_time [%d]\r\n", modeswitch_time, modeswitch_gap_time);
        //printf("[%d]: [%d] ", modeswitch_time, modeswitch_gap_time);
        //printf("idx = %d\r\n", triggercode_idx);
        //printf("opmode: %d\r\n", opmode);
        //printf("flag_connect_pw_auth: %d\r\n", flag_connect_pw_auth);
        //printf(" >> UART2Ether - ringbuf: %d, u2e_size: %d\r\n", BUFFER_USED_SIZE(data0_rx), u2e_size[0]);
        //printf("Ether2UART - RX_RSR: %d, e2u_size: %d\r\n", getSn_RX_RSR(sock), e2u_size);
        //printf("sock_state: %x\r\n", getSn_SR(sock));
        //printf("\r\nringbuf_usedlen = %d\r\n", BUFFER_USED_SIZE(data0_rx));
        //printf(" >> UART: [Rx] %u / [Tx] %u\r\n", get_data_transfer_bytecount(channel, SEG_UART_RX), get_data_transfer_bytecount(channel, SEG_UART_TX));
        //printf(" >> ETHER: [Rx] %u / [Tx] %u\r\n", get_data_transfer_bytecount(channel, SEG_ETHER_RX), get_data_transfer_bytecount(channel, SEG_ETHER_TX));
        //printf(" >> RINGBUFFER_USED_SIZE: [Rx] %d\r\n", BUFFER_USED_SIZE(data0_rx));
        tmp_timeflag_for_debug = 0;
    }
#endif
    
    // Firmware update: Do not run SEG process
//    if(firmware_update->fwup_flag == SEG_ENABLE) return;
    
    // Serial AT command mode enabled, initial settings
    if((opmode == DEVICE_GW_MODE) && (sw_modeswitch_at_mode_on == SEG_ENABLE))
    {
        // Socket disconnect (TCP only) / close
        process_socket_termination(sock, 100);
        if(get_wiz_tls_init_state() == ENABLE)
        {
            wiz_tls_deinit(&s2e_tlsContext);
            set_wiz_tls_init_state(DISABLE);
        }

        // Mode switch
        init_trigger_modeswitch(channel, get_segcp_uart(), DEVICE_AT_MODE);
        
        // Mode switch flag disabled
        sw_modeswitch_at_mode_on = SEG_DISABLE;
    }
    
    if(opmode == DEVICE_GW_MODE) 
    {
        switch(network_connection->working_mode)
        {
            case TCP_CLIENT_MODE:
                proc_SEG_tcp_client(uartNum, sock);
                break;
                
            case TCP_SERVER_MODE:
                proc_SEG_tcp_server(uartNum, sock);
                break;
            
            case TCP_MIXED_MODE:
                proc_SEG_tcp_mixed(uartNum, sock);
                break;
            
            case UDP_MODE:
                proc_SEG_udp(uartNum, sock);
                break;
            
#ifdef __USE_S2E_OVER_TLS__
            case SSL_TCP_CLIENT_MODE:
                proc_SEG_tcp_client_over_tls(uartNum, sock);
                break;
#endif

            case MQTT_CLIENT_MODE:
                proc_SEG_mqtt_client(uartNum, sock);
                break;

#ifdef __USE_S2E_OVER_TLS__                
            case MQTTS_CLIENT_MODE:
                proc_SEG_mqtt_client(uartNum, sock);
                break;    
#endif

            default:
                break;
        }
        
        // XON/XOFF Software flow control: Check the Buffer usage and Send the start/stop commands
        // [WIZnet Device] -> [Peer]
        if((serial_option[channel].flow_control == flow_xon_xoff) || (serial_option[channel].flow_control == flow_rts_cts))
        {
            check_uart_flow_control(uartNum, serial_option[channel].flow_control);
        }
    }
}

void set_device_status_all(teDEVSTATUS status)
{
    uint8_t i;

    for(i = 0; i < DEVICE_UART_CNT; i++)
    {
        set_device_status(i, status);
    }
}

void set_device_status(uint8_t channel, teDEVSTATUS status)
{
    struct __network_connection *network_connection = (struct __network_connection *)get_DevConfig_pointer()->network_connection;
    
    if(channel >= DEVICE_UART_CNT) channel = 0;

    switch(status)
    {
        case ST_OPEN:       // TCP connection state: disconnected (or UDP mode)
            network_connection[channel].working_state = ST_OPEN;
            break;
        
        case ST_CONNECT:    // TCP connection state: connected
            network_connection[channel].working_state = ST_CONNECT;
            break;
        
        case ST_UPGRADE:    // TCP connection state: disconnected
            network_connection[channel].working_state = ST_UPGRADE;
            break;
        
        case ST_ATMODE:     // TCP connection state: disconnected
            network_connection[channel].working_state = ST_ATMODE;
            break;
        
        case ST_UDP:        // UDP mode
            network_connection[channel].working_state = ST_UDP;
        default:
            break;
    }
    
    // Status indicator pins
    if(network_connection[channel].working_state == ST_CONNECT)
        set_connection_status_io(STATUS_TCPCONNECT_PIN, channel, ON); // Status I/O pin to low
    else
        set_connection_status_io(STATUS_TCPCONNECT_PIN, channel, OFF); // Status I/O pin to high
}

uint8_t get_device_status(uint8_t channel)
{
    struct __network_connection *network_connection = (struct __network_connection *)get_DevConfig_pointer()->network_connection;
    if(channel >= DEVICE_UART_CNT) channel = 0;

    return network_connection[channel].working_state;
}


void proc_SEG_udp(uint8_t uartNum, uint8_t sock)
{
    struct __network_connection *network_connection = (struct __network_connection *)get_DevConfig_pointer()->network_connection;
    struct __serial_common *serial_common = (struct __serial_common *)&get_DevConfig_pointer()->serial_common;
    struct __serial_data_packing *serial_data_packing = (struct __serial_data_packing *)get_DevConfig_pointer()->serial_data_packing;

    // Serial channel
    uint8_t channel = get_seg_channel(uartNum);
    
    // Serial communication mode
    uint8_t serial_mode = get_serial_communation_protocol(channel);

    // Socket state
    uint8_t state = getSn_SR(sock);

    switch(state)
    {
        case SOCK_UDP:
            if(serial_mode == SEG_SERIAL_PROTOCOL_NONE)
            {
                // Serial to Ethernet process
                if(get_uart_buffer_usedsize(uartNum) || u2e_size[channel])
                {
                    uart_to_ether(uartNum, sock);
                }

                // Ethernet to serial process
                if(getSn_RX_RSR(sock) || e2u_size[channel])
                {
                    ether_to_uart(uartNum, sock);
                }
            }
#ifdef __USE_MODBUS_PROTOCOL__
            else if(serial_mode == SEG_SERIAL_MODBUS_RTU)
            {
                // ModbusTCP to ModbusRTU process
                if(getSn_RX_RSR(sock))
                {
                    mbTCPtoRTU(uartNum);
                }

                // ModbusRTU to ModbusTCP process
                if(mb_state_rtu_finish == TRUE)
                {
                    mb_state_rtu_finish = FALSE;
                    mbRTUtoTCP(sock, network_connection[channel].remote_ip, network_connection[channel].remote_port);
                }
            }
            else if(serial_mode == SEG_SERIAL_MODBUS_ASCII)
            {
                // ModbusTCP to ModbusASCII process
                if(getSn_RX_RSR(sock))
                {
                    mbTCPtoASCII(uartNum);
                }

                // ModbusASCII to ModbusTCP process
                if(mb_state_ascii_finish == TRUE)
                {
                    mb_state_ascii_finish = FALSE;
                    mbASCIItoTCP(sock, network_connection[channel].remote_ip, network_connection[channel].remote_port);
                }
            }
#endif

            break;
            
        case SOCK_CLOSED:
            if(serial_mode == SEG_SERIAL_PROTOCOL_NONE)
            {
                // UART Ring buffer clear
                uart_rx_flush(uartNum);
            }
            
            u2e_size[channel] = 0;
            e2u_size[channel] = 0;

            if(socket(sock, Sn_MR_UDP, network_connection[channel].local_port, 0) == sock)
            {
                set_device_status(channel, ST_UDP);
                
                if(serial_data_packing->packing_time)
                {
                    modeswitch_gap_time = serial_data_packing->packing_time; // replace the GAP time (default: 500ms)
                }
                
                if(serial_common->serial_debug_en)
                {
                    printf(" > SEG:UDP_MODE:SOCKOPEN\r\n");
                }
            }
            break;
        default:
            break;
    }
}

void proc_SEG_tcp_client(uint8_t uartNum, uint8_t sock)
{
    struct __tcp_option *tcp_option = (struct __tcp_option *)get_DevConfig_pointer()->tcp_option;
    struct __network_connection *network_connection = (struct __network_connection *)get_DevConfig_pointer()->network_connection;
    struct __serial_data_packing *serial_data_packing = (struct __serial_data_packing *)get_DevConfig_pointer()->serial_data_packing;
    struct __serial_common *serial_common = (struct __serial_common *)&get_DevConfig_pointer()->serial_common;
    struct __serial_command *serial_command = (struct __serial_command *)&get_DevConfig_pointer()->serial_command;
    struct __device_option *device_option = (struct __device_option *)&(get_DevConfig_pointer()->device_option);

    uint16_t source_port;
    uint8_t destip[4] = {0, };
    uint16_t destport = 0;

    // Serial channel
    uint8_t channel = get_seg_channel(uartNum);
    
    // Serial communication mode
    uint8_t serial_mode = get_serial_communation_protocol(channel);

    // Socket state
    uint8_t state = getSn_SR(sock);

    switch(state)
    {
        case SOCK_INIT:
            if(reconnection_time[channel] >= tcp_option[channel].reconnection)
            {
                reconnection_time[channel] = 0; // reconnection time variable clear
                
                // TCP connect exception checker; e.g., dns failed / zero srcip ... and etc.
                if(check_tcp_connect_exception(channel) == ON) return;
                
                // TCP connect
                connect(sock, network_connection[channel].remote_ip, network_connection[channel].remote_port);
#ifdef _SEG_DEBUG_
                printf(" > SEG:TCP_CLIENT_MODE:CLIENT_CONNECTION\r\n");
#endif
            }
            break;
        
        case SOCK_ESTABLISHED:
            if(getSn_IR(sock) & Sn_IR_CON)
            {
                ///////////////////////////////////////////////////////////////////////////////////////////////////
                // S2E: TCP client mode initialize after connection established (only once)
                ///////////////////////////////////////////////////////////////////////////////////////////////////

                set_device_status(channel, ST_CONNECT);
                
                if(!inactivity_time[channel] && tcp_option[channel].inactivity)     enable_inactivity_timer[channel] = SEG_ENABLE;
                if(!keepalive_time[channel] && tcp_option[channel].keepalive_en)    enable_keepalive_timer[channel] = SEG_ENABLE;
                
                // TCP server mode only, This flag have to be enabled always at TCP client mode
                //if(option->pw_connect_en == SEG_ENABLE)   flag_connect_pw_auth = SEG_ENABLE;
                flag_connect_pw_auth[channel] = SEG_ENABLE;
                
                // Reconnection timer disable
                if(enable_reconnection_timer[channel] == SEG_ENABLE)
                {
                    enable_reconnection_timer[channel] = SEG_DISABLE;
                    reconnection_time[channel] = 0;
                }
                
                // Serial debug message printout
                if(serial_common->serial_debug_en)
                {
                    getsockopt(sock, SO_DESTIP, &destip);
                    getsockopt(sock, SO_DESTPORT, &destport);
                    PRT_SEG(" > SEG:CONNECTED TO - %d.%d.%d.%d : %d\r\n", destip[0], destip[1], destip[2], destip[3], destport);
                }
                
                if(serial_mode == SEG_SERIAL_PROTOCOL_NONE)
                {
                    // UART Ring buffer clear
                    uart_rx_flush(uartNum);
                }
                
                // Debug message enable flag: TCP client socket open
                isSocketOpen_TCPclient[channel] = OFF;
                
                // Send auto-message (If this option is set)
//                send_sid(sock, device_option->send_auto_msg);

                // Interrupt clear
                setSn_IR(sock, Sn_IR_CON);
            }

            if(serial_mode == SEG_SERIAL_PROTOCOL_NONE)
            {
                // Serial to Ethernet process
                if(get_uart_buffer_usedsize(uartNum) || u2e_size[channel])
                {
                    uart_to_ether(uartNum, sock);
                }

                // Ethernet to serial process
                if(getSn_RX_RSR(sock) || e2u_size[channel])
                {
                    ether_to_uart(uartNum, sock);
                }
            }
#if __USE_MODBUS_PROTOCOL__
            else if(serial_mode == SEG_SERIAL_MODBUS_RTU)
            {
                // ModbusTCP to ModbusRTU process
                if(getSn_RX_RSR(sock))
                {
                    mbTCPtoRTU(uartNum);
                }

                // ModbusRTU to ModbusTCP process
                if(mb_state_rtu_finish == TRUE)
                {
                    mb_state_rtu_finish = FALSE;
                    mbRTUtoTCP(sock, network_connection[channel].remote_ip, network_connection[channel].remote_port);
                }
            }
            else if(serial_mode == SEG_SERIAL_MODBUS_ASCII)
            {
                // ModbusTCP to ModbusASCII process
                if(getSn_RX_RSR(sock))
                {
                    mbTCPtoASCII(uartNum);
                }

                // ModbusASCII to ModbusTCP process
                if(mb_state_ascii_finish == TRUE)
                {
                    mb_state_ascii_finish = FALSE;
                    mbASCIItoTCP(sock, network_connection[channel].remote_ip, network_connection[channel].remote_port);
                }
            }
#endif
            
            // Check the inactivity timer
            if((enable_inactivity_timer[channel] == SEG_ENABLE) && (inactivity_time[channel] >= tcp_option[channel].inactivity))
            {
                //disconnect(sock);
                process_socket_termination(sock, 100);
                
                // Keep-alive timer disabled
                enable_keepalive_timer[channel] = DISABLE;
                keepalive_time[channel] = 0;
#ifdef _SEG_DEBUG_
                printf(" > INACTIVITY TIMER: TIMEOUT\r\n");
#endif
            }
            
            // Check the keep-alive timer
            if((tcp_option[channel].keepalive_en == SEG_ENABLE) && (enable_keepalive_timer[channel] == SEG_ENABLE))
            {
                // Send the first keep-alive packet
                if((flag_sent_first_keepalive[channel] == SEG_DISABLE) &&
                   (keepalive_time[channel] >= tcp_option[channel].keepalive_wait_time) &&
                   (tcp_option[channel].keepalive_wait_time != 0))
                {
#ifdef _SEG_DEBUG_
                    printf(" >> send_keepalive_packet_first [%d]\r\n", keepalive_time[channel]);
#endif
                    send_keepalive_packet_manual(sock); // <-> send_keepalive_packet_auto()
                    keepalive_time[channel] = 0;
                    
                    flag_sent_first_keepalive[channel] = SEG_ENABLE;
                }
                // Send the keep-alive packet periodically
                if((flag_sent_first_keepalive[channel] == SEG_ENABLE) &&
                   (keepalive_time[channel] >= tcp_option[channel].keepalive_retry_time) &&
                   (tcp_option[channel].keepalive_retry_time != 0))
                {
#ifdef _SEG_DEBUG_
                    printf(" >> send_keepalive_packet_manual [%d]\r\n", keepalive_time[channel]);
#endif
                    send_keepalive_packet_manual(sock);
                    keepalive_time[channel] = 0;
                }
            }
            
            break;
        
        case SOCK_CLOSE_WAIT:
            if(serial_mode == SEG_SERIAL_PROTOCOL_NONE)
            {
                while(getSn_RX_RSR(sock) || e2u_size[channel])
                {
                    ether_to_uart(uartNum, sock); // receive remaining packets
                }
            }

            process_socket_termination(sock, 100);
            break;
        
        case SOCK_FIN_WAIT:
        case SOCK_CLOSED:
            set_device_status(channel, ST_OPEN);
            reset_SEG_timeflags(channel);

            u2e_size[channel] = 0;
            e2u_size[channel] = 0;
            
            if(network_connection[channel].fixed_local_port)
            {
                source_port = network_connection[channel].local_port;
            }
            else
            {
                source_port = get_tcp_any_port(channel);
            }


#ifdef _SEG_DEBUG_
            printf(" > TCP CLIENT: client_any_port = %d\r\n", client_any_port[channel]);
#endif
            // ## 20180208 Added by Eric, TCP Connect function in TCP client/mixed mode operates in non-block mode
            if(socket(sock, Sn_MR_TCP, source_port, (SF_TCP_NODELAY | SF_IO_NONBLOCK)) == sock)
            {
                // Replace the command mode switch code GAP time (default: 500ms)
                if((serial_command->serial_command == SEG_ENABLE) && serial_data_packing[channel].packing_time)
                {
                    modeswitch_gap_time = serial_data_packing[channel].packing_time;
                }
                
                // Enable the reconnection Timer
                if((enable_reconnection_timer[channel] == SEG_DISABLE) && tcp_option[channel].reconnection)
                {
                    enable_reconnection_timer[channel] = SEG_ENABLE;
                }
                
                if(serial_common->serial_debug_en)
                {
                    if(isSocketOpen_TCPclient[channel] == OFF)
                    {
                        printf(" > SEG:TCP_CLIENT_MODE:SOCKOPEN\r\n");
                        isSocketOpen_TCPclient[channel] = ON;
                    }
                }
            }
            break;
            
        default:
            break;
    }
}

#ifdef __USE_S2E_OVER_TLS__
void proc_SEG_tcp_client_over_tls(uint8_t uartNum, uint8_t sock)
{
    struct __tcp_option *tcp_option = (struct __tcp_option *)get_DevConfig_pointer()->tcp_option;
    struct __network_connection *network_connection = (struct __network_connection *)get_DevConfig_pointer()->network_connection;
    struct __serial_data_packing *serial_data_packing = (struct __serial_data_packing *)get_DevConfig_pointer()->serial_data_packing;
    struct __serial_common *serial_common = (struct __serial_common *)&get_DevConfig_pointer()->serial_common;
    struct __serial_command *serial_command = (struct __serial_command *)&get_DevConfig_pointer()->serial_command;
    struct __device_option *device_option = (struct __device_option *)&(get_DevConfig_pointer()->device_option);

    uint16_t source_port;
    uint8_t destip[4] = {0, };
    uint16_t destport = 0;

    // Serial channel
    uint8_t channel = get_seg_channel(uartNum);

    // Serial communication mode
    uint8_t serial_mode = get_serial_communation_protocol(channel);

    // Socket state
    uint8_t state = getSn_SR(sock);

    static const char * host;
    int tls_ret = 0;

//    if(device_option->s2e_tls_enable != ENABLE) return;

    switch(state)
    {
        case SOCK_INIT:
            if(reconnection_time[channel] >= tcp_option[channel].reconnection)
            {
                reconnection_time[channel] = 0; // reconnection time variable clear

                // TCP connect exception checker; e.g., dns failed / zero srcip ... and etc.
                if(check_tcp_connect_exception(channel) == ON) return;

                // TCP connect
                tls_ret = wiz_tls_connect_timeout(&s2e_tlsContext,
                                                  (char *)network_connection[channel].remote_ip,
                                                  (unsigned int)network_connection[channel].remote_port,
                                                  S2E_OVER_TLS_CONNECT_TIME_MS);

                if(tls_ret != 0) // TLS connection failed
                {
                    process_socket_termination(sock, 100); // including disconnect(sock) function
                    if(get_wiz_tls_init_state() == ENABLE)
                    {
                        wiz_tls_deinit(&s2e_tlsContext);
                        set_wiz_tls_init_state(DISABLE);
                    }
                    if(serial_common->serial_debug_en)
                        PRT_SEG(" > SEG:TCP_CLIENT_OVER_TLS_MODE: CONNECTION FAILED\r\n");
                    break;
                }
                PRT_SEG(" > SEG:TCP_CLIENT_OVER_TLS_MODE: CLIENT CONNECTED\r\n");
            }
            break;

        case SOCK_ESTABLISHED:
            if(getSn_IR(sock) & Sn_IR_CON)
            {
                ///////////////////////////////////////////////////////////////////////////////////////////////////
                // S2E: TCP client mode initialize after connection established (only once)
                ///////////////////////////////////////////////////////////////////////////////////////////////////

                set_device_status(channel, ST_CONNECT);

                if(!inactivity_time[channel] && tcp_option[channel].inactivity)     enable_inactivity_timer[channel] = SEG_ENABLE;
                if(!keepalive_time[channel] && tcp_option[channel].keepalive_en)    enable_keepalive_timer[channel] = SEG_ENABLE;

                // TCP server mode only, This flag have to be enabled always at TCP client mode
                //if(option->pw_connect_en == SEG_ENABLE)   flag_connect_pw_auth = SEG_ENABLE;
                flag_connect_pw_auth[channel] = SEG_ENABLE;

                // Reconnection timer disable
                if(enable_reconnection_timer[channel] == SEG_ENABLE)
                {
                    enable_reconnection_timer[channel] = SEG_DISABLE;
                    reconnection_time[channel] = 0;
                }

                // Serial debug message printout
                if(serial_common->serial_debug_en)
                {
                    getsockopt(sock, SO_DESTIP, &destip);
                    getsockopt(sock, SO_DESTPORT, &destport);
                    PRT_SEG(" > SEG:CONNECTED TO - %d.%d.%d.%d : %d\r\n", destip[0], destip[1], destip[2], destip[3], destport);
                }

                if(serial_mode == SEG_SERIAL_PROTOCOL_NONE)
                {
                    // UART Ring buffer clear
                    uart_rx_flush(uartNum);
                }

                // Debug message enable flag: TCP client socket open
                isSocketOpen_TCPclient[channel] = OFF;

                // Interrupt clear
                setSn_IR(sock, Sn_IR_CON);
            }

            if(serial_mode == SEG_SERIAL_PROTOCOL_NONE)
            {
                // Serial to Ethernet process
                if(get_uart_buffer_usedsize(uartNum) || u2e_size[channel])
                {
                    uart_to_ether(uartNum, sock);
                }

                // Ethernet to serial process
                if(getSn_RX_RSR(sock) || e2u_size[channel])
                {
                    ether_to_uart(uartNum, sock);
                }
            }
            
            // Check the inactivity timer
            if((enable_inactivity_timer[channel] == SEG_ENABLE) && (inactivity_time[channel] >= tcp_option[channel].inactivity))
            {
                //disconnect(sock);
                wiz_tls_close_notify(&s2e_tlsContext);
                process_socket_termination(sock, 100); // including disconnect(sock) function

                if(get_wiz_tls_init_state() == ENABLE)
                {
                    wiz_tls_deinit(&s2e_tlsContext);
                    set_wiz_tls_init_state(DISABLE);
                }
                break;

                // Keep-alive timer disabled
                enable_keepalive_timer[channel] = DISABLE;
                keepalive_time[channel] = 0;
                PRT_SEG(" > INACTIVITY TIMER: TIMEOUT\r\n");
            }

            // Check the keep-alive timer
            if((tcp_option[channel].keepalive_en == SEG_ENABLE) && (enable_keepalive_timer[channel] == SEG_ENABLE))
            {
                // Send the first keee-alive packet
                if((flag_sent_first_keepalive[channel] == SEG_DISABLE) &&
                   (keepalive_time[channel] >= tcp_option[channel].keepalive_wait_time) &&
                   (tcp_option[channel].keepalive_wait_time != 0))
                {
                    PRT_SEG(" >> send_keepalive_packet_first [%d]\r\n", keepalive_time[channel]);
                    send_keepalive_packet_manual(sock); // <-> send_keepalive_packet_auto()
                    keepalive_time[channel] = 0;

                    flag_sent_first_keepalive[channel] = SEG_ENABLE;
                }
                // Send the keep-alive packet periodically
                if((flag_sent_first_keepalive[channel] == SEG_ENABLE) &&
                   (keepalive_time[channel] >= tcp_option[channel].keepalive_retry_time) &&
                   (tcp_option[channel].keepalive_retry_time != 0))
                {
                    PRT_SEG(" >> send_keepalive_packet_manual [%d]\r\n", keepalive_time[channel]);
                    send_keepalive_packet_manual(sock);
                    keepalive_time[channel] = 0;
                }
            }

            break;

        case SOCK_CLOSE_WAIT:
            if(serial_mode == SEG_SERIAL_PROTOCOL_NONE)
            {
                while(getSn_RX_RSR(sock) || e2u_size[channel])
                {
                    ether_to_uart(uartNum, sock); // receive remaining packets
                }
            }

            wiz_tls_close_notify(&s2e_tlsContext);
            process_socket_termination(sock, 100); // including disconnect(sock) function

            if(get_wiz_tls_init_state() == ENABLE)
            {
                wiz_tls_deinit(&s2e_tlsContext);
                set_wiz_tls_init_state(DISABLE);
            }
            break;

        case SOCK_FIN_WAIT:
        case SOCK_CLOSED:
            close(sock);
            if(get_wiz_tls_init_state() != ENABLE)
            {

                host = (const char *)network_connection[channel].dns_domain_name;
                PRT_SEG("host = %s\r\n", host);
            }
            else
            {
                wiz_tls_deinit(&s2e_tlsContext);
                set_wiz_tls_init_state(DISABLE);
            }

            if(wiz_tls_init(&s2e_tlsContext, sock, host) > 0)
            {
                set_device_status(channel, ST_OPEN);
                reset_SEG_timeflags(channel);

                u2e_size[channel] = 0;
                e2u_size[channel] = 0;

                if(network_connection[channel].fixed_local_port)
                {
                    source_port = network_connection[channel].local_port;
                }
                else
                {
                    source_port = get_tcp_any_port(channel);
                }

                PRT_SEG(" > TCP CLIENT over TLS: client_any_port = %d\r\n", client_any_port[channel]);
                if(wiz_tls_socket(&s2e_tlsContext, source_port) == sock)
                {
                    // Replace the command mode switch code GAP time (default: 500ms)
                    if((serial_command->serial_command == SEG_ENABLE) && serial_data_packing[channel].packing_time)
                    {
                        modeswitch_gap_time = serial_data_packing[channel].packing_time;
                    }

                    // Enable the reconnection Timer
                    if((enable_reconnection_timer[channel] == SEG_DISABLE) && tcp_option[channel].reconnection)
                    {
                        enable_reconnection_timer[channel] = SEG_ENABLE;
                    }

                    if(serial_common->serial_debug_en)
                    {
                        if(isSocketOpen_TCPclient[channel] == OFF)
                        {
                            PRT_SEG(" > SEG:TCP_CLIENT_OVER_TLS_MODE:SOCKOPEN\r\n");
                            isSocketOpen_TCPclient[channel] = ON;
                        }
                    }

                    set_wiz_tls_init_state(ENABLE);
                }
                else
                {
                    PRT_SEG("wiz_tls_socket() failed\r\n");
                    if(get_wiz_tls_init_state() == ENABLE)
                    {
                        wiz_tls_deinit(&s2e_tlsContext);
                        set_wiz_tls_init_state(DISABLE);
                    }
                }
            }
            else
            {
                PRT_SEGCP("wiz_tls_init() failed\r\n");

                if(get_wiz_tls_init_state() == ENABLE)
                {
                    wiz_tls_deinit(&s2e_tlsContext);
                    set_wiz_tls_init_state(DISABLE);
                }
            }
            break;

        default:
            break;
    }
}

void proc_SEG_mqtt_client(uint8_t uartNum, uint8_t sock)
{
    struct __tcp_option *tcp_option = (struct __tcp_option *)get_DevConfig_pointer()->tcp_option;
    struct __network_connection *network_connection = (struct __network_connection *)get_DevConfig_pointer()->network_connection;
    struct __serial_data_packing *serial_data_packing = (struct __serial_data_packing *)get_DevConfig_pointer()->serial_data_packing;
    struct __serial_common *serial_common = (struct __serial_common *)&get_DevConfig_pointer()->serial_common;
    struct __serial_command *serial_command = (struct __serial_command *)&get_DevConfig_pointer()->serial_command;
    struct __device_option *device_option = (struct __device_option *)&(get_DevConfig_pointer()->device_option);
    struct __mqtt_option *mqtt_option = (struct __mqtt_option *)&(get_DevConfig_pointer()->mqtt_option);
    struct __ssl_option *ssl_option = (struct __ssl_option *)&(get_DevConfig_pointer()->ssl_option);

    uint16_t source_port;
    uint8_t destip[4] = {0, };
    uint16_t destport = 0;

    // Serial channel
    uint8_t channel = get_seg_channel(uartNum);
    
    // Serial communication mode
    uint8_t serial_mode = get_serial_communation_protocol(channel);

    // Socket state
    uint8_t state = getSn_SR(sock);

    int ret;

    switch(state)
    {
        case SOCK_INIT:
            if(reconnection_time[channel] >= tcp_option[channel].reconnection)
            { 
                reconnection_time[channel] = 0; // reconnection time variable clear
                
                // MQTT connect exception checker; e.g., dns failed / zero srcip ... and etc.
                if(check_tcp_connect_exception(channel) == ON) return;

                if (network_connection->working_mode == MQTTS_CLIENT_MODE)
                {
                    wizchip_mqtt_tls_enable(WIZ_TLS_ENABLED);
                    if (ssl_option->root_ca_option == MBEDTLS_SSL_VERIFY_NONE)
                        NewNetwork_TLS(&mqtt_n, sock, network_connection[channel].remote_ip, channel);
                    else
                        NewNetwork_TLS(&mqtt_n, sock, network_connection[channel].dns_domain_name, channel);
                }
                else //MQTT_CLIENT_MODE
                {
                    wizchip_mqtt_tls_enable(WIZ_TLS_DISABLED);
                    NewNetwork(&mqtt_n, sock, channel);
                }
                MQTTClientInit(&mqtt_c, &mqtt_n, MQTT_TIMEOUT_MS, g_send_mqtt_buf, DATA_BUF_SIZE, g_recv_buf, DATA_BUF_SIZE);

                mqtt_data.username.cstring = mqtt_option->user_name;
                mqtt_data.clientID.cstring = mqtt_option->client_id;
                mqtt_data.password.cstring = mqtt_option->password;
                
                // MQTT connect
                //connect(sock, network_connection[channel].remote_ip, network_connection[channel].remote_port);

                ret = ConnectNetwork(&mqtt_n, network_connection[channel].remote_ip, network_connection[channel].remote_port);
                if (ret < 0)
                {
                    PRT_SEG(" > SEG:MQTT_CLIENT_MODE:ConnectNetwork Err %d\r\n", ret);
                    wizchip_mqtt_network_disconnect(&mqtt_n);
                    break;
                }

                ret = MQTTConnect(&mqtt_c, &mqtt_data); 
                if (ret < 0)
                {
                    PRT_SEG(" > SEG:MQTT_CLIENT_MODE:MQTTConnect Err %d\r\n", ret);
                    wizchip_mqtt_network_disconnect(&mqtt_n);
                    break;
                }
                PRT_SEG(" > SEG:MQTT_CLIENT_MODE:MQTT_CONNECTION\r\n");

                if (mqtt_option->sub_topic_0[0] != 0 && mqtt_option->sub_topic_0[0] != 0xFF)
                {
					ret = MQTTSubscribe(&mqtt_c, (char *)mqtt_option->sub_topic_0, mqtt_option->qos, mqtt_subscribeMessageHandler);
					if (ret < 0)
					{
						PRT_SEG(" > SEG:MQTT_CLIENT_MODE:MQTTSubscribe Err %d\r\n", ret);
						wizchip_mqtt_network_disconnect(&mqtt_n);
						break;
					}
                }
                if (mqtt_option->sub_topic_1[0] != 0 && mqtt_option->sub_topic_1[0] != 0xFF)
				{
					ret = MQTTSubscribe(&mqtt_c, (char *)mqtt_option->sub_topic_1, mqtt_option->qos, mqtt_subscribeMessageHandler);
					if (ret < 0)
					{
						PRT_SEG(" > SEG:MQTT_CLIENT_MODE:MQTTSubscribe Err %d\r\n", ret);
						wizchip_mqtt_network_disconnect(&mqtt_n);
						break;
					}
				}

                if (mqtt_option->sub_topic_2[0] != 0 && mqtt_option->sub_topic_2[0] != 0xFF)
				{
					ret = MQTTSubscribe(&mqtt_c, (char *)mqtt_option->sub_topic_2, mqtt_option->qos, mqtt_subscribeMessageHandler);
					if (ret < 0)
					{
						PRT_SEG(" > SEG:MQTT_CLIENT_MODE:MQTTSubscribe Err %d\r\n", ret);
						wizchip_mqtt_network_disconnect(&mqtt_n);
						break;
					}
				}
                PRT_SEG(" > SEG:MQTT_CLIENT_MODE:MQTTSubscribed\r\n");
            }
            break;
        
        case SOCK_ESTABLISHED:
            if(getSn_IR(sock) & Sn_IR_CON)
            {
                ///////////////////////////////////////////////////////////////////////////////////////////////////
                // S2E: TCP client mode initialize after connection established (only once)
                ///////////////////////////////////////////////////////////////////////////////////////////////////

                set_device_status(channel, ST_CONNECT);
                
                if(!inactivity_time[channel] && tcp_option[channel].inactivity)     enable_inactivity_timer[channel] = SEG_ENABLE;
                if(!keepalive_time[channel] && tcp_option[channel].keepalive_en)    enable_keepalive_timer[channel] = SEG_ENABLE;
                
                // TCP server mode only, This flag have to be enabled always at TCP client mode
                //if(option->pw_connect_en == SEG_ENABLE)   flag_connect_pw_auth = SEG_ENABLE;
                flag_connect_pw_auth[channel] = SEG_ENABLE;
                
                // Reconnection timer disable
                if(enable_reconnection_timer[channel] == SEG_ENABLE)
                {
                    enable_reconnection_timer[channel] = SEG_DISABLE;
                    reconnection_time[channel] = 0;
                }
                
                // Serial debug message printout
                if(serial_common->serial_debug_en)
                {
                    getsockopt(sock, SO_DESTIP, &destip);
                    getsockopt(sock, SO_DESTPORT, &destport);
                    printf(" > SEG:CONNECTED TO - %d.%d.%d.%d : %d\r\n", destip[0], destip[1], destip[2], destip[3], destport);
                }
                
                if(serial_mode == SEG_SERIAL_PROTOCOL_NONE)
                {
                    // UART Ring buffer clear
                    uart_rx_flush(uartNum);
                }
                
                // Debug message enable flag: TCP client socket open
                isSocketOpen_TCPclient[channel] = OFF;
                
                // Send auto-message (If this option is set)
//                send_sid(sock, device_option->send_auto_msg);

                // Interrupt clear
                setSn_IR(sock, Sn_IR_CON);
            }
            
            MQTTYield(&mqtt_c, mqtt_option->keepalive);

            if(serial_mode == SEG_SERIAL_PROTOCOL_NONE)
            {
                // Serial to Ethernet process
                if(get_uart_buffer_usedsize(uartNum) || u2e_size[channel])
                {
                    uart_to_ether(uartNum, sock);
                }

#if 0
                // Ethernet to serial process
                if(getSn_RX_RSR(sock) || e2u_size[channel])
                {
                    ether_to_uart(uartNum, sock);
                }
#endif
            }
            
            // Check the inactivity timer
            if((enable_inactivity_timer[channel] == SEG_ENABLE) && (inactivity_time[channel] >= tcp_option[channel].inactivity))
            {
                //disconnect(sock);

                wizchip_mqtt_network_disconnect(&mqtt_n);
                process_socket_termination(sock, 100);
                
                // Keep-alive timer disabled
                enable_keepalive_timer[channel] = DISABLE;
                keepalive_time[channel] = 0;
                PRT_SEG(" > INACTIVITY TIMER: TIMEOUT\r\n");
            }
            
            // Check the keep-alive timer
            if((tcp_option[channel].keepalive_en == SEG_ENABLE) && (enable_keepalive_timer[channel] == SEG_ENABLE))
            {
                // Send the first keep-alive packet
                if((flag_sent_first_keepalive[channel] == SEG_DISABLE) &&
                   (keepalive_time[channel] >= tcp_option[channel].keepalive_wait_time) &&
                   (tcp_option[channel].keepalive_wait_time != 0))
                {
#ifdef _SEG_DEBUG_
                    printf(" >> send_keepalive_packet_first [%d]\r\n", keepalive_time[channel]);
#endif
                    send_keepalive_packet_manual(sock); // <-> send_keepalive_packet_auto()
                    keepalive_time[channel] = 0;
                    
                    flag_sent_first_keepalive[channel] = SEG_ENABLE;
                }
                // Send the keep-alive packet periodically
                if((flag_sent_first_keepalive[channel] == SEG_ENABLE) &&
                   (keepalive_time[channel] >= tcp_option[channel].keepalive_retry_time) &&
                   (tcp_option[channel].keepalive_retry_time != 0))
                {
#ifdef _SEG_DEBUG_
                    printf(" >> send_keepalive_packet_manual [%d]\r\n", keepalive_time[channel]);
#endif
                    send_keepalive_packet_manual(sock);
                    keepalive_time[channel] = 0;
                }
            }
            
            break;
        
        case SOCK_CLOSE_WAIT:
#if 0
            if(serial_mode == SEG_SERIAL_PROTOCOL_NONE)
            {
                while(getSn_RX_RSR(sock) || e2u_size[channel])
                {
                    ether_to_uart(uartNum, sock); // receive remaining packets
                }
            }
#endif
            wizchip_mqtt_network_disconnect(&mqtt_n);
            process_socket_termination(sock, 100);
            break;
        
        case SOCK_FIN_WAIT:
        case SOCK_CLOSED:
            set_device_status(channel, ST_OPEN);
            reset_SEG_timeflags(channel);

            u2e_size[channel] = 0;
            e2u_size[channel] = 0;
            
            if(network_connection[channel].fixed_local_port)
            {
                source_port = network_connection[channel].local_port;
            }
            else
            {
                source_port = get_tcp_any_port(channel);
            }

            PRT_SEG(" > TCP CLIENT: client_any_port = %d\r\n", client_any_port[channel]);
            // ## 20180208 Added by Eric, TCP Connect function in TCP client/mixed mode operates in non-block mode
            if(socket(sock, Sn_MR_TCP, source_port, (SF_TCP_NODELAY | SF_IO_NONBLOCK)) == sock)
            {
                // Replace the command mode switch code GAP time (default: 500ms)
                if((serial_command->serial_command == SEG_ENABLE) && serial_data_packing[channel].packing_time)
                {
                    modeswitch_gap_time = serial_data_packing[channel].packing_time;
                }
                
                // Enable the reconnection Timer
                if((enable_reconnection_timer[channel] == SEG_DISABLE) && tcp_option[channel].reconnection)
                {
                    enable_reconnection_timer[channel] = SEG_ENABLE;
                }
                
                if(serial_common->serial_debug_en)
                {
                    if(isSocketOpen_TCPclient[channel] == OFF)
                    {
                        PRT_SEG(" > SEG:TCP_CLIENT_MODE:SOCKOPEN\r\n");
                        isSocketOpen_TCPclient[channel] = ON;
                    }
                }
            }
            break;
            
        default:
            break;
    }
}


int get_wiz_tls_init_state(void)
{
    return wiz_tls_init_state;
}


void set_wiz_tls_init_state(int state)
{
    if(state > 0)
    {
        wiz_tls_init_state = ENABLE;
    }
    else
    {
        wiz_tls_init_state = DISABLE;
    }
}

#endif


void proc_SEG_tcp_server(uint8_t uartNum, uint8_t sock)
{
    struct __tcp_option *tcp_option = (struct __tcp_option *)get_DevConfig_pointer()->tcp_option;
    struct __serial_common *serial_common = (struct __serial_common *)&(get_DevConfig_pointer()->serial_common);
    struct __network_connection *network_connection = (struct __network_connection *)get_DevConfig_pointer()->network_connection;
    struct __serial_command *serial_command = (struct __serial_command *)&get_DevConfig_pointer()->serial_command;
    struct __serial_data_packing *serial_data_packing = (struct __serial_data_packing *)get_DevConfig_pointer()->serial_data_packing;
    struct __device_option *device_option = (struct __device_option *)&(get_DevConfig_pointer()->device_option);
    
    uint8_t destip[4] = {0, };
    uint16_t destport = 0;

    // Serial channel
    uint8_t channel = get_seg_channel(uartNum);
    
    // Serial communication mode
    uint8_t serial_mode = get_serial_communation_protocol(channel);

    // Socket state
    uint8_t state = getSn_SR(sock);

    switch(state)
    {
        case SOCK_INIT:
            listen(sock); //Function call Immediately after socket open operation
            break;
        
        case SOCK_LISTEN:
            break;
        
        case SOCK_ESTABLISHED:
            if(getSn_IR(sock) & Sn_IR_CON)
            {
                ///////////////////////////////////////////////////////////////////////////////////////////////////
                // S2E: TCP server mode initialize after connection established (only once)
                ///////////////////////////////////////////////////////////////////////////////////////////////////
                set_device_status(channel, ST_CONNECT);

                if(!inactivity_time[channel] && tcp_option[channel].inactivity)
                {
                    enable_inactivity_timer[channel] = SEG_ENABLE;
                }
                //if(!keepalive_time && tcp_option[channel->keepalive_en)  enable_keepalive_timer[channel] = SEG_ENABLE;
                
                if(tcp_option[channel].pw_connect_en == SEG_DISABLE)
                {
                    flag_connect_pw_auth[channel] = SEG_ENABLE;		// TCP server mode only (+ mixed_server)
                }
                else
                {
                    // Connection password auth timer initialize
                    enable_connection_auth_timer[channel] = SEG_ENABLE;
                    connection_auth_time[channel] = 0;
                }
                
                // Serial debug message printout
                if(serial_common->serial_debug_en)
                {
                    getsockopt(sock, SO_DESTIP, &destip);
                    getsockopt(sock, SO_DESTPORT, &destport);
                    printf(" > SEG:CONNECTED FROM - %d.%d.%d.%d : %d\r\n",destip[0], destip[1], destip[2], destip[3], destport);
                }
                
                if(serial_mode == SEG_SERIAL_PROTOCOL_NONE)
                {
                    // UART Ring buffer clear
                    uart_rx_flush(uartNum);
                }

                // Send auto-message (If this option is set)
//                send_sid(sock, device_option->send_auto_msg);

                // Interrupt clear
                setSn_IR(sock, Sn_IR_CON);
            }
            
            if(serial_mode == SEG_SERIAL_PROTOCOL_NONE)
            {
                // Serial to Ethernet process
                if(get_uart_buffer_usedsize(uartNum) || u2e_size[channel])
                {
                    uart_to_ether(uartNum, sock);
                }
                // Ethernet to serial process
                if(getSn_RX_RSR(sock) || e2u_size[channel])
                {
                    ether_to_uart(uartNum, sock);
                }
            }

#ifdef __USE_MODBUS_PROTOCOL__
            else if(serial_mode == SEG_SERIAL_MODBUS_RTU)
            {
                // ModbusTCP to ModbusRTU process
                if(getSn_RX_RSR(sock))
                {
                    mbTCPtoRTU(uartNum);
                }

                // ModbusRTU to ModbusTCP process
                if(mb_state_rtu_finish == TRUE)
                {
                    mb_state_rtu_finish = FALSE;
                    mbRTUtoTCP(sock, network_connection->remote_ip, network_connection->remote_port);
                }
            }
            else if(serial_mode == SEG_SERIAL_MODBUS_ASCII)
            {
                // ModbusTCP to ModbusASCII process
                if(getSn_RX_RSR(sock))
                {
                    mbTCPtoASCII(uartNum);
                }

                // ModbusASCII to ModbusTCP process
                if(mb_state_ascii_finish == TRUE)
                {
                    mb_state_ascii_finish = FALSE;
                    mbASCIItoTCP(sock, network_connection->remote_ip, network_connection->remote_port);
                }
            }
#endif

            // Check the inactivity timer
            if((enable_inactivity_timer[channel] == SEG_ENABLE) && (inactivity_time[channel] >= tcp_option[channel].inactivity))
            {
                process_socket_termination(sock, 100);
                
                // Keep-alive timer disabled
                enable_keepalive_timer[channel] = DISABLE;
                keepalive_time[channel] = 0;
#ifdef _SEG_DEBUG_
                printf(" > INACTIVITY TIMER: TIMEOUT\r\n");
#endif
            }
            
            // Check the keee-alive timer
            if((tcp_option[channel].keepalive_en == SEG_ENABLE) && (enable_keepalive_timer[channel] == SEG_ENABLE))
            {
                // Send the first keee-alive packet
                if((flag_sent_first_keepalive[channel] == SEG_DISABLE) &&
                   (keepalive_time[channel] >= tcp_option[channel].keepalive_wait_time) &&
                   (tcp_option[channel].keepalive_wait_time != 0))
                {
#ifdef _SEG_DEBUG_
                    printf(" >> send_keepalive_packet_first [%d]\r\n", keepalive_time[channel]);
#endif
                    send_keepalive_packet_manual(sock); // <-> send_keepalive_packet_auto()
                    keepalive_time[channel] = 0;
                    
                    flag_sent_first_keepalive[channel] = SEG_ENABLE;
                }
                // Send the keee-alive packet periodically
                if((flag_sent_first_keepalive[channel] == SEG_ENABLE) &&
                   (keepalive_time[channel] >= tcp_option[channel].keepalive_retry_time) &&
                   (tcp_option[channel].keepalive_retry_time != 0))
                {
#ifdef _SEG_DEBUG_
                    printf(" >> send_keepalive_packet_manual [%d]\r\n", keepalive_time[channel]);
#endif
                    send_keepalive_packet_manual(sock);
                    keepalive_time[channel] = 0;
                }
            }
            
            // Check the connection password auth timer
            if(tcp_option[channel].pw_connect_en == SEG_ENABLE)
            {
                if((flag_connect_pw_auth[channel] == SEG_DISABLE) &&
                   (connection_auth_time[channel] >= MAX_CONNECTION_AUTH_TIME)) // timeout default: 5000ms (5 sec)
                {
                    process_socket_termination(sock, 100);
                    
                    enable_connection_auth_timer[channel] = DISABLE;
                    connection_auth_time[channel] = 0;
#ifdef _SEG_DEBUG_
                    printf(" > CONNECTION PW: AUTH TIMEOUT\r\n");
#endif
                }
            }
            break;
        
        case SOCK_CLOSE_WAIT:
            if(serial_mode == SEG_SERIAL_PROTOCOL_NONE)
            {
                while(getSn_RX_RSR(sock) || e2u_size[channel])
                {
                    ether_to_uart(uartNum, sock); // receive remaining packets
                }
            }
            process_socket_termination(sock, 100);
            break;
        
        case SOCK_FIN_WAIT:
        case SOCK_CLOSED:

            close(sock);
            set_device_status(channel, ST_OPEN);
            reset_SEG_timeflags(channel);
            
            u2e_size[channel] = 0;
            e2u_size[channel] = 0;

            if(socket(sock, Sn_MR_TCP, network_connection[channel].local_port, (SF_TCP_NODELAY | SF_IO_NONBLOCK)) == sock)
            {
                // Replace the command mode switch code GAP time (default: 500ms)
                if((serial_command->serial_command == SEG_ENABLE) && serial_data_packing[channel].packing_time)
                {
                    modeswitch_gap_time = serial_data_packing[channel].packing_time;
                }
                
                // TCP Server listen
                listen(sock);
                
                if(serial_common->serial_debug_en)
                {
                    printf(" > SEG:TCP_SERVER_MODE:SOCKOPEN\r\n");
                }
            }
            break;
            
        default:
            break;
    }
}


void proc_SEG_tcp_mixed(uint8_t uartNum, uint8_t sock)
{
    struct __tcp_option *tcp_option = (struct __tcp_option *)get_DevConfig_pointer()->tcp_option;
    struct __network_connection *network_connection = (struct __network_connection *)get_DevConfig_pointer()->network_connection;
    struct __serial_common *serial_common = (struct __serial_common *)&get_DevConfig_pointer()->serial_common;
    struct __serial_command *serial_command = (struct __serial_command *)&get_DevConfig_pointer()->serial_command;
    struct __serial_data_packing *serial_data_packing = (struct __serial_data_packing *)get_DevConfig_pointer()->serial_data_packing;
    struct __device_option *device_option = (struct __device_option *)&(get_DevConfig_pointer()->device_option);

    uint16_t source_port = 0;
    uint8_t destip[4] = {0, };
    uint16_t destport = 0;

    // Serial channel
    uint8_t channel = get_seg_channel(uartNum);
    
    // Serial communication mode
    uint8_t serial_mode = get_serial_communation_protocol(channel);

    // Socket state
    uint8_t state = getSn_SR(sock);

#ifdef MIXED_CLIENT_LIMITED_CONNECT
    static uint8_t reconnection_count;
#endif
    
    switch(state)
    {
        case SOCK_INIT:
            if(mixed_state == MIXED_CLIENT)
            {
                if(reconnection_time[channel] >= tcp_option[channel].reconnection)
                {
                    reconnection_time[channel] = 0; // reconnection time variable clear
                    
                    // TCP connect exception checker; e.g., dns failed / zero srcip ... and etc.
                    if(check_tcp_connect_exception(channel) == ON)
                    {
#ifdef MIXED_CLIENT_LIMITED_CONNECT
                        process_socket_termination(sock, 100);
                        reconnection_count = 0;
                        uart_rx_flush(uartNum);
                        mixed_state[channel] = MIXED_SERVER;
#endif
                        return;
                    }
                    
                    // TCP connect
                    connect(sock, network_connection[channel].remote_ip, network_connection[channel].remote_port);
                    
#ifdef MIXED_CLIENT_LIMITED_CONNECT
                    reconnection_count++;
                    if(reconnection_count >= MAX_RECONNECTION_COUNT)
                    {
                        process_socket_termination(sock, 100);
                        reconnection_count = 0;
                        uart_rx_flush(uartNum);
                        mixed_state[channel] = MIXED_SERVER;
                    }
    #ifdef _SEG_DEBUG_
                    if(reconnection_count != 0)
                        printf(" > SEG:TCP_MIXED_MODE:CLIENT_CONNECTION [%d]\r\n", reconnection_count);
                    else
                        printf(" > SEG:TCP_MIXED_MODE:CLIENT_CONNECTION_RETRY FAILED\r\n");
    #endif
#endif
                }
            }			
            break;
        
        case SOCK_LISTEN:
            // UART Rx interrupt detection in MIXED_SERVER mode
            // => Switch to MIXED_CLIENT mode
            if((mixed_state[channel] == MIXED_SERVER) && (get_uart_buffer_usedsize(uartNum)))
            {
                process_socket_termination(sock, 100);
                mixed_state[channel] = MIXED_CLIENT;
                
                reconnection_time[channel] = tcp_option[channel].reconnection; // rapid initial connection
            }
            break;
        
        case SOCK_ESTABLISHED:
            if(getSn_IR(sock) & Sn_IR_CON)
            {
                ///////////////////////////////////////////////////////////////////////////////////////////////////
                // S2E: TCP mixed (server or client) mode initialize after connection established (only once)
                ///////////////////////////////////////////////////////////////////////////////////////////////////
                set_device_status(channel, ST_CONNECT);
                
                if(!inactivity_time[channel] && tcp_option[channel].inactivity)     enable_inactivity_timer[channel] = SEG_ENABLE;
                if(!keepalive_time[channel] && tcp_option[channel].keepalive_en)    enable_keepalive_timer[channel] = SEG_ENABLE;
                
                // Connection Password option: TCP server mode only (+ mixed_server)
                if((tcp_option[channel].pw_connect_en == SEG_DISABLE) || (mixed_state[channel] == MIXED_CLIENT))
                {
                    flag_connect_pw_auth[channel] = SEG_ENABLE;
                }
                else if((mixed_state[channel] == MIXED_SERVER) && (flag_connect_pw_auth[channel] == SEG_DISABLE))
                {
                    // Connection password auth timer initialize
                    enable_connection_auth_timer[channel] = SEG_ENABLE;
                    connection_auth_time[channel] = 0;
                }
                
                // Serial debug message printout
                if(serial_common->serial_debug_en)
                {
                    getsockopt(sock, SO_DESTIP, &destip);
                    getsockopt(sock, SO_DESTPORT, &destport);
                    
                    if(mixed_state[channel] == MIXED_SERVER)
                        printf(" > SEG:CONNECTED FROM - %d.%d.%d.%d : %d\r\n", destip[0], destip[1], destip[2], destip[3], destport);
                    else
                        printf(" > SEG:CONNECTED TO - %d.%d.%d.%d : %d\r\n", destip[0], destip[1], destip[2], destip[3], destport);
                }

                // Mixed mode option init
                if(mixed_state[channel] == MIXED_SERVER)
                {
                    if(serial_mode == SEG_SERIAL_PROTOCOL_NONE)
                    {
                        // UART Ring buffer clear
                        uart_rx_flush(uartNum);
                    }
                }
                else if(mixed_state[channel] == MIXED_CLIENT)
                {
                    // Mixed-mode flag switching in advance
                    mixed_state[channel] = MIXED_SERVER;
                }
                
#ifdef MIXED_CLIENT_LIMITED_CONNECT
                reconnection_count = 0;
#endif
                
                // Send auto-message (If this option is set)
//                send_sid(sock, device_option->send_auto_msg);

                // Interrupt clear
                setSn_IR(sock, Sn_IR_CON);
            }
            
            if(serial_mode == SEG_SERIAL_PROTOCOL_NONE)
            {
                // Serial to Ethernet process
                if(get_uart_buffer_usedsize(uartNum) || u2e_size[channel])
                {
                    uart_to_ether(uartNum, sock);
                }
                // Ethernet to serial process
                if(getSn_RX_RSR(sock) || e2u_size[channel])
                {
                    ether_to_uart(uartNum, sock);
                }
            }

#ifdef __USE_MODBUS_PROTOCOL__
            else if(serial_mode == SEG_SERIAL_MODBUS_RTU)
            {
                // ModbusTCP to ModbusRTU process
                if(getSn_RX_RSR(sock))
                {
                    mbTCPtoRTU(uartNum);
                }

                // ModbusRTU to ModbusTCP process
                if(mb_state_rtu_finish == TRUE)
                {
                    mb_state_rtu_finish = FALSE;
                    mbRTUtoTCP(sock, network_connection[channel].remote_ip, network_connection[channel].remote_port);
                }
            }
            else if(serial_mode == SEG_SERIAL_MODBUS_ASCII)
            {
                // ModbusTCP to ModbusASCII process
                if(getSn_RX_RSR(sock))
                {
                    mbTCPtoASCII(uartNum);
                }

                // ModbusASCII to ModbusTCP process
                if(mb_state_ascii_finish == TRUE)
                {
                    mb_state_ascii_finish = FALSE;
                    mbASCIItoTCP(sock, network_connection[channel].remote_ip, network_connection[channel].remote_port);
                }
            }
#endif
            
            // Check the inactivity timer
            if((enable_inactivity_timer[channel] == SEG_ENABLE) && (inactivity_time[channel] >= tcp_option[channel].inactivity))
            {
                //disconnect(sock);
                process_socket_termination(sock, 100);
                
                // Keep-alive timer disabled
                enable_keepalive_timer[channel] = DISABLE;
                keepalive_time[channel] = 0;
#ifdef _SEG_DEBUG_
                printf(" > INACTIVITY TIMER: TIMEOUT\r\n");
#endif
                // TCP mixed mode state transition: initial state
                mixed_state[channel] = MIXED_SERVER;
            }
            
            // Check the keee-alive timer
            if((tcp_option[channel].keepalive_en == SEG_ENABLE) && (enable_keepalive_timer[channel] == SEG_ENABLE))
            {
                // Send the first keee-alive packet
                if((flag_sent_first_keepalive[channel] == SEG_DISABLE) &&
                   (keepalive_time[channel] >= tcp_option[channel].keepalive_wait_time) &&
                   (tcp_option[channel].keepalive_wait_time != 0))
                {
#ifdef _SEG_DEBUG_
                    printf(" >> send_keepalive_packet_first [%d]\r\n", keepalive_time[channel]);
#endif
                    send_keepalive_packet_manual(sock); // <-> send_keepalive_packet_auto()
                    keepalive_time[channel] = 0;
                    
                    flag_sent_first_keepalive[channel] = SEG_ENABLE;
                }
                // Send the keee-alive packet periodically
                if((flag_sent_first_keepalive[channel] == SEG_ENABLE) &&
                   (keepalive_time[channel] >= tcp_option[channel].keepalive_retry_time) &&
                   (tcp_option[channel].keepalive_retry_time != 0))
                {
#ifdef _SEG_DEBUG_
                    printf(" >> send_keepalive_packet_manual [%d]\r\n", keepalive_time[channel]);
#endif
                    send_keepalive_packet_manual(sock);
                    keepalive_time[channel] = 0;
                }
            }
            
            // Check the connection password auth timer
            if((mixed_state[channel] == MIXED_SERVER) && (tcp_option[channel].pw_connect_en == SEG_ENABLE))
            {
                if((flag_connect_pw_auth[channel] == SEG_DISABLE) && (connection_auth_time[channel] >= MAX_CONNECTION_AUTH_TIME)) // timeout default: 5000ms (5 sec)
                {
                    process_socket_termination(sock, 100);
                    
                    enable_connection_auth_timer[channel] = DISABLE;
                    connection_auth_time[channel] = 0;
#ifdef _SEG_DEBUG_
                    printf(" > CONNECTION PW: AUTH TIMEOUT\r\n");
#endif
                }
            }
            break;
        
        case SOCK_CLOSE_WAIT:
            if(serial_mode == SEG_SERIAL_PROTOCOL_NONE)
            {
                while(getSn_RX_RSR(sock) || e2u_size[channel])
                {
                    ether_to_uart(uartNum, sock); // receive remaining packets
                }
            }
            process_socket_termination(sock, 100);
            break;
        
        case SOCK_FIN_WAIT:
        case SOCK_CLOSED:
            set_device_status(channel, ST_OPEN);

            if(mixed_state[channel] == MIXED_SERVER) // MIXED_SERVER
            {
                reset_SEG_timeflags(channel);
                
                u2e_size[channel] = 0;
                e2u_size[channel] = 0;
                
                // ## 20180208 Added by Eric, TCP Connect function in TCP client/mixed mode operates in non-block mode
                if(socket(sock, Sn_MR_TCP, network_connection[channel].local_port, (SF_TCP_NODELAY | SF_IO_NONBLOCK)) == sock)
                {
                    // Replace the command mode switch code GAP time (default: 500ms)
                    if((serial_command->serial_command == SEG_ENABLE) && serial_data_packing[channel].packing_time)
                    {
                        modeswitch_gap_time = serial_data_packing[channel].packing_time;
                    }
                    
                    // TCP Server listen
                    listen(sock);
                    
                    if(serial_common->serial_debug_en)
                    {
                        printf(" > SEG:TCP_MIXED_MODE:SERVER_SOCKOPEN\r\n");
                    }
                }
            }
            else	// MIXED_CLIENT
            {
                e2u_size[channel] = 0;

                if(network_connection[channel].fixed_local_port)
                {
                    source_port = network_connection[channel].local_port;
                }
                else
                {
                    source_port = get_tcp_any_port(channel);
                }


#ifdef _SEG_DEBUG_
                printf(" > TCP CLIENT: any_port = %d\r\n", source_port);
#endif		
                if(socket(sock, Sn_MR_TCP, source_port, (SF_TCP_NODELAY | SF_IO_NONBLOCK)) == sock)
                {
                    // Replace the command mode switch code GAP time (default: 500ms)
                    if((serial_command->serial_command == SEG_ENABLE) && serial_data_packing[channel].packing_time)
                    {
                        modeswitch_gap_time = serial_data_packing[channel].packing_time;
                    }
                    
                    // Enable the reconnection Timer
                    if((enable_reconnection_timer[channel] == SEG_DISABLE) && tcp_option[channel].reconnection)
                    {
                        enable_reconnection_timer[channel] = SEG_ENABLE;
                    }
                    
                    if(serial_common->serial_debug_en)
                    {
                        printf(" > SEG:TCP_MIXED_MODE:CLIENT_SOCKOPEN\r\n");
                    }
                }
            }
            break;
            
        default:
            break;
    }
}

void uart_to_ether(uint8_t uartNum, uint8_t sock)
{
    struct __network_connection *network_connection = (struct __network_connection *)get_DevConfig_pointer()->network_connection;
    struct __serial_common *serial_common = (struct __serial_common *)&get_DevConfig_pointer()->serial_common;
    struct __tcp_option *tcp_option = (struct __tcp_option *)get_DevConfig_pointer()->tcp_option;

    struct __device_option *device_option = (struct __device_option *)&(get_DevConfig_pointer()->device_option);
    struct __mqtt_option *mqtt_option = (struct __mqtt_option *)&(get_DevConfig_pointer()->mqtt_option);

    uint16_t len;
    int16_t sent_len = 0;

    uint8_t channel = get_seg_channel(uartNum);

#if ((DEVICE_BOARD_NAME == WIZ2000_MB))
    if(get_phylink() != PHY_LINK_ON) return; // PHY link down
#endif
    
    // UART ring buffer -> user's buffer
    len = get_serial_data(uartNum);

    if(len > 0)
    {
        add_data_transfer_bytecount(channel, SEG_UART_RX, len);

        if((serial_common->serial_debug_en == SEG_DEBUG_S2E) || (serial_common->serial_debug_en == SEG_DEBUG_ALL))
        {
            debugSerial_dataTransfer(g_send_buf, len, SEG_DEBUG_S2E);
        }
        
        switch(getSn_SR(sock))
        {
            case SOCK_UDP: // UDP_MODE
                if((network_connection[channel].remote_ip[0] == 0x00) &&
                   (network_connection[channel].remote_ip[1] == 0x00) &&
                   (network_connection[channel].remote_ip[2] == 0x00) &&
                   (network_connection[channel].remote_ip[3] == 0x00))
                {
                    if((peerip[channel][0] == 0x00) && (peerip[channel][1] == 0x00) && (peerip[channel][2] == 0x00) && (peerip[channel][3] == 0x00))
                    {
                        if(serial_common->serial_debug_en)
                            PRT_SEG(" > SEG:UDP_MODE:DATA SEND FAILED - UDP Peer IP/Port required (0.0.0.0)\r\n");
                    }
                    else
                    {
                        // UDP 1:N mode
                        sent_len = (int16_t)sendto(sock, g_send_buf, len, peerip[channel], peerport[channel]);
                    }
                }
                else
                {
                    // UDP 1:1 mode
                    sent_len = (int16_t)sendto(sock, g_send_buf, len, network_connection[channel].remote_ip, network_connection[channel].remote_port);
                }
                
                if(sent_len > 0) u2e_size[channel]-=sent_len;
                
                break;
            
            case SOCK_ESTABLISHED: // TCP_SERVER_MODE, TCP_CLIENT_MODE, TCP_MIXED_MODE
            case SOCK_CLOSE_WAIT:
                // Connection password is only checked in the TCP SERVER MODE / TCP MIXED MODE (MIXED_SERVER)
                if(flag_connect_pw_auth[channel] == SEG_ENABLE)
                {
                    //if(device_option->s2e_tls_enable)
                    if (network_connection->working_mode == SSL_TCP_CLIENT_MODE)
                    {
                        sent_len = wiz_tls_write(&s2e_tlsContext, g_send_buf, len);
                    }
                    else if (network_connection->working_mode == MQTT_CLIENT_MODE || network_connection->working_mode == MQTTS_CLIENT_MODE)
                    {
                        sent_len = wizchip_mqtt_publish(&mqtt_c, mqtt_option->pub_topic, mqtt_option->qos, g_send_buf, len);
                    }
                    else
                    {
                        sent_len = (int16_t)send(sock, g_send_buf, len);
                    }

                    if(sent_len > 0) u2e_size[channel]-=sent_len;
                    
                    add_data_transfer_bytecount(channel, SEG_UART_TX, len);
                    
                    if(tcp_option->keepalive_en == ENABLE)
                    {
                        if(flag_sent_first_keepalive[channel] == DISABLE)
                        {
                            enable_keepalive_timer[channel] = SEG_ENABLE;
                        }
                        else
                        {
                            flag_sent_first_keepalive[channel] = SEG_DISABLE;
                        }
                        keepalive_time[channel] = 0;
                    }
                }
                break;
            
            case SOCK_LISTEN:
                u2e_size[channel] = 0;
                return;
            
            default:
                break;
        }
    }
    
    inactivity_time[channel] = 0;
    //flag_serial_input_time_elapse = SEG_DISABLE; // this flag is cleared in the 'Data packing delimiter:time' checker routine
}

uint16_t get_serial_data(uint8_t uartNum)
{
    struct __serial_data_packing *serial_data_packing = (struct __serial_data_packing *)get_DevConfig_pointer()->serial_data_packing;

    uint16_t i;
    uint16_t len;
    //int32_t chch;

    // Serial channel
    uint8_t channel = get_seg_channel(uartNum);

    len = get_uart_buffer_usedsize(uartNum);
    
    if((len + u2e_size[channel]) >= DATA_BUF_SIZE) // Avoiding u2e buffer (g_send_buf) overflow
    {
        /* Checking Data packing option: character delimiter */
        if((serial_data_packing[channel].packing_delimiter[0] != 0x00) && (len == 1))
        {
            g_send_buf[u2e_size[channel]] = (uint8_t)uart_getc(uartNum);
            if(serial_data_packing[channel].packing_delimiter[0] == g_send_buf[u2e_size[channel]])
            {
                return u2e_size[channel];
            }
        }
        
        // serial data length value update for avoiding u2e buffer overflow
        len = DATA_BUF_SIZE - u2e_size[channel];
    }
    
    if((!serial_data_packing[channel].packing_time) &&
       (!serial_data_packing[channel].packing_size) &&
       (!serial_data_packing[channel].packing_delimiter[0])) // No Data Packing tiem / size / delimiters.
    {
        // ## 20150427 bugfix: Incorrect serial data storing (UART ring buffer to g_send_buf)
        for(i = 0; i < len; i++)
        {
            g_send_buf[u2e_size[channel]++] = (uint8_t)uart_getc(uartNum);

#ifdef _UART_DEBUG_
            //printf("[%d]%.2x ", (u2e_size[channel]-1), g_send_buf[(u2e_size[channel]-1)]);
            //uart_putc(DEBUG_UART_PORTNUM, ch);// ## UART echo; for debugging
#endif
        }
        
        return u2e_size[channel];
    }
    else
    {
        /* Checking Data packing options */
        for(i = 0; i < len; i++)
        {
            g_send_buf[u2e_size[channel]++] = (uint8_t)uart_getc(uartNum);
            
            // Packing delimiter: character option
            if((serial_data_packing[channel].packing_delimiter[0] != 0x00) &&
               (serial_data_packing[channel].packing_delimiter[0] == g_send_buf[u2e_size[channel] - 1]))
            {
                return u2e_size[channel];
            }
            
            // Packing delimiter: size option
            if((serial_data_packing[channel].packing_size != 0) && (serial_data_packing[channel].packing_size == u2e_size[channel]))
            {
                return u2e_size[channel];
            }
        }
    }
    
    // Packing delimiter: time option
    if((serial_data_packing->packing_time != 0) && (u2e_size[channel] != 0) && (flag_serial_input_time_elapse[channel]))
    {
        if(get_uart_buffer_usedsize(uartNum) == 0)
            flag_serial_input_time_elapse[channel] = SEG_DISABLE; // ##
        
        return u2e_size[channel];
    }
    
    return 0;
}

void ether_to_uart(uint8_t uartNum, uint8_t sock)
{
    struct __serial_option *serial_option = (struct __serial_option *)get_DevConfig_pointer()->serial_option;
    struct __serial_common *serial_common = (struct __serial_common *)&(get_DevConfig_pointer()->serial_common);
    struct __network_connection *network_connection = (struct __network_connection *)get_DevConfig_pointer()->network_connection;
    struct __tcp_option *tcp_option = (struct __tcp_option *)get_DevConfig_pointer()->tcp_option;

    struct __device_option *device_option = (struct __device_option *)&(get_DevConfig_pointer()->device_option);

    uint16_t len;
    uint16_t i;

    // Serial channel
    uint8_t channel = get_seg_channel(uartNum);

    if(serial_option->flow_control == flow_rts_cts)
    {
#ifdef __USE_GPIO_HARDWARE_FLOWCONTROL__
        if(get_uart_cts_pin(uartNum) != UART_CTS_LOW) return; // DATA0 (RS-232) only
#else
        ; // check the CTS reg
#endif
    }

    // H/W Socket buffer -> User's buffer
    len = getSn_RX_RSR(sock);
    if(len > DATA_BUF_SIZE) len = DATA_BUF_SIZE; // avoiding buffer overflow
    
    if(len > 0)
    {
        switch(getSn_SR(sock))
        {
            case SOCK_UDP: // UDP_MODE
                e2u_size[channel] = recvfrom(sock, g_recv_buf, len, peerip[channel], &peerport[channel]);
                
                if(memcmp(peerip_tmp[channel], peerip[channel], 4) !=  0)
                {
                    memcpy(peerip_tmp[channel], peerip[channel], 4);
                    if(serial_common->serial_debug_en)
                    {
                        printf(" > UDP Peer IP/Port: %d.%d.%d.%d : %d\r\n",
                                peerip[channel][0], peerip[channel][1], peerip[channel][2], peerip[channel][3], peerport[channel]);
                    }
                }
                break;
            
            case SOCK_ESTABLISHED: // TCP_SERVER_MODE, TCP_CLIENT_MODE, TCP_MIXED_MODE
            case SOCK_CLOSE_WAIT:
                //if(device_option->s2e_tls_enable)
                if (network_connection->working_mode == SSL_TCP_CLIENT_MODE)
                {
                    e2u_size[channel] = wiz_tls_read(&s2e_tlsContext, g_recv_buf, len);
                }
                else if ((network_connection->working_mode == MQTT_CLIENT_MODE) || (network_connection->working_mode == MQTTS_CLIENT_MODE))
                {
                    break;
                }
                else
                {
                    e2u_size[channel] = recv(sock, g_recv_buf, len);
                }
                break;
            
            default:
                break;
        }
        
        inactivity_time[channel] = 0;
        keepalive_time[channel] = 0;
        flag_sent_first_keepalive[channel] = DISABLE;
        
        add_data_transfer_bytecount(channel, SEG_ETHER_RX, e2u_size[channel]);
    }
    
    if((network_connection[channel].working_state == TCP_SERVER_MODE) ||
      ((network_connection[channel].working_state == TCP_MIXED_MODE) && (mixed_state[channel] == MIXED_SERVER)))
    {
        // Connection password authentication
        if((tcp_option[channel].pw_connect_en == SEG_ENABLE) && (flag_connect_pw_auth[channel] == SEG_DISABLE))
        {
            if(check_connect_pw_auth(channel, g_recv_buf, len) == SEG_ENABLE)
            {
                flag_connect_pw_auth[channel] = SEG_ENABLE;
            }
            else
            {
                flag_connect_pw_auth[channel] = SEG_DISABLE;
            }
            
            e2u_size[channel] = 0;
            
            if(flag_connect_pw_auth[channel] == SEG_DISABLE)
            {
                disconnect(sock);
                return;
            }
        }
    }
    
    // Ethernet data transfer to DATA UART
    if(e2u_size[channel] != 0)
    {
        if(serial_option[channel].dsr_en == SEG_ENABLE) // DTR / DSR handshake (flow control)
        {
            if(get_flowcontrol_dsr_pin() == IO_LOW) return;
        }
//////////////////////////////////////////////////////////////////////
#ifdef __USE_UART_485_422__
        if((serial_option[channel].uart_interface == UART_IF_RS422) ||
           (serial_option[channel].uart_interface == UART_IF_RS485))
        {
            if((serial_common->serial_debug_en == SEG_DEBUG_E2S) || (serial_common->serial_debug_en == SEG_DEBUG_ALL))
            {
                debugSerial_dataTransfer(g_recv_buf, e2u_size[channel], SEG_DEBUG_E2S);
            }
            
            uart_rs485_enable(uartNum);
            for(i = 0; i < e2u_size[channel]; i++) uart_putc(uartNum, g_recv_buf[i]);
            uart_rs485_disable(uartNum);
            
            add_data_transfer_bytecount(channel, SEG_ETHER_TX, e2u_size[channel]);
            e2u_size[channel] = 0;
        }
//////////////////////////////////////////////////////////////////////
        else if(serial_option->flow_control == flow_xon_xoff)
#else
        if(serial_option->flow_control == flow_xon_xoff)
#endif
        {
            if(isXON[channel] == SEG_ENABLE)
            {
                if((serial_common->serial_debug_en == SEG_DEBUG_E2S) || (serial_common->serial_debug_en == SEG_DEBUG_ALL))
                {
                    debugSerial_dataTransfer(g_recv_buf, e2u_size[channel], SEG_DEBUG_E2S);
                }
                
                for(i = 0; i < e2u_size[channel]; i++) uart_putc(uartNum, g_recv_buf[i]);
                add_data_transfer_bytecount(channel, SEG_ETHER_TX, e2u_size[channel]);
                e2u_size[channel] = 0;
            }
            //else
            //{
            //	;//XOFF!!
            //}
        }
        else
        {
            if((serial_common->serial_debug_en == SEG_DEBUG_E2S) || (serial_common->serial_debug_en == SEG_DEBUG_ALL))
            {
                debugSerial_dataTransfer(g_recv_buf, e2u_size[channel], SEG_DEBUG_E2S);
            }
            
            for(i = 0; i < e2u_size[channel]; i++) uart_putc(uartNum, g_recv_buf[i]);
            
            add_data_transfer_bytecount(channel, SEG_ETHER_TX, e2u_size[channel]);
            e2u_size[channel] = 0;
        }
    }
}


uint16_t get_tcp_any_port(uint8_t channel)
{
    if(client_any_port[channel])
    {
        if(client_any_port[channel] < 0xffff)
            client_any_port[channel]++;
        else
            client_any_port[channel] = 0;
    }
    
    if(client_any_port[channel] == 0)
    {
        // todo: gen random seed (srand + random value)
        client_any_port[channel] = (rand() % 10000) + 35000; // 35000 ~ 44999
    }
    
    return client_any_port[channel];
}


uint8_t get_serial_communation_protocol(uint8_t channel)
{
    struct __serial_option *serial_option = (struct __serial_option *)&(get_DevConfig_pointer()->serial_option);

    // SEG_SERIAL_PROTOCOL_NONE
    // SEG_SERIAL_MODBUS_RTU
    // SEG_SERIAL_MODBUS_ASCII

    return serial_option[channel].protocol;
}


void send_keepalive_packet_manual(uint8_t sock)
{
    setsockopt(sock, SO_KEEPALIVESEND, 0);
    
#ifdef _SEG_DEBUG_
    printf(" > SOCKET[%x]: SEND KEEP-ALIVE PACKET\r\n", sock);
#endif 
}


uint8_t process_socket_termination(uint8_t sock, uint32_t timeout)
{
    struct __network_connection *network_connection = (struct __network_connection *)get_DevConfig_pointer()->network_connection;

    int8_t ret;
    uint8_t sock_status = getSn_SR(sock);
    uint32_t tickStart = millis();

    if(sock_status == SOCK_CLOSED) return sock;
    
    if(network_connection->working_mode != UDP_MODE) // TCP_SERVER_MODE / TCP_CLIENT_MODE / TCP_MIXED_MODE
    {
        if((sock_status == SOCK_ESTABLISHED) || (sock_status == SOCK_CLOSE_WAIT))
        {
            do {
                //ret = disconnect_nb(sock);	//hoon
            	ret = disconnect(sock);
                if((ret == SOCK_OK) || (ret == SOCKERR_TIMEOUT)) break;
            } while ((millis() - tickStart) < timeout);
        }
    }
    
    close(sock);
    
    return sock;
}


uint8_t check_connect_pw_auth(uint8_t channel, uint8_t * buf, uint16_t len)
{
    struct __tcp_option *tcp_option = (struct __tcp_option *)get_DevConfig_pointer()->tcp_option;

    uint8_t ret = SEG_DISABLE;
    uint8_t pwbuf[11] = {0,};
    
    if(len >= sizeof(pwbuf))
    {
        len = sizeof(pwbuf) - 1;
    }
    
    memcpy(pwbuf, buf, len);
    if((len == strlen(tcp_option[channel].pw_connect)) && (memcmp(tcp_option[channel].pw_connect, pwbuf, len) == 0))
    {
        ret = SEG_ENABLE; // Connection password auth success
    }
    
#ifdef _SEG_DEBUG_
    printf(" > Connection password: %s, len: %d\r\n", tcp_option[channel].pw_connect, strlen(tcp_option[channel].pw_connect));
    printf(" > Entered password: %s, len: %d\r\n", pwbuf, len);
    printf(" >> Auth %s\r\n", ret ? "success":"failed");
#endif
    
    return ret;
}


void init_trigger_modeswitch(uint8_t channel, uint8_t uartNum, uint8_t mode)
{
    struct __serial_common *serial_common = (struct __serial_common *)&(get_DevConfig_pointer()->serial_common);
    struct __network_connection *network_connection = (struct __network_connection *)(get_DevConfig_pointer()->network_connection);
    
    if(mode == DEVICE_AT_MODE)
    {
        opmode = DEVICE_AT_MODE;
        set_device_status(channel, ST_ATMODE);
        
        if(serial_common->serial_debug_en)
        {
            printf(" > SEG:AT Mode\r\n");
            uart_puts(uartNum, (uint8_t *)"SEG:AT Mode\r\n", sizeof("SEG:AT Mode\r\n"));
        }
    }
    else // DEVICE_GW_MODE
    {
        opmode = DEVICE_GW_MODE;
        set_device_status(channel, ST_OPEN);

        if(network_connection[channel].working_mode == TCP_MIXED_MODE)
        {
            mixed_state[channel] = MIXED_SERVER;
        }
                
        if(serial_common->serial_debug_en)
        {
            printf(" > SEG:GW Mode\r\n");
            uart_puts(uartNum, (uint8_t *)"SEG:GW Mode\r\n", sizeof("SEG:GW Mode\r\n"));
        }
    }
    
    u2e_size[channel] = 0;
    uart_rx_flush(uartNum);
    
    enable_inactivity_timer[channel] = SEG_DISABLE;
    enable_keepalive_timer[channel] = SEG_DISABLE;
    enable_serial_input_timer[channel] = SEG_DISABLE;
    
    inactivity_time[channel] = 0;
    keepalive_time[channel] = 0;
    serial_input_time[channel] = 0;

    flag_serial_input_time_elapse[channel] = 0;

    enable_modeswitch_timer = SEG_DISABLE;
    modeswitch_time = 0;
}

uint8_t check_modeswitch_trigger(uint8_t ch)
{
    struct __serial_command *serial_command = (struct __serial_command *)&(get_DevConfig_pointer()->serial_command);
    
    uint8_t modeswitch_failed = SEG_DISABLE;
    uint8_t ret = 0;
    
    if(opmode != DEVICE_GW_MODE) 				        return 0;
    if(serial_command->serial_command == SEG_DISABLE) 	return 0;
    
    switch(triggercode_idx)
    {
        case 0:
            if((ch == serial_command->serial_trigger[triggercode_idx]) && (modeswitch_time == modeswitch_gap_time)) // comparison succeed
            {
                ch_tmp[triggercode_idx] = ch;
                triggercode_idx++;
                enable_modeswitch_timer = SEG_ENABLE;
            }
            break;
            
        case 1:
        case 2:
            if((ch == serial_command->serial_trigger[triggercode_idx]) && (modeswitch_time < modeswitch_gap_time)) // comparison succeed
            {
                ch_tmp[triggercode_idx] = ch;
                triggercode_idx++;
            }
            else // comparison failed: invalid trigger code
            {
                modeswitch_failed = SEG_ENABLE; 
            }
            break;
        case 3:
            if(modeswitch_time < modeswitch_gap_time) // comparison failed: end gap
            {
                modeswitch_failed = SEG_ENABLE;
            }
            break;
    }
    
    if(modeswitch_failed == SEG_ENABLE)
    {
        restore_serial_data(triggercode_idx);
    }
    
    modeswitch_time = 0; // reset the inter-gap time count for each trigger code recognition (Allowable interval)
    ret = triggercode_idx;
    
    return ret;
}

// when serial command mode trigger code comparison failed
void restore_serial_data(uint8_t idx)
{
    uint8_t i;
    
    for(i = 0; i < idx; i++)
    {
        put_byte_to_uart_buffer(SEG_DATA0_UART, ch_tmp[i]);
        ch_tmp[i] = 0x00;
    }
    
    enable_modeswitch_timer = SEG_DISABLE;
    triggercode_idx = 0;
}

uint8_t check_serial_store_permitted(uint8_t channel, uint8_t ch)
{
    struct __network_connection *network_connection = (struct __network_connection *)&(get_DevConfig_pointer()->network_connection);
    struct __serial_option *serial_option = (struct __serial_option *)(get_DevConfig_pointer()->serial_option);
    
    uint8_t ret = SEG_DISABLE; // SEG_DISABLE: Doesn't put the serial data in a ring buffer
    
    switch(network_connection[channel].working_state)
    {
        case ST_OPEN:
            if(network_connection[channel].working_mode != TCP_MIXED_MODE) break;
        case ST_CONNECT:
        case ST_UDP:
        case ST_ATMODE:
            ret = SEG_ENABLE;
            break;
        default:
            break;
    }
    
    // Software flow control: Check the XON/XOFF start/stop commands
    // [Peer] -> [WIZnet Device]
    if((ret == SEG_ENABLE) && (serial_option->flow_control == flow_xon_xoff))
    {
        if(ch == UART_XON)
        {
            isXON[channel] = SEG_ENABLE;
            ret = SEG_DISABLE; 
        }
        else if(ch == UART_XOFF)
        {
            isXON[channel] = SEG_DISABLE;
            ret = SEG_DISABLE;
        }
    }
    return ret;
}

void reset_SEG_timeflags(uint8_t channel)
{
    // Timer disable
    enable_inactivity_timer[channel] = SEG_DISABLE;
    enable_serial_input_timer[channel] = SEG_DISABLE;
    enable_keepalive_timer[channel] = SEG_DISABLE;
    enable_connection_auth_timer[channel] = SEG_DISABLE;
    
    // Flag clear
    flag_serial_input_time_elapse[channel] = SEG_DISABLE;
    flag_sent_keepalive[channel] = SEG_DISABLE;
    flag_connect_pw_auth[channel] = SEG_DISABLE; // TCP_SERVER_MODE only (+ MIXED_SERVER)
    
    // Timer value clear
    inactivity_time[channel] = 0;
    serial_input_time[channel] = 0;
    keepalive_time[channel] = 0;
    connection_auth_time[channel] = 0;
}

void init_time_delimiter_timer(uint8_t channel)
{
    struct __serial_data_packing *serial_data_packing = (struct __serial_data_packing *)get_DevConfig_pointer()->serial_data_packing;
    
    if(opmode == DEVICE_GW_MODE)
    {
        if(serial_data_packing[channel].packing_time != 0)
        {
            if(enable_serial_input_timer[channel] == SEG_DISABLE)
            {
                enable_serial_input_timer[channel] = SEG_ENABLE;
            }
            serial_input_time[channel] = 0;
        }
    }
}

uint8_t check_tcp_connect_exception(uint8_t channel)
{
    struct __network_option *network_option = (struct __network_option *)&get_DevConfig_pointer()->network_option;
    struct __serial_common *serial_common = (struct __serial_common *)&get_DevConfig_pointer()->serial_common;
    struct __network_connection *network_connection = (struct __network_connection *)get_DevConfig_pointer()->network_connection;
    
    uint8_t srcip[4] = {0, };
    uint8_t ret = OFF;
    
    if(channel >= DEVICE_UART_CNT) channel = 0;

    getSIPR(srcip);
    
    // DNS failed
    if((network_connection[channel].dns_use == SEG_ENABLE) && (flag_process_dns_success[channel] != ON))
    {
        if(serial_common->serial_debug_en) printf(" > SEG:CONNECTION FAILED - DNS Failed flag_process_dns_success = %d\r\n", flag_process_dns_success[channel]);
        ret = ON;
    }
    // if dhcp failed (0.0.0.0), this case do not connect to peer
    else if((srcip[0] == 0x00) && (srcip[1] == 0x00) && (srcip[2] == 0x00) && (srcip[3] == 0x00))
    {
        if(serial_common->serial_debug_en) printf(" > SEG:CONNECTION FAILED - Invalid IP address: Zero IP\r\n");
        ret = ON;
    }
    // Destination zero IP
    else if((network_connection[channel].remote_ip[0] == 0x00) &&
            (network_connection[channel].remote_ip[1] == 0x00) &&
            (network_connection[channel].remote_ip[2] == 0x00) &&
            (network_connection[channel].remote_ip[3] == 0x00))
    {
        if(serial_common->serial_debug_en) printf(" > SEG:CONNECTION FAILED - Invalid Destination IP address: Zero IP\r\n");
        ret = ON;
    }
     // Duplicate IP address
    else if((srcip[0] == network_connection[channel].remote_ip[0]) &&
            (srcip[1] == network_connection[channel].remote_ip[1]) &&
            (srcip[2] == network_connection[channel].remote_ip[2]) &&
            (srcip[3] == network_connection[channel].remote_ip[3]))
    {
        if(serial_common->serial_debug_en) printf(" > SEG:CONNECTION FAILED - Duplicate IP address\r\n");
        ret = ON;
    }
    else if((srcip[0] == 192) && (srcip[1] == 168)) // local IP address == Class C private IP
    {
        // Static IP address obtained
        if((network_option->dhcp_use == SEG_DISABLE) && ((network_connection[channel].remote_ip[0] == 192) &&
                                                         (network_connection[channel].remote_ip[1] == 168)))
        {
            if(srcip[2] != network_connection[channel].remote_ip[2]) // Class C Private IP network mismatch
            {
                if(serial_common->serial_debug_en)
                    printf(" > SEG:CONNECTION FAILED - Invalid IP address range (%d.%d.[%d].%d)\r\n",
                                                                network_connection[channel].remote_ip[0],
                                                                network_connection[channel].remote_ip[1],
                                                                network_connection[channel].remote_ip[2],
                                                                network_connection[channel].remote_ip[3]);
                ret = ON; 
            }
        }
    }
    
    return ret;
}
    

void clear_data_transfer_bytecount(uint8_t channel, teDATADIR dir)
{
    switch(dir)
    {
        case SEG_ALL:
            seg_byte_cnt[channel][SEG_UART_RX] = 0;
            seg_byte_cnt[channel][SEG_UART_TX] = 0;
            seg_byte_cnt[channel][SEG_ETHER_RX] = 0;
            seg_byte_cnt[channel][SEG_ETHER_TX] = 0;
            break;
        
        case SEG_UART_RX:
        case SEG_UART_TX:
        case SEG_ETHER_RX:
        case SEG_ETHER_TX:
            seg_byte_cnt[channel][dir] = 0;
            break;

        default:
            break;
    }
}


void clear_data_transfer_megacount(uint8_t channel, teDATADIR dir)
{
    switch(dir)
    {
        case SEG_ALL:
            seg_mega_cnt[channel][SEG_UART_RX] = 0;
            seg_mega_cnt[channel][SEG_UART_TX] = 0;
            seg_mega_cnt[channel][SEG_ETHER_RX] = 0;
            seg_mega_cnt[channel][SEG_ETHER_TX] = 0;
            break;
        
        case SEG_UART_RX:
        case SEG_UART_TX:
        case SEG_ETHER_RX:
        case SEG_ETHER_TX:
            seg_mega_cnt[channel][dir] = 0;
            break;

        default:
            break;
    }
}

void add_data_transfer_bytecount(uint8_t channel, teDATADIR dir, uint16_t len)
{
    if(dir >= SEG_ALL) return;
    if(channel >= DEVICE_UART_CNT) channel = 0;

    if(len > 0)
    {
        if(seg_byte_cnt[channel][dir] < SEG_MEGABYTE)
        {
            seg_byte_cnt[channel][dir] += len;
        }
        else
        {
            seg_mega_cnt[channel][dir]++;
            seg_byte_cnt[channel][dir] = 0;
        }
    }
}
int wizchip_mqtt_publish(MQTTClient *mqtt_c, uint8_t *pub_topic, uint8_t qos, uint8_t *pub_data, uint32_t pub_data_len)
{
    MQTTMessage msg = {qos, 0, 0, 0, NULL, 0};

    msg.retained = FALSE;
    msg.payload = (char *)pub_data;
    msg.payloadlen = pub_data_len;

    PRT_SEG("MQTT PUB Len = %d\r\n", msg.payloadlen);
    PRT_SEG("MQTT PUB Data = %.*s\r\n", msg.payloadlen, (char *)msg.payload);

    if(MQTTPublish(mqtt_c, (char *)pub_topic, &msg) == FAILURE)
    {
        return FAILURE;
    }
    return pub_data_len;
}


void mqtt_subscribeMessageHandler(MessageData* md)
{
    MQTTMessage* message = md->message;
//    char topic[SUBSCRIBE_TOPIC_MAX_SIZE];

    if(message->payloadlen)
    {
#if 0
        TLS_DBGPRT_WIZNET("topic len = %d, toppic name = %s\r\n", md->topicName->lenstring.len, md->topicName->lenstring.data);
        TLS_DBGPRT_WIZNET("payload len = %d, payload name = %s\r\n", message->payloadlen, message->payload);

    	sprintf(topic, "%.*s", md->topicName->lenstring.len, md->topicName->lenstring.data);
		uart_sendStr(topic);
		uart_sendStr(" -> ");
		uart_sendStr((char*)message->payload);
		uart_sendStr("\r\n");

        //wizdevice_gcp_mqtt_subscribe_handler(recvbuffer, message->payloadlen);
        memset(message->payload, NULL, message->payloadlen);
        memset(md->topicName->lenstring.data, NULL, md->topicName->lenstring.len);
#endif

        e2u_size[mqtt_n.channel] = message->payloadlen;
        memcpy(g_recv_buf, message->payload, message->payloadlen);
        ether_to_uart(mqtt_n.channel, mqtt_n.my_socket);

    }
}



uint32_t get_data_transfer_bytecount(uint8_t channel, teDATADIR dir)
{
    if(channel >= DEVICE_UART_CNT) channel = 0;
    return seg_byte_cnt[channel][dir];
}


uint32_t get_data_transfer_megacount(uint8_t channel, teDATADIR dir)
{
    if(channel >= DEVICE_UART_CNT) channel = 0;
    return seg_mega_cnt[channel][dir];
}


uint16_t debugSerial_dataTransfer(uint8_t * buf, uint16_t size, teDEBUGTYPE type)
{
    uint16_t bytecnt = 0;
    
//#ifdef __USE_DEBUG_UPTIME__
    if(getDeviceUptime_day() > 0)
        printf(" [%ldd/%02d:%02d:%02d]", getDeviceUptime_day(), getDeviceUptime_hour(), getDeviceUptime_min(), getDeviceUptime_sec());
    else
        printf(" [%02d:%02d:%02d]", getDeviceUptime_hour(), getDeviceUptime_min(), getDeviceUptime_sec());
//#endif
    
    if((type == SEG_DEBUG_S2E) || (type == SEG_DEBUG_E2S))
    {
        printf("[%s][%04d] ", (type == SEG_DEBUG_S2E)?"S2E":"E2S", size);
        for(bytecnt = 0; bytecnt < size; bytecnt++) printf("%02X ", buf[bytecnt]);
        printf("\r\n");
    }
    
    return bytecnt;
}


void send_sid(uint8_t sock, uint8_t link_message, uint8_t uart_channel)
{
    DevConfig *dev_config = get_DevConfig_pointer();

    uint8_t buf[45] = {0, };
    uint8_t len = 0;

    switch(link_message)
    {
        case SEG_LINK_MSG_NONE:
            break;

        case SEG_LINK_MSG_DEVNAME:
            len = snprintf((char *)buf, sizeof(buf), "%s", dev_config->device_common.device_name);
            break;

        case SEG_LINK_MSG_MAC:
            len = snprintf((char *)buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
                                                     dev_config->network_common.mac[0],
                                                     dev_config->network_common.mac[1],
                                                     dev_config->network_common.mac[2],
                                                     dev_config->network_common.mac[3],
                                                     dev_config->network_common.mac[4],
                                                     dev_config->network_common.mac[5]);
            break;

        case SEG_LINK_MSG_IP:
            len = snprintf((char *)buf, sizeof(buf), "%d.%d.%d.%d",
                                                     dev_config->network_common.local_ip[0],
                                                     dev_config->network_common.local_ip[1],
                                                     dev_config->network_common.local_ip[2],
                                                     dev_config->network_common.local_ip[3]);
            break;

        case SEG_LINK_MSG_DEVID:
            len = snprintf((char *)buf, sizeof(buf), "%s-%02X%02X%02X%02X%02X%02X",
                                                     dev_config->device_common.device_name,
                                                     dev_config->network_common.mac[0],
                                                     dev_config->network_common.mac[1],
                                                     dev_config->network_common.mac[2],
                                                     dev_config->network_common.mac[3],
                                                     dev_config->network_common.mac[4],
                                                     dev_config->network_common.mac[5]);
            break;


        case SEG_LINK_MSG_DEVALIAS:
            len = snprintf((char *)buf, sizeof(buf), "%s", dev_config->device_option.device_alias);
            break;

        case SEG_LINK_MSG_DEVGROUP:
            len = snprintf((char *)buf, sizeof(buf), "%s", dev_config->device_option.device_group);
            break;

        default:
            break;
    }

    if(len > 0)
    {
#if 1
        send(sock, buf, len);
#else
        //if(dev_config->device_option.s2e_tls_enable)
        if (dev_config->network_connection[uart_channel].working_mode == SSL_TCP_CLIENT_MODE)
        {
            wiz_tls_write(&s2e_tlsContext, buf, len);
        }
        else
        {
            send(sock, buf, len);
        }
#endif
    }
}



// This function have to call every 1 millisecond by Timer IRQ handler routine.
void seg_timer_msec(void)
{
    struct __serial_data_packing *serial_data_packing = (struct __serial_data_packing *)&(get_DevConfig_pointer()->serial_data_packing);
    
    uint8_t i;
    // Firmware update timer for timeout
    // DHCP timer for timeout
    // SEGCP Keep-alive timer (for configuration tool, TCP mode)

    for(i = 0; i < DEVICE_UART_CNT; i++)
    {
        // Reconnection timer: Time count routine (msec)
        if(enable_reconnection_timer[i])
        {
            if(reconnection_time[i] < 0xFFFF)
                reconnection_time[i]++;
            else
                reconnection_time[i] = 0;
        }

        // Keep-alive timer: Time count routine (msec)
        if(enable_keepalive_timer[i])
        {
            if(keepalive_time[i] < 0xFFFF)
                keepalive_time[i]++;
            else
                keepalive_time[i] = 0;
        }

        // Serial data packing time delimiter timer
        if(enable_serial_input_timer[i])
        {
            if(serial_input_time[i] < serial_data_packing[i].packing_time)
            {
                serial_input_time[i]++;
            }
            else
            {
                serial_input_time[i] = 0;
                enable_serial_input_timer[i] = 0;
                flag_serial_input_time_elapse[i] = SEG_ENABLE;
            }
        }

        // Connection password auth timer
        if(enable_connection_auth_timer[i])
        {
            if(connection_auth_time[i] < 0xffff)
                connection_auth_time[i]++;
            else
                connection_auth_time[i] = 0;
        }
    }

    // Mode switch timer: Time count routine (msec) (GW mode <-> Serial command mode, for s/w mode switch trigger code)
    if(modeswitch_time < modeswitch_gap_time) modeswitch_time++;

    if((enable_modeswitch_timer) && (modeswitch_time == modeswitch_gap_time))
    {
        // result of command mode trigger code comparison
        if(triggercode_idx == 3)
            sw_modeswitch_at_mode_on = SEG_ENABLE;  // success
        else
            restore_serial_data(triggercode_idx);   // failed

        triggercode_idx = 0;
        enable_modeswitch_timer = SEG_DISABLE;
    }
}

// This function have to call every 1 second by Timer IRQ handler routine.
void seg_timer_sec(void)
{
    uint8_t i;

    for(i = 0; i < DEVICE_UART_CNT; i++)
    {
        // Inactivity timer: Time count routine (sec)
        if(enable_inactivity_timer[i])
        {
            if(inactivity_time[i] < 0xFFFF) inactivity_time[i]++;
        }
    }

    tmp_timeflag_for_debug = 1;
}


