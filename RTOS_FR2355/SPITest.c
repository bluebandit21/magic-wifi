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

void initSPI(char*);

void SPImain(void)
{
    WDT_A_hold(WDT_A_BASE); //shut up watchdog

    //Target frequency for MCLK in kHz
    const int CS_SMCLK_DESIRED_FREQUENCY_IN_KHZ = 8000;
    //MCLK/FLLRef Ratio
    const int CS_SMCLK_FLLREF_RATIO = (int)(CS_SMCLK_DESIRED_FREQUENCY_IN_KHZ * 1000.0 / 32768.0); // = Desired HZ / 32768
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

    initSPI("A0");

    volatile unsigned int i = 0;
    for(;;){
    EUSCI_A_SPI_transmitData(EUSCI_A0_BASE, 0xAA); //send 1010 1010

//    for(i = 0; i < 20000; ++i); //delay
    }
}

//initializes SPI given which one (SPIBank = 'A' or 'B', SPISlot = 0 or 1)
void initSPI(char* SPILine) {
    char SPIBank = SPILine[0];
    char SPISlot = SPILine[1];
    //pinMode() equivalent
    //B0: 1.0 ~ 1.3, B1: 4.4 ~ 4.7
    //A0: 1.4 ~ 1.7, A1: 4.0 ~ 4.3

    // Set up GPIO Pins
    uint8_t port;
    uint16_t pins;
    if(SPIBank == 'A'){
        if(SPISlot == '0'){
            port = GPIO_PORT_P1;
            pins = GPIO_PIN4 + GPIO_PIN5 + GPIO_PIN6 + GPIO_PIN7;
        }else if(SPISlot == '1'){
            port = GPIO_PORT_P4;
            pins = GPIO_PIN0 + GPIO_PIN1 + GPIO_PIN2 + GPIO_PIN3;
        }
    }else if(SPIBank == 'B'){
        if(SPISlot == '0'){
            port = GPIO_PORT_P1;
            pins = GPIO_PIN0 + GPIO_PIN1 + GPIO_PIN2 + GPIO_PIN3;
        }else if(SPISlot == '1'){
            port = GPIO_PORT_P4;
            pins = GPIO_PIN4 + GPIO_PIN5 + GPIO_PIN6 + GPIO_PIN7;
        }
    }
    GPIO_setAsPeripheralModuleFunctionInputPin(
        port, pins, GPIO_PRIMARY_MODULE_FUNCTION);

    // Disable the GPIO power-on default high-impedance mode
    PMM_unlockLPM5();

    // Setup EUSCI SPI access and interrupts
    const int CS_SMCLK_DESIRED_FREQUENCY_IN_KHZ = 8000;
    if(SPIBank == 'A'){
       uint16_t base = EUSCI_A0_BASE;
       if(SPISlot == '1'){
           base = EUSCI_A1_BASE;
       }
       EUSCI_A_SPI_initMasterParam param = {0};
           param.selectClockSource = EUSCI_A_SPI_CLOCKSOURCE_SMCLK;
           param.clockSourceFrequency = CS_getSMCLK(); //SMCLK capable of up to 24MHz. Non-jank around 8MHz.
           param.desiredSpiClock = CS_SMCLK_DESIRED_FREQUENCY_IN_KHZ * 1000;
           param.msbFirst = EUSCI_A_SPI_MSB_FIRST;
           param.clockPhase = EUSCI_A_SPI_PHASE_DATA_CHANGED_ONFIRST_CAPTURED_ON_NEXT;
           param.clockPolarity = EUSCI_A_SPI_CLOCKPOLARITY_INACTIVITY_LOW;
           param.spiMode = EUSCI_A_SPI_4PIN_UCxSTE_ACTIVE_HIGH; //TODO implement CS ourselves.. or can use EUSCI_A_SPI_4PIN_UCxSTE_ACTIVE_HIGH
       EUSCI_A_SPI_initMaster(base, &param);

       //Enable SPI Module
       EUSCI_A_SPI_enable(base);
       //Clear receive interrupt flag
       EUSCI_A_SPI_clearInterrupt(base,
             EUSCI_A_SPI_RECEIVE_INTERRUPT);
       //Enable Receive interrupt
       EUSCI_A_SPI_enableInterrupt(base,
             EUSCI_A_SPI_RECEIVE_INTERRUPT);
    }else if(SPIBank == 'B'){
       uint16_t base = EUSCI_B0_BASE;
       if(SPISlot == '1'){
           base = EUSCI_B1_BASE;
       }
       EUSCI_B_SPI_initMasterParam param = {0};
           param.selectClockSource = EUSCI_B_SPI_CLOCKSOURCE_SMCLK;
           param.clockSourceFrequency = CS_getSMCLK(); //SMCLK capable of up to 24MHz. Non-jank around 8MHz.
           param.desiredSpiClock = CS_SMCLK_DESIRED_FREQUENCY_IN_KHZ * 1000;
           param.msbFirst = EUSCI_B_SPI_MSB_FIRST;
           param.clockPhase = EUSCI_B_SPI_PHASE_DATA_CHANGED_ONFIRST_CAPTURED_ON_NEXT;
           param.clockPolarity = EUSCI_B_SPI_CLOCKPOLARITY_INACTIVITY_LOW;
           param.spiMode = EUSCI_B_SPI_4PIN_UCxSTE_ACTIVE_HIGH; //TODO implement CS ourselves.. or can use EUSCI_A_SPI_4PIN_UCxSTE_ACTIVE_HIGH
       EUSCI_B_SPI_initMaster(base, &param);

       //Enable SPI Module
       EUSCI_B_SPI_enable(base);
       //Clear receive interrupt flag
       EUSCI_B_SPI_clearInterrupt(base,
             EUSCI_B_SPI_RECEIVE_INTERRUPT);
       //Enable Receive interrupt
       EUSCI_B_SPI_enableInterrupt(base,
             EUSCI_B_SPI_RECEIVE_INTERRUPT);
    }
}
