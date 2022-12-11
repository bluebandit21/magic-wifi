/**
 *
 * \file
 *
 * \brief This module contains NMC1000 bus wrapper APIs implementation.
 *
 * Copyright (c) 2016-2021 Microchip Technology Inc. and its subsidiaries.
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

#include <stdio.h>
#include "bsp/include/nm_bsp.h"
#include "common/include/nm_common.h"
#include "bus_wrapper/include/nm_bus_wrapper.h"
#include "config/conf_winc.h"

#define NM_BUS_MAX_TRX_SZ	256

tstrNmBusCapabilities egstrNmBusCapabilities =
{
	NM_BUS_MAX_TRX_SZ
};


//struct spi_module master;
//struct spi_slave_inst slave_inst;

//ASSUMES SMCLK is setup correctly.
static inline sint8 spi_rw_pio(uint8* pu8Mosi, uint8* pu8Miso, uint16 u16Sz)
{
		// EUSCI_A_SPI_transmitData(CONF_WINC_SPI_BASE, pstrParam->pu8InBuf);
		// 	pstrParam->pu8OutBuf = EUSCI_A_SPI_receiveData(CONF_WINC_SPI_BASE);
	uint8 u8Dummy = 0;
	uint8 u8SkipMosi = 0, u8SkipMiso = 0;
	// uint16_t txd_data = 0;
	// uint16_t rxd_data = 0;

	if(((pu8Miso == NULL) && (pu8Mosi == NULL)) ||(u16Sz == 0)) {
		return M2M_ERR_INVALID_ARG;
	}

	if (!pu8Mosi) {
		pu8Mosi = &u8Dummy;
		u8SkipMosi = 1;
	}
	else if(!pu8Miso) {
		pu8Miso = &u8Dummy;
		u8SkipMiso = 1;
	}
	else {
		return M2M_ERR_BUS_FAIL;
	}
	GPIO_setOutputLowOnPin(CONF_WINC_SPI_CS_PORT, CONF_WINC_SPI_CS_PIN);

	// spi_select_slave(&master, &slave_inst, true);

	while (u16Sz) {
		// txd_data = *pu8Mosi;
    //    while(!spi_is_ready_to_write(&master));
    //    while(spi_write(&master, txd_data) != STATUS_OK);
		//while(EUSCI_A_SPI_isBusy(CONF_WINC_SPI_BASE) == EUSCI_A_SPI_BUSY);
		EUSCI_A_SPI_transmitData(CONF_WINC_SPI_BASE, *pu8Mosi);
        while(EUSCI_A_SPI_isBusy(CONF_WINC_SPI_BASE) == EUSCI_A_SPI_BUSY);

		/* Read SPI master data register. */
		// while(!spi_is_ready_to_read(&master));
		// while(spi_read(&master, &rxd_data) != STATUS_OK);
		// *pu8Miso = rxd_data;

		//wait here somehow

		*pu8Miso = EUSCI_A_SPI_receiveData(CONF_WINC_SPI_BASE);
        while(EUSCI_A_SPI_isBusy(CONF_WINC_SPI_BASE) == EUSCI_A_SPI_BUSY);

		u16Sz--;
		if (!u8SkipMiso)
			pu8Miso++;
		if (!u8SkipMosi)
			pu8Mosi++;
	}

	// while (!spi_is_write_complete(&master))
	// 	;

	//spi_select_slave(&master, &slave_inst, false);
	GPIO_setOutputHighOnPin(CONF_WINC_SPI_CS_PORT, CONF_WINC_SPI_CS_PIN);

	return M2M_SUCCESS;
}

sint8 nm_spi_rw(uint8* pu8Mosi, uint8* pu8Miso, uint16 u16Sz)
{
	{
		return spi_rw_pio(pu8Mosi, pu8Miso, u16Sz);
	}
}

/*
*	@fn		nm_bus_init
*	@brief	Initialize the bus wrapper
*	@return	M2M_SUCCESS in case of success and M2M_ERR_BUS_FAIL in case of failure
*/
sint8 nm_bus_init(void *pvinit)
{
	GPIO_setAsPeripheralModuleFunctionInputPin(
			CONF_WINC_SPI_MODULE_PORT, 
			CONF_WINC_SPI_SCK_PIN + CONF_WINC_SPI_MISO_PIN + CONF_WINC_SPI_MOSI_PIN,
			GPIO_PRIMARY_MODULE_FUNCTION);
	
	GPIO_setAsOutputPin(CONF_WINC_SPI_CS_PORT, CONF_WINC_SPI_CS_PIN);
	GPIO_setOutputHighOnPin(CONF_WINC_SPI_CS_PORT, CONF_WINC_SPI_CS_PIN);

	PMM_unlockLPM5();
	EUSCI_A_SPI_initMasterParam param = {0};
		param.selectClockSource = EUSCI_A_SPI_CLOCKSOURCE_SMCLK;
		param.clockSourceFrequency = CS_getSMCLK(); //SMCLK capable of up to 24MHz. Non-jank around 8MHz.
		param.desiredSpiClock = 1000000;
		param.msbFirst = EUSCI_A_SPI_MSB_FIRST;
		param.clockPhase = EUSCI_A_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT;
		param.clockPolarity = EUSCI_A_SPI_CLOCKPOLARITY_INACTIVITY_LOW;
		param.spiMode = EUSCI_A_SPI_3PIN;
	EUSCI_A_SPI_initMaster(CONF_WINC_SPI_BASE, &param);

	//Enable SPI Module
	EUSCI_A_SPI_enable(CONF_WINC_SPI_BASE);
	//Clear receive interrupt flag
	EUSCI_A_SPI_clearInterrupt(CONF_WINC_SPI_BASE, EUSCI_A_SPI_RECEIVE_INTERRUPT);

	nm_bsp_reset();
	nm_bsp_sleep(1);
	return M2M_SUCCESS;
}

/*
*	@fn		nm_bus_ioctl
*	@brief	send/receive from the bus
*	@param[IN]	u8Cmd
*					IOCTL command for the operation
*	@param[IN]	pvParameter
*					Arbitrary parameter depending on IOCTL
*	@return	M2M_SUCCESS in case of success and M2M_ERR_BUS_FAIL in case of failure
*	@note	For SPI only, it's important to be able to send/receive at the same time
*/
sint8 nm_bus_ioctl(uint8 u8Cmd, void* pvParameter)
{
	sint8 s8Ret = 0;
	switch(u8Cmd)
	{
		case NM_BUS_IOCTL_RW: {
			tstrNmSpiRw *pstrParam = (tstrNmSpiRw *)pvParameter;
			s8Ret = nm_spi_rw(pstrParam->pu8InBuf, pstrParam->pu8OutBuf, pstrParam->u16Sz);
		}
		break;
		default:
			s8Ret = -1;
			M2M_ERR("invalid ioctl cmd\n");
			break;
	}
	return s8Ret;
}

/*
*	@fn		nm_bus_deinit
*	@brief	De-initialize the bus wrapper
*/
sint8 nm_bus_deinit(void)
{
	//TODO assuming we never need this?
	return M2M_SUCCESS;
}

/*
*	@fn			nm_bus_reinit
*	@brief		re-initialize the bus wrapper
*	@param [in]	void *config
*					re-init configuration data
*	@return		M2M_SUCCESS in case of success and M2M_ERR_BUS_FAIL in case of failure
*/
sint8 nm_bus_reinit(void* config)
{
    nm_bus_init(0);
	return M2M_SUCCESS;
}

