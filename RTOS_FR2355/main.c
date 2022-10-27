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
 * main.c
 * Implements overall structure for the RTOS.
 *
 *  Created on: Oct 20, 2022
 *      Author: gzm20
 */

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "driverlib.h"

//provides system setup.
static void prvSetup(void);
void vApplicationMallocFailedHook( void );
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName );

void SPImain(void);
void initSPI(char, char);
void clockSetup(void);

extern void main_defer_interrupt(void);

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

int main(void){
    prvSetup();
//    initSPI('A', '0');
    main_defer_interrupt();
    //SPImain();
    return 0;
}

#define CS_MCLK_DESIRED_KHZ (16000)
#define CS_MCLK_FLL_RATIO   (488) //= rounding MCLK desired HZ / 32768
//Sets up Clock signals for the FR2355: MCLK @ 16M, SMCLK (SPI, etc.) @ 8M.

void clockSetup(void){
    CS_initClockSignal(CS_FLLREF, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_1);

    // Set Ratio and Desired MCLK Frequency and initialize DCO. Returns 1 (STATUS_SUCCESS) if good
    // By default, MCLK = DCOCLK = 16M after this settles.
    CS_initFLLSettle(CS_MCLK_DESIRED_KHZ, CS_MCLK_FLL_RATIO);

    // SMCLK = DCOCLK / 2 = 8M
    CS_initClockSignal(CS_SMCLK, CS_DCOCLKDIV_SELECT, CS_CLOCK_DIVIDER_2);

    //Sanity checking? enable and breakpoint on the following values.
    volatile uint32_t MCLK_val = CS_getMCLK();      //15,990,784 (16M)
    volatile uint32_t SMCLK_val = CS_getSMCLK();    // 7,995,392 (8M)
}

void SPImain(void)
{
    clockSetup();
    initSPI('A', '0');
    int i = 0;
    for(i = 0; i < 10; ++i){
        EUSCI_A_SPI_transmitData(EUSCI_A0_BASE, 0xAC); //send 1010 1100
    }
    main_defer_interrupt();
    return 0;
}

//initializes SPI given which one (SPIBank = 'A' or 'B', SPISlot = '0' or '1')
void initSPI(char SPIBank, char SPISlot) {
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
    if(SPIBank == 'A'){
       uint16_t base = EUSCI_A0_BASE;
       if(SPISlot == '1'){
           base = EUSCI_A1_BASE;
       }
       EUSCI_A_SPI_initMasterParam param = {0};
           param.selectClockSource = EUSCI_A_SPI_CLOCKSOURCE_SMCLK;
           param.clockSourceFrequency = CS_getSMCLK(); //SMCLK capable of up to 24MHz. Non-jank around 8MHz.
           param.desiredSpiClock = 8000000;
           param.msbFirst = EUSCI_A_SPI_MSB_FIRST;
           param.clockPhase = EUSCI_A_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT;
           param.clockPolarity = EUSCI_A_SPI_CLOCKPOLARITY_INACTIVITY_LOW;
           param.spiMode = EUSCI_A_SPI_3PIN;
       EUSCI_A_SPI_initMaster(base, &param);

       //Enable SPI Module
       EUSCI_A_SPI_enable(base);
       //Clear receive interrupt flag
       EUSCI_A_SPI_clearInterrupt(base,
             EUSCI_A_SPI_RECEIVE_INTERRUPT);
       //Enable Receive interrupt NOTE: enabling without implementing an interrupt jumps to ISR trap.
//       EUSCI_A_SPI_enableInterrupt(base,
//             EUSCI_A_SPI_RECEIVE_INTERRUPT);
    }else if(SPIBank == 'B'){
       uint16_t base = EUSCI_B0_BASE;
       if(SPISlot == '1'){
           base = EUSCI_B1_BASE;
       }
       EUSCI_B_SPI_initMasterParam param = {0};
           param.selectClockSource = EUSCI_B_SPI_CLOCKSOURCE_SMCLK;
           param.clockSourceFrequency = CS_getSMCLK(); //SMCLK capable of up to 24MHz. Non-jank around 8MHz.
           param.desiredSpiClock = 8000000;
           param.msbFirst = EUSCI_B_SPI_MSB_FIRST;
           param.clockPhase = EUSCI_A_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT;
           param.clockPolarity = EUSCI_B_SPI_CLOCKPOLARITY_INACTIVITY_LOW;
           param.spiMode = EUSCI_A_SPI_3PIN;
       EUSCI_B_SPI_initMaster(base, &param);

       //Enable SPI Module
       EUSCI_B_SPI_enable(base);
       //Clear receive interrupt flag
       EUSCI_B_SPI_clearInterrupt(base,
             EUSCI_B_SPI_RECEIVE_INTERRUPT);
       //Enable Receive interrupt NOTE: enabling without implementing an interrupt jumps to ISR trap.
//       EUSCI_B_SPI_enableInterrupt(base,
//             EUSCI_B_SPI_RECEIVE_INTERRUPT);
    }
}

