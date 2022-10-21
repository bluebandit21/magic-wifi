/*
 * FreeRTOS V202112.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://aws.amazon.com/freertos
 *
 */

/*
 * main_structure.c
 * Implements overall structure for the program.
 *
 *  Created on: Oct 20, 2022
 *      Author: gzm20
 */


/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Standard demo includes, used so the tick hook can exercise some FreeRTOS
functionality in an interrupt. */
#include "EventGroupsDemo.h"
#include "TaskNotify.h"
#include "ParTest.h" /* LEDs - a historic name for "Parallel Port". */

/* TI includes. */
#include "driverlib.h"

//provides system setup.
static void prvSetup(void);

//FreeRTOS system hooks - not sure if relevant.
void vApplicationMallocFailedHook( void );
void vApplicationIdleHook( void );
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName );
void vApplicationTickHook( void );

//From demo main.c
/* The heap is allocated here so the "persistent" qualifier can be used.  This
requires configAPPLICATION_ALLOCATED_HEAP to be set to 1 in FreeRTOSConfig.h.
See http://www.freertos.org/a00111.html for more information. */
#ifdef __ICC430__
    __persistent                    /* IAR version. */
#else
    #pragma PERSISTENT( ucHeap )    /* CCS version. */
#endif
uint8_t ucHeap[ configTOTAL_HEAP_SIZE ] = { 0 };

//EUSCI A1 (P4.0~P4.3) touches a button and TX/RX pins used in debugging.
//Don't use this by default (only TRUE when we are working on final board)
//When false, allows P4.1 button to be used for input.
#define USE_EUSCI_A1 (false)

//EUSCI B0 (P1.0~P1.3) touches the red LED at the bottom of the board.
//For now, because we assume P1.0 (STE/Chip Select) is used, we disable use of the LED.
//We could use another line and have both be usable.
#define USE_EUSCI_B0 (false)

int main(void){
    //prvSetup();
    SPImain();
    //TODO put main here
    return 0;
}

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


void vApplicationIdleHook( void )
{
    __bis_SR_register( LPM4_bits + GIE );
    __no_operation();
}
/*-----------------------------------------------------------*/

void vApplicationTickHook( void )
{
//  #if( mainCREATE_SIMPLE_BLINKY_DEMO_ONLY == 1)
//  {
//      /* Call the periodic event group from ISR demo. */
//      vPeriodicEventGroupsProcessing();
//
//      /* Call the code that 'gives' a task notification from an ISR. */
//      xNotifyTaskFromISR();
//  }
//  #endif
}

void vApplicationMallocFailedHook( void )
{
    /* Called if a call to pvPortMalloc() fails because there is insufficient
    free memory available in the FreeRTOS heap.  pvPortMalloc() is called
    internally by FreeRTOS API functions that create tasks, queues, software
    timers, and semaphores.  The size of the FreeRTOS heap is set by the
    configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */

    configASSERT( ( volatile void * ) NULL );
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
    //Remove compiler warnings.
    ( void ) pcTaskName;
    ( void ) pxTask;

    /* Run time stack overflow checking is performed if
    configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
    function is called if a stack overflow is detected.
    See http://www.freertos.org/Stacks-and-stack-overflow-checking.html */

    configASSERT( ( volatile void * ) NULL );
}

/* The MSP430X port uses this callback function to configure its tick interrupt.
This allows the application to choose the tick interrupt source.
configTICK_VECTOR must also be set in FreeRTOSConfig.h to the correct
interrupt vector for the chosen tick interrupt source.  This implementation of
vApplicationSetupTimerInterrupt() generates the tick from timer A0, so in this
case configTICK_VECTOR is set to TIMER0_A0_VECTOR. */
void vApplicationSetupTimerInterrupt( void )
{
    const unsigned short usACLK_Frequency_Hz = 32768;

    /* Ensure the timer is stopped. */
    TB0CTL = 0;

    /* Run the timer from the ACLK. */
    TB0CTL = TBSSEL_1;

    /* Clear everything to start with. */
    TB0CTL |= TBCLR;

    /* Set the compare match value according to the tick rate we want. */
    TB0CCR0 = usACLK_Frequency_Hz / configTICK_RATE_HZ;

    /* Enable the interrupts. */
    TB0CCTL0 = CCIE;

    /* Start up clean. */
    TB0CTL |= TBCLR;

    /* Up mode. */
    TB0CTL |= MC_1;
}
/*-----------------------------------------------------------*/


//Desired Timeout for XT1 initialization
#define CS_XT1_TIMEOUT 50000

//Target frequency for MCLK in kHz
#define CS_MCLK_DESIRED_FREQUENCY_IN_KHZ   1000

