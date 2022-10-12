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


void main(void)
{
    WDT_A_hold(WDT_A_BASE); //shut up watchdog
    GPIO_setAsPeripheralModuleFunctionInputPin(
            GPIO_PORT_P1,
            GPIO_PIN4+GPIO_PIN5+GPIO_PIN6+GPIO_PIN7,
            GPIO_PRIMARY_MODULE_FUNCTION
        );
    PMM_unlockLPM5();

    //Self
    //TODO figure out how to get it to lightspeed
    EUSCI_A_SPI_initMasterParam param = {0};
    param.selectClockSource = EUSCI_A_SPI_CLOCKSOURCE_SMCLK;
    param.clockSourceFrequency = CS_getSMCLK(); //SMCLK capable of up to 24MHz.
    param.desiredSpiClock = 1000000; //TODO can we get this high? It said 8MHz max.
    param.msbFirst = EUSCI_A_SPI_MSB_FIRST;
    param.clockPhase = EUSCI_A_SPI_PHASE_DATA_CHANGED_ONFIRST_CAPTURED_ON_NEXT;
    param.clockPolarity = EUSCI_A_SPI_CLOCKPOLARITY_INACTIVITY_LOW;
    param.spiMode = EUSCI_A_SPI_3PIN; //TODO implement CS ourselves.. or can use EUSCI_A_SPI_4PIN_UCxSTE_ACTIVE_HIGH
    EUSCI_A_SPI_initMaster(EUSCI_A0_BASE, &param);
    //Enable SPI Module
    EUSCI_A_SPI_enable(EUSCI_A0_BASE);
    //Clear receive interrupt flag
    EUSCI_A_SPI_clearInterrupt(EUSCI_A0_BASE,
          EUSCI_A_SPI_RECEIVE_INTERRUPT);

    //Enable Receive interrupt
    EUSCI_A_SPI_enableInterrupt(EUSCI_A0_BASE,
          EUSCI_A_SPI_RECEIVE_INTERRUPT);

    volatile unsigned int i = 0;
    for(;;){
        //! \param baseAddress is the base address of the EUSCI_A_SPI module.
        //! \param transmitData data to be transmitted from the SPI module
        //!
//        extern void EUSCI_A_SPI_transmitData(uint16_t baseAddress,
//                                             uint8_t transmitData);
        EUSCI_A_SPI_transmitData(EUSCI_A0_BASE, 0xAA); //send 1010 1010

        for(i = 0; i < 20000; ++i); //delay
    }
}

