/**
 *
 * \file
 *
 * \brief This module contains SAMD21 BSP APIs implementation.
 *
 * Copyright (c) 2016-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */

#include "bsp/include/nm_bsp.h"
#include "bsp/include/nm_bsp_internal.h"
#include "common/include/nm_common.h"
#include "config/conf_winc.h"

static tpfNmBspIsr gpfIsr;


#pragma vector=PORT3_VECTOR
__interrupt void Port_3(void)
{
	if (gpfIsr) {
		gpfIsr();
	}
    GPIO_clearInterrupt(CONF_WINC_IRQ_PORT, CONF_WINC_IRQ_PIN);
}

/*
 *	@fn		init_chip_pins
 *	@brief	Initialize ALL pins except SPI pins (reset, EN, ~wake~)
*/
static void init_chip_pins(void)
{
	GPIO_setAsOutputPin(CONF_WINC_RST_PORT, CONF_WINC_RST_PIN);
	GPIO_setOutputHighOnPin(CONF_WINC_RST_PORT, CONF_WINC_RST_PIN);

	GPIO_setAsInputPin(CONF_WINC_IRQ_PORT, CONF_WINC_IRQ_PIN);
	PM5CTL0 &= ~LOCKLPM5;                   // Disable the GPIO power-on default high-impedance mode

}

/*
 *	@fn		nm_bsp_init
 *	@brief	Initialize BSP
 *	@return	0 in case of success and -1 in case of failure
 */
sint8 nm_bsp_init(void)
{
	gpfIsr = 0; //NULL

	/* Initialize chip IOs. */
	init_chip_pins();
	nm_bsp_reset();

//	?? below needed?
//    /* Make sure a 1ms Systick is configured. */
//    if (!(SysTick->CTRL & SysTick_CTRL_ENABLE_Msk && SysTick->CTRL & SysTick_CTRL_TICKINT_Msk)) {
//	    delay_init();
//    }
//
//	system_interrupt_enable_global();

	return M2M_SUCCESS;
}

/**
 *	@fn		nm_bsp_deinit
 *	@brief	De-iInitialize BSP
 *	@return	0 in case of success and -1 in case of failure
 */
sint8 nm_bsp_deinit(void)
{
	//not convinced this would ever happen and we probably don't need to do anything about it...
	return M2M_SUCCESS;
}

/**
 *	@fn		nm_bsp_reset
 *	@brief	Reset NMC1500 SoC by setting CHIP_EN and RESET_N signals low,
 *           CHIP_EN high then RESET_N high
 */
void nm_bsp_reset(void)
{
	GPIO_setOutputLowOnPin(CONF_WINC_RST_PORT, CONF_WINC_RST_PIN);
	nm_bsp_sleep(100);
	GPIO_setOutputHighOnPin(CONF_WINC_RST_PORT, CONF_WINC_RST_PIN);
	nm_bsp_sleep(100);
}

/*
 *	@fn		nm_bsp_sleep
 *	@brief	Sleep in units of mSec
 *	@param[IN]	u32TimeMsec
 *				Time in milliseconds
 */
void nm_bsp_sleep(uint32 u32TimeMsec)
{
    for(;u32TimeMsec > 0; --u32TimeMsec){
        __delay_cycles(CONF_WINC_MCLK_FREQ_KHZ);
    }
}
//TODO test conversion is OK.

/*
 *	@fn		nm_bsp_register_isr
 *	@brief	Register interrupt service routine
 *	@param[IN]	pfIsr
 *				Pointer to ISR handler
 */

void nm_bsp_register_isr(tpfNmBspIsr pfIsr)
{
	gpfIsr = pfIsr;
	GPIO_setAsInputPinWithPullUpResistor(CONF_WINC_IRQ_PORT, CONF_WINC_IRQ_PIN);
    GPIO_clearInterrupt(CONF_WINC_IRQ_PORT, CONF_WINC_IRQ_PIN);
    GPIO_enableInterrupt(CONF_WINC_IRQ_PORT, CONF_WINC_IRQ_PIN);
    GPIO_selectInterruptEdge(CONF_WINC_IRQ_PORT, CONF_WINC_IRQ_PIN, GPIO_HIGH_TO_LOW_TRANSITION); //ATWINC has IRQn (active low)
    _enable_interrupt();

}

/*
 *	@fn		nm_bsp_interrupt_ctrl
 *	@brief	Enable/Disable interrupts
 *	@param[IN]	u8Enable
 *				'0' disable interrupts. '1' enable interrupts
 */
void nm_bsp_interrupt_ctrl(uint8 u8Enable)
{
	if (u8Enable) {
		GPIO_enableInterrupt(CONF_WINC_IRQ_PORT, CONF_WINC_IRQ_PIN);
	}
	else
	{
		GPIO_disableInterrupt(CONF_WINC_IRQ_PORT, CONF_WINC_IRQ_PIN);
	}
}
