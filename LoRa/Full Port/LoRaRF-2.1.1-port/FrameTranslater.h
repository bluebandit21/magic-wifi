/*
 *
 * This class will translate ethernet frames into LoRa packets, adding parity
 */

#include "SX127x.h"

class FrameTranslater
{

public:
    FrameTranslater(SX127x *send, SX127x *receive);
    void sendFrame(uint8_t* ptr, uint16_t length);
    void receiveFrame(uint8_t* dest, uint16_t length);

private:
    const uint8_t lora_frame_max = 7;
    SX127x* const lora_send;
    SX127x* const lora_receive;


    uint8_t* frame_ptr;
    uint16_t frame_length;
};
