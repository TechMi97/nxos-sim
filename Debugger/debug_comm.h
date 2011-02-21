/** @file debug_comm.h
 *  @brief Shared C/ASM header file for debugger communications
 *
 */

/* Copyright (C) 2007-2010 the NxOS developers
 *
 * Module Developed by: TC Wan <tcwan@cs.usm.my>
 *
 * See AUTHORS for a full list of the developers.
 *
 * See COPYING for redistribution license
 *
 */

#ifndef __DEBUG_COMM_H__
#define __DEBUG_COMM_H__

#include "_c_arm_macros.h"

/* This is a place holder header file to allow for interfacing with C Routines in either
 * NxOS or NXT Firmware.
 *
 * Since the header files from the original source trees were meant for C programs, we can't
 * include them directly. Here we just use .extern to reference the routines.
 */

#ifdef __NXOS__

  .extern		nx_usb_is_connected
  .extern       nx_usb_can_write
  .extern       nx_usb_write
  .extern       nx_usb_data_written
  .extern       nx_usb_read
  .extern       nx_usb_data_read

#else           /* NXT Firmware */

  .extern      cCommInit
  .extern      cCommCtrl
  .extern      cCommExit
  .extern      dUsbWrite
  .extern      dUsbRead
  .extern      dUsbIsConfigured
  .equ         nxt_UBYTE_TRUE, 1
  .equ         nxt_UBYTE_FALSE, 0

#endif

#endif
