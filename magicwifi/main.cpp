#include <msp430.h> 
#include <cstring>
#include "enc28j60.h"
#include "driverlib.h"
#include "SX127x.h"
#include "FrameTranslater.h"
#include "defines.h"

#include <memory> //For placement new

extern "C" {
    #include "driver/include/m2m_wifi.h"
    #include "driver/source/m2m_hif.h"
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

// This should be in units of secondss
volatile uint32_t time_elapsed = 0;


//-----------------------------------ISR HANDLING------------------

// For real time clock

#pragma vector=RTC_VECTOR
__interrupt void rtc_ISR(void)
{
    time_elapsed++;
    RTC_clearInterrupt(RTC_BASE,  RTC_OVERFLOW_INTERRUPT_FLAG);
}


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

void initRTC(void) {
    // Setup Real-Time Clock, for 100ms intervals
    RTC_init(RTC_BASE, 4000, RTC_CLOCKPREDIVIDER_100); //4000 will take our 4 MHz clock above 0.1 seconds to tick to
    RTC_clearInterrupt(RTC_BASE,  RTC_OVERFLOW_INTERRUPT_FLAG);
    RTC_enableInterrupt(RTC_BASE, RTC_OVERFLOW_INTERRUPT);
    RTC_start(RTC_BASE, RTC_CLOCKSOURCE_SMCLK);
    __enable_interrupt();

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

volatile uint8 wifi_connected = 0;
volatile bool pending_received_wifi_frame = false;
volatile int pending_received_wifi_frame_length;

volatile uint32_t last_received_wifi_frame_timestamp = 0;
volatile uint32_t last_wifi_heartbeat_sent = 0;

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

    return m2m_wifi_init(&param);
}

tstrM2MAPConfig strM2MAPConfig{
    .au8SSID = MAIN_WLAN_SSID,
    .u8ListenChannel = MAIN_WLAN_CHANNEL,
    .u8SecType = M2M_WIFI_SEC_OPEN,
    .au8DHCPServerIP = {192, 168, 1, 1},
    .u8SsidHide = 1
};
sint8 init_AP(void)
{
    return m2m_wifi_enable_ap(&strM2MAPConfig);
}

void eth_rx_cb(uint8 u8MsgType, void *pvMsg, void *pvCtrlBuf){

    tstrM2mIpCtrlBuf *ctrl = (tstrM2mIpCtrlBuf *)pvCtrlBuf;
    switch (u8MsgType)
    {
    case M2M_WIFI_RESP_ETHERNET_RX_PACKET:
        //TODO: Check if this is actually a pulse

        //After this function is called, the rest of the nightmare stack of m2m_wifi_handle_events
        // does many things including immediately reading one (all??) pending frames into our buffer
        // This resets the internal buffer pointer to start of our data.
        m2m_wifi_set_receive_buffer(eth_out_wifi_buff, ETH_BUFF_SIZE + ETH_WIFI_HEADER_SIZE);

        pending_received_wifi_frame_length = ctrl->u16DataSize;
        pending_received_wifi_frame = true;
        hif_yield();

        last_received_wifi_frame_timestamp = time_elapsed;
        break;
    default:
        break;
    }

}

void update_wifi_disconnected(){
    wifi_connected = M2M_WIFI_DISCONNECTED;
    GPIO_setOutputLowOnPin(GPIO_PORT_P6, GPIO_PIN6);
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
            GPIO_setOutputHighOnPin(GPIO_PORT_P6, GPIO_PIN6);
            wifi_connected = M2M_WIFI_CONNECTED;

            last_received_wifi_frame_timestamp = time_elapsed;
        }
        else if (pstrWifiState->u8CurrState == M2M_WIFI_DISCONNECTED)
        {
           update_wifi_disconnected();
        }
    }
    break;
    default:
        break;
    }
}


void setup_wifi() {

    // Fill the beginning of the special wifi buffer with 0xFF
    //TODO: Maybe explain why / what that does at some point
    for(int i = 0; i < ETH_WIFI_HEADER_SIZE; ++i) {
        eth_in_wifi_buff[i] = 0xFF;
    }

    int8_t ret = init_wifi();
    if(M2M_SUCCESS != ret) {
        while(1){
           //Spin - failed to properly initialize Wifi module
        }
    }

#ifdef BOARD_A
    ret = init_AP();
    if(M2M_SUCCESS != ret) {
            while(1){

               //Spin - failed to properly initialize Wifi module as AP
            }
        }

#else
    m2m_wifi_connect((char *)MAIN_WLAN_SSID, sizeof(MAIN_WLAN_SSID), M2M_WIFI_SEC_OPEN, (char *)0, MAIN_WLAN_CHANNEL);
#endif



}

//-----------------------------------MISC--------------------------

bool isFrameHighPriority(const byte* frame){
    //If 802.1Q field present, location where Ethernet length field would usually be will have 0x8100
    if(frame[12] == 0x81 && frame[13] == 0x00){
        //802.1Q tagging information is present
        //Read priority field
        uint8_t priority = frame[14] >> 5;
        return (priority >= 2); //1 and 0 correspond to low-priority items as given by 802.1p
    }
    //Assume that if host isn't tagging priority, we can't safely drop anything.
    return true;
}



