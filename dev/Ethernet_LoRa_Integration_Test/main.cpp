#include <msp430.h> 
#include "enc28j60.h"
#include "driverlib.h"
#include "SX127x.h"
#include "FrameTranslater.h"


#define DEBUG_ABORT() while(1){}


//-----------------------------------ISR HANDLING------------------


//3.4 is the Wifi IRQ, 3.1 is the receive LoRa IRQ
void (*lora_receive_isr)(void) = nullptr;
void (*wifi_isr)(void) = nullptr;


#pragma vector=PORT3_VECTOR
__interrupt void port3_ISR(void)
{
    bool known = false;

    if(GPIO_getInterruptStatus(GPIO_PORT_P3, GPIO_PIN4)){
        if(wifi_isr) wifi_isr();
        GPIO_clearInterrupt(GPIO_PORT_P3, GPIO_PIN4);
        known = true;
    }

    if(GPIO_getInterruptStatus(GPIO_PORT_P3, GPIO_PIN1)){
        if(lora_receive_isr) lora_receive_isr();
        GPIO_clearInterrupt(GPIO_PORT_P3, GPIO_PIN1);
        known = true;
    }

    if(!known) DEBUG_ABORT();
}

//2.1 is the Ethernet IRQ, 2.4 is the send LoRa IRQ
void (*lora_send_isr)(void) = nullptr;
void (*ethernet_isr)(void) = nullptr;

#pragma vector=PORT2_VECTOR
__interrupt void port2_ISR(void)
{
    bool known = false;

    if(GPIO_getInterruptStatus(GPIO_PORT_P2, GPIO_PIN4)){
        if(lora_send_isr) lora_send_isr();
        GPIO_clearInterrupt(GPIO_PORT_P2, GPIO_PIN4);
        known = true;
    }

    if(GPIO_getInterruptStatus(GPIO_PORT_P2, GPIO_PIN1)){
        if(ethernet_isr) ethernet_isr();
        GPIO_clearInterrupt(GPIO_PORT_P2, GPIO_PIN1);
        known = true;
    }

    if(!known) DEBUG_ABORT();
}

//-----------------------------------ETHERNET----------------------
ENC28J60 ether;
constexpr ETH_MTU = 1518;
byte ENC28J60::buffer[ETH_MTU];

static byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };

void setup_ethernet() {
    // put your setup code here, to run once:
    ether.initialize(sizeof Ethernet::buffer, mymac);

    //Enable all the things
    ether.enablePromiscuous(false);
    ether.enableMulticast();
    ether.enableBroadcast(false);
}

//-----------------------------------LORA--------------------------

SX127x TransmitLoRa(0);
SX127x ReceiveLoRa(1);


void setup_Transmitlora() {
    if(!TransmitLoRa.begin()){
        while(1){
            //Spin and cry
        }
    }

    // ADDED, set Tx Frequency
    TransmitLoRa.setFrequency(916000000);

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
    ReceiveLoRa.setFrequency(917E6);

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



/**
 * main.c
 */
int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    //setup_ethernet();
    setup_Transmitlora();
    setup_Receivelora();

    char message[] = "Longer Message, chunk into a few frames each";
    uint8_t nBytes = sizeof(message);
    uint8_t counter = 0;
    char receiveBuffer[100];

    while(1){
        /*
         *
         int len = ether.packetReceive();
        for(int i=len;i<ETH_MTU;i++){
            ENC28J60::buffer[i] = 0;
        }

        if(len > 0){
            volatile int x;
            x = 1;
        }
        */


        /*------------____TRANSMIT GARBAGE--------------------

        // Transmit message and counter
                  // write() method must be placed between beginPacket() and endPacket()
        TransmitLoRa.beginPacket();
        TransmitLoRa.write(message, nBytes);
        TransmitLoRa.write(counter);
        TransmitLoRa.endPacket();
        counter++;

        // Print message and counter in serial
        // Serial.write(message, nBytes);
        // Serial.print("  ");
        // Serial.println(counter++);

        // Wait until modulation process for transmitting packet finish
        TransmitLoRa.wait();

        // Print transmit time
        // Serial.print("Transmit time: ");
        // Serial.print(LoRa.transmitTime());
        // Serial.println(" ms");
        // Serial.println();
         *
         */


        ReceiveLoRa.request();
        // Wait for incoming LoRa packet
        ReceiveLoRa.wait();

        // Put received packet to message and counter variable
        // read() and available() method must be called after request() method
        const uint8_t msgLen = ReceiveLoRa.available() - 1;
        //char message[msgLen];
        ReceiveLoRa.read(receiveBuffer, msgLen);
        uint8_t counter = ReceiveLoRa.read();

        // Print received message and counter in serial
        // Serial.write(message, msgLen);
        // Serial.print("  ");
        // Serial.println(counter);

        // Print packet / signal status
        // Serial.print("RSSI: ");
        // Serial.print(LoRa.packetRssi());
        // Serial.print(" dBm | SNR: ");
        // Serial.print(LoRa.snr());
        // Serial.println(" dB");

        // Show received status in case CRC or header error occur
        volatile int error = 0;
        uint8_t status = ReceiveLoRa.status();
        if (status == SX127X_STATUS_CRC_ERR) error = 1; // Serial.println("CRC error");
        else if (status == SX127X_STATUS_HEADER_ERR) error = 2; // Serial.println("Packet header error");
        //Serial.println();
        error = error; // *** Place breakpoint here ***

        // Don't load RF module with continous transmit
        for(volatile int i = 0; i<24000;i++){
                for(volatile int j=0;j<10;j++); //Delay 2 ms ~= 24000 clock cycles
        }



    }

    return 0;
}
