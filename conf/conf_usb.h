/*This file is prepared for Doxygen automatic documentation generation.*/
//! \file *********************************************************************
//!
//! \brief This file contains the possible external configuration of the USB
//!
//! - Compiler:           IAR EWAVR and GNU GCC for AVR
//! - Supported devices:  AT90USB1287, AT90USB1286, AT90USB647, AT90USB646
//!
//! \author               Atmel Corporation: http://www.atmel.com \n
//!                       Support and FAQ: http://support.atmel.no/
//!
//! ***************************************************************************

/* Copyright (c) 2009 Atmel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an Atmel
 * AVR product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE EXPRESSLY AND
 * SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _CONF_USB_H_
#define _CONF_USB_H_

#include "modules/usb/usb_commun.h"
#include "modules/usb/usb_commun_hid.h"


//! @defgroup usb_general_conf USB application configuration
//!
//! @{


   // _________________ USB MODE CONFIGURATION ____________________________
   //
   //! @defgroup USB_op_mode USB operating modes configuration
   //! defines to enable device or host usb operating modes
   //! supported by the application
   //! @{

      //! @brief ENABLE to activate the host software library support
      //!
      //! Possible values ENABLE or DISABLE
      #define USB_HOST_FEATURE            ENABLED

      //! @brief ENABLE to activate the device software library support
      //!
      //! Possible values ENABLE or DISABLE
      #define USB_DEVICE_FEATURE          DISABLED

   //! @}

   // _________________ USB REGULATOR CONFIGURATION _______________________
   //
   //! @defgroup USB_reg_mode USB regulator configuration
   //! @{

   //! @brief Enable the internal regulator for USB pads
   //!
   //! When the application voltage is lower than 3.5V, to optimize power consumption
   //! the internal USB pads regulatr can be disabled.
#ifndef USE_USB_PADS_REGULATOR
   #define USE_USB_PADS_REGULATOR   ENABLE      // Possible values ENABLE or DISABLE
#endif
   //! @}

   // _________________ HOST MODE CONFIGURATION ____________________________
   //
   //! @defgroup USB_host_mode_cfg USB host operating mode configuration
   //!
   //! @{

extern U8  HID_Device_PipeIn;


   //!   @brief VID/PID supported table list
   //!
   //!   This table contains the VID/PID that are supported by the reduced host application
   //!   VID_PID_TABLE format definition:
   //!
   //!   #define VID_PID_TABLE      {VID1, number_of_pid_for_this_VID1, PID11_value,..., PID1X_Value \n
   //!                              ...\n
   //!                              ,VIDz, number_of_pid_for_this_VIDz, PIDz1_value,..., PIDzX_Value}
   #define VID_PID_TABLE            {0x1781, 1, 0x0e56,\
	   								 0x061c, 1, 0x10}	 /*	futaba and Act labs interlink */

   //!   @brief CLASS/SUBCLASS_PROTOCOL supported table list
   //!
   //!   This table contains the list of Classes supported by the reduced host application
   //!
   //!   CLASS format definition: \n
   //!   #define CLASS_SUBCLASS_PROTOCOL  {CLASS1, CLASS2,CLASSn,.. }
 
   #define CLASSES_SUPPORTED     { HID_CLASS }
	   
   //! The size of RAM buffer reserved of descriptors manipulation
   #define SIZEOF_DATA_STAGE        250

   //! The address that will be assigned to the connected device
   #define DEVICE_BASE_ADDRESS      0x05

   //! The maximum number of endpoints per interface supported
   #define MAX_EP_PER_INTERFACE     2

   //! The maximum number of interface supported per device
   #define MAX_INTERFACE_FOR_DEVICE 3

   //! The maximum number of devices in the USB tree
   #define MAX_DEVICE_IN_USB_TREE   1

#define SAVE_INTERRUPT_PIPE_FOR_DMS_INTERFACE   ENABLE

   //! Configuration for Hub support in host mode