alignas(FrameTranslater) unsigned char frameTranslaterBuff[sizeof(FrameTranslater)];


uint8 HEARTBEAT_FRAME[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

/**
 * main.c
 */
int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    PMM_unlockLPM5(); // Unlock GPIO pins -- WILL NOT work without this line, do not remove
    GPIO_setAsOutputPin(GPIO_PORT_P6, GPIO_PIN6);
    GPIO_setOutputLowOnPin(GPIO_PORT_P6, GPIO_PIN6);

    clockSetup();
    initRTC();


    setup_ethernet();
    setup_Transmitlora();
    setup_Receivelora();
    setup_wifi();

    FrameTranslater* frameTranslater = new(frameTranslaterBuff) FrameTranslater (&TransmitLoRa, &ReceiveLoRa);

    bool in_progress_lora_send = false;
    int ethernet_len; //We don't actually use this anywhere, funnily enough, which is maybe a problem?
    // (Do we need to zero-fill the remainder of our Ethernet buffer, or is it ok if we send random garbage following a valid full frame?)

    ReceiveLoRa.request();

    uint32_t last_wifi_connection_attempt_timestamp = 0;

    while(1){
        //Updates current wifi connectivity status and also updates pending received wifi frame status
        m2m_wifi_handle_events(NULL);

        if(pending_received_wifi_frame){
            // If we received less than this, it's not a valid Ethernet frame. Possibly a control frame/heartbeat, possibly random garbage.
            if(pending_received_wifi_frame_length >= ETH_HEADER_SIZE + ETH_WIFI_HEADER_SIZE){
                ether.packetSend(pending_received_wifi_frame_length - ETH_WIFI_HEADER_SIZE); //TODO: Do we need to subtract wifi header length from here?
            }
            pending_received_wifi_frame = false;
            //TODO: Reset LoRa frame assembly logic
        }

        if(wifi_connected && in_progress_lora_send){
            //TODO: Optionally handle this better here, say by sending our current in-progress-sent LoRa Eth. frame over wifi
            in_progress_lora_send = false;
        }

        //If there is a frame available and we're not already busy with an old one, handle the next
        if(eth_available && !in_progress_lora_send){
            //Read in pending frame
            ethernet_len = ether.packetReceive();

            if(wifi_connected){
                //We always want to send a full frame over wifi if available, because it's better in every way
                m2m_wifi_send_ethernet_pkt(eth_in_wifi_buff, ethernet_len + ETH_WIFI_HEADER_SIZE);
                check_set_eth_pending();
            }else{
                //Only send high-priority traffic when we're in LoRa mode due to bandwidth constraints.
                //TODO: We can be a little more clever here and not drop stuff if we don't have any pending Ethernet frames
                if(isFrameHighPriority(eth_in_buff)){
                    //Begin process of sending frame over multiple iterations through LoRa.
                    frameTranslater->initSend(eth_in_buff, ETH_BUFF_SIZE); //TODO: Fixme
                    in_progress_lora_send = true;
                }
            }
        }

        if(in_progress_lora_send){
            in_progress_lora_send = frameTranslater->sendNextSubframe(eth_in_buff, ethernet_len);

            for(volatile uint32_t i=0;i<50000;i++); //Wait so we don't send the LoRa frames too fast!

            if(!in_progress_lora_send){
                //We just finished fully sending our Ethernet frame :)
                // Check if we have more to send
                check_set_eth_pending();
            }
        }

        if(ReceiveLoRa._statusIrq != 0){
            frameTranslater->receiveFrame(eth_out_buff, ETH_BUFF_SIZE);
            uint16_t lora_received_len = 0;
            if(frameTranslater->checkFrame(eth_out_buff, lora_received_len)){
                ether.packetSend(lora_received_len);
            }
            ReceiveLoRa.request();
        }

#ifdef BOARD_B
        if(!wifi_connected){
            if((time_elapsed - last_wifi_connection_attempt_timestamp) > delay_between_wifi_connect_attempts_ts){
                m2m_wifi_connect((char *)MAIN_WLAN_SSID, sizeof(MAIN_WLAN_SSID), M2M_WIFI_SEC_OPEN, (char *)0, MAIN_WLAN_CHANNEL);
                last_wifi_connection_attempt_timestamp = time_elapsed;
            }
        }
#endif
        if(wifi_connected && ((time_elapsed - last_received_wifi_frame_timestamp) > wifi_connection_timeout)){
            update_wifi_disconnected();
        }

        if(wifi_connected && ((time_elapsed - last_wifi_heartbeat_sent) > wifi_heartbeat_period)){
            m2m_wifi_send_ethernet_pkt(HEARTBEAT_FRAME, sizeof(HEARTBEAT_FRAME));
            last_wifi_heartbeat_sent = time_elapsed;
        }
    }
}
