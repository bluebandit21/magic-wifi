#include <msp430.h> 
#include "enc28j60.h"
#include "driverlib.h"
#include "SX127x.h"
#include "FrameTranslater.h"


// ethernet interface mac address, must be unique on the LAN
static byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };

ENC28J60 ether;
byte ENC28J60::buffer[1518];

void setup() {
    // put your setup code here, to run once:
    ether.initialize(sizeof Ethernet::buffer, mymac);

    //Enable all the things
    ether.enablePromiscuous(false);
    ether.enableMulticast();
    ether.enableBroadcast(false);
}

const byte broadcastEth[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, //Broadcast Eth MAC dest
    0x74, 0x69, 0x69, 0x2D, 0x30, 0x31, //Source Eth MAC addr (copied from above)
    0x00, 0x05,                         //Length (five bytes)
    'H', 'e', 'l', 'l', 'o'
};


void loop() {
  // put your main code here, to run repeatedly:
    memcpy(ENC28J60::buffer, broadcastEth, sizeof(broadcastEth));
    ether.packetSend(sizeof(broadcastEth));

    for(volatile int i=0;i<240;i++){
        for(volatile int j=0;j<100;j++){
            volatile int x = 1;
            x += 1;
        }
    }





}

/**
 * main.c
 */
int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    setup();
    while(1) loop();

    return 0;
}
