#include <SX127x_driver.h>

uint16_t baseAddress = SX127X_SPI_0;
EUSCI_B_SPI_initMasterParam sx127x_spi_params;
uint32_t sx127x_spiFrequency = SX127X_SPI_FREQUENCY;
int8_t sx127x_nss_port;
int8_t sx127x_nss_pin;
int8_t sx127x_reset_port;
int8_t sx127x_reset_pin;
void (*isr_ptr)(void);


// TODO:: REPLACE with RTOS block function to eliminate busy wait, for TEST use ONLY
void delay(int delay) {
    for(int i = 0; i < delay; ++i) {
        for(volatile int j = 0; j < 10; ++j);
    }
}

void sx127x_setSPI(EUSCI_B_SPI_initMasterParam &SpiObject, LORA port)
{
    sx127x_spi_params = SpiObject;
    baseAddress =     (port == LORA::RECEIVER) ? SX127X_SPI_1 : SX127X_SPI_0;
    sx127x_nss_port = (port == LORA::RECEIVER) ? SX127X_PORT_1_NSS : SX127X_PORT_0_NSS;
    sx127x_nss_pin  = (port == LORA::RECEIVER) ? SX127X_PIN_1_NSS : SX127X_PIN_0_NSS;

    sx127x_reset_port = (port == LORA::RECEIVER) ? SX127X_PORT_1_RESET : SX127X_PORT_0_RESET;
    sx127x_reset_pin  =  (port == LORA::RECEIVER) ? SX127X_PIN_1_RESET : SX127X_PIN_0_RESET;
}


void sx127x_reset()
{
    GPIO_setOutputHighOnPin(sx127x_reset_port, sx127x_reset_pin);
    delay(1);
    GPIO_setOutputLowOnPin(sx127x_reset_port, sx127x_reset_pin);
    delay(5);
}

void sx127x_begin(LORA port)
{
    GPIO_setAsOutputPin(sx127x_nss_port, sx127x_nss_pin);
    GPIO_setAsOutputPin(sx127x_reset_port, sx127x_reset_pin);
    GPIO_setOutputHighOnPin(sx127x_nss_port, sx127x_nss_pin);

    if(port == LORA::SENDER){
        GPIO_setAsInputPinWithPullUpResistor(LORA_SEND_IRQ_PORT, LORA_SEND_IRQ_PIN);
        GPIO_selectInterruptEdge(LORA_SEND_IRQ_PORT, LORA_SEND_IRQ_PIN, GPIO_LOW_TO_HIGH_TRANSITION);
    }else{
        GPIO_setAsInputPinWithPullUpResistor(LORA_RECEIVE_IRQ_PORT, LORA_RECEIVE_IRQ_PIN);
        GPIO_selectInterruptEdge(LORA_RECEIVE_IRQ_PORT, LORA_RECEIVE_IRQ_PIN, GPIO_LOW_TO_HIGH_TRANSITION);
    }

    EUSCI_B_SPI_initMaster(baseAddress, &sx127x_spi_params);
    EUSCI_B_SPI_enable(baseAddress);
    _enable_interrupt();
    //EUSCI_B_SPI_clearInterrupt(EUSCI_B0_BASE,
             // EUSCI_B_SPI_RECEIVE_INTERRUPT);
        //Enable Receive interrupt
    //EUSCI_B_SPI_enableInterrupt(EUSCI_B0_BASE,
              //EUSCI_B_SPI_RECEIVE_INTERRUPT);
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

    delay(10);
    GPIO_setOutputLowOnPin(sx127x_nss_port, sx127x_nss_pin);
    delay(10);

    EUSCI_B_SPI_transmitData(baseAddress, address);
    EUSCI_B_SPI_transmitData(baseAddress, data);
    delay(10);
    uint8_t response = *(volatile uint8_t*)EUSCI_B_SPI_getReceiveBufferAddress(baseAddress);

    delay(10);
    GPIO_setOutputHighOnPin(sx127x_nss_port, sx127x_nss_pin);

    return response;
}

void sx127x_interruptEnable(void (*isr)(void), LORA port) {
    switch(port){
        case LORA::SENDER:
            lora_send_isr = isr;
            GPIO_clearInterrupt(LORA_SEND_IRQ_PORT, LORA_SEND_IRQ_PIN);
            GPIO_enableInterrupt(LORA_SEND_IRQ_PORT, LORA_SEND_IRQ_PIN);
            break;
        case LORA::RECEIVER:
            lora_receive_isr = isr;
            GPIO_clearInterrupt(LORA_RECEIVE_IRQ_PORT, LORA_RECEIVE_IRQ_PIN);
            GPIO_enableInterrupt(LORA_RECEIVE_IRQ_PORT, LORA_RECEIVE_IRQ_PIN);
            break;
    }
}

void sx127x_interruptDisable(LORA port) {
    switch(port){
        case LORA::SENDER:
            GPIO_disableInterrupt(LORA_SEND_IRQ_PORT, LORA_SEND_IRQ_PIN);
            break;
        case LORA::RECEIVER:
            GPIO_disableInterrupt(LORA_RECEIVE_IRQ_PORT, LORA_RECEIVE_IRQ_PIN);
            break;
    }
}
