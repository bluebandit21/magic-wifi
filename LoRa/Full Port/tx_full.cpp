/* This is a the ported simple test of the driver library. It contains none of the abstractions
available in the SX127X.cpp library, so this will transmitted verbaitim

*/



//Abandoned: https://github.com/fmilburn3/MSP430F5529_driverlib_examples/blob/master/13A_EUSCI_B_SPI_MCP41010_digiPot

//From: MSP430FR2xx_4xx_DriverLib_Users_Guide-2_91_13_01.pdf, 12.3
    //Initialize slave to MSB first, inactive high clock polarity and 3 wire SPI
//    EUSCI_A_SPI_initSlaveParam param = {0};
//    param.msbFirst = EUSCI_A_SPI_MSB_FIRST;
//    param.clockPhase = EUSCI_A_SPI_PHASE_DATA_CHANGED_ONFIRST_CAPTURED_ON_NEXT;
//    param.clockPolarity = EUSCI_A_SPI_CLOCKPOLARITY_INACTIVITY_HIGH;
//    param.spiMode = EUSCI_A_SPI_3PIN;
//    EUSCI_A_SPI_initSlave(EUSCI_A0_BASE, &param);
//    //Enable SPI Module
//    EUSCI_A_SPI_enable(EUSCI_A0_BASE);
//    //Enable Receive interrupt
//    EUSCI_A_SPI_enableInterrupt(EUSCI_A0_BASE,
//        EUSCI_A_SPI_RECEIVE_INTERRUPT);

//Referring mostly from: https://blog.fearcat.in/a?ID=01200-d182dba9-37eb-4290-acf8-19dc5e11adae

#include <SX127x.h>



// Message to transmit
char message[] = "HeLoRa World!";
uint8_t nBytes = sizeof(message);
uint8_t counter = 0;



// Must implement delay TODO:: calibrate
void delayMicroseconds(unsigned int us) {
  for(volatile uint32_t j = 0; j < us; ++j) {
      for(volatile uint32_t i = 0; i < 100; ++i);
  }
}

void SPImain(void)
{

    SX127x LoRa(0);
      if (!LoRa.begin()){
        // Serial.println("Something wrong, can't begin LoRa radio");
        while(1);
      }


      // ADDED, set Tx Frequency
      LoRa.setFrequency(916000000);

      // Set TX power, this function will set PA config with optimal setting for requested TX power
      // Serial.println("Set TX power to +17 dBm");
      LoRa.setTxPower(17, SX127X_TX_POWER_PA_BOOST);                    // TX power +17 dBm using PA boost pin

      // Configure modulation parameter including spreading factor (SF), bandwidth (BW), and coding rate (CR)
      // Transmitter must have same SF and BW setting so receiver can receive LoRa packet
      // Serial.println("Set modulation parameters:\n\tSpreading factor = 7\n\tBandwidth = 125 kHz\n\tCoding rate = 4/5");
      LoRa.setSpreadingFactor(7);                                       // LoRa spreading factor: 7
      LoRa.setBandwidth(500000);                                        // Bandwidth: 125 kHz
      LoRa.setCodeRate(5);                                              // Coding rate: 4/5

      // Configure packet parameter including header type, preamble length, payload length, and CRC type
      // The explicit packet includes header contain CR, number of byte, and CRC type
      // Packet with explicit header can't be received by receiver with implicit header mode
      // Serial.println("Set packet parameters:\n\tExplicit header type\n\tPreamble length = 12\n\tPayload Length = 15\n\tCRC on");
      LoRa.setHeaderType(SX127X_HEADER_EXPLICIT);                       // Explicit header mode
      LoRa.setPreambleLength(12);                                       // Set preamble length to 12
      LoRa.setPayloadLength(15);                                        // Initialize payloadLength to 15
      LoRa.setCrcEnable(true);                                          // Set CRC enable

      // Set syncronize word
      // Serial.println("Set syncronize word to 0x34");
      LoRa.setSyncWord(0x34);

      // Serial.println("\n-- LORA TRANSMITTER --\n");

      for(;;){
          // Transmit message and counter
          // write() method must be placed between beginPacket() and endPacket()
          LoRa.beginPacket();
          LoRa.write(message, nBytes);
          LoRa.write(counter);
          LoRa.endPacket();

          // Print message and counter in serial
          // Serial.write(message, nBytes);
          // Serial.print("  ");
          // Serial.println(counter++);

          // Wait until modulation process for transmitting packet finish
          LoRa.wait();

          // Print transmit time
          // Serial.print("Transmit time: ");
          // Serial.print(LoRa.transmitTime());
          // Serial.println(" ms");
          // Serial.println();

          // Don't load RF module with continous transmit
          delayMicroseconds(5000);
    }


}

