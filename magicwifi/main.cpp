#include <msp430.h> 
#include <cstring>
#include "enc28j60.h"
#include "driverlib.h"
#include "SX127x.h"
#include "FrameTranslater.h"
#include "defines.h"

extern "C" {
    #include "driver/include/m2m_wifi.h"
}

extern "C" {
    void eth_rx_cb(uint8 u8MsgType, void *pvMsg, void *pvCtrlBuf);
}

extern "C" {
    void wifi_cb(uint8_t u8MsgType, void *pvMsg);
}

#define DEBUG_ABORT() while(1){}


//----------------------------------GLOBALS----------------------
SX127x TransmitLoRa(LORA::SENDER);
SX127x ReceiveLoRa(LORA::RECEIVER);

//TODO: Describe thoroughly and possibly give better names!
byte eth_in_wifi_buff[ETH_BACKING_SIZE];
byte* eth_in_buff = eth_in_wifi_buff + ETH_WIFI_HEADER_SIZE;
byte eth_out_wifi_buff[ETH_BACKING_SIZE];
byte* eth_out_buff = eth_out_wifi_buff + ETH_WIFI_HEADER_SIZE;


//-----------------------------------ISR HANDLING------------------

//3.4 is the Wifi IRQ, 3.1 is the receive LoRa IRQ


#pragma vector=PORT3_VECTOR
__interrupt void port3_ISR(void)
{
    bool known = false;

    if(GPIO_getInterruptStatus(WIFI_IRQ_PORT, WIFI_IRQ_PIN)){
        if(wifi_isr) wifi_isr();
        GPIO_clearInterrupt(WIFI_IRQ_PORT, WIFI_IRQ_PIN);
        known = true;
    }

    if(GPIO_getInterruptStatus(LORA_RECEIVE_IRQ_PORT, LORA_RECEIVE_IRQ_PIN)){
        (ReceiveLoRa.*(ReceiveLoRa.curr_callback))();
        GPIO_clearInterrupt(LORA_RECEIVE_IRQ_PORT, LORA_RECEIVE_IRQ_PIN);
        known = true;
    }

    if(!known) DEBUG_ABORT();
}

//2.1 is the Ethernet IRQ, 2.4 is the send LoRa IRQ

void (*ethernet_isr)(void) = nullptr;

#pragma vector=PORT2_VECTOR
__interrupt void port2_ISR(void)
{
    bool known = false;

    if(GPIO_getInterruptStatus(LORA_SEND_IRQ_PORT, LORA_SEND_IRQ_PIN)){
        (TransmitLoRa.*(TransmitLoRa.curr_callback))();
        GPIO_clearInterrupt(LORA_SEND_IRQ_PORT, LORA_SEND_IRQ_PIN);
        known = true;
    }

    if(GPIO_getInterruptStatus(ETH_IRQ_PORT, ETH_IRQ_PIN)){
        if(ethernet_isr) ethernet_isr();
        GPIO_clearInterrupt(ETH_IRQ_PORT, ETH_IRQ_PIN);
        known = true;
    }

    if(!known) DEBUG_ABORT();
}

//---------------------------------CLOCK SETUP--------------------------------
constexpr unsigned CS_MCLK_DESIRED_KHZ = (16000);
constexpr unsigned CS_MCLK_FLL_RATIO  = (((uint32_t) CS_MCLK_DESIRED_KHZ) * 1000) / 32768;
//Sets up Clock signals for the FR2355: MCLK @ 16M, SMCLK (SPI, etc.) @ 8M.
void clockSetup(void){
    CS_initClockSignal(CS_FLLREF, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_1);

    // Set Ratio and Desired MCLK Frequency and initialize DCO. Returns 1 (STATUS_SUCCESS) if good
    // By default, MCLK = DCOCLK = 16M after this settles.
    CS_initFLLSettle(CS_MCLK_DESIRED_KHZ, CS_MCLK_FLL_RATIO);

    // SMCLK = DCOCLK / 2 = 8M
    CS_initClockSignal(CS_SMCLK, CS_DCOCLKDIV_SELECT, CS_CLOCK_DIVIDER_4);
}

