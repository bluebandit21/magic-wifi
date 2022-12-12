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

#include "driverlib.h"
#include <SX127x_driver.h>

// Pin setting
int8_t nssPin = 10, resetPin = 9, irqPin = 2, rxenPin = -1, txenPin = -1;

// RF frequency setting
uint32_t frequency = 915000000;

// PA and TX power setting
uint8_t paConfig = 0xC0;
uint8_t txPower = 17;
uint8_t paPin = SX127X_TX_POWER_PA_BOOST;

// Define modulation parameters setting
uint8_t sf = 7;
uint8_t bw = 7;                               // 125 khz
uint8_t cr = 1;                               // 5/4

// Define packet parameters setting
uint8_t headerType = SX127X_HEADER_EXPLICIT;
uint16_t preambleLen = 12;
uint8_t crcEn = 1;

// SyncWord setting
uint8_t syncword = 0x12;

// debug reg
void debug(uint8_t val) {
    static uint8_t debug_reg[50];
    static int i;
    if(i >= 50) i = 0;
    debug_reg[i++] = val;
}

volatile bool transmitted = false;

// TOD0:: replace mock with MSP timer
// Times will not be accurate but mock should mimic behavior, will need to setup timer for this
uint32_t millis() {
  static uint32_t i = 0;
  return i++;
}

// Must implement delay TODO:: calibrate
void delayMicroseconds(unsigned int us) {
  for(volatile uint32_t j = 0; j < us; ++j) {
      for(volatile uint32_t i = 0; i < 100; ++i);
  }
}

void checkTransmitDone() {
  transmitted = true;
}

void settingFunction(EUSCI_A_SPI_initMasterParam &SpiObject) {

  ////Serial.println("-- SETTING FUNCTION --");

  // Pin setting
  //Serial.println("Setting pins");
  sx127x_setPins(nssPin);
//   pinMode(irqPin, INPUT);
//   if (txenPin != -1 && rxenPin != -1) {
//     pinMode(txenPin, OUTPUT);
//     pinMode(rxenPin, OUTPUT);
//   }

  // Reset RF module by setting resetPin to LOW and begin SPI communication
  sx127x_reset(resetPin);
  sx127x_setSPI(SpiObject);
  sx127x_begin();
  volatile uint8_t version = sx127x_readRegister(SX127X_REG_VERSION);
  debug(version);
  if (version == 0x12 || version == 0x22) {
    //Serial.println("Resetting RF module");
    version = version;
  } else {
    //Serial.println("Something wrong, can't reset LoRa radio");
  }

  // Set modem type to LoRa and put device to standby mode
  sx127x_writeRegister(SX127X_REG_OP_MODE, SX127X_MODE_SLEEP);
  sx127x_writeRegister(SX127X_REG_OP_MODE, SX127X_LONG_RANGE_MODE);
  sx127x_writeRegister(SX127X_REG_OP_MODE, SX127X_LONG_RANGE_MODE | SX127X_MODE_STDBY);
  //Serial.println("Going to standby mode");
  //Serial.println("Set packet type to LoRa");

  // Set frequency
  uint64_t frf = ((uint64_t) frequency << 19) / 32000000;
  sx127x_writeRegister(SX127X_REG_FRF_MSB, (uint8_t) (frf >> 16));
  sx127x_writeRegister(SX127X_REG_FRF_MID, (uint8_t) (frf >> 8));
  sx127x_writeRegister(SX127X_REG_FRF_LSB, (uint8_t) frf);
  //Serial.print("Set frequency to ");
  //Serial.print(frequency / 1000000);
  //Serial.println(" MHz");

  // Set tx power to selected TX power
  //Serial.print("Set TX power to ");
  //Serial.print(txPower, DEC);
  //Serial.println(" dBm");
  uint8_t outputPower = txPower - 2;
  uint8_t paDac = txPower > 17 ? 0x07 : 0x04;
  sx127x_writeRegister(SX127X_REG_PA_DAC, paDac);
  sx127x_writeRegister(SX127X_REG_PA_CONFIG, paConfig | outputPower);

  // Set modulation param and packet param
  //Serial.println("Set modulation with predefined parameters");
  //Serial.println("Set packet with predefined parameters");
  sx127x_writeBits(SX127X_REG_MODEM_CONFIG_2, sf, 4, 4);
  sx127x_writeBits(SX127X_REG_MODEM_CONFIG_1, bw, 4, 4);
  sx127x_writeBits(SX127X_REG_MODEM_CONFIG_1, cr, 1, 3);
  sx127x_writeBits(SX127X_REG_MODEM_CONFIG_1, headerType, 0, 1);
  sx127x_writeBits(SX127X_REG_MODEM_CONFIG_2, crcEn, 2, 1);
  sx127x_writeRegister(SX127X_REG_PREAMBLE_MSB, preambleLen >> 8);
  sx127x_writeRegister(SX127X_REG_PREAMBLE_LSB, preambleLen);

  // Show modulation param and packet param registers
  volatile uint8_t reg, reg_;
  reg = sx127x_readRegister(SX127X_REG_MODEM_CONFIG_1);
  debug(reg);
  //Serial.print("Modem config 1 : 0x");
  //Serial.println(reg, HEX);
  reg = sx127x_readRegister(SX127X_REG_MODEM_CONFIG_2);
  debug(reg);
  //Serial.print("Modem config 2 : 0x");
  //Serial.println(reg, HEX);
  reg = sx127x_readRegister(SX127X_REG_PREAMBLE_MSB);
  debug(reg);
  reg_ = sx127x_readRegister(SX127X_REG_PREAMBLE_LSB);
  debug(reg_);
  //Serial.print("Preamble length : 0x");
  //Serial.println(reg * 256 + reg_, HEX);

  // Set synchronize word
  sx127x_writeRegister(SX127X_REG_SYNC_WORD, syncword);
  reg = sx127x_readRegister(SX127X_REG_SYNC_WORD);
  debug(reg);
  //Serial.print("Set syncWord to 0x");
  //Serial.println(reg, HEX);
}

