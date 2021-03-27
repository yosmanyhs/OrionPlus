/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * This file is part of the TinyUSB stack.
 */

#ifndef _TUSB_OPTION_H_
#define _TUSB_OPTION_H_

#define TUSB_VERSION_MAJOR     0
#define TUSB_VERSION_MINOR     8
#define TUSB_VERSION_REVISION  0
#define TUSB_VERSION_STRING    TU_STRING(TUSB_VERSION_MAJOR) "." TU_STRING(TUSB_VERSION_MINOR) "." TU_STRING(TUSB_VERSION_REVISION)

/** \defgroup group_mcu Supported MCU
 * \ref CFG_TUSB_MCU must be defined to one of these
 *  @{ */

#define OPT_MCU_NONE                0

// STM32
#define OPT_MCU_STM32F0           300 ///< ST STM32F0
#define OPT_MCU_STM32F1           301 ///< ST STM32F1
#define OPT_MCU_STM32F2           302 ///< ST STM32F2
#define OPT_MCU_STM32F3           303 ///< ST STM32F3
#define OPT_MCU_STM32F4           304 ///< ST STM32F4
#define OPT_MCU_STM32F7           305 ///< ST STM32F7
#define OPT_MCU_STM32H7           306 ///< ST STM32H7
#define OPT_MCU_STM32L0           307 ///< ST STM32L0
#define OPT_MCU_STM32L1           308 ///< ST STM32L1
#define OPT_MCU_STM32L4           309 ///< ST STM32L4

/** @} */

/** \defgroup group_supported_os Supported RTOS
 *  \ref CFG_TUSB_OS must be defined to one of these
 *  @{ */
#define OPT_OS_FREERTOS   2  ///< FreeRTOS
/** @} */


// Allow to use command line to change the config name/location
#ifdef CFG_TUSB_CONFIG_FILE
  #include CFG_TUSB_CONFIG_FILE
#else
  #include "tusb_config.h"
#endif



/** \addtogroup group_configuration
 *  @{ */


//--------------------------------------------------------------------
// RootHub Mode Configuration
// CFG_TUSB_RHPORTx_MODE contains operation mode and speed for that port
//--------------------------------------------------------------------

// Lower 4-bit is operational mode
#define OPT_MODE_NONE         0x00 ///< Disabled
#define OPT_MODE_DEVICE       0x01 ///< Device Mode
#define OPT_MODE_HOST         0x02 ///< Host Mode

// Higher 4-bit is max operational speed (corresponding to tusb_speed_t)
#define OPT_MODE_FULL_SPEED   0x00 ///< Max Full Speed
#define OPT_MODE_LOW_SPEED    0x10 ///< Max Low Speed
#define OPT_MODE_HIGH_SPEED   0x20 ///< Max High Speed


#ifndef CFG_TUSB_RHPORT0_MODE
  #define CFG_TUSB_RHPORT0_MODE OPT_MODE_NONE
#endif


#ifndef CFG_TUSB_RHPORT1_MODE
  #define CFG_TUSB_RHPORT1_MODE OPT_MODE_NONE
#endif

#if (((CFG_TUSB_RHPORT0_MODE) & OPT_MODE_HOST  ) && ((CFG_TUSB_RHPORT1_MODE) & OPT_MODE_HOST  )) || \
    (((CFG_TUSB_RHPORT0_MODE) & OPT_MODE_DEVICE) && ((CFG_TUSB_RHPORT1_MODE) & OPT_MODE_DEVICE))
  #error "TinyUSB currently does not support same modes on more than 1 roothub port"
#endif

// Which roothub port is configured as host
#define TUH_OPT_RHPORT          ( ((CFG_TUSB_RHPORT0_MODE) & OPT_MODE_HOST) ? 0 : (((CFG_TUSB_RHPORT1_MODE) & OPT_MODE_HOST) ? 1 : -1) )
#define TUSB_OPT_HOST_ENABLED   ( TUH_OPT_RHPORT >= 0 )