//-----------------------------------ETHERNET----------------------
ENC28J60 ether;
volatile bool eth_available = false;

static byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };

void ethernet_receive_handler(){
    eth_available = true;
}

void check_set_eth_pending(){
    eth_available = !GPIO_getInputPinValue(ETH_IRQ_PORT, ETH_IRQ_PIN);
    //Prevent possible race condition -- the above line could theoretically be pre-empted by the Ethernet ISR in between
    // us checking the GPIO's value and actually writing it into eth_available. This would, if it happened, result
    // in us blocking forever as the ISR would never trigger again (falling edge interrupt) and this function would never be called.
    // In that very specific case, re-checking the value of the pin *after* the assignment lets us safely avoid this scenario being a possibility.
    if(!GPIO_getInputPinValue(ETH_IRQ_PORT, ETH_IRQ_PIN)){
        eth_available = true;
    }
}

void setup_ethernet() {
    // put your setup code here, to run once:
    ether.initialize(ETH_BACKING_SIZE, mymac, eth_out_buff, eth_in_buff);

    //Enable all the things
    ether.enablePromiscuous(false);

    GPIO_setAsInputPin(ETH_IRQ_PORT, ETH_IRQ_PIN);
    GPIO_selectInterruptEdge(ETH_IRQ_PORT, ETH_IRQ_PIN, GPIO_HIGH_TO_LOW_TRANSITION);

    ethernet_isr = ethernet_receive_handler;
    GPIO_clearInterrupt(ETH_IRQ_PORT, ETH_IRQ_PIN);
    GPIO_enableInterrupt(ETH_IRQ_PORT, ETH_IRQ_PIN);

    __enable_interrupt();
}



//-----------------------------------LORA--------------------------



void setup_Transmitlora() {
    if(!TransmitLoRa.begin()){
        while(1){
            //Spin and cry
        }
    }

    // ADDED, set Tx Frequency
    TransmitLoRa.setFrequency(LORA_TRANSMIT_FREQ);

    // Set TX power, this function will set PA config with optimal setting for requested TX power
    // Serial.println("Set TX power to +17 dBm");
    TransmitLoRa.setTxPower(17, SX127X_TX_POWER_PA_BOOST);                    // TX power +17 dBm using PA boost pin

    // Configure modulation parameter including spreading factor (SF), bandwidth (BW), and coding rate (CR)
    // Transmitter must have same SF and BW setting so receiver can receive LoRa packet
    // Serial.println("Set modulation parameters:\n\tSpreading factor = 7\n\tBandwidth = 125 kHz\n\tCoding rate = 4/5");
    TransmitLoRa.setSpreadingFactor(7);                                       // LoRa spreading factor: 7
    TransmitLoRa.setBandwidth(500000);                                        // Bandwidth: 125 kHz
    TransmitLoRa.setCodeRate(5);                                              // Coding rate: 4/5

    // Configure packet parameter including header type, preamble length, payload length, and CRC type
    // The explicit packet includes header contain CR, number of byte, and CRC type
    // Packet with explicit header can't be received by receiver with implicit header mode
    // Serial.println("Set packet parameters:\n\tExplicit header type\n\tPreamble length = 12\n\tPayload Length = 15\n\tCRC on");
    TransmitLoRa.setHeaderType(SX127X_HEADER_EXPLICIT);                       // Explicit header mode
    TransmitLoRa.setPreambleLength(12);                                       // Set preamble length to 12
    TransmitLoRa.setPayloadLength(15);                                        // Initialize payloadLength to 15
    TransmitLoRa.setCrcEnable(true);                                          // Set CRC enable

    // Set syncronize word
    // Serial.println("Set syncronize word to 0x34");
    TransmitLoRa.setSyncWord(0x34);
}

