/*
 *
 * This class will translate ethernet frames into LoRa packets, adding parity
 */

#include "SX127x.h"

#define USE_PARITY
#define SUBFRAMES_NUM_MASK 0b111
#define SUBFRAMES_NUM_SHIFT 3 // note this is +1 due to zero index eg. 7 -> 8, must be power of 2
#define LORA_FRAME_MAX 217
#define MAX_SUBFRAME_ID 7 // note this is +1 due to zero index eg. 7 -> 8, must be power of 2


class FrameTranslater
{

public:
    FrameTranslater(SX127x *send, SX127x *receive);
    void sendFrame(uint8_t* ptr, uint16_t length);

    void receiveFrame(uint8_t* dest, uint16_t length);
    bool checkFrame(uint8_t* dest, uint16_t length);

#ifndef TEST
private:
#endif
    const uint8_t lora_frame_max = LORA_FRAME_MAX;
    SX127x* const lora_send;
    SX127x* const lora_receive;

    uint8_t active_frames[8] = {0};
    uint8_t frame_number = 0;


#ifdef USE_PARITY
    void parity_fix();
    uint8_t parity_frame[LORA_FRAME_MAX];

#endif
};
