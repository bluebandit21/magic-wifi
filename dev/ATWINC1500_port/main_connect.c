/*
 * main.c
 *
 *  Created on: Oct 31, 2022
 *      Author: Tom
 */

#include <msp430.h>
#include "driver/include/m2m_wifi.h"
#include "driver/source/nmasic.h"
//#include <stdio.h>
#include <string.h>

#define MAIN_WLAN_SSID           "MSetup"
#define MAIN_WLAN_AUTH           M2M_WIFI_SEC_OPEN
//#define MAIN_WLAN_PSK            (0)
/** Index of scan list to request scan result. */
static uint8_t scan_request_index = 0;
/** Number of APs found. */
static uint8_t num_founded_ap = 0;
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
static void wifi_cb(uint8_t u8MsgType, void *pvMsg)
{
    switch (u8MsgType) {
    case M2M_WIFI_RESP_SCAN_DONE:
    {
        tstrM2mScanDone *pstrInfo = (tstrM2mScanDone *)pvMsg;
        scan_request_index = 0;
        if (pstrInfo->u8NumofCh >= 1) {
            m2m_wifi_req_scan_result(scan_request_index);
            scan_request_index++;
        } else {
            m2m_wifi_request_scan(M2M_WIFI_CH_ALL);
        }

        break;
    }

    case M2M_WIFI_RESP_SCAN_RESULT:
    {
        tstrM2mWifiscanResult *pstrScanResult = (tstrM2mWifiscanResult *)pvMsg;
        uint16_t demo_ssid_len;
        uint16_t scan_ssid_len = strlen((const char *)pstrScanResult->au8SSID);

        /* display founded AP. */
        printf("[%d] SSID:%s\r\n", scan_request_index, pstrScanResult->au8SSID);

        num_founded_ap = m2m_wifi_get_num_ap_found();
        if (scan_ssid_len) {
            /* check same SSID. */
            demo_ssid_len = strlen((const char *)MAIN_WLAN_SSID);
            if
            (
                (demo_ssid_len == scan_ssid_len) &&
                (!memcmp(pstrScanResult->au8SSID, (uint8_t *)MAIN_WLAN_SSID, demo_ssid_len))
            ) {
                /* A scan result matches an entry in the preferred AP List.
                 * Initiate a connection request.
                 */
                printf("Found %s \r\n", MAIN_WLAN_SSID);
                m2m_wifi_connect((char *)MAIN_WLAN_SSID,
                        sizeof(MAIN_WLAN_SSID),
                        MAIN_WLAN_AUTH,
                        (void *)0, //passwd
                        6);//channel
                break;
            }
        }

        if (scan_request_index < num_founded_ap) {
            m2m_wifi_req_scan_result(scan_request_index);
            scan_request_index++;
        } else {
            printf("can not find AP %s\r\n", MAIN_WLAN_SSID);
            m2m_wifi_request_scan(M2M_WIFI_CH_ALL);
        }

        break;
    }

    case M2M_WIFI_RESP_CON_STATE_CHANGED:
    {
        tstrM2mWifiStateChanged *pstrWifiState = (tstrM2mWifiStateChanged *)pvMsg;
        if (pstrWifiState->u8CurrState == M2M_WIFI_CONNECTED) {
            printf("Wi-Fi waiting for DHCP...");
            GPIO_setOutputHighOnPin(GPIO_PORT_P6, GPIO_PIN6);
            m2m_wifi_request_dhcp_client();
        } else if (pstrWifiState->u8CurrState == M2M_WIFI_DISCONNECTED) {
            printf("Wi-Fi disconnected\r\n");

            /* Request scan. */
            m2m_wifi_request_scan(M2M_WIFI_CH_ALL);
        }

        break;
    }

    case M2M_WIFI_REQ_DHCP_CONF:
    {
        uint8_t *pu8IPAddress = (uint8_t *)pvMsg;
        printf("Wi-Fi connected\r\n");
        printf("Wi-Fi IP is %u.%u.%u.%u\r\n", pu8IPAddress[0], pu8IPAddress[1], pu8IPAddress[2], pu8IPAddress[3]);
        break;
    }

    default:
    {
        break;
    }
    }
}

//makes an AP.
void main(void) {
    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;                   // Disable the GPIO power-on default high-impedance mode
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0); GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0); //Red ON as soon as we have power
    GPIO_setAsOutputPin(GPIO_PORT_P6, GPIO_PIN6); GPIO_setOutputLowOnPin(GPIO_PORT_P6, GPIO_PIN6);

    tstrWifiInitParam param;
    int8_t ret;

    /* Initialize the board. */
    //Do our thing
    clockSetup();

    /* Initialize the UART console. */
    //configure_console();

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
    printf("requesting scan\r\n");

    /* Request scan. */
    m2m_wifi_request_scan(M2M_WIFI_CH_ALL);
    GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);
    while (1) {
        /* Handle pending events from network controller. */
        while (m2m_wifi_handle_events(NULL) != M2M_SUCCESS) {
        }
    }
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
    volatile uint32_t MCLK_val = CS_getMCLK();      //15,990,784 (16M)
    volatile uint32_t SMCLK_val = CS_getSMCLK();    // 7,995,392 (8M)
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