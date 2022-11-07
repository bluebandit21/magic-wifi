#ifndef DEFINES_H
#define DEFINES_H

#include "driverlib.h"

constexpr short WIFI_IRQ_PORT         = GPIO_PORT_P3;
constexpr short WIFI_IRQ_PIN          = GPIO_PIN4;

constexpr short LORA_RECEIVE_IRQ_PORT = GPIO_PORT_P3;
constexpr short LORA_RECEIVE_IRQ_PIN  = GPIO_PIN1;

constexpr short LORA_SEND_IRQ_PORT    = GPIO_PORT_P2;
constexpr short LORA_SEND_IRQ_PIN     = GPIO_PIN4;

constexpr short ETH_IRQ_PORT          = GPIO_PORT_P2;
constexpr short ETH_IRQ_PIN           = GPIO_PIN1;


//These live in main.cpp
extern void (*lora_receive_isr)(void);
extern void (*wifi_isr)(void);
extern void (*lora_send_isr)(void);
extern void (*ethernet_isr)(void);

#endif
