#ifndef DEFINES_H
#define DEFINES_H


#define BOARD_A 1
#define PCB 1

#ifdef __cplusplus

#include "driverlib.h"

//To handle asymmetric send+receive frequencies, later will also be used for asymmetric host/client wifi
#ifdef BOARD_A

//TODO:: warning!! constexpr unsigned OVERFLOWS!
constexpr long LORA_TRANSMIT_FREQ = 916000000;
constexpr long LORA_RECEIVE_FREQ = 917000000;

#else //Board B

constexpr long LORA_TRANSMIT_FREQ = 917000000;
constexpr long LORA_RECEIVE_FREQ = 916000000;

#endif


// The ethernet max MTU is 1518, which includes the 4-byte checksum and all Ethernet headers and all (but not the preamble)
// As a result, because we are having the chip generate the checksum for us and are not passing it around anywhere, we have
// 1514 bytes for the buffer we're using, which does include the Ethernet headers but not the CRC at the end.
// However, as our frames may have 801.1Q tagging information added, we need the additional 4 bytes to hold it if present.
constexpr unsigned ETH_BUFF_SIZE = 1518 - 4 + 4;

// The wifi library requires us to have 14 bytes of leading info at the start of our contiguous data array.
constexpr unsigned ETH_WIFI_HEADER_SIZE = 14;

// However, as our LoRa subframing stuff divides evenly into 7 groups of 217, w/ 7*217 = 1519,
// we have intentional trailing random garbage at the end that we'll never actually use but that's there so we avoid
// UB caused by reading random garbage memory past the end of our array.
constexpr unsigned ETH_BACKING_SIZE = ETH_WIFI_HEADER_SIZE + ETH_BUFF_SIZE + 5;

constexpr unsigned ETH_HEADER_SIZE = 14; //The minimum size a valid Ethernet frame could be



constexpr uint16_t delay_between_wifi_connect_attempts_ts = 10 * 1; //1 second * 10 ticks / second

constexpr uint16_t wifi_connection_timeout = 10 * 5; //5 seconds * 10 ticks / second
constexpr uint16_t wifi_heartbeat_period = 10 * 4; //4 seconds * 10 ticks / second


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

// WiFi channel settings
#define MAIN_WLAN_SSID "MAGIC_WIFI"
#define MAIN_WLAN_CHANNEL (6)

#endif //#ifdef __cplusplus
#endif //#ifndef DEFINES_H
