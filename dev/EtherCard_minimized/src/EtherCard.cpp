// This code slightly follows the conventions of, but is not derived from:
//      EHTERSHIELD_H library for Arduino etherShield
//      Copyright (c) 2008 Xing Yu.  All right reserved. (this is LGPL v2.1)
// It is however derived from the enc28j60 and ip code (which is GPL v2)
//      Author: Pascal Stang
//      Modified by: Guido Socher
//      DHCP code: Andrew Lindsay
// Hence: GPL V2
//
// 2010-05-19 <jc@wippler.nl>

#include "EtherCard.h"
#include <stdarg.h>
#include <avr/eeprom.h>

EtherCard ether;

uint8_t EtherCard::mymac[ETH_LEN];  // my MAC address
uint16_t EtherCard::delaycnt = 0; //request gateway ARP lookup

uint8_t EtherCard::begin (const uint16_t size,
                          const uint8_t* macaddr,
                          uint8_t csPin) {
    copyMac(mymac, macaddr);
    return initialize(size, mymac, csPin);
}


char* EtherCard::wtoa(uint16_t value, char* ptr)
{
    if (value > 9)
        ptr = wtoa(value / 10, ptr);
    *ptr = '0' + value % 10;
    *++ptr = 0;
    return ptr;
}