void setup_Receivelora(){
    if(!ReceiveLoRa.begin()){
        while(1){
            //Spin
        }
    }

    // Set frequency to 917 Mhz
    //Serial.println("Set frequency to 917 Mhz");
    ReceiveLoRa.setFrequency(LORA_RECEIVE_FREQ);

    // Set RX gain. RX gain option are power saving gain or boosted gain
    // Serial.println("Set RX gain to power saving gain");
    ReceiveLoRa.setRxGain(SX127X_RX_GAIN_POWER_SAVING, SX127X_RX_GAIN_AUTO); // AGC on, Power saving gain

    // Configure modulation parameter including spreading factor (SF), bandwidth (BW), and coding rate (CR)
    // Transmitter must have same SF and BW setting so receiver can receive LoRa packet
    // Serial.println("Set modulation parameters:\n\tSpreading factor = 7\n\tBandwidth = 125 kHz\n\tCoding rate = 4/5");
    ReceiveLoRa.setSpreadingFactor(7);                                       // LoRa spreading factor: 7
    ReceiveLoRa.setBandwidth(500000);                                        // Bandwidth: 125 kHz
    ReceiveLoRa.setCodeRate(5);                                              // Coding rate: 4/5

    // Configure packet parameter including header type, preamble length, payload length, and CRC type
    // The explicit packet includes header contain CR, number of byte, and CRC type
    // Packet with explicit header can't be received by receiver with implicit header mode
    // Serial.println("Set packet parameters:\n\tExplicit header type\n\tPreamble length = 12\n\tPayload Length = 15\n\tCRC on");
    ReceiveLoRa.setHeaderType(SX127X_HEADER_EXPLICIT);                       // Explicit header mode
    ReceiveLoRa.setPreambleLength(12);                                       // Set preamble length to 12
    ReceiveLoRa.setPayloadLength(15);                                        // Initialize payloadLength to 15
    ReceiveLoRa.setCrcEnable(true);                                          // Set CRC enable

    // Set syncronize word
    // Serial.println("Set syncronize word to 0x34");
    ReceiveLoRa.setSyncWord(0x34);

}

//-----------------------------------WIFI--------------------------

uint8 wifi_connected;

sint8 init_wifi(void)
{
    nm_bsp_init();
    tstrWifiInitParam param = {
       .pfAppWifiCb = wifi_cb,
       .strEthInitParam.pfAppEthCb = eth_rx_cb,
       .strEthInitParam.au8ethRcvBuf = eth_out_wifi_buff,
       .strEthInitParam.u16ethRcvBufSize = ETH_BACKING_SIZE,
       .strEthInitParam.u8EthernetEnable = 1
    };

    /* Reworked to make cleaner
    memset((uint8_t *)&param, 0, sizeof(tstrWifiInitParam));
    param.pfAppWifiCb = wifi_cb;
    param.strEthInitParam.pfAppEthCb = eth_rx_cb;
    param.strEthInitParam.au8ethRcvBuf = eth_out_wifi_buff;
    param.strEthInitParam.u16ethRcvBufSize = ETH_BACKING_SIZE;
    param.strEthInitParam.u8EthernetEnable = 1;
    */
    return m2m_wifi_init(&param);
}


sint8 init_AP(void)
{
    tstrM2MAPConfig strM2MAPConfig = {
        .u8ListenChannel = MAIN_WLAN_CHANNEL,
        .u8SecType = M2M_WIFI_SEC_OPEN
    };
    strcpy((char *)&strM2MAPConfig.au8SSID, MAIN_WLAN_SSID); // TODO:: Fix??

    /*
    memset(&strM2MAPConfig, 0x00, sizeof(tstrM2MAPConfig));
    strcpy((char *)&strM2MAPConfig.au8SSID, MAIN_WLAN_SSID);
    strM2MAPConfig.u8ListenChannel = MAIN_WLAN_CHANNEL;
    strM2MAPConfig.u8SecType = M2M_WIFI_SEC_OPEN;
    strM2MAPConfig.au8DHCPServerIP[0] = 192; // TODO do we care?
    strM2MAPConfig.au8DHCPServerIP[1] = 168;
    strM2MAPConfig.au8DHCPServerIP[2] = 1;
    strM2MAPConfig.au8DHCPServerIP[3] = 1;
     */
    return m2m_wifi_enable_ap(&strM2MAPConfig);
}