// Which roothub port is configured as device
#define TUD_OPT_RHPORT          ( ((CFG_TUSB_RHPORT0_MODE) & OPT_MODE_DEVICE) ? 0 : (((CFG_TUSB_RHPORT1_MODE) & OPT_MODE_DEVICE) ? 1 : -1) )

#if TUD_OPT_RHPORT == 0
#define TUD_OPT_HIGH_SPEED      ( (CFG_TUSB_RHPORT0_MODE) & OPT_MODE_HIGH_SPEED )
#else
#define TUD_OPT_HIGH_SPEED      ( (CFG_TUSB_RHPORT1_MODE) & OPT_MODE_HIGH_SPEED )
#endif

#define TUSB_OPT_DEVICE_ENABLED ( TUD_OPT_RHPORT >= 0 )

//--------------------------------------------------------------------+
// COMMON OPTIONS
//--------------------------------------------------------------------+

// Debug enable to print out error message
#ifndef CFG_TUSB_DEBUG
  #define CFG_TUSB_DEBUG 0
#endif

// place data in accessible RAM for usb controller
#ifndef CFG_TUSB_MEM_SECTION
  #define CFG_TUSB_MEM_SECTION
#endif

#ifndef CFG_TUSB_MEM_ALIGN
  #define CFG_TUSB_MEM_ALIGN      TU_ATTR_ALIGNED(4)
#endif

#ifndef CFG_TUSB_OS
  #define CFG_TUSB_OS             OPT_OS_NONE
#endif

//--------------------------------------------------------------------
// DEVICE OPTIONS
//--------------------------------------------------------------------

#ifndef CFG_TUD_ENDPOINT0_SIZE
  #define CFG_TUD_ENDPOINT0_SIZE  64
#endif

#ifndef CFG_TUD_CDC
  #define CFG_TUD_CDC             0
#endif

#ifndef CFG_TUD_MSC
  #define CFG_TUD_MSC             0
#endif

#ifndef CFG_TUD_HID
  #define CFG_TUD_HID             0
#endif

#ifndef CFG_TUD_AUDIO
  #define CFG_TUD_AUDIO           0
#endif

#ifndef CFG_TUD_MIDI
  #define CFG_TUD_MIDI            0
#endif

#ifndef CFG_TUD_VENDOR
  #define CFG_TUD_VENDOR          0
#endif

#ifndef CFG_TUD_USBTMC
  #define CFG_TUD_USBTMC          0
#endif

#ifndef CFG_TUD_DFU_RUNTIME
  #define CFG_TUD_DFU_RUNTIME          0
#endif

#ifndef CFG_TUD_NET
  #define CFG_TUD_NET             0
#endif

#ifndef CFG_TUD_BTH
  #define CFG_TUD_BTH             0
#endif

//--------------------------------------------------------------------
// HOST OPTIONS
//--------------------------------------------------------------------
#if TUSB_OPT_HOST_ENABLED
  #ifndef CFG_TUSB_HOST_DEVICE_MAX
    #define CFG_TUSB_HOST_DEVICE_MAX 1
    #warning CFG_TUSB_HOST_DEVICE_MAX is not defined, default value is 1
  #endif

  //------------- HUB CLASS -------------//
  #if CFG_TUH_HUB && (CFG_TUSB_HOST_DEVICE_MAX == 1)
    #error there is no benefit enable hub with max device is 1. Please disable hub or increase CFG_TUSB_HOST_DEVICE_MAX
  #endif

  //------------- HID CLASS -------------//
  #define HOST_CLASS_HID   ( CFG_TUH_HID_KEYBOARD + CFG_TUH_HID_MOUSE + CFG_TUSB_HOST_HID_GENERIC )

  #ifndef CFG_TUSB_HOST_ENUM_BUFFER_SIZE
    #define CFG_TUSB_HOST_ENUM_BUFFER_SIZE 256
  #endif

  //------------- CLASS -------------//
#endif // TUSB_OPT_HOST_ENABLED


//------------------------------------------------------------------
// Configuration Validation
//------------------------------------------------------------------
#if CFG_TUD_ENDPOINT0_SIZE > 64
  #error Control Endpoint Max Packet Size cannot be larger than 64
#endif

#endif /* _TUSB_OPTION_H_ */

/** @} */