uint8_t transmitFunction(char* message, uint8_t length) {

  //Serial.println("\n-- TRANSMIT FUNCTION --");
  volatile uint8_t reg, reg_;

  // Configure FIFO address and address pointer for TX operation
  sx127x_writeRegister(SX127X_REG_FIFO_TX_BASE_ADDR, 0x00);
  reg = sx127x_readRegister(SX127X_REG_FIFO_TX_BASE_ADDR);
  debug(reg);
  sx127x_writeRegister(SX127X_REG_FIFO_ADDR_PTR, 0x00);
  reg_ = sx127x_readRegister(SX127X_REG_FIFO_ADDR_PTR);
  debug(reg_);
  //Serial.print("Set FIFO TX base address and address pointer (0x");
  //Serial.print(reg, HEX);
  //Serial.print(" | 0x");
  //Serial.print(reg_, HEX);
  //Serial.println(")");

  // Write message to FIFO
  //Serial.print("Write message \'");
  //Serial.print(message);
  //Serial.println("\' in buffer");
  //Serial.print("Message in bytes : [ ");
  for (uint8_t i = 0; i < length; i++) {
    sx127x_writeRegister(SX127X_REG_FIFO, message[i]);
    //Serial.print((uint8_t) message[i]);
    //Serial.print("  ");
  }
  //Serial.println("]");

  // Set payload length
  sx127x_writeRegister(SX127X_REG_PAYLOAD_LENGTH, length);
  reg = sx127x_readRegister(SX127X_REG_PAYLOAD_LENGTH);
  debug(reg);
  //Serial.print("Set payload length same as message length (");
  //Serial.print(reg);
  //Serial.println(")");

  // Activate interrupt when transmit done on DIO0
  //Serial.println("Set TX done and timeout IRQ on DIO0");
  sx127x_writeRegister(SX127X_REG_DIO_MAPPING_1, SX127X_DIO0_TX_DONE);
  // Attach irqPin to DIO0
  //Serial.println("Attach interrupt on IRQ pin");
  //attachInterrupt(digitalPinToInterrupt(irqPin), checkTransmitDone, RISING);

  // Set txen and rxen pin state for transmitting packet
  // if (txenPin != -1 && rxenPin != -1) {
  //   digitalWrite(txenPin, HIGH);
  //   digitalWrite(rxenPin, LOW);
  // }

  // Transmit message
  //Serial.println("Transmitting message...");
  sx127x_writeRegister(SX127X_REG_OP_MODE, SX127X_LORA_MODEM | SX127X_MODE_TX);
  uint32_t tStart = millis(), tTrans = 0;

  // Wait for TX done interrupt and calcualte transmit time
  //Serial.println("Wait for TX done interrupt");
  delayMicroseconds(400000); // TODO:: this may be slow, interrupt disabled
  tTrans = millis() - tStart;
  // Clear transmit interrupt flag
  transmitted = false;
  //Serial.println("Transmit done");

  // Display transmit time
  //Serial.print("Transmit time = ");
  //Serial.print(tTrans);
  //Serial.println(" ms");

  // Show IRQ flag and Clear interrupt
  uint8_t irqStat = sx127x_readRegister(SX127X_REG_IRQ_FLAGS);
  sx127x_writeRegister(SX127X_REG_IRQ_FLAGS, 0xFF);
  //Serial.println("Clear IRQ status");
  // if (txenPin != -1) {
  //   digitalWrite(txenPin, LOW);
  // }

  // return interrupt status
  debug(irqStat);
  return irqStat;
}