//Failure hooks
void vApplicationMallocFailedHook( void )
{
    /* Called if a call to pvPortMalloc() fails because there is insufficient
    free memory available in the FreeRTOS heap.  pvPortMalloc() is called
    internally by FreeRTOS API functions that create tasks, queues, software
    timers, and semaphores.  The size of the FreeRTOS heap is set by the
    configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */

    configASSERT( ( volatile void * ) NULL );
}

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
vApplicationSetupTimerInterrupt() generates the tick from timer B0, so in this
case configTICK_VECTOR is set to TIMER0_B0_VECTOR. */
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

#define CS_MCLK_DESIRED_KHZ (16000)
#define CS_MCLK_FLL_RATIO   (488) //= rounding MCLK desired HZ / 32768
//Sets up Clock signals for the FR2355: MCLK @ 16M, SMCLK (SPI, etc.) @ 8M.
void clockSetup(void){
    CS_initClockSignal(CS_FLLREF, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_1);

    // Set Ratio and Desired MCLK Frequency and initialize DCO. Returns 1 (STATUS_SUCCESS) if good
    // By default, MCLK = DCOCLK = 16M after this settles.
    CS_initFLLSettle(CS_MCLK_DESIRED_KHZ, CS_MCLK_FLL_RATIO);

    // SMCLK = DCOCLK / 2 = 8M
    CS_initClockSignal(CS_SMCLK, CS_DCOCLKDIV_SELECT, CS_CLOCK_DIVIDER_2);

    //Sanity checking? enable and breakpoint on the following values.
    volatile uint32_t MCLK_val = CS_getMCLK();      //15,990,784 (16M)
    volatile uint32_t SMCLK_val = CS_getSMCLK();    // 7,995,392 (8M)
}

static void prvSetup( void )
{
    //LEDs
    //Red
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0); GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);
    
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
        GPIO_PORT_P1, GPIO_PIN5 + GPIO_PIN6 + GPIO_PIN7, GPIO_PRIMARY_MODULE_FUNCTION);
    //A1
    if (USE_EUSCI_A1){
        GPIO_setAsPeripheralModuleFunctionInputPin(
            GPIO_PORT_P4, GPIO_PIN1 + GPIO_PIN2 + GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);
    }
    //B0
    GPIO_setAsPeripheralModuleFunctionInputPin(
        GPIO_PORT_P1, GPIO_PIN1 + GPIO_PIN2 + GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);
    
    //B1
    GPIO_setAsPeripheralModuleFunctionInputPin(
        GPIO_PORT_P4, GPIO_PIN5 + GPIO_PIN6 + GPIO_PIN7, GPIO_PRIMARY_MODULE_FUNCTION);

    //Disable the GPIO power-on default high-impedance mode. Enables GPIO for actual use.
    PMM_unlockLPM5();

    //Clock config
    clockSetup();

int _system_pre_init( void )
{
    //Always runs on system init - no need for us to define this elsewhere.
    WDT_A_hold( __MSP430_BASEADDRESS_WDT_A__ );
    return 1;
}
