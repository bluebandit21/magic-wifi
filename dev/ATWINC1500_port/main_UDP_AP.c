/**
 *
 * \file
 *
 * \brief WINC1500 UDP Server Example.
 *
 * Copyright (c) 2016-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */

/** \mainpage
 * \section intro Introduction
 * This example demonstrates the use of the WINC1500 with the SAMD21 Xplained Pro
 * board to test UDP server socket.<br>
 * It uses the following hardware:
 * - the SAMD21 Xplained Pro.
 * - the WINC1500 on EXT1.
 *
 * \section files Main Files
 * - main.c : Initialize the WINC1500 and test UDP server.
 *
 * \section usage Usage
 * -# Configure below code in the main.h for AP information to be connected.
 * \code
 *    #define MAIN_WLAN_SSID                    "DEMO_AP"
 *    #define MAIN_WLAN_AUTH                    M2M_WIFI_SEC_WPA_PSK
 *    #define MAIN_WLAN_PSK                     "12345678"
 *    #define MAIN_WIFI_M2M_PRODUCT_NAME        "NMCTemp"
 *    #define MAIN_WIFI_M2M_SERVER_IP           0xFFFFFFFF // "255.255.255.255"
 *    #define MAIN_WIFI_M2M_SERVER_PORT         (6666)
 *    #define MAIN_WIFI_M2M_REPORT_INTERVAL     (1000)
 * \endcode
 * -# Build the program and download it into the board.
 * -# On the computer, open and configure a terminal application as the follows.
 * \code
 *    Baud Rate : 115200
 *    Data : 8bit
 *    Parity bit : none
 *    Stop bit : 1bit
 *    Flow control : none
 * \endcode
 * -# Start the application.
 * -# In the terminal window, the following text should appear:
 * \code
 *    -- WINC1500 UDP server example --
 *    -- SAMD21_XPLAINED_PRO --
 *    -- Compiled: xxx xx xxxx xx:xx:xx --
 *    wifi_cb: M2M_WIFI_RESP_CON_STATE_CHANGED : CONNECTED
 *    wifi_cb: M2M_WIFI_REQ_DHCP_CONF : IP is xxx.xxx.xxx.xxx
 *    socket_cb: bind success!
 *    socket_cb: received app message.(1)
 *    . . .
 *    socket_cb: received app message.(10)
 *    UDP Server test Complete!
 * \endcode
 *
 * \section compinfo Compilation Information
 * This software was written for the GNU GCC compiler using Atmel Studio 6.2
 * Other compilers may or may not work.
 *
 * \section contactinfo Contact Information
 * For further information, visit
 * <A href="http://www.microchip.com">Microchip</A>.\n
 */

#include <msp430.h>
#include "driver/source/nmasic.h"
#include "common/include/nm_common.h" // from lib
#include "driver/include/m2m_wifi.h" // from lib
#include "socket/include/socket.h" // from lib
#include <string.h>


#define MAIN_WLAN_SSID           "DEMO_AP"
#define MAIN_WLAN_AUTH           M2M_WIFI_SEC_OPEN
#define MAIN_WLAN_CHANNEL        (6)

/****** Define Variables with no previously given definition *******/
#define MAIN_WIFI_M2M_BUFFER_SIZE   10 //uint16_t
#define MAIN_WIFI_M2M_PACKET_COUNT  3

/******* SERVER PORT and IP *******/
#define MAIN_WIFI_M2M_SERVER_PORT         (6666) //TODO Probably need to change
#define MAIN_WIFI_M2M_SERVER_IP           0xC0A80101 // "192.168.1.1" initialized for (Structure: "255.255.255.255" (8 digits long))


#define STRING_EOL    "\r\n"
#define STRING_HEADER "-- WINC1500 UDP server example --"STRING_EOL \
    "-- "BOARD_NAME " --"STRING_EOL \
    "-- Compiled: "__DATE__ " "__TIME__ " --"STRING_EOL

/** UART module for debug. */
//static struct usart_module cdc_uart_module;

/** Test buffer */
static uint8_t gau8SocketTestBuffer[MAIN_WIFI_M2M_BUFFER_SIZE] = {0};

/** Socket for Rx */
static SOCKET rx_socket = -1;

/** Wi-Fi connection state */
static uint8_t wifi_connected;

/** UDP packet count */
static uint8_t packetCnt = 0;

