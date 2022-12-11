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
//*****************************************************************************
//
// icc.c - Driver for the icc Module.
//
//*****************************************************************************

//*****************************************************************************
//
//! \addtogroup icc_api icc
//! @{
//
//*****************************************************************************

#include "inc/hw_memmap.h"

#ifdef __MSP430_HAS_ICC__
#include "icc.h"

#include <assert.h>

void ICC_enable(void)
{
	HWREG16(ICC_BASE + OFS_ICCSC) |= ICCEN;
}

void ICC_disable(void)
{
	HWREG16(ICC_BASE + OFS_ICCSC) &= ~ICCEN;
}

void ICC_setInterruptLevel( uint32_t ILSRmask, uint8_t interruptLevel){
	uint32_t mask = 0x1;
	uint8_t i;

	for(i = 0; i < 8; i++)
	{
		if(ILSRmask & mask)
		{
			HWREG16(ICC_BASE + OFS_ICCILSR0) &= ~(0x3 << (i << 1));
			HWREG16(ICC_BASE + OFS_ICCILSR0) |= (interruptLevel << (i << 1));
		}
		mask <<= 1;
	}
	for(i = 0; i < 8; i++)
	{
		if(ILSRmask & mask)
		{
			HWREG16(ICC_BASE + OFS_ICCILSR1) &= ~(0x3 << (i << 1));
			HWREG16(ICC_BASE + OFS_ICCILSR1) |= (interruptLevel << (i << 1));
		}
		mask <<= 1;
	}
	for(i = 0; i < 8; i++)
	{
		if(ILSRmask & mask)
		{
			HWREG16(ICC_BASE + OFS_ICCILSR2) &= ~(0x3 << (i << 1));
			HWREG16(ICC_BASE + OFS_ICCILSR2) |= (interruptLevel << (i << 1));
		}
		mask <<= 1;
	}
	for(i = 0; i < 8; i++)
	{
		if(ILSRmask & mask)
		{
			HWREG16(ICC_BASE + OFS_ICCILSR3) &= ~(0x3 << (i << 1));
			HWREG16(ICC_BASE + OFS_ICCILSR3) |= (interruptLevel << (i << 1));
		}
		mask <<= 1;
	}
}

uint8_t ICC_getInterruptLevel( uint32_t interruptSource){
    uint32_t mask = 0x1;
	uint8_t i;

	for(i = 0; i < 8; i++)
	{
		if(interruptSource & mask)
		{
			return (HWREG16(ICC_BASE + OFS_ICCILSR0) & (0x3 << (i << 1))) >> (i << 1);
		}
		mask <<= 1;
	}
	for(i = 0; i < 8; i++)
	{
		if(interruptSource & mask)
		{
			return (HWREG16(ICC_BASE + OFS_ICCILSR1) & (0x3 << (i << 1))) >> (i << 1);
		}
		mask <<= 1;
	}
	for(i = 0; i < 8; i++)
	{
		if(interruptSource & mask)
		{
			return (HWREG16(ICC_BASE + OFS_ICCILSR2) & (0x3 << (i << 1))) >> (i << 1);
		}
		mask <<= 1;
	}
	for(i = 0; i < 8; i++)
	{
		if(interruptSource & mask)
		{
			return (HWREG16(ICC_BASE + OFS_ICCILSR3) & (0x3 << (i << 1))) >> (i << 1);
		}
		mask <<= 1;
	}
	
	//Error code
	return 4;
}

bool ICC_isVirtualStackEmpty(void)
{
	return (HWREG16(ICC_BASE + OFS_ICCSC) & VSEFLG) ? true : false;
}

bool ICC_isVirtualStackFull(void)
{
	return (HWREG16(ICC_BASE + OFS_ICCSC) & VSFFLG) ? true : false;
}

uint8_t ICC_getCurrentICM(void)
{
	return (HWREG16(ICC_BASE + OFS_ICCSC) & ICMC);
}

uint8_t ICC_getMVSStackPointer(void)
{
	return ((HWREG16(ICC_BASE + OFS_ICCMVS) & MVSSP) >> 8);
}

uint8_t ICC_getICM3(void)
{
	return ((HWREG16(ICC_BASE + OFS_ICCMVS) & ICM3) >> 6);
}

uint8_t ICC_getICM2(void)
{
	return ((HWREG16(ICC_BASE + OFS_ICCMVS) & ICM2) >> 4);
}

uint8_t ICC_getICM1(void)
{
	return ((HWREG16(ICC_BASE + OFS_ICCMVS) & ICM1) >> 2);
}

uint8_t ICC_getICM0(void)
{
	return (HWREG16(ICC_BASE + OFS_ICCMVS) & ICM0);
}

#endif
//*****************************************************************************
//
//! Close the doxygen group for icc_api
//! @}
//
//*****************************************************************************
