#include "EtherCard.h"

// ethernet interface mac address, must be unique on the LAN
static byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };

byte Ethernet::buffer[700];

void setup() {
    // put your setup code here, to run once:
    Serial.begin(57600);
    Serial.println("\n[pings]");
    if (ether.begin(sizeof Ethernet::buffer, mymac, SS) == 0)
        Serial.println(F("Failed to access Ethernet controller"));

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
    int len = ether.packetReceive();
    if(len > 0){
        Serial.println("Received Ethernet packet!");
        Serial.println("Begin hexdump:");
        for(int i=0;i<len;i++){
            Serial.print(Ethernet::buffer[i], HEX);
            Serial.print("  ");
        }
    }
    Serial.println("");
    Serial.println("End hexdump.");
    delay(1000);
    memcpy(Ethernet::buffer, broadcastEth, sizeof(broadcastEth));
    ether.packetSend(sizeof(broadcastEth));

    delay(1000);
}