void clockSetup(void);


/**
 * \brief Callback to get the Data from socket.
 *
 * \param[in] sock socket handler.
 * \param[in] u8Msg socket event type. Possible values are:
 *  - SOCKET_MSG_BIND
 *  - SOCKET_MSG_LISTEN
 *  - SOCKET_MSG_ACCEPT
 *  - SOCKET_MSG_CONNECT
 *  - SOCKET_MSG_RECV
 *  - SOCKET_MSG_SEND
 *  - SOCKET_MSG_SENDTO
 *  - SOCKET_MSG_RECVFROM
 * \param[in] pvMsg is a pointer to message structure. Existing types are:
 *  - tstrSocketBindMsg
 *  - tstrSocketListenMsg
 *  - tstrSocketAcceptMsg
 *  - tstrSocketConnectMsg
 *  - tstrSocketRecvMsg
 */
static void socket_cb(SOCKET sock, uint8_t u8Msg, void *pvMsg)
{
    if (u8Msg == SOCKET_MSG_BIND) {
        tstrSocketBindMsg *pstrBind = (tstrSocketBindMsg *)pvMsg;
        if (pstrBind && pstrBind->status == 0) {
            /* Prepare next buffer reception. */
            printf("socket_cb: bind success!\r\n");
            recvfrom(sock, gau8SocketTestBuffer, MAIN_WIFI_M2M_BUFFER_SIZE, 0);
        } else {
            printf("socket_cb: bind error!\r\n");
        }
    } else if (u8Msg == SOCKET_MSG_RECVFROM) {
        tstrSocketRecvMsg *pstrRx = (tstrSocketRecvMsg *)pvMsg;
        if (packetCnt >= MAIN_WIFI_M2M_PACKET_COUNT) {
            return;
        }

        if (pstrRx->pu8Buffer && pstrRx->s16BufferSize) {
            packetCnt++;
            printf("socket_cb: received app message.(%u)\r\n", packetCnt);
            /* Prepare next buffer reception. */
            recvfrom(sock, gau8SocketTestBuffer, MAIN_WIFI_M2M_BUFFER_SIZE, 0);
        } else {
            if (pstrRx->s16BufferSize == SOCK_ERR_TIMEOUT) {
                /* Prepare next buffer reception. */
                recvfrom(sock, gau8SocketTestBuffer, MAIN_WIFI_M2M_BUFFER_SIZE, 0);
            }
        }
    }
}

/**
 * \brief Callback to get the Wi-Fi status update.
 *
 * \param[in] u8MsgType type of Wi-Fi notification. Possible types are:
 *  - [M2M_WIFI_RESP_CURRENT_RSSI](@ref M2M_WIFI_RESP_CURRENT_RSSI)
 *  - [M2M_WIFI_RESP_CON_STATE_CHANGED](@ref M2M_WIFI_RESP_CON_STATE_CHANGED)
 *  - [M2M_WIFI_RESP_CONNTION_STATE](@ref M2M_WIFI_RESP_CONNTION_STATE)
 *  - [M2M_WIFI_RESP_SCAN_DONE](@ref M2M_WIFI_RESP_SCAN_DONE)
 *  - [M2M_WIFI_RESP_SCAN_RESULT](@ref M2M_WIFI_RESP_SCAN_RESULT)
 *  - [M2M_WIFI_REQ_WPS](@ref M2M_WIFI_REQ_WPS)
 *  - [M2M_WIFI_RESP_IP_CONFIGURED](@ref M2M_WIFI_RESP_IP_CONFIGURED)
 *  - [M2M_WIFI_RESP_IP_CONFLICT](@ref M2M_WIFI_RESP_IP_CONFLICT)
 *  - [M2M_WIFI_RESP_P2P](@ref M2M_WIFI_RESP_P2P)
 *  - [M2M_WIFI_RESP_AP](@ref M2M_WIFI_RESP_AP)
 *  - [M2M_WIFI_RESP_CLIENT_INFO](@ref M2M_WIFI_RESP_CLIENT_INFO)
 * \param[in] pvMsg A pointer to a buffer containing the notification parameters
 * (if any). It should be casted to the correct data type corresponding to the
 * notification type. Existing types are:
 *  - tstrM2mWifiStateChanged
 *  - tstrM2MWPSInfo
 *  - tstrM2MP2pResp
 *  - tstrM2MAPResp
 *  - tstrM2mScanDone
 *  - tstrM2mWifiscanResult
 */
