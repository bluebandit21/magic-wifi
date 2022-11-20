//Board B (STA)
#include <msp430.h>
#include "driver/include/m2m_wifi.h"
#include <string.h>

//------- Defines -------
#define MAIN_WLAN_SSID           "DEMO_AP"
#define MAIN_WLAN_CHANNEL        (6)
#define ETH_MTU                  1518
#define FAKE_ETH_HDR_LEN         14

//TODO modernize without breaking everything
#define CS_MCLK_DESIRED_KHZ (2000)
#define CS_MCLK_FLL_RATIO   (61) //= rounding MCLK desired HZ / 32768

typedef uint8 byte;

//------- Globals -------
byte eth_full_rx_buf[ETH_MTU + FAKE_ETH_HDR_LEN];
byte eth_full_tx_buf[ETH_MTU + FAKE_ETH_HDR_LEN];

byte* eth_rx_buf = eth_full_rx_buf + FAKE_ETH_HDR_LEN;
byte* eth_tx_buf = eth_full_tx_buf + FAKE_ETH_HDR_LEN;

uint16 eth_rx_full_buf_len, eth_tx_full_buf_len;

uint8 wifi_connected;

//------- Headers -------
void set_leds(uint8 red, uint8 green);

//------- Callback functions -------
void os_hook_isr(){return;}

void wifi_cb(uint8_t u8MsgType, void *pvMsg){
    switch (u8MsgType) {
    case M2M_WIFI_RESP_CON_STATE_CHANGED:
    {
        tstrM2mWifiStateChanged *pstrWifiState = (tstrM2mWifiStateChanged *)pvMsg;
        if (pstrWifiState->u8CurrState == M2M_WIFI_CONNECTED) {
            wifi_connected = M2M_WIFI_CONNECTED;
            set_leds(1,0);
            nm_bsp_sleep(500);
            generate_test_pkt(50, 0x01);
            send_wifi_tx_buf();
            //m2m_wifi_request_dhcp_client(); //?
        } else if (pstrWifiState->u8CurrState == M2M_WIFI_DISCONNECTED) {
            m2m_wifi_connect((char *)MAIN_WLAN_SSID, sizeof(MAIN_WLAN_SSID), M2M_WIFI_SEC_OPEN, (char *)0, MAIN_WLAN_CHANNEL);
            nm_bsp_sleep(500);
            wifi_connected = M2M_WIFI_DISCONNECTED;
            set_leds(0,0);
        }
    }
    break;
    default:
        break;
    }
}

void eth_rx_cb(uint8 u8MsgType, void* pvMsg, void* pvCtrlBuf){
    tstrM2mIpCtrlBuf *ctrl = (tstrM2mIpCtrlBuf *)pvCtrlBuf;
    switch(u8MsgType){
        case M2M_WIFI_RESP_ETHERNET_RX_PACKET:
            eth_rx_full_buf_len = ctrl->u16DataSize;
            m2m_wifi_set_receive_buffer(eth_full_rx_buf, ETH_MTU + 14);
            set_leds(1,1);
            nm_bsp_sleep(200);
            generate_test_pkt(50, 0x01);
            send_wifi_tx_buf();
            set_leds(1,0);
            break;
        default:
            break;
    }
}

//------- Setup functions -------
void clockSetup(void){
    CS_initClockSignal(CS_FLLREF, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    CS_initFLLSettle(CS_MCLK_DESIRED_KHZ, CS_MCLK_FLL_RATIO);
    CS_initClockSignal(CS_SMCLK, CS_DCOCLKDIV_SELECT, CS_CLOCK_DIVIDER_2);
}

sint8 init_wifi(void){
    nm_bsp_init();
    tstrWifiInitParam param;
    memset((uint8_t *)&param, 0, sizeof(tstrWifiInitParam));
    param.pfAppWifiCb = wifi_cb;
    param.strEthInitParam.pfAppEthCb = eth_rx_cb;
    param.strEthInitParam.au8ethRcvBuf = eth_full_rx_buf;
    param.strEthInitParam.u16ethRcvBufSize = ETH_MTU + FAKE_ETH_HDR_LEN;
    param.strEthInitParam.u8EthernetEnable = 1;
    return m2m_wifi_init(&param);
}

sint8 init_AP(void){
    tstrM2MAPConfig strM2MAPConfig;
    memset(&strM2MAPConfig, 0x00, sizeof(tstrM2MAPConfig));
    strcpy((char *)&strM2MAPConfig.au8SSID, MAIN_WLAN_SSID);
    strM2MAPConfig.u8ListenChannel = MAIN_WLAN_CHANNEL;
    strM2MAPConfig.u8SecType = M2M_WIFI_SEC_OPEN;
    strM2MAPConfig.au8DHCPServerIP[0] = 192; //TODO do we care?
    strM2MAPConfig.au8DHCPServerIP[1] = 168;
    strM2MAPConfig.au8DHCPServerIP[2] = 1;
    strM2MAPConfig.au8DHCPServerIP[3] = 1;
    return m2m_wifi_enable_ap(&strM2MAPConfig);
}
//------- Helper functions -------
void set_leds(uint8 red, uint8 green){
    if(red){
        GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);
    }else{
        GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);
    }
    if(green){
        GPIO_setOutputHighOnPin(GPIO_PORT_P6, GPIO_PIN6);
    }else{
        GPIO_setOutputLowOnPin(GPIO_PORT_P6, GPIO_PIN6);
    }
}

//Method to set tx buffer and size.
void set_wifi_tx_buf(byte* in_buf, uint16 in_buf_len){
    memcpy(eth_tx_buf, in_buf, in_buf_len);
    eth_tx_full_buf_len = in_buf_len + FAKE_ETH_HDR_LEN;
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
//TextStart: what first element begins as, so full buffer is replaced in memory browser
void generate_test_pkt(uint16 length, uint8 textStart){
    if(length < 46 || length > 1500){
        //printf("You goofed - test pkt can't have this length\r\n");
        return;
    }
    eth_tx_full_buf_len = 14 + length + 18; //14 for fake headers, 18 for dest/src/len/crc
    uint16 i = 0;
    for (;i < eth_tx_full_buf_len - 14; ++i){
        eth_tx_buf[i] = ((i + textStart) % 0xFF);
    }
}

//------- Main program -------

void main(void){
    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;                   // Disable the GPIO power-on default high-impedance mode
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);
    GPIO_setAsOutputPin(GPIO_PORT_P6, GPIO_PIN6);
    set_leds(1,1);
    clockSetup();

    //Wifi Initialization
    sint8 ret = init_wifi();
    if (M2M_SUCCESS != ret) {
        //main: m2m_wifi_init call error
        while (1) {
        }
    }

    // Fill in fake buffer portion.
    memset(eth_full_tx_buf, 0xFF, FAKE_ETH_HDR_LEN); //TODO: Affected by sharing with LoRa?

    // Request connection with host.
    m2m_wifi_connect((char *)MAIN_WLAN_SSID, sizeof(MAIN_WLAN_SSID), M2M_WIFI_SEC_OPEN, (char *)0, MAIN_WLAN_CHANNEL);

    //Indicate initialization success
    set_leds(0,0);

    while(1){
        m2m_wifi_handle_events(NULL);
    }
}