//MCLK/FLLRef Ratio
#define CS_MCLK_FLLREF_RATIO   30 //can be calculated.

static void prvSetup( void )
{
    // Stop watchdog timer (TODO: still needed if we use _system_pre_init?)
    WDT_A_hold( __MSP430_BASEADDRESS_WDT_A__ );

    //LEDs
    //Red (Warning: Also used for SPI B0 STE/CS)
    if (!USE_EUSCI_B0){
        GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0); GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);
    }
    //Green
    GPIO_setAsOutputPin(GPIO_PORT_P6, GPIO_PIN6); GPIO_setOutputLowOnPin(GPIO_PORT_P6, GPIO_PIN6);

    //Onboard buttons
    //left button on board (Warning: Also used for SPI A1 CLK)
    if (!USE_EUSCI_A1){
        GPIO_setAsInputPin(GPIO_PORT_P4, GPIO_PIN1);
    }
    //right button on board
    GPIO_setAsInputPin(GPIO_PORT_P2, GPIO_PIN3);

    //SPI lines
    //A0
    GPIO_setAsPeripheralModuleFunctionInputPin(
        GPIO_PORT_P1, GPIO_PIN4 + GPIO_PIN5 + GPIO_PIN6 + GPIO_PIN7, GPIO_PRIMARY_MODULE_FUNCTION);
    //A1
    if (USE_EUSCI_A1){
        GPIO_setAsPeripheralModuleFunctionInputPin(
            GPIO_PORT_P4, GPIO_PIN0 + GPIO_PIN1 + GPIO_PIN2 + GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);
    }
    //B0
    if (USE_EUSCI_B0){
        GPIO_setAsPeripheralModuleFunctionInputPin(
           GPIO_PORT_P1, GPIO_PIN0 + GPIO_PIN1 + GPIO_PIN2 + GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);
    }
    //B1
    GPIO_setAsPeripheralModuleFunctionInputPin(
        GPIO_PORT_P4, GPIO_PIN4 + GPIO_PIN5 + GPIO_PIN6 + GPIO_PIN7, GPIO_PRIMARY_MODULE_FUNCTION);

    //Disable the GPIO power-on default high-impedance mode. Enables GPIO for actual use.
    PMM_unlockLPM5();

    //---------------------------------------Clock configuration------------------------------------------
    //from the google drive dude

    //Port select XT1
    GPIO_setAsPeripheralModuleFunctionInputPin(
        GPIO_PORT_P2,
        GPIO_PIN6 + GPIO_PIN7,
        GPIO_SECONDARY_MODULE_FUNCTION);

    //Initializes the XT1 and XT2 crystal frequencies being used
    CS_setExternalClockSource(32768); //32,768 Hz ext. oscillator

    //Initialize XT1. Returns STATUS_SUCCESS if initializes successfully
    uint8_t returnValue = CS_turnOnXT1LFWithTimeout(
        CS_XT1_DRIVE_0,
        CS_XT1_TIMEOUT);

    //Select XT1 as ACLK source
    CS_initClockSignal(
        CS_ACLK,
        CS_XT1CLK_SELECT,
        CS_CLOCK_DIVIDER_1);

    //Set DCO FLL reference = REFO
    CS_initClockSignal(
        CS_FLLREF,
        CS_REFOCLK_SELECT,
        CS_CLOCK_DIVIDER_1);

    //Create struct variable to store proper software trim values
    CS_initFLLParam param = {0};

    //Set Ratio/Desired MCLK Frequency, initialize DCO, save trim values
    CS_initFLLCalculateTrim(
        CS_MCLK_DESIRED_FREQUENCY_IN_KHZ,
        CS_MCLK_FLLREF_RATIO,
        &param);

    //Clear all OSC fault flag
    CS_clearAllOscFlagsWithTimeout(1000);

    //For demonstration purpose, change DCO clock freq to 16MHz
    CS_initFLLSettle(
        16000,
        487
        );

    //Clear all OSC fault flag
    CS_clearAllOscFlagsWithTimeout(1000);

    //Reload DCO trim values that were calculated earlier
    CS_initFLLLoadTrim(
        CS_MCLK_DESIRED_FREQUENCY_IN_KHZ,
        CS_MCLK_FLLREF_RATIO,
        &param);

    //Clear all OSC fault flag
    CS_clearAllOscFlagsWithTimeout(1000); //TODO these needed?

    //Enable oscillator fault interrupt
    SFR_enableInterrupt(SFR_OSCILLATOR_FAULT_INTERRUPT);

    // Enable global interrupt
    __bis_SR_register(GIE);
}

int _system_pre_init( void )
{
    /* Stop Watchdog timer. */
    WDT_A_hold( __MSP430_BASEADDRESS_WDT_A__ );
    return 1;
}