/*** Changed from given wifi_cb in UDP_server (taken from host example) ***/
/*static void wifi_cb(uint8_t u8MsgType, void *pvMsg)
{
    switch (u8MsgType) {
    case M2M_WIFI_RESP_CON_STATE_CHANGED:
    {
        tstrM2mWifiStateChanged *pstrWifiState = (tstrM2mWifiStateChanged *)pvMsg;
        if (pstrWifiState->u8CurrState == M2M_WIFI_CONNECTED) {
            printf("Station connected?\r\n");
            GPIO_setOutputHighOnPin(GPIO_PORT_P6, GPIO_PIN6);
            GPIO_setOutputHighOnPin(GPIO_PORT_P6, GPIO_PIN6);
        } else if (pstrWifiState->u8CurrState == M2M_WIFI_DISCONNECTED) {
            GPIO_setOutputLowOnPin(GPIO_PORT_P6, GPIO_PIN6);
            printf("Station disconnected\r\n");
        }

        break;
    }

    case M2M_WIFI_REQ_DHCP_CONF:
    {
        uint8_t *pu8IPAddress = (uint8_t *)pvMsg;
        wifi_connected = M2M_WIFI_CONNECTED;
        printf("Station connected\r\n");
        printf("Station IP is %u.%u.%u.%u\r\n", pu8IPAddress[0], pu8IPAddress[1], pu8IPAddress[2], pu8IPAddress[3]);
        break;
    }

    default:
    {
        break;
    }
    }
}*/
static void wifi_cb(uint8_t u8MsgType, void *pvMsg)
{
    switch (u8MsgType) {
    case M2M_WIFI_RESP_CON_STATE_CHANGED:
    {
        tstrM2mWifiStateChanged *pstrWifiState = (tstrM2mWifiStateChanged *)pvMsg;
        if (pstrWifiState->u8CurrState == M2M_WIFI_CONNECTED) {
            printf("wifi_cb: M2M_WIFI_RESP_CON_STATE_CHANGED: CONNECTED\r\n");
            GPIO_setOutputHighOnPin(GPIO_PORT_P6, GPIO_PIN6);
            m2m_wifi_request_dhcp_client();
        } else if (pstrWifiState->u8CurrState == M2M_WIFI_DISCONNECTED) {
            printf("wifi_cb: M2M_WIFI_RESP_CON_STATE_CHANGED: DISCONNECTED\r\n");
            GPIO_setOutputLowOnPin(GPIO_PORT_P6, GPIO_PIN6);
            m2m_wifi_connect((char *)MAIN_WLAN_SSID, sizeof(MAIN_WLAN_SSID), MAIN_WLAN_AUTH, (char *)0, M2M_WIFI_CH_ALL);
        }
    }
    break;

    case M2M_WIFI_REQ_DHCP_CONF:
    {
        uint8_t *pu8IPAddress = (uint8_t *)pvMsg;
        wifi_connected = M2M_WIFI_CONNECTED;
        printf("wifi_cb: M2M_WIFI_REQ_DHCP_CONF : IP is %u.%u.%u.%u\r\n", pu8IPAddress[0], pu8IPAddress[1], pu8IPAddress[2], pu8IPAddress[3]);
    }
    break;

    default:
        break;
    }
}


/**
 * \brief Main application function.
 *
 * Initialize system, UART console, network then start function of UDP socket.
 *
 * \return program return value.
 */
