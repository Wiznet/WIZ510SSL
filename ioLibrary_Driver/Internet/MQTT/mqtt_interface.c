//*****************************************************************************
//! \file mqtt_interface.c
//! \brief Paho MQTT to WIZnet Chip interface implement file.
//! \details The process of porting an interface to use paho MQTT.
//! \version 1.0.0
//! \date 2016/12/06
//! \par  Revision history
//!       <2016/12/06> 1st Release
//!
//! \author Peter Bang & Justin Kim
//! \copyright
//!
//! Copyright (c)  2016, WIZnet Co., LTD.
//! All rights reserved.
//!
//! Redistribution and use in source and binary forms, with or without
//! modification, are permitted provided that the following conditions
//! are met:
//!
//!     * Redistributions of source code must retain the above copyright
//! notice, this list of conditions and the following disclaimer.
//!     * Redistributions in binary form must reproduce the above copyright
//! notice, this list of conditions and the following disclaimer in the
//! documentation and/or other materials provided with the distribution.
//!     * Neither the name of the <ORGANIZATION> nor the names of its
//! contributors may be used to endorse or promote products derived
//! from this software without specific prior written permission.
//!
//! THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//! AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//! IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//! ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//! LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//! CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//! SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//! INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//! CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//! ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//! THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"

#include "wizchip_conf.h"
#include "socket.h"
#include "wiz_debug.h"

// MQTT
#include "mqtt_interface.h"
#include "wizchip_conf.h"
#include "MQTTClient.h"

// TLS support
#include "SSLInterface.h"
//#include "gcp_ciotc_config.h"

/* Private define ------------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
static uint16_t get_client_any_port(void);

/* Private variables ---------------------------------------------------------*/
volatile uint32_t MilliTimer = 0;
static uint8_t brokerip[4] = {0, };
static uint16_t brokerport = 0;

// TLS support
wiz_tls_context mqttTlsContext;
static uint8_t MQTT_TLS_ENABLED = FALSE;

Network mqtt_n;
MQTTClient mqtt_c = DefaultClient;
MQTTPacket_connectData mqtt_data = MQTTPacket_connectData_initializer;

extern uint16_t e2u_size[];
extern volatile uint16_t inactivity_time[];
extern volatile uint16_t keepalive_time[];
extern uint8_t flag_sent_first_keepalive[];


int8_t wizchip_mqtt_network_connect(Network * n)
{
    int8_t ret;
    ret = ConnectNetwork(n, (char *)brokerip, (unsigned int)brokerport);

    return ret;
}

void wizchip_mqtt_network_disconnect(Network * n)
{
    if(MQTT_TLS_ENABLED)
    {
        wiz_tls_close_notify(&mqttTlsContext);
        wiz_tls_disconnect(&mqttTlsContext, 100);
        wiz_tls_session_reset(&mqttTlsContext);
        wiz_tls_deinit(&mqttTlsContext);
    }
    else
    {
        disconnect(n->my_socket);
    }
}

uint8_t wizchip_mqtt_tls_enable(uint8_t tls)
{
    if(tls == WIZ_TLS_ENABLED)
        MQTT_TLS_ENABLED = TRUE;
    else
        MQTT_TLS_ENABLED = FALSE;

    return MQTT_TLS_ENABLED;
}

uint8_t wizchip_mqtt_tls_get(void)
{
    return MQTT_TLS_ENABLED;
}

void wizchip_mqtt_set_ipport(uint8_t * bip, uint16_t bport)
{
    strncpy((char *)brokerip, (char *)bip, 4);
    brokerport = bport;
}

uint32_t get_mqtt_time(void) {
    return MilliTimer;
}

/*
 * @brief MQTT MilliTimer handler
 * @note MUST BE register to your system 1m Tick timer handler.
 */
void MilliTimer_Handler(void) {
	MilliTimer++;
}

/*
 * @brief Timer Initialize
 * @param  timer : pointer to a Timer structure
 *         that contains the configuration information for the Timer.
 */
void TimerInit(Timer* timer) {
	timer->end_time = 0;
}

/*
 * @brief expired Timer
 * @param  timer : pointer to a Timer structure
 *         that contains the configuration information for the Timer.
 */
char TimerIsExpired(Timer* timer) {
	long left = timer->end_time - MilliTimer;
	return (left < 0);
}

/*
 * @brief Countdown millisecond Timer
 * @param  timer : pointer to a Timer structure
 *         that contains the configuration information for the Timer.
 *         timeout : setting timeout millisecond.
 */
void TimerCountdownMS(Timer* timer, unsigned int timeout) {
	timer->end_time = MilliTimer + timeout;
}

/*
 * @brief Countdown second Timer
 * @param  timer : pointer to a Timer structure
 *         that contains the configuration information for the Timer.
 *         timeout : setting timeout millisecond.
 */
void TimerCountdown(Timer* timer, unsigned int timeout) {
	timer->end_time = MilliTimer + (timeout * 1000);
}

/*
 * @brief left millisecond Timer
 * @param  timer : pointer to a Timer structure
 *         that contains the configuration information for the Timer.
 */
int TimerLeftMS(Timer* timer) {
	long left = timer->end_time - MilliTimer;
	return (left < 0) ? 0 : left;
}

/*
 * @brief New network setting
 * @param  n : pointer to a Network structure
 *         that contains the configuration information for the Network.
 *         sn : socket number where x can be (0..7).
 *         channel : uart channel for subscribe
 * @retval None
 */
void NewNetwork(Network* n, int sn, uint8_t channel)
{
	n->my_socket = sn;
    n->mqttread = wizchip_read;
    n->mqttwrite = wizchip_write;
    n->disconnect = wizchip_disconnect;
    n->channel = channel;
}