void eth_rx_cb(uint8 u8MsgType, void *pvMsg, void *pvCtrlBuf){
    /*
    tstrM2mIpCtrlBuf *ctrl = (tstrM2mIpCtrlBuf *)pvCtrlBuf;
    switch (u8MsgType)
    {
    case M2M_WIFI_RESP_ETHERNET_RX_PACKET:
        eth_rx_full_buf_len = ctrl->u16DataSize;
        m2m_wifi_set_receive_buffer(eth_full_rx_buf, ETH_MTU + 14);
        set_leds(1, 1);
        nm_bsp_sleep(200);
        // send a packet back
        generate_test_pkt(50, 0x01);
        send_wifi_tx_buf();
        set_leds(1, 0);
        break;
    default:
        break;
    }
    */
}

void wifi_cb(uint8_t u8MsgType, void *pvMsg)
{
    switch (u8MsgType)
    {
    case M2M_WIFI_RESP_CON_STATE_CHANGED:
    {
        tstrM2mWifiStateChanged *pstrWifiState = (tstrM2mWifiStateChanged *)pvMsg;
        if (pstrWifiState->u8CurrState == M2M_WIFI_CONNECTED)
        {
            wifi_connected = M2M_WIFI_CONNECTED;
        }
        else if (pstrWifiState->u8CurrState == M2M_WIFI_DISCONNECTED)
        {
            wifi_connected = M2M_WIFI_DISCONNECTED;
        }
    }
    break;
    default:
        break;
    }
}


void setup_wifi() {
    int8_t ret = init_wifi();
    if(M2M_SUCCESS != ret) {
        while(1){
           //Spin - failed to properly initialize Wifi module
        }
    }

#ifdef BOARD_A
    int8_t ret =init_AP();
    if(M2M_SUCCESS != ret) {
            while(1){
               //Spin - failed to properly initialize Wifi module as AP
            }
        }

#else
    m2m_wifi_connect((char *)MAIN_WLAN_SSID, sizeof(MAIN_WLAN_SSID), M2M_WIFI_SEC_OPEN, (char *)0, MAIN_WLAN_CHANNEL);
#endif



}



/**
 * main.c
 */
int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    PMM_unlockLPM5(); // Unlock GPIO pins -- WILL NOT work without this line, do not remove

    clockSetup();

    // Fill the beginning of the special wifi buffer with 0xFF
    for(int i = 0; i < ETH_WIFI_HEADER_SIZE; ++i) {
        eth_in_wifi_buff[i] = 0xFF;
    }


    setup_ethernet();
    setup_Transmitlora();
    setup_Receivelora();
    setup_wifi();

    FrameTranslater frameTranslater = FrameTranslater(&TransmitLoRa, &ReceiveLoRa);


    bool in_progress_ethernet = false;
    int ethernet_len; //We don't actually use this anywhere, funnily enough, which is maybe a problem?
    // (Do we need to zero-fill the remainder of our Ethernet buffer, or is it ok if we send random garbage following a valid full frame?)

    ReceiveLoRa.request();

    while(1){

        if(eth_available && !in_progress_ethernet){
            ethernet_len = ether.packetReceive();
            frameTranslater.initSend(eth_in_buff, ETH_BUFF_SIZE);
            in_progress_ethernet = true;
        }

        if(in_progress_ethernet){
            in_progress_ethernet = frameTranslater.sendNextSubframe(eth_in_buff, ETH_BUFF_SIZE);

            for(volatile uint32_t i=0;i<50000;i++); //Wait so we don't send the LoRa frames too fast!

            if(!in_progress_ethernet){
                //We just finished fully sending our Ethernet frame :)
                // Check if we have more to send
                check_set_eth_pending();
            }
        }

        if(ReceiveLoRa._statusIrq != 0){
            frameTranslater.receiveFrame(eth_out_buff, ETH_BUFF_SIZE);
            if(frameTranslater.checkFrame(eth_out_buff, ETH_BUFF_SIZE)){
                ether.packetSend(ETH_BUFF_SIZE);
            }
            ReceiveLoRa.request();
        }

    }

    return 0;
}
