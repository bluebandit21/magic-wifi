// Some common utilities needed for IP and web applications
// Author: Guido Socher
// Copyright: GPL V2
//
// 2010-05-20 <jc@wippler.nl>

#include "EtherCard.h"

void EtherCard::copyIp (uint8_t *dst, const uint8_t *src) {
    memcpy(dst, src, IP_LEN);
}

void EtherCard::copyMac (uint8_t *dst, const uint8_t *src) {
    memcpy(dst, src, ETH_LEN);
}

void EtherCard::printIp (const char* msg, const uint8_t *buf) {
    Serial.print(msg);
    EtherCard::printIp(buf);
    Serial.println();
}

void EtherCard::printIp (const __FlashStringHelper *ifsh, const uint8_t *buf) {
    Serial.print(ifsh);
    EtherCard::printIp(buf);
    Serial.println();
}

void EtherCard::printIp (const uint8_t *buf) {
    for (uint8_t i = 0; i < IP_LEN; ++i) {
        Serial.print( buf[i], DEC );
        if (i < 3)
            Serial.print('.');
    }
}

// convert a single hex digit character to its integer value
unsigned char h2int(char c)
{
    if (isdigit(c)) {
        return((unsigned char)c - '0');
    }
    else if (islower(c)) {
        return((unsigned char)c - 'a' + 10);
    }
    else if (isupper(c)) {
        return((unsigned char)c - 'A' + 10);
    }
    return(0);
}

// decode a url string e.g "hello%20joe" or "hello+joe" becomes "hello joe"
void EtherCard::urlDecode (char *urlbuf)
{
    char c;
    char *dst = urlbuf;
    while ((c = *urlbuf) != 0) {
        if (c == '+') c = ' ';
        if (c == '%') {
            c = *++urlbuf;
            c = (h2int(c) << 4) | h2int(*++urlbuf);
        }
        *dst++ = c;
        urlbuf++;
    }
    *dst = '\0';
}

// convert a single character to a 2 digit hex str
// a terminating '\0' is added
void int2h(char c, char *hstr)
{
    hstr[1]=(c & 0xf)+'0';
    if ((c & 0xf) >9) {
        hstr[1]=(c & 0xf) - 10 + 'a';
    }
    c=(c>>4)&0xf;
    hstr[0]=c+'0';
    if (c > 9) {
        hstr[0]=c - 10 + 'a';
    }
    hstr[2]='\0';
}

// end of webutil.c
