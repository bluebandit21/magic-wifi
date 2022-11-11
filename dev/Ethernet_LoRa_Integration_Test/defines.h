#ifndef DEFINES_H
#define DEFINES_H

#include "driverlib.h"


#define BOARD_A 1

//To handle asymmetric send+receive frequencies, later will also be used for asymmetric host/client wifi
#ifdef BOARD_A

constexpr unsigned LORA_TRANSMIT_FREQ = 916000000;
constexpr unsigned LORA_RECEIVE_FREQ  = 917000000;

#else //Board B

constexpr unsigned LORA_TRANSMIT_FREQ = 917000000;
constexpr unsigned LORA_RECEIVE_FREQ  = 916000000;

#endif





constexpr short WIFI_IRQ_PORT         = GPIO_PORT_P3;
constexpr short WIFI_IRQ_PIN          = GPIO_PIN4;

constexpr short LORA_RECEIVE_IRQ_PORT = GPIO_PORT_P3;
constexpr short LORA_RECEIVE_IRQ_PIN  = GPIO_PIN1;

constexpr short LORA_SEND_IRQ_PORT    = GPIO_PORT_P2;
constexpr short LORA_SEND_IRQ_PIN     = GPIO_PIN4;

constexpr short ETH_IRQ_PORT          = GPIO_PORT_P2;
constexpr short ETH_IRQ_PIN           = GPIO_PIN1;


//These live in main.cpp
extern void (*wifi_isr)(void);
extern void (*ethernet_isr)(void);

#endif