/*
 * @brief New TLS network setting
 * @param  n : pointer to a Network structure
 *         that contains the configuration information for the Network.
 *         sn : socket number where x can be (0..7).
 *         host : host name
 *         cert : certificate
 *         channel : uart channel for subscribe
 * @retval None
 */
void NewNetwork_TLS(Network* n, int sn, const char * host, uint8_t channel)
{
    static unsigned char is_tls_init;

    n->my_socket = sn;
    n->mqttread = wizchip_tls_read;
    n->mqttwrite = wizchip_tls_write;
    n->disconnect = wizchip_tls_disconnect;
    n->host = host;
    n->channel = channel;

    if(is_tls_init == TRUE)
    {
        wiz_tls_deinit(&mqttTlsContext);
        is_tls_init = FALSE;
    }

    if(wiz_tls_init(&mqttTlsContext, n->my_socket, n->host) > 0)
    {
        is_tls_init = TRUE;
    }
}

/*
 * @brief read function
 * @param  n : pointer to a Network structure
 *         that contains the configuration information for the Network.
 *         buffer : pointer to a read buffer.
 *         len : buffer length.
 */
int wizchip_read(Network* n, unsigned char* buffer, unsigned int len, int timeout_ms)
{
    int size = 0;
	if((getSn_SR(n->my_socket) == SOCK_ESTABLISHED) && (getSn_RX_RSR(n->my_socket) > 0))
	    size = (int)recv(n->my_socket, buffer, len);
	return size;
}

/*
 * @brief write function
 * @param  n : pointer to a Network structure
 *         that contains the configuration information for the Network.
 *         buffer : pointer to a read buffer.
 *         len : buffer length.
 */
int wizchip_write(Network* n, unsigned char* buffer, unsigned int len, int timeout_ms)
{
    int size = 0;
	if(getSn_SR(n->my_socket) == SOCK_ESTABLISHED)
	    size = (int)send(n->my_socket, buffer, len);
	return size;
}

/*
 * @brief disconnect function
 * @param  n : pointer to a Network structure
 *         that contains the configuration information for the Network.
 */
void wizchip_disconnect(Network* n)
{
	disconnect(n->my_socket);
}

/*
 * @brief read function
 * @param  n : pointer to a Network structure
 *         that contains the configuration information for the Network.
 *         buffer : pointer to a read buffer.
 *         len : buffer length.
 */
int wizchip_tls_read(Network* n, unsigned char* buffer, unsigned int len, int timeout_ms)
{
    int i;
    int size = 0;
    //if((getSn_SR(n->my_socket) == SOCK_ESTABLISHED) && (getSn_RX_RSR(n->my_socket) > 0))
    if(getSn_SR(n->my_socket) == SOCK_ESTABLISHED)
        size = wiz_tls_read(&mqttTlsContext, buffer, len);

    return size;
}

/*
 * @brief write function
 * @param  n : pointer to a Network structure
 *         that contains the configuration information for the Network.
 *         buffer : pointer to a read buffer.
 *         len : buffer length.
 */
int wizchip_tls_write(Network* n, unsigned char* buffer, unsigned int len, int timeout_ms)
{
    int size = 0;
    if(getSn_SR(n->my_socket) == SOCK_ESTABLISHED)
        size = wiz_tls_write(&mqttTlsContext, buffer, len);

    return size;
}

/*
 * @brief disconnect function
 * @param  n : pointer to a Network structure
 *         that contains the configuration information for the Network.
 */
void wizchip_tls_disconnect(Network* n)
{
    wiz_tls_close_notify(&mqttTlsContext);
    wiz_tls_disconnect(&mqttTlsContext, 100);
    wiz_tls_session_reset(&mqttTlsContext);
}

/*
 * @brief connect network function
 * @param  n : pointer to a Network structure
 *         that contains the configuration information for the Network.
 *         ip : server iP.
 *         port : server port.
 */
int ConnectNetwork(Network* n, char* ip, unsigned int port)
{
    int8_t ret;
	uint16_t myport = get_client_any_port();

    PRT_MQTT("MQTT_TLS_ENABLED = %d\r\n", MQTT_TLS_ENABLED);
    PRT_MQTT("Socket = %d\r\n", n->my_socket);
    PRT_MQTT("CONNECT TO - %d.%d.%d.%d : %d\r\n", ip[0], ip[1], ip[2], ip[3], port);
    
	if(MQTT_TLS_ENABLED)
	{
	    //wiz_tls_init(&tlsContext, n->my_socket, n->host, n->cert);
	    ret = wiz_tls_session_reset(&mqttTlsContext);
	    if(ret == 0)
	    {
	        ret = wiz_tls_socket_connect_timeout(&mqttTlsContext, ip, port, myport, 30000);
	        //ret = wiz_tls_connect_timeout(&mqttTlsContext, ip, port, 10000);

            if(ret == 0)
                ret = 1;
	    }
	}
	else
	{
        socket(n->my_socket, Sn_MR_TCP, myport, 0);
        ret = connect((uint8_t)n->my_socket, (uint8_t *)ip, (uint16_t)port);
	}
	return ret;
}

/*
 * @brief get client port function
 */
static uint16_t get_client_any_port(void)
{
    static uint16_t client_any_port;

    if(client_any_port)
    {
        if(client_any_port < 0xffff)
            client_any_port++;
        else
            client_any_port = 0;
    }

    if(client_any_port == 0)
    {
        srand(1);
        client_any_port = (rand() % 10000) + 45000; // 45000 ~ 54999
    }
    return client_any_port;
}

