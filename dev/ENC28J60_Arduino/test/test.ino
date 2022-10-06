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
}
