/*
 * FrameTranslater.cpp
 *
 *  Created on: Oct 31, 2022
 *      Author: andy
 */

#include "FrameTranslater.h"

FrameTranslater::FrameTranslater(SX127x *send, SX127x *receive) :
    lora_send(send), lora_receive(receive)
{
    // EMPTY CTOR for now
}

void FrameTranslater::sendFrame(uint8_t* ptr, uint16_t length)
{
    for(int i = 0; i < (length/lora_frame_max); i++) {
        lora_send->beginPacket();

        // packet number
        lora_send->write(i);

        // message
        lora_send->write(ptr, lora_frame_max);
        ptr += lora_frame_max;
        lora_send->write('\0');
        lora_send->endPacket();
        lora_send->wait();
    }
}

void FrameTranslater::receiveFrame(uint8_t* dest, uint16_t length){
    // Request for receiving new LoRa packet
    lora_receive->request();
    // Wait for incoming LoRa packet
    lora_receive->wait();

    // Put received packet to message and counter variable
    // read() and available() method must be called after request() method
    const uint8_t msgLen = lora_receive->available() - 2; // also skip the header

    // Read header (contains frame number
    uint8_t subFrameLen = 0;
    lora_receive->read(&subFrameLen, 1);

    // read into the appropriate segment of the ethernet frame
    lora_receive->read(dest + (subFrameLen * lora_frame_max), msgLen);

}



