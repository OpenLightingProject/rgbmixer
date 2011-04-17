/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * RDMEnums.h
 * Copyright (C) 2011 Simon Newton
 * Various static RDM values.
 */

#include "WProgram.h"

#ifndef RDMENUMS_H
#define RDMENUMS_H


// RDM Status Codes. These are used to communicate the state of a request back
// to the host.
typedef enum {
  RDM_STATUS_OK = 0,
  RDM_STATUS_BROADCAST = 1,
  RDM_STATUS_FAILED = 2,
  RDM_STATUS_FAILED_CHECKSUM = 3,
  RDM_STATUS_INVALID_DESTINATION = 4,
  RDM_STATUS_INVALID_COMMAND = 5,
} rdm_status_codes;

// Various RDM Constants
const byte START_CODE = 0xcc;
const byte SUB_START_CODE = 0x01;
// min packet size including the checksum
const byte MINIMUM_RDM_PACKET_SIZE = 26;

typedef enum {
  GET_COMMAND = 0x20,
  GET_COMMAND_RESPONSE = 0x21,
  SET_COMMAND = 0x30,
  SET_COMMAND_RESPONSE = 0x31,
} rdm_command_class;

typedef enum {
  RDM_RESPONSE_ACK = 0,
  // RDM_RESPONSE_ACK_TIMER = 1,
  RDM_RESPONSE_NACK = 2,
  // RDM_RESONSE_ACK_OVERFLOW = 3,
} rdm_response_type;

typedef enum {
  NR_UNKNOWN_PID = 0x0000,
  NR_FORMAT_ERROR = 0x0001,
  // NR_HARDWARE_FAULT = 0x0002,
  // NR_PROXY_REJECT = 0x0003,
  // NR_WRITE_PROTECT = 0x0004,
  NR_UNSUPPORTED_COMMAND_CLASS = 0x0005,
  NR_DATA_OUT_OF_RANGE = 0x0006,
  // NR_BUFFER_FULL = 0x0007,
  // NR_PACKET_SIZE_UNSUPPORTED = 0x0008,
  NR_SUB_DEVICE_OUT_OF_RANGE = 0x0009,
  // NR_PROXY_BUFFER_FULL = 0x000A,
} rdm_nack_reason;


typedef enum {
  /*
  PID_QUEUED_MESSAGE = 0x0020,
  PID_STATUS_MESSAGES = 0x0030,
  PID_STATUS_ID_DESCRIPTION = 0x0031,
  PID_CLEAR_STATUS_ID = 0x0032,
  PID_SUB_DEVICE_STATUS_REPORT_THRESHOLD = 0x0033,
  */
  // RDM information
  PID_SUPPORTED_PARAMETERS = 0x0050,
  PID_PARAMETER_DESCRIPTION = 0x0051,
  // production information
  PID_DEVICE_INFO = 0x0060,
  PID_PRODUCT_DETAIL_ID_LIST = 0x0070,
  PID_DEVICE_MODEL_DESCRIPTION = 0x0080,
  PID_MANUFACTURER_LABEL = 0x0081,
  PID_DEVICE_LABEL = 0x0082,
  /*
  PID_FACTORY_DEFAULTS = 0x0090,
  */
  PID_LANGUAGE_CAPABILITIES = 0x00A0,
  PID_LANGUAGE = 0x00B0,
  PID_SOFTWARE_VERSION_LABEL = 0x00C0,
  /*
  PID_BOOT_SOFTWARE_VERSION_ID = 0x00C1,
  PID_BOOT_SOFTWARE_VERSION_LABEL = 0x00C2,
  // dmx512
  PID_DMX_PERSONALITY = 0x00E0,
  PID_DMX_PERSONALITY_DESCRIPTION = 0x00E1,
  */
  PID_DMX_START_ADDRESS = 0x00F0,
  /*
  PID_SLOT_INFO = 0x0120,
  PID_SLOT_DESCRIPTION = 0x0121,
  PID_DEFAULT_SLOT_VALUE = 0x0122,
  */
  // sensors
  PID_SENSOR_DEFINITION = 0x0200,
  PID_SENSOR_VALUE = 0x0201,
  PID_RECORD_SENSORS = 0x0202,
  // power/lamp settings
  /*
  PID_DEVICE_HOURS = 0x0400,
  PID_LAMP_HOURS = 0x0401,
  PID_LAMP_STRIKES = 0x0402,
  PID_LAMP_STATE =  0x0403,
  PID_LAMP_ON_MODE = 0x0404,
  */
  PID_DEVICE_POWER_CYCLES = 0x0405,
  /*
  // display settings
  PID_DISPLAY_INVERT = 0x0500,
  PID_DISPLAY_LEVEL = 0x0501,
  // configuration
  PID_PAN_INVERT = 0x0600,
  PID_TILT_INVERT = 0x0601,
  PID_PAN_TILT_SWAP = 0x0602,
  PID_REAL_TIME_CLOCK = 0x0603,
  */
  // control
  PID_IDENTIFY_DEVICE = 0x1000,
  /*
  PID_RESET_DEVICE = 0x1001,
  PID_POWER_STATE = 0x1010,
  PID_PERFORM_SELFTEST = 0x1020,
  PID_SELF_TEST_DESCRIPTION = 0x1021,
  PID_CAPTURE_PRESET = 0x1030,
  PID_PRESET_PLAYBACK = 0x1031,
  */

  // Manufacturer PID follow
  PID_MANUFACTURER_SET_SERIAL = 0x8000,
} rdm_pid;

#endif  // RDMENUMS_H
