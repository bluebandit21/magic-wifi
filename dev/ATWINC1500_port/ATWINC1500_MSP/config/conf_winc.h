
/**
 *
 * \file
 *
 * \brief WINC1500 configuration.
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

#ifndef CONF_WINC_H_INCLUDED
#define CONF_WINC_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

//#include "board.h"
#include "driverlib.h"
#include "stdio.h"
//m2m_types keeps saying 'expected a type specifier' for checking alignment. Since Arduino removed all of those, disabling them for now.
#define _lint (1)
/*
   ---------------------------------
   ------ PIN & SPI settings -------
   ---------------------------------
*/

#define CONF_WINC_USE_SPI				(1)

/** SPI port and pin as well as module setups. */
#define CONF_WINC_SPI_BASE				(EUSCI_A0_BASE)

#define CONF_WINC_SPI_MODULE_PORT       (GPIO_PORT_P1)
#define CONF_WINC_SPI_MOSI_PIN          (GPIO_PIN7)
#define CONF_WINC_SPI_MISO_PIN          (GPIO_PIN6)
#define CONF_WINC_SPI_SCK_PIN           (GPIO_PIN5)

#define CONF_WINC_SPI_CS_PORT           (GPIO_PORT_P4)
#define CONF_WINC_SPI_CS_PIN			    (GPIO_PIN0)

#define CONF_WINC_IRQ_PORT              (GPIO_PORT_P3)
#define CONF_WINC_IRQ_PIN               (GPIO_PIN4)
//Changing IRQ port/pin? Also see nm_bsp_msp430fr2355.c -> change port and pin and vector.

#define CONF_WINC_RST_PORT              (GPIO_PORT_P3)
#define CONF_WINC_RST_PIN               (GPIO_PIN2)

// SPI frequency (for init)
#define CONF_WINC_SPI_CLOCK				(1000000)
// CPU frequency (for delay func)
#define CONF_WINC_MCLK_FREQ_KHZ			(2000)

/*
   ---------------------------------
   --------- Debug Options ---------
   ---------------------------------
*/

#define CONF_WINC_DEBUG					(0)
#define CONF_WINC_PRINTF				printf

#ifdef __cplusplus
}
#endif

#endif /* CONF_WINC_H_INCLUDED */
