#include <msp430.h> 
#include "enc28j60.h"
#include "driverlib.h"


/**
 * main.c
 */
int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	
	return 0;
}
