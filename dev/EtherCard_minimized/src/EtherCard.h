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
//
//
// PIN Connections (Using Arduino UNO):
//   VCC -   3.3V
//   GND -    GND
//   SCK - Pin 13
//   SO  - Pin 12
//   SI  - Pin 11
//   CS  - Pin  8
//
/** @file */

#ifndef EtherCard_h
#define EtherCard_h
#ifndef __PROG_TYPES_COMPAT__
  #define __PROG_TYPES_COMPAT__
#endif

#if ARDUINO >= 100
#include <Arduino.h> // Arduino 1.0
#define WRITE_RESULT size_t
#define WRITE_RETURN return 1;
#else
#include <WProgram.h> // Arduino 0022
#define WRITE_RESULT void
#define WRITE_RETURN
#endif

#include <avr/pgmspace.h>
#include "enc28j60.h"
#include "net.h"

/** This class provides the main interface to a ENC28J60 based network interface card and is the class most users will use.
*   @note   All TCP/IP client (outgoing) connections are made from source port in range 2816-3071. Do not use these source ports for other purposes.
*/
class EtherCard : public Ethernet {
public:
    static uint8_t mymac[ETH_LEN];  ///< MAC address
    static uint8_t myip[IP_LEN];    ///< IP address
    static uint8_t netmask[IP_LEN]; ///< Netmask
    static uint8_t broadcastip[IP_LEN]; ///< Subnet broadcast address
    static uint8_t gwip[IP_LEN];   ///< Gateway
    static uint8_t dhcpip[IP_LEN]; ///< DHCP server IP address
    static uint8_t dnsip[IP_LEN];  ///< DNS server IP address
    static uint8_t hisip[IP_LEN];  ///< DNS lookup result
    static uint16_t hisport;  ///< TCP port to connect to (default 80)
    static bool using_dhcp;   ///< True if using DHCP
    static bool persist_tcp_connection; ///< False to break connections on first packet received
    static uint16_t delaycnt; ///< Counts number of cycles of packetLoop when no packet received - used to trigger periodic gateway ARP request

    // EtherCard.cpp
    /**   @brief  Initialise the network interface
    *     @param  size Size of data buffer
    *     @param  macaddr Hardware address to assign to the network interface (6 bytes)
    *     @param  csPin Arduino pin number connected to chip select. Default = 8
    *     @return <i>uint8_t</i> Firmware version or zero on failure.
    */
    static uint8_t begin (const uint16_t size, const uint8_t* macaddr,
                          uint8_t csPin = SS);

    // webutil.cpp
    /**   @brief  Copies an IP address
    *     @param  dst Pointer to the 4 byte destination
    *     @param  src Pointer to the 4 byte source
    *     @note   There is no check of source or destination size. Ensure both are 4 bytes
    */
    static void copyIp (uint8_t *dst, const uint8_t *src);

    /**   @brief  Copies a hardware address
    *     @param  dst Pointer to the 6 byte destination
    *     @param  src Pointer to the 6 byte destination
    *     @note   There is no check of source or destination size. Ensure both are 6 bytes
    */
    static void copyMac (uint8_t *dst, const uint8_t *src);

    /**   @brief  Output to serial port in dotted decimal IP format
    *     @param  buf Pointer to 4 byte IP address
    *     @note   There is no check of source or destination size. Ensure both are 4 bytes
    */
    static void printIp (const uint8_t *buf);

    /**   @brief  Output message and IP address to serial port in dotted decimal IP format
    *     @param  msg Pointer to null terminated string
    *     @param  buf Pointer to 4 byte IP address
    *     @note   There is no check of source or destination size. Ensure both are 4 bytes
    */
    static void printIp (const char* msg, const uint8_t *buf);

    /**   @brief  Output Flash String Helper and IP address to serial port in dotted decimal IP format
    *     @param  ifsh Pointer to Flash String Helper
    *     @param  buf Pointer to 4 byte IP address
    *     @note   There is no check of source or destination size. Ensure both are 4 bytes
    *     @todo   What is a FlashStringHelper?
    */
    static void printIp (const __FlashStringHelper *ifsh, const uint8_t *buf);


    /**   @brief  Convert an IP address from dotted decimal formated string to 4 bytes
    *     @param  bytestr Pointer to the 4 byte array to store IP address
    *     @param  str Pointer to string to parse
    *     @return <i>uint8_t</i> 0 on success
    */
    static uint8_t parseIp(uint8_t *bytestr,char *str);

    /**   @brief  Convert a 16-bit integer into a string
    *     @param  value The number to convert
    *     @param  ptr The string location to write to
    */
    char* wtoa(uint16_t value, char* ptr);
};

extern EtherCard ether; //!< Global presentation of EtherCard class

#endif
