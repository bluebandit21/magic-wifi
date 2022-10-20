#include <SX127x_driver.h>

uint16_t baseAddress = SX127X_SPI;
EUSCI_A_SPI_initMasterParam* sx127x_spi_params;
uint32_t sx127x_spiFrequency = SX127X_SPI_FREQUENCY;
int8_t sx127x_nss_port = SX127X_PORT_NSS;
int8_t sx127x_nss_pin = SX127X_PIN_NSS;

// TODO:: REPLACE with RTOS block function to eliminate busy wait, for TEST use ONLY
void delay(int i) {
    for(int j = 0; j < 100000; ++j);
}

void sx127x_setSPI(EUSCI_A_SPI_initMasterParam &SpiObject)
{
    sx127x_spi_params = &SpiObject;
}

void sx127x_setPins(int8_t nss)
{
    // This function is vestigial, MSP will need both port and pin, currently hardcoded //sx127x_nss =  ;
}

void sx127x_reset(int8_t reset)
{
    GPIO_setAsOutputPin(SX127X_PORT_RESET, SX127X_PIN_RESET);
    GPIO_setOutputHighOnPin(SX127X_PORT_RESET, SX127X_PIN_RESET);
    delay(1);
    GPIO_setOutputLowOnPin(SX127X_PORT_RESET, SX127X_PIN_RESET);
    delay(5);
}

void sx127x_begin()
{
    GPIO_setAsOutputPin(sx127x_nss_port, sx127x_nss_pin);
    EUSCI_A_SPI_initMaster(baseAddress, sx127x_spi_params);
    EUSCI_A_SPI_enable(baseAddress);
    //EUSCI_A_SPI_clearInterrupt(EUSCI_A0_BASE,
             // EUSCI_A_SPI_RECEIVE_INTERRUPT);
        //Enable Receive interrupt
    //EUSCI_A_SPI_enableInterrupt(EUSCI_A0_BASE,
              //EUSCI_A_SPI_RECEIVE_INTERRUPT);
}

void sx127x_writeBits(uint8_t address, uint8_t data, uint8_t position, uint8_t length)
{
    uint8_t read = sx127x_transfer(address & 0x7F, 0x00);
    uint8_t mask = (0xFF >> (8 - length)) << position;
    uint8_t write = (data << position) | (read & ~mask);
    sx127x_transfer(address | 0x80, write);
}

void sx127x_writeRegister(uint8_t address, uint8_t data)
{
    sx127x_transfer(address | 0x80, data);
}

uint8_t sx127x_readRegister(uint8_t address)
{
    return sx127x_transfer(address & 0x7F, 0x00);
}

uint8_t sx127x_transfer(uint8_t address, uint8_t data)
{
    GPIO_setOutputLowOnPin(sx127x_nss_port, sx127x_nss_pin);

    EUSCI_A_SPI_transmitData(baseAddress, address);
    EUSCI_A_SPI_transmitData(baseAddress, data);
    uint8_t response = EUSCI_A_SPI_receiveData(baseAddress); // TODO:: verify this tx/rx pattern works, buffering should make full duplex? 
    //EUSCI_A_SPI_disable(baseAddress); // TODO:: this might be slow ?? Since bus is dedicated, this likely unnecessary

    GPIO_setOutputHighOnPin(sx127x_nss_port, sx127x_nss_pin);

    return response;
}