int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;                   // Disable the GPIO power-on default high-impedance mode
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0); GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0); //Red ON as soon as we have power
    GPIO_setAsOutputPin(GPIO_PORT_P6, GPIO_PIN6); GPIO_setOutputLowOnPin(GPIO_PORT_P6, GPIO_PIN6);


    tstrWifiInitParam param;
    int8_t ret;
    struct sockaddr_in addr;
    tstrM2MAPConfig strM2MAPConfig;

    /* Initialize the board. */
    //system_init();
    clockSetup();

    /* Initialize the UART console. */
    //configure_console();
    //printf(STRING_HEADER);

    /* Initialize the BSP. */
    nm_bsp_init();

    /* Initialize Wi-Fi parameters structure. */
    memset((uint8_t *)&param, 0, sizeof(tstrWifiInitParam));

    /* Initialize Wi-Fi driver with data and status callbacks. */
    param.pfAppWifiCb = wifi_cb;
    ret = m2m_wifi_init(&param);
    if (M2M_SUCCESS != ret) {
        printf("main: m2m_wifi_init call error!(%d)\r\n", ret);
        while (1) {
        }
    }

    /* Initialize AP mode parameters structure with SSID, channel and OPEN security type. */
        memset(&strM2MAPConfig, 0x00, sizeof(tstrM2MAPConfig));
        strcpy((char *)&strM2MAPConfig.au8SSID, MAIN_WLAN_SSID);
        strM2MAPConfig.u8ListenChannel = MAIN_WLAN_CHANNEL;
        strM2MAPConfig.u8SecType = MAIN_WLAN_AUTH;

        strM2MAPConfig.au8DHCPServerIP[0] = 192;
        strM2MAPConfig.au8DHCPServerIP[1] = 168;
        strM2MAPConfig.au8DHCPServerIP[2] = 1;
        strM2MAPConfig.au8DHCPServerIP[3] = 1;
    #if USE_WEP
        strcpy((char *)&strM2MAPConfig.au8WepKey, MAIN_WLAN_WEP_KEY);
        strM2MAPConfig.u8KeySz = strlen(MAIN_WLAN_WEP_KEY);
        strM2MAPConfig.u8KeyIndx = MAIN_WLAN_WEP_KEY_INDEX;
    #endif

    /* Bring up AP mode with parameters structure. */
    ret = m2m_wifi_enable_ap(&strM2MAPConfig);
    if (M2M_SUCCESS != ret) {
        //printf("main: m2m_wifi_enable_ap call error!\r\n");
        while (1) {
        }
    }

    //printf("AP mode started. You can connect to %s.\r\n", (char *)MAIN_WLAN_SSID);
    GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);

    /* Initialize socket address structure. */
    addr.sin_family = AF_INET;
    addr.sin_port = _htons(MAIN_WIFI_M2M_SERVER_PORT);
    addr.sin_addr.s_addr = _htonl(MAIN_WIFI_M2M_SERVER_IP);

    /* Initialize socket module */
    socketInit();
    registerSocketCallback(socket_cb, NULL);

    /* Connect to router. */
    //m2m_wifi_connect((char *)MAIN_WLAN_SSID, sizeof(MAIN_WLAN_SSID), MAIN_WLAN_AUTH, (char *)MAIN_WLAN_PSK, M2M_WIFI_CH_ALL);


    packetCnt = 0;
    GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);
    printf("Here\r\n");
    while (1) {
        if (packetCnt >= MAIN_WIFI_M2M_PACKET_COUNT) {
            printf("UDP Server test Complete!\r\n");
            close(rx_socket);
            rx_socket = -1;
            break;
        }

        m2m_wifi_handle_events(NULL);

        if (wifi_connected == M2M_WIFI_CONNECTED) {
            /* Create socket for Rx UDP */
            if (rx_socket < 0) {
                if ((rx_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
                    printf("main : failed to create RX UDP Client socket error!\r\n");
                    continue;
                }

                /* Socket bind */
                bind(rx_socket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
            }
        }
    }

    return 0;
}


#define CS_MCLK_DESIRED_KHZ (2000)
#define CS_MCLK_FLL_RATIO   (61) //= rounding MCLK desired HZ / 32768
//Sets up Clock signals for the FR2355: MCLK @ 16M, SMCLK (SPI, etc.) @ 8M.

void clockSetup(void){
    CS_initClockSignal(CS_FLLREF, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_1);

    // Set Ratio and Desired MCLK Frequency and initialize DCO. Returns 1 (STATUS_SUCCESS) if good
    // By default, MCLK = DCOCLK = 16M after this settles.
    CS_initFLLSettle(CS_MCLK_DESIRED_KHZ, CS_MCLK_FLL_RATIO);

    // SMCLK = DCOCLK / 2 = 8M
    CS_initClockSignal(CS_SMCLK, CS_DCOCLKDIV_SELECT, CS_CLOCK_DIVIDER_2);

    //Sanity checking? enable and breakpoint on the following values.
//    volatile uint32_t MCLK_val = CS_getMCLK();      //15,990,784 (16M)
//    volatile uint32_t SMCLK_val = CS_getSMCLK();    // 7,995,392 (8M)
}
