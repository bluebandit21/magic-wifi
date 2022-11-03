/*
 * MillisClock.c
 *
 *  Created on: Nov 3, 2022
 *      Author: gzm20
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


#include "msp430.h"
#include "driverlib.h"
//Referring to https://gist.github.com/malja/9ef92b0180a8f5666483666ec7b52db6
static volatile unsigned long TIMER_MS_COUNT = 0;



void millisSetup(void)  //Main program

{
   clockSetup();
   /* Ensure the timer is stopped. */
   TB1CTL = 0;

   /* Run the timer from the SMCLK. */
   TB1CTL = TBSSEL_2;

   /* Clear everything to start with. */
   TB1CTL |= TBCLR;

   /* Set the compare match value according to the tick rate we want. */
   TB1CCR0 = 8000; //8000 ticks per 1ms

   /* Enable the interrupts. */
   TB1CCTL0 = CCIE;

   /* Start up clean. */
   TB1CTL |= TBCLR;

   /* Up mode. */
   TB1CTL |= MC_1;

   __bis_SR_register(GIE);


   //GPIO: show LED
   GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0); GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);

   while(1) //Loop forever...
   {
       if(millis() % 100 < 50){
           GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);
       }else{
           GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);
       }
   }
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
}
unsigned long millis() {
    unsigned long ms = 0;
    ms = TIMER_MS_COUNT;
    return ms;
}

#pragma vector=TIMER1_B0_VECTOR
__interrupt void millis_isr(){
    //increment millis counter
    TIMER_MS_COUNT++;
}

