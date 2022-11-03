/*
 * main.c
 *
 *  Created on: Oct 31, 2022
 *      Author: Tom
 */

#include <msp430.h>
#include "driver/include/m2m_wifi.h"
#include "driver/source/nmasic.h"
#include "utility/MillisClock.h"
void main(void) {
    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;                   // Disable the GPIO power-on default high-impedance mode


    //TODO do clock setup too
    millisSetup();
}