void SPImain(void)
{
    WDT_A_hold(WDT_A_BASE); //shut up watchdog

    GPIO_setAsPeripheralModuleFunctionInputPin( //pinMode() of TI
            GPIO_PORT_P1,
            GPIO_PIN4+GPIO_PIN5+GPIO_PIN6+GPIO_PIN7,
            GPIO_PRIMARY_MODULE_FUNCTION);

    GPIO_setAsOutputPin(GPIO_PORT_P1,GPIO_PIN3);



     /* Disable the GPIO power-on default high-impedance mode. */
     PMM_unlockLPM5();

     //Target frequency for MCLK in kHz
     #define CS_SMCLK_DESIRED_FREQUENCY_IN_KHZ 24000
     //MCLK/FLLRef Ratio
     #define CS_SMCLK_FLLREF_RATIO 732 // = Desired HZ / 32768
     //Variable to store current Clock values
     volatile uint32_t clockValue = 0;
     // Set DCO FLL reference = REFO
     CS_initClockSignal(
         CS_FLLREF,
         CS_REFOCLK_SELECT,
         CS_CLOCK_DIVIDER_1);
     // Set ACLK = REFO
     CS_initClockSignal(
         CS_ACLK,
         CS_REFOCLK_SELECT,
         CS_CLOCK_DIVIDER_1);

     // Set Ratio and Desired MCLK Frequency and initialize DCO
     // Returns 1 (STATUS_SUCCESS) if good

     volatile int result = CS_initFLLSettle(
         CS_SMCLK_DESIRED_FREQUENCY_IN_KHZ,
         CS_SMCLK_FLLREF_RATIO
         );
     //Verify if the Clock settings are as expected
     //clockValue = CS_getSMCLK (); //23986176Hz (24.0M)

    //Self
    //TODO figure out how to get it to lightspeed
    EUSCI_A_SPI_initMasterParam param = {0};
    param.selectClockSource = EUSCI_A_SPI_CLOCKSOURCE_SMCLK;
    param.clockSourceFrequency = CS_getSMCLK(); //SMCLK capable of up to 24MHz.
    param.desiredSpiClock = 2000000; //TODO can we get this high? It said 8MHz max.
    param.msbFirst = EUSCI_A_SPI_MSB_FIRST;
    //param.clockPhase = EUSCI_A_SPI_PHASE_DATA_CHANGED_ONFIRST_CAPTURED_ON_NEXT;
    param.clockPhase = EUSCI_A_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT;
    param.clockPolarity = EUSCI_A_SPI_CLOCKPOLARITY_INACTIVITY_LOW;
    param.spiMode = EUSCI_A_SPI_3PIN; //TODO implement CS ourselves.. or can use EUSCI_A_SPI_4PIN_UCxSTE_ACTIVE_HIGH
    //EUSCI_A_SPI_initMaster(EUSCI_A0_BASE, &param);

    // All spi setup should be ported into the driver file
    settingFunction(param);

    // TODO:: verify this is  not needed
    //Enable SPI Module
    //EUSCI_A_SPI_enable(EUSCI_A0_BASE);
    // //Clear receive interrupt flag
    // EUSCI_A_SPI_clearInterrupt(EUSCI_A0_BASE,
    //       EUSCI_A_SPI_RECEIVE_INTERRUPT);
    // //Enable Receive interrupt
    // EUSCI_A_SPI_enableInterrupt(EUSCI_A0_BASE,
    //       EUSCI_A_SPI_RECEIVE_INTERRUPT);

    static char message[] = "Hello World!!! Can I really send a long message?";
    for(;;){
        // Message to transmit

        uint8_t length = sizeof(message);


        // Transmit message
        volatile uint8_t status = transmitFunction(message, length);
        debug(0);

        // Don't load RF module with continous transmit
        delayMicroseconds(10000);

        //for(volatile int i = 0; i < 20000; ++i); //delay
    }


}

int main(void) {
    //GPIO_setOutputHighOnPin(GPIO_PORT_P1,GPIO_PIN3);
    SPImain();
    //GPIO_setOutputLowOnPin(GPIO_PORT_P1,GPIO_PIN3);
}