#if (MAX_DEVICE_IN_USB_TREE>1)
   #define USB_HUB_SUPPORT          ENABLE
   #define USB_MAX_HUB_NUMBER       4
   #define HUB_MAX_NB_PORT          4
#else
   #define USB_HUB_SUPPORT          DISABLE
#endif

   //! The host controller will be limited to the strict VID/PID list.
   //! When enabled, if the device PID/VID does not belongs  to the supported list,
   //! the host controller library will not go to deeper configuration, but to error state.
   #define HOST_STRICT_VID_PID_TABLE      ENABLE
   
   // for class type interfaces vid-pid check makes no sense 
   #define HOST_VID_PID_CHECK			ENABLE

   //! Try to configure the host pipe according to the device descriptors received
   #define HOST_AUTO_CFG_ENDPOINT         ENABLE

   //! Host start of frame interrupt always enable
   #define HOST_CONTINUOUS_SOF_INTERRUPT  DISABLE

   //! When Host error state detected, goto unattached state
   #define HOST_ERROR_RESTART             ENABLE

   //! USB host pipes transfers use USB communication interrupt (allows to use none blocking functions)
   #define USB_HOST_PIPE_INTERRUPT_TRANSFER  ENABLE

   //! Force WDT reset upon ID pin change
   #define ID_PIN_CHANGE_GENERATE_RESET   ENABLE

   //! Enable Timeout delay (time) for host transfer
   #define TIMEOUT_DELAY_ENABLE           ENABLE

   //! delay 1/4sec (250ms) before timeout value
   #define TIMEOUT_DELAY               10

   //! Enable cpt NAK Timeout for host transfer
   #define NAK_TIMEOUT_ENABLE          DISABLE

   //! Number of NAK handshake before timeout for transmit functions (up to 0xFFFF)
   #define NAK_SEND_TIMEOUT            0x0FFF

   //! NAKNumber of NAK handshake before timeout for receive functions (up to 0xFFFF)
   #define NAK_RECEIVE_TIMEOUT         0x0FFF

   //! For reduced host only allows to control VBUS generator with PIO PE.7
   #define SOFTWARE_VBUS_CTRL          ENABLE

   #if (HOST_AUTO_CFG_ENDPOINT==FALSE)
      //! If no auto configuration of EP, map here user function
      #define        User_configure_endpoint()
   #endif

   //! @defgroup host_cst_actions USB host custom actions
   //!
   //! @{
   // write here the action to associate to each USB host event
   // be carefull not to waste time in order not disturbing the functions
   #define Usb_id_transition_action()
   #define Host_device_disconnection_action()
   #define Host_device_connection_action()
   #define Host_sof_action()
   #define Host_suspend_action()
   #define Host_hwup_action()
   #define Host_device_not_supported_action()
   #define Host_device_class_not_supported_action()
   #define Host_device_supported_action()
   #define Host_device_error_action()
   //! @}


   //! @}


// _________________ DEVICE MODE CONFIGURATION __________________________

   //! @defgroup USB_device_mode_cfg USB device operating mode configuration
   //!
   //! @{

#define USB_DEVICE_SN_USE          DISABLE            // DISABLE
#define USE_DEVICE_SN_UNIQUE       DISABLE            // ignore if USB_DEVICE_SN_USE = DISABLE


#define Usb_unicode(a)         ((U16)(a))

   //! @defgroup device_cst_actions USB device custom actions
   //!
   //! @{
   // write here the action to associate to each USB event
   // be carefull not to waste time in order not disturbing the functions
#define Usb_sof_action()
#define Usb_wake_up_action()
#define Usb_resume_action()
#define Usb_suspend_action()
#define Usb_reset_action()
#define Usb_vbus_on_action()
#define Usb_vbus_off_action()
#define Usb_set_configuration_action()
   //! @}

   //! @}


//! @}

#endif // _CONF_USB_H_
