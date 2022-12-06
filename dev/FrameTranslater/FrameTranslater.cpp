/*
 * FrameTranslater.cpp
 *
 *  Created on: Oct 31, 2022
 *      Author: andy
 */

#include "FrameTranslater.h"
#include <cassert>

FrameTranslater::FrameTranslater(SX127x *send, SX127x *receive) :
    lora_send(send), lora_receive(receive)
{
}

void FrameTranslater::initSend(uint8_t* ptr, uint16_t length)
{
    //TODO: Fixme -- length here isn't actually used and breaks if correctly supplied!

#ifdef USE_PARITY
    for(int i = 0; i < (length/lora_frame_max) + 1; i++) {
        for(int j = 0; j < lora_frame_max; j++) {
            parity_frame[j] ^= ptr[j];
        }
        ptr += lora_frame_max;
    }
#endif

    curr_send_subframe = 0;
}

bool FrameTranslater::sendNextSubframe(uint8_t* ptr, uint16_t length)
{
    assert(curr_send_subframe <= (length/lora_frame_max));
    //TODO: Check that asserts actually work!

    //TODO: check if can be sent single
    if(length <= LORA_FRAME_MAX) {
        // send as single instead
        lora_send->beginPacket();
        // send packet number with single bit flag
        lora_send->write(curr_send_subframe + (frame_number << SUBFRAMES_NUM_SHIFT) + SINGLE_FRAME_MASK);

        // message
        lora_send->write(ptr , lora_frame_max);

        lora_send->endPacket();
        lora_send->wait(1000);

        // always done if sent as a single
        return false; // NO need to call again
    }


#ifdef USE_PARITY
    if(curr_send_subframe == (length/lora_frame_max)){
        // send parity frame if applicable
        lora_send->beginPacket();
        // packet number
        lora_send->write(MAX_SUBFRAME_ID + (frame_number << SUBFRAMES_NUM_SHIFT));

        // message
        lora_send->write(parity_frame, lora_frame_max);
        //lora_send->write('\0');
        lora_send->endPacket();
        lora_send->wait(1000);
        curr_send_subframe++; //If we re-call this function without re-initializing, this will trigger assert above.
        frame_number++;

        return false;
    }
#endif

    lora_send->beginPacket();

    // packet number
    lora_send->write(curr_send_subframe + (frame_number << SUBFRAMES_NUM_SHIFT));

    // message

    lora_send->write(ptr + lora_frame_max * curr_send_subframe, lora_frame_max);

    lora_send->endPacket();
    lora_send->wait(1000);
    curr_send_subframe++;

#ifdef USE_PARITY
    return true;
#else
    return curr_send_subframe < (length/lora_frame_max);
#endif
}

void FrameTranslater::receiveFrame(uint8_t* dest, uint16_t length){
    uint8_t * dest_ptr;

    // Put received packet to message and counter variable
    // read() and available() method must be called after request() method
    const uint8_t msgLen = lora_receive->available() - 1; // skip the header

    // Read header (contains frame number)
    uint8_t subFrameNum = 0;
    lora_receive->read(&subFrameNum, 1);

    // check whether to set single flag
    if(subFrameNum & SINGLE_FRAME_MASK) {
           received_single = true;
    }

    // update segment tracker array, note offset of 1 to ensure correct initialization
    active_frames[subFrameNum & SUBFRAMES_NUM_MASK] = ((subFrameNum &~ SUBFRAMES_NUM_MASK) >> SUBFRAMES_NUM_SHIFT) + 1;
    subFrameNum &= SUBFRAMES_NUM_MASK; // remove packet number for ease of use



    // read into the appropriate segment of the ethernet frame
    dest_ptr = dest + (subFrameNum * lora_frame_max);

#ifdef USE_PARITY
    // check if parity frame case
    if(subFrameNum == MAX_SUBFRAME_ID) {
        dest_ptr = parity_frame;
    }
#endif

    lora_receive->read(dest_ptr, msgLen);

}


bool FrameTranslater::checkFrame(uint8_t* dest, uint16_t &length) {
    // TODO:: check if single received
    if(received_single) {
        received_single = false;
        length = LORA_FRAME_MAX;
        return true;
    }

    // if not single, then length is full
    length = ETH_BUFF_SIZE;

    // TODO:: clear the active frame or  something
    bool parity_needed = false;
    uint8_t defective_frame;

    uint8_t active_frame_num = active_frames[0];

    // find newest subframe, only can reconstruct newest
    for(int i = 1; i < MAX_SUBFRAME_ID; ++i) {
           if(active_frames[i] > active_frame_num) {
               active_frame_num = active_frames[i];
           }
       }

    // verify all subframes correspond to correct frame
    for(int i = 1; i < MAX_SUBFRAME_ID; ++i) {
        if(active_frame_num != active_frames[i]) {
            // cannot handle more than one bad
#ifdef USE_PARITY
            if(parity_needed) return false;
            defective_frame = i;
            parity_needed = true;
#elif
            return false;
#endif
        }
    }

    // no correction needed, return frame valid
    if(!parity_needed) return true;

#ifdef USE_PARITY
    // perform correction
    uint8_t* defective_ptr = dest + (defective_frame * lora_frame_max);

    // verify parity subframe matches the same id
    if(active_frame_num != active_frames[MAX_SUBFRAME_ID]) {
        return false;
    }

    // replace defective frame with parity frame
    memcpy(defective_ptr, &parity_frame, lora_frame_max);


    // reconstruct
    for(int i = 0; i < MAX_SUBFRAME_ID; ++i) {
        uint8_t* subframe_ptr = dest + (i * lora_frame_max);

        // XOR for all other subframes
        if(defective_ptr != subframe_ptr) {
            for(int j = 0; j < lora_frame_max; ++j) {
                defective_ptr[j] ^= subframe_ptr[j];
            }
        }
    }
    // return true if successful
    return true;

#endif

    return false;


}



