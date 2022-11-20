#include <msp430.h>
#include "driver/include/m2m_wifi.h"
#include <stdio.h>
#include <string.h>

#define MAIN_WLAN_SSID           "DEMO_AP2"
#define MAIN_WLAN_AUTH           M2M_WIFI_SEC_OPEN
#define ETH_MTU 1518
#define WINC_FAKE_HDR_LEN 14
typedef uint8 byte;

// Leaving 14 to pad packet since WINC's overwrite first 12B with send/recv of MAC's between the two
byte eth_full_tx_buf[ETH_MTU + WINC_FAKE_HDR_LEN];
byte* eth_tx_buf = eth_full_tx_buf + WINC_FAKE_HDR_LEN;
uint16 eth_tx_full_buf_len;


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

//placeholder for tx
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
    CS_initFLLSettle(CS_MCLK_DESIRED_KHZ, CS_MCLK_FLL_RATIO);
    CS_initClockSignal(CS_SMCLK, CS_DCOCLKDIV_SELECT, CS_CLOCK_DIVIDER_2);
}

//Method to set tx buffer and size.
void set_wifi_tx_buf(byte* in_buf, uint16 in_buf_len){
    memcpy(eth_tx_buf, in_buf, in_buf_len);
    eth_tx_full_buf_len = in_buf_len + WINC_FAKE_HDR_LEN;
}

sint8 send_wifi_tx_buf(){
    //Check if wifi still connected.
    if (wifi_connected == M2M_WIFI_CONNECTED){
        sint8 ret = m2m_wifi_send_ethernet_pkt(eth_full_tx_buf, eth_tx_full_buf_len);
        return ret;
    }
    return M2M_ERR_SEND;
}


//Length: length of data section (46 - 1500)
void generate_test_pkt(uint16 length, uint8 textStart){
    if(length < 46 || length > 1500){
        printf("You goofed - test pkt can't have this length\r\n");
        return;
    }
    eth_tx_full_buf_len = 14 + length + 18; //14 for fake headers, 18 for dest/src/len/crc
    uint16 i = 0;
    for (;i < eth_tx_full_buf_len - 14; ++i){
        eth_tx_buf[i] = ((i + textStart) % 0xFF);
    }
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

    /* Initialize buffer. */
    memset(eth_full_tx_buf, 0xFF, WINC_FAKE_HDR_LEN);

    /* Connect to router. */
    printf("requesting connect\r\n");
    m2m_wifi_connect((char *)MAIN_WLAN_SSID, sizeof(MAIN_WLAN_SSID), MAIN_WLAN_AUTH, (char *)0, M2M_WIFI_CH_ALL);
    GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);

    uint8 alt = 0;

    while (1) {
        m2m_wifi_handle_events(NULL);
        nm_bsp_sleep(100);
        if(wifi_connected == M2M_WIFI_CONNECTED){
            if(alt){
                generate_test_pkt(1500, 0x01);
                ret = send_wifi_tx_buf();
                alt = 0;
            }else{
                generate_test_pkt(50, 0x03);
                ret = send_wifi_tx_buf();
                alt = 1;
            }
            if(ret == M2M_SUCCESS){
                GPIO_setOutputHighOnPin(GPIO_PORT_P6, GPIO_PIN6);
            }else{
                GPIO_setOutputLowOnPin(GPIO_PORT_P6, GPIO_PIN6);
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
