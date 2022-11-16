#include <msp430.h>
#include "driver/include/m2m_wifi.h"
//#include <stdio.h>
#include <string.h>


/**
 * \brief Callback to get the Wi-Fi status update.
 *
 * \param[in] u8MsgType type of Wi-Fi notification. Possible types are:
 *  - [M2M_WIFI_RESP_CON_STATE_CHANGED](@ref M2M_WIFI_RESP_CON_STATE_CHANGED)
 *  - [M2M_WIFI_RESP_SCAN_DONE](@ref M2M_WIFI_RESP_SCAN_DONE)
 *  - [M2M_WIFI_RESP_SCAN_RESULT](@ref M2M_WIFI_RESP_SCAN_RESULT)
 *  - [M2M_WIFI_REQ_DHCP_CONF](@ref M2M_WIFI_REQ_DHCP_CONF)
 * \param[in] pvMsg A pointer to a buffer containing the notification parameters
 * (if any). It should be casted to the correct data type corresponding to the
 * notification type.
 */

#define MAIN_WLAN_SSID           "DEMO_AP2"
#define MAIN_WLAN_AUTH           M2M_WIFI_SEC_OPEN

/** Wi-Fi connection state */
static uint8_t wifi_connected;
static void wifi_cb(uint8_t u8MsgType, void *pvMsg)
{
    switch (u8MsgType) {
    case M2M_WIFI_RESP_CON_STATE_CHANGED:
    {
        tstrM2mWifiStateChanged *pstrWifiState = (tstrM2mWifiStateChanged *)pvMsg;
        if (pstrWifiState->u8CurrState == M2M_WIFI_CONNECTED) {
            printf("wifi_cb: M2M_WIFI_RESP_CON_STATE_CHANGED: CONNECTED\r\n");
            wifi_connected = M2M_WIFI_CONNECTED;

            //m2m_wifi_request_dhcp_client();
        } else if (pstrWifiState->u8CurrState == M2M_WIFI_DISCONNECTED) {
            printf("wifi_cb: M2M_WIFI_RESP_CON_STATE_CHANGED: DISCONNECTED\r\n");
            m2m_wifi_connect((char *)MAIN_WLAN_SSID, sizeof(MAIN_WLAN_SSID), MAIN_WLAN_AUTH, (char *)0, M2M_WIFI_CH_ALL);
            wifi_connected = M2M_WIFI_DISCONNECTED;
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
//makes an AP.

/** Message format declarations. */




#define WINC_RX_BUF_SZ  256
static uint8_t rx_buf[WINC_RX_BUF_SZ];

void winc_netif_rx_callback(uint8 u8MsgType, void* pvMsg, void* pvCtrlBuf){
    switch(u8MsgType){
    volatile tstrM2mIpCtrlBuf *ctrl = (tstrM2mIpCtrlBuf *)pvCtrlBuf;
        case M2M_WIFI_RESP_ETHERNET_RX_PACKET:
            printf("Processing raw packet\r\n");
            break;
        default:
            break;
    }
}
/**
 * \brief Configure RX callback and buffer.
 */
void winc_fill_callback_info(tstrEthInitParam *info)
{
    info->pfAppEthCb = winc_netif_rx_callback;
    info->au8ethRcvBuf = rx_buf;
    info->u16ethRcvBufSize = sizeof(rx_buf);
    info->u8EthernetEnable = 1; //For bypassing the TCPIP Stack of WINC
}

void os_hook_isr(){
    return;
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
}

void main(void) {
    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;                   // Disable the GPIO power-on default high-impedance mode
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0); GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0); //Red ON as soon as we have power
    GPIO_setAsOutputPin(GPIO_PORT_P6, GPIO_PIN6); GPIO_setOutputLowOnPin(GPIO_PORT_P6, GPIO_PIN6);

    tstrWifiInitParam param;
    int8_t ret;

    /* Initialize the board. */
    clockSetup();

    /* Initialize the BSP. */
    nm_bsp_init();

    /* Initialize Wi-Fi parameters structure. */
    memset((uint8_t *)&param, 0, sizeof(tstrWifiInitParam));

    /* Initialize Wi-Fi driver with data and status callbacks. */
    param.pfAppWifiCb = wifi_cb;
    winc_fill_callback_info(&param.strEthInitParam);
    ret = m2m_wifi_init(&param);
    if (M2M_SUCCESS != ret) {
        printf("main: m2m_wifi_init call error!(%d)\r\n", ret);
        while (1) {
        }
    }

    /* Connect to router. */
    printf("requesting connect\r\n");
    m2m_wifi_connect((char *)MAIN_WLAN_SSID, sizeof(MAIN_WLAN_SSID), MAIN_WLAN_AUTH, (char *)0, M2M_WIFI_CH_ALL);
    GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);

    uint8 msg[68] = {0x44, 0xe5, 0x17, 0xd2, 0xe6, 0xf5, 0x2c, 0x21, 0x31, 0x27, 0x58, 0xf0, 0x08, 0x00, 0x45, 0x80, 0x00, 0x36, 0x00, 0x00, 0x40, 0x00, 0x3b, 0x11, 0xca, 0xf8, 0xac, 0xd9, 0x00, 0xae, 0x23, 0x03, 0xa3, 0xb4, 0x01, 0xbb, 0xe6, 0xfb, 0x00, 0x22, 0xea, 0xec, 0x5e, 0x35, 0x94, 0x2a, 0x16, 0x08, 0x36, 0xb4, 0xc1, 0x12, 0x98, 0xc7, 0x19, 0x5b, 0x3b, 0x67, 0xe3, 0xcf, 0xc8, 0x55, 0x99, 0x38, 0x78, 0xd1, 0x0b, 0xdf};

    /** UDP packet count */
    static uint8_t packetCnt = 0;
    while (1) {
        m2m_wifi_handle_events(NULL);

        if (wifi_connected == M2M_WIFI_CONNECTED) {

//            ret = sendto(tx_socket, &msg_wifi_product_main, sizeof(t_msg_wifi_product_main), 0, (struct sockaddr *)&addr, sizeof(addr));
            //printf("About to send message");
            nm_bsp_sleep(10);
            ret = m2m_wifi_send_ethernet_pkt(msg, 68);
            printf("%u, %u", sizeof(msg), sizeof(msg[0]));
            if (ret == M2M_SUCCESS) {
                GPIO_setOutputHighOnPin(GPIO_PORT_P6, GPIO_PIN6);
                //printf("main: message sent\r\n");
                //packetCnt += 1;
                if (packetCnt == 100) {
                    printf("Eth level client test Complete!\r\n");
                }
            } else {
                GPIO_setOutputLowOnPin(GPIO_PORT_P6, GPIO_PIN6);
                printf("main: failed to send status report error!\r\n");
            }
        }
    }
}


/*From Example*/
/**
 *
 * \file
 *
 * \brief WINC1500 AP Scan Example.
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
 * This example demonstrates the use of the WINC1500 with the SAM Xplained Pro
 * board to scan for access point.<br>
 * It uses the following hardware:
 * - the SAM Xplained Pro.
 * - the WINC1500 on EXT1.
 *
 * \section files Main Files
 * - main.c : Initialize the WINC1500 and scan AP until defined AP is founded.
 *
 * \section usage Usage
 * -# Configure below code in the main.h for AP to be connected.
 * \code
 *    #define MAIN_WLAN_SSID         "DEMO_AP"
 *    #define MAIN_WLAN_AUTH         M2M_WIFI_SEC_WPA_PSK
 *    #define MAIN_WLAN_PSK          "12345678"
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
 *    -- WINC1500 AP scan example --
 *    -- SAM_XPLAINED_PRO --
 *    -- Compiled: xxx xx xxxx xx:xx:xx --
 *
 *    [1] SSID:DEMO_AP1
 *    [2] SSID:DEMO_AP2
 *    [3] SSID:DEMO_AP
 *    Found DEMO_AP
 *    Wi-Fi connected
 *    Wi-Fi IP is xxx.xxx.xxx.xxx
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
