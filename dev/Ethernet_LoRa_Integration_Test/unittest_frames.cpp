#define TEST

#include <msp430.h> 
#include "enc28j60.h"
#include "driverlib.h"
#include "SX127x.h"
#include "FrameTranslater.h"
#include "defines.h"


byte buffer[1518];
FrameTranslater frameTranslater = FrameTranslater(nullptr, nullptr);

#define DEBUG_ABORT() while(1){}
#define SUCCESSFUL() while(1){}




void main() {
    //-----Test Good------
    // set to good parity value
    frameTranslater.parity_frame[1] = 0xF8;
    // populate test data
    buffer[1] = 0xFF;
    buffer[218] = 0xFE;
    buffer[435] = 0xFD;
    buffer[652] = 0xFC;
    buffer[869] = 0xFB;
    buffer[1086] = 0xFA;
    buffer[1303] = 0xF9;

    // test all the realtistic use cases, should reach end
    for(int i = 0; i < 6; ++i) {
        frameTranslater.active_frames[i] = 1;
        // not all frames, check should fail
        if(frameTranslater.checkFrame(buffer, 1518)) DEBUG_ABORT();
    }

    frameTranslater.active_frames[6] = 1;
    // all frames, check should pass even without parity
    if(!frameTranslater.checkFrame(buffer, 1518)) DEBUG_ABORT();

    frameTranslater.active_frames[7] = 1;
    // all frames, check should pass
    if(!frameTranslater.checkFrame(buffer, 1518)) DEBUG_ABORT();

    frameTranslater.active_frames[1] = 2;
    // check should fail since newest frame is not complete
    if(frameTranslater.checkFrame(buffer, 1518)) DEBUG_ABORT();

    frameTranslater.active_frames[1] = 0;
    buffer[218] = 0xDE;
    // missing frame #1, check should still complete, and fix error
    if(!frameTranslater.checkFrame(buffer, 1518)) DEBUG_ABORT();
    if(buffer[218] != 0xFE) DEBUG_ABORT();

    frameTranslater.active_frames[4] = 0;
    // check should fail since two frames are missing
    if(frameTranslater.checkFrame(buffer, 1518)) DEBUG_ABORT();

    buffer[869] = 0x31;
    frameTranslater.active_frames[1] = 1;
    // missing frame #4, check should still complete, and fix error
    if(!frameTranslater.checkFrame(buffer, 1518)) DEBUG_ABORT();
    if(buffer[869] != 0xFB) DEBUG_ABORT();

    // should reach here if the code is working properly
    SUCCESSFUL();

}