void SPImainReceive(void)
{

    SX127x LoRa(0);
    char receiveBuffer[100];

    // Begin LoRa radio and set NSS, reset, txen, and rxen pin with connected arduino pins
      // IRQ pin not used in this example (set to -1). Set txen and rxen pin to -1 if RF module doesn't have one
      //Serial.println("Begin LoRa radio");
      if (!LoRa.begin()){
        //Serial.println("Something wrong, can't begin LoRa radio");
        while(1);
      }

      // Set frequency to 915 Mhz
      //Serial.println("Set frequency to 915 Mhz");
      LoRa.setFrequency(915E6);

      // Set RX gain. RX gain option are power saving gain or boosted gain
      // Serial.println("Set RX gain to power saving gain");
      LoRa.setRxGain(SX127X_RX_GAIN_POWER_SAVING, SX127X_RX_GAIN_AUTO); // AGC on, Power saving gain

      // Configure modulation parameter including spreading factor (SF), bandwidth (BW), and coding rate (CR)
      // Transmitter must have same SF and BW setting so receiver can receive LoRa packet
      // Serial.println("Set modulation parameters:\n\tSpreading factor = 7\n\tBandwidth = 125 kHz\n\tCoding rate = 4/5");
      LoRa.setSpreadingFactor(7);                                       // LoRa spreading factor: 7
      LoRa.setBandwidth(125000);                                        // Bandwidth: 125 kHz
      LoRa.setCodeRate(5);                                              // Coding rate: 4/5

      // Configure packet parameter including header type, preamble length, payload length, and CRC type
      // The explicit packet includes header contain CR, number of byte, and CRC type
      // Packet with explicit header can't be received by receiver with implicit header mode
      // Serial.println("Set packet parameters:\n\tExplicit header type\n\tPreamble length = 12\n\tPayload Length = 15\n\tCRC on");
      LoRa.setHeaderType(SX127X_HEADER_EXPLICIT);                       // Explicit header mode
      LoRa.setPreambleLength(12);                                       // Set preamble length to 12
      LoRa.setPayloadLength(15);                                        // Initialize payloadLength to 15
      LoRa.setCrcEnable(true);                                          // Set CRC enable

      // Set syncronize word
      // Serial.println("Set syncronize word to 0x34");
      LoRa.setSyncWord(0x34);

      // Serial.println("\n-- LORA RECEIVER --\n");

      for(;;){
          // Request for receiving new LoRa packet
            LoRa.request();
            // Wait for incoming LoRa packet
            LoRa.wait();

            // Put received packet to message and counter variable
            // read() and available() method must be called after request() method
            const uint8_t msgLen = LoRa.available() - 1;
            //char message[msgLen];
            LoRa.read(receiveBuffer, msgLen);
            uint8_t counter = LoRa.read();

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
            uint8_t status = LoRa.status();
            if (status == SX127X_STATUS_CRC_ERR) error = 1; // Serial.println("CRC error");
            else if (status == SX127X_STATUS_HEADER_ERR) error = 2; // Serial.println("Packet header error");
            //Serial.println();
            error = error; // *** Place breakpoint here ***
    }
}

int main(void) {
    WDT_A_hold(WDT_A_BASE); //shut up watchdog
    //GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P4, GPIO_PIN1);
    //GPIO_selectInterruptEdge(GPIO_PORT_P4, GPIO_PIN1, GPIO_HIGH_TO_LOW_TRANSITION);
    //GPIO_clearInterrupt(GPIO_PORT_P4, GPIO_PIN1);
    //GPIO_enableInterrupt(GPIO_PORT_P4, GPIO_PIN1);
    //_enable_interrupt();
    //volatile uint16_t test = HWREG16(0x220 + OFS_PAIFG);
    //test = test;


    SPImain();
}

//#pragma vector=PORT4_VECTOR
//__interrupt void Port_4(void)
//{
//    volatile uint16_t test = HWREG16(0x220 + OFS_PAIFG);
//    test = test;
//    delayMicroseconds(1);
//    GPIO_clearInterrupt(GPIO_PORT_P4, GPIO_PIN1);
//}

