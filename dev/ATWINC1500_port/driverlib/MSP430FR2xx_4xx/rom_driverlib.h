/* --COPYRIGHT--,BSD
 * Copyright (c) 2017, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
/*******************************************************************
*                                                                  *
* This file is a generic ROM include file, that                    *
* helps find the appropriate ROM device header file                *
*                                                                  *
*******************************************************************/

#ifndef __MSP430_ROM_DRIVERLIB_H__
#define __MSP430_ROM_DRIVERLIB_H__

#if defined(__MSP430FR2532__) || defined (__MSP430FR2533__) \
    || defined (__MSP430FR2632__) || defined (__MSP430FR2633__) \
    || defined (__MSP430FR2433__) || defined (__MSP430FR2522__) \
    || defined (__MSP430FR2422__) || defined (__MSP430FR2512__)
#include "rom_headers/rom_driverlib_fr253x_fr263x.h"

#elif defined (__MSP430FR2355__) || defined (__MSP430FR2353__) \
    || defined (__MSP430FR2155__) || defined (__MSP430FR2153__)
#include "rom_headers/rom_driverlib_fr235x.h"

#elif defined (__MSP430FR2676__) || defined (__MSP430FR2675__)
#include "rom_headers/rom_driverlib_fr2676.h"

#else
#error "No driverlib ROM functions found for this device"
#endif

#endif /* #ifndef __MSP430_ROM_DRIVERLIB_H__ */
