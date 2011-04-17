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
 * RDMHandlers.cpp
 * Copyright (C) 2011 Simon Newton
 */

#include "Common.h"
#include "RDMEnums.h"
#include "RDMHandlers.h"
#include "RDMSender.h"
#include "WidgetSettings.h"

// GET Handlers
void HandleGetSupportedParameters(const byte *received_message);
void HandleGetParameterDescription(const byte *received_message);
void HandleGetDeviceInfo(const byte *received_message);
void HandleGetProductDetailId(const byte *received_message);
void HandleGetDeviceModelDescription(const byte *received_message);
void HandleGetManufacturerLabel(const byte *received_message);
void HandleGetDeviceLabel(const byte *received_message);
void HandleGetLanguage(const byte *received_message);
void HandleGetSoftwareVersion(const byte *received_message);
void HandleGetStartAddress(const byte *received_message);
void HandleGetSensorDefinition(const byte *received_message);
void HandleGetSensorValue(const byte *received_message);
void HandleGetDevicePowerCycles(const byte *received_message);
void HandleGetIdentifyDevice(const byte *received_message);

// SET Handlers
void HandleSetLanguage(bool was_broadcast, int sub_device,
                       const byte *received_message);
void HandleSetDeviceLabel(bool was_broadcast, int sub_device,
                          const byte *received_message);
void HandleSetStartAddress(bool was_broadcast,
                           int sub_device,
                           const byte *received_message);
void HandleSetSensorValue(bool was_broadcast, int sub_device,
                          const byte *received_message);
void HandleRecordSensor(bool was_broadcast, int sub_device,
                        const byte *received_message);
void HandleSetIdentifyDevice(bool was_broadcast, int sub_device,
                             const byte *received_message);
void HandleSetSerial(bool was_broadcast, int sub_device,
                     const byte *received_message);

// The definition for a PID, this includes which functions to call to handle
// GET/SET requests and if we should include this PID in the list of
// supported parameters.
typedef struct {
  unsigned int pid;
  void (*get_handler)(const byte *message);
  void (*set_handler)(bool was_broadcast, int sub_device,
                      const byte *received_message);
  byte get_argument_size;
  bool include_in_supported_params;
} pid_definition;


// The list of all pids that we support
pid_definition PID_DEFINITIONS[] = {
  {PID_SUPPORTED_PARAMETERS, HandleGetSupportedParameters, NULL, 0, false},
  {PID_PARAMETER_DESCRIPTION, HandleGetParameterDescription, NULL, 2, false},
  {PID_DEVICE_INFO, HandleGetDeviceInfo, NULL, 0, false},
  {PID_PRODUCT_DETAIL_ID_LIST, HandleGetProductDetailId, NULL, 0, true},
  {PID_DEVICE_MODEL_DESCRIPTION, HandleGetDeviceModelDescription, NULL, 0,
   true},
  {PID_MANUFACTURER_LABEL, HandleGetManufacturerLabel, NULL, 0, true},
  {PID_DEVICE_LABEL, HandleGetDeviceLabel, HandleSetDeviceLabel, 0, true},
  {PID_LANGUAGE_CAPABILITIES, HandleGetLanguage, NULL, 0, true},
  {PID_LANGUAGE, HandleGetLanguage, HandleSetLanguage, 0, true},
  {PID_SOFTWARE_VERSION_LABEL, HandleGetSoftwareVersion, NULL, 0, false},
  {PID_DMX_START_ADDRESS, HandleGetStartAddress, HandleSetStartAddress, 0,
   false},
  {PID_SENSOR_DEFINITION, HandleGetSensorDefinition, NULL, 1, true},
  {PID_SENSOR_VALUE, HandleGetSensorValue, HandleSetSensorValue, 1, true},
  {PID_RECORD_SENSORS, NULL, HandleRecordSensor, 0, true},
  {PID_DEVICE_POWER_CYCLES, HandleGetDevicePowerCycles, NULL, 0, true},
  {PID_IDENTIFY_DEVICE, HandleGetIdentifyDevice, HandleSetIdentifyDevice,
   0, false},
  {PID_MANUFACTURER_SET_SERIAL, NULL, HandleSetSerial, 4, true},
};


// Pin constants
const byte IDENTIFY_LED_PIN = 12;
const byte TEMP_SENSOR_PIN = 0;

// Various constants used in RDM messages
char SUPPORTED_LANGUAGE[] = "en";
unsigned long SOFTWARE_VERSION = 1;
char SOFTWARE_VERSION_STRING[] = "1.0";
const int MAX_DMX_ADDRESS = 512;
enum { MAX_LABEL_SIZE = 32 };
const char SET_SERIAL_PID_DESCRIPTION[] = "Set Serial Number";
const char TEMPERATURE_SENSOR_DESCRIPTION[] = "Case Temperature";

// The identify state
bool identify_mode_enabled = false;

// global objects
RDMSender rdm_sender(&sender);


/**
 * Initialize the I/O pins used as part of the RDM responder.
 */
void SetupRDMHandling() {
  pinMode(IDENTIFY_LED_PIN, OUTPUT);
  digitalWrite(IDENTIFY_LED_PIN, identify_mode_enabled);
}


/**
 * Verify a RDM checksum
 * @param message a pointer to an RDM message starting with the SUB_START_CODE
 * @param size the size of the message data
 * @return true if the checksum is ok, false otherwise
 */
bool VerifyChecksum(const byte *message, int size) {
  // don't checksum the checksum itself (last two bytes)
  unsigned int checksum = 0;
  for (int i = 0; i < size - 2; i++)
    checksum += message[i];

  byte checksum_offset = message[2];
  return (checksum >> 8 == message[checksum_offset] &&
          (checksum & 0xff) == message[checksum_offset + 1]);
}


/**
 * Read the value of the temperature sensor.
 * @return the temp in degrees C * 10
 */
int ReadTemperatureSensor() {
  // v = input / 1024 * 5 V
  // t = 100 * v
  // we multiple the result by 10
  return 10 * 5.0 * analogRead(TEMP_SENSOR_PIN) * 100.0 / 1024.0;
}


/**
 * Send a sensor response, this is used for both PID_SENSOR_VALUE &
 * PID_RECORD_SENSORS.
 */
void SendSensorResponse(const byte *received_message) {
  rdm_sender.StartRDMAckResponse(received_message, 9);
  rdm_sender.SendByteAndChecksum(received_message[24]);
  rdm_sender.SendIntAndChecksum(ReadTemperatureSensor());  // current
  rdm_sender.SendIntAndChecksum(0);  // lowest
  rdm_sender.SendIntAndChecksum(0);  // highest
  rdm_sender.SendIntAndChecksum(WidgetSettings.SensorValue());  // recorded
  rdm_sender.EndRDMResponse();
}


/**
 * Send a RDM message with a string as param data. Used for DEVICE_LABEL,
 * MANUFACTURER_LABEL, etc.
 */
void HandleStringRequest(const byte *received_message,
                         char *label,
                         byte label_size) {
  rdm_sender.StartRDMResponse(received_message, RDM_RESPONSE_ACK, label_size);
  for (unsigned int i = 0; i < label_size; ++i)
    rdm_sender.SendByteAndChecksum(label[i]);
  rdm_sender.EndRDMResponse();
}


/**
 * Handle a GET SUPPORTED_PARAMETERS request
 */
void HandleGetSupportedParameters(const byte *received_message) {
  byte supported_params = 0;
  for (byte i = 0; i < sizeof(PID_DEFINITIONS) / sizeof(pid_definition); ++i) {
    if (PID_DEFINITIONS[i].include_in_supported_params)
      supported_params++;
  }

  rdm_sender.StartRDMAckResponse(received_message, supported_params * 2);
  for (byte i = 0; i < sizeof(PID_DEFINITIONS) / sizeof(pid_definition); ++i) {
    if (PID_DEFINITIONS[i].include_in_supported_params)
      rdm_sender.SendIntAndChecksum(PID_DEFINITIONS[i].pid);
  }
  rdm_sender.EndRDMResponse();
}


/**
 * Handle a GET PARAMETER_DESCRIPTION request
 */
void HandleGetParameterDescription(const byte *received_message) {
  unsigned int param_id = (((unsigned int) received_message[24] << 8) +
                           received_message[25]);

  if (param_id != 0x8000) {
    rdm_sender.SendNack(received_message, NR_DATA_OUT_OF_RANGE);
    return;
  }

  rdm_sender.StartRDMAckResponse(received_message,
                                 20 + sizeof(SET_SERIAL_PID_DESCRIPTION) - 1);
  rdm_sender.SendIntAndChecksum(0x8000);
  rdm_sender.SendByteAndChecksum(4);  // pdl size
  rdm_sender.SendByteAndChecksum(0x03);  // data type, uint8
  rdm_sender.SendByteAndChecksum(0x02);  // command class, set only
  rdm_sender.SendByteAndChecksum(0);  // type
  rdm_sender.SendByteAndChecksum(0);  // unit, none
  rdm_sender.SendByteAndChecksum(0);  // prefix, none
  rdm_sender.SendLongAndChecksum(0);  // min
  rdm_sender.SendLongAndChecksum(0xfffffffe);  // max
  rdm_sender.SendLongAndChecksum(1);  // default

  for (unsigned int i = 0; i < sizeof(SET_SERIAL_PID_DESCRIPTION) - 1; ++i)
    rdm_sender.SendByteAndChecksum(SET_SERIAL_PID_DESCRIPTION[i]);
  rdm_sender.EndRDMResponse();
}


/**
 * Handle a GET DEVICE_INFO request
 */
void HandleGetDeviceInfo(const byte *received_message) {
  rdm_sender.StartRDMAckResponse(received_message, 19);
  rdm_sender.SendIntAndChecksum(256);  // protocol version
  rdm_sender.SendIntAndChecksum(2);  // device model
  rdm_sender.SendIntAndChecksum(0x0508);  // product category
  rdm_sender.SendLongAndChecksum(SOFTWARE_VERSION);  // software version
  rdm_sender.SendIntAndChecksum(3);  // DMX footprint
  rdm_sender.SendIntAndChecksum(0x0101);  // DMX Personality
  // DMX Start Address
  rdm_sender.SendIntAndChecksum(WidgetSettings.StartAddress());
  rdm_sender.SendIntAndChecksum(0);  // Sub device count
  rdm_sender.SendByteAndChecksum(1);  // Sensor Count
  rdm_sender.EndRDMResponse();
}


/**
 * Handle a GET PRODUCT_DETAIL_ID request
 */
void HandleGetProductDetailId(const byte *received_message) {
  rdm_sender.StartRDMAckResponse(received_message, 2);
  rdm_sender.SendIntAndChecksum(0x0403);  // PWM dimmer
  rdm_sender.EndRDMResponse();
}


/**
 * Handle a GET PID_DEVICE_MODEL_DESCRIPTION request
 */
void HandleGetDeviceModelDescription(const byte *received_message) {
  HandleStringRequest(received_message, DEVICE_NAME, DEVICE_NAME_SIZE);
}


/**
 * Handle a GET MANUFACTURER_NAME request
 */
void HandleGetManufacturerLabel(const byte *received_message) {
  HandleStringRequest(received_message, MANUFACTURER_NAME,
                      MANUFACTURER_NAME_SIZE);
}


/**
 * Handle a GET DEVICE_LABEL request
 */
void HandleGetDeviceLabel(const byte *received_message) {
  char device_label[MAX_LABEL_SIZE];
  byte size = WidgetSettings.DeviceLabel(device_label, sizeof(device_label));
  HandleStringRequest(received_message, device_label, size);
}


/**
 * Handle a GET LANGUAGE / LANGUAGE_CAPABILITIES request
 */
void HandleGetLanguage(const byte *received_message) {
  HandleStringRequest(received_message,
                      SUPPORTED_LANGUAGE,
                      sizeof(SUPPORTED_LANGUAGE) - 1);
}


/**
 * Handle a GET SOFTWARE_VERSION_LABEL request
 */
void HandleGetSoftwareVersion(const byte *received_message) {
  rdm_sender.StartRDMAckResponse(received_message,
                                 sizeof(SOFTWARE_VERSION_STRING));
  for (unsigned int i = 0; i < sizeof(SOFTWARE_VERSION_STRING); ++i)
    rdm_sender.SendByteAndChecksum(SOFTWARE_VERSION_STRING[i]);
  rdm_sender.EndRDMResponse();
}


/**
 * Handle a GET DMX_START_ADDRESS request
 */
void HandleGetStartAddress(const byte *received_message) {
  int start_address = WidgetSettings.StartAddress();
  rdm_sender.StartRDMAckResponse(received_message, sizeof(start_address));
  rdm_sender.SendIntAndChecksum(start_address);
  rdm_sender.EndRDMResponse();
}


/**
 * Handle a GET SENSOR_DEFINITION request
 */
void HandleGetSensorDefinition(const byte *received_message) {
  if (received_message[24]) {
    rdm_sender.SendNack(received_message, NR_DATA_OUT_OF_RANGE);
    return;
  }

  rdm_sender.StartRDMAckResponse(
      received_message,
      13 + sizeof(TEMPERATURE_SENSOR_DESCRIPTION) - 1);
  rdm_sender.SendByteAndChecksum(received_message[24]);
  rdm_sender.SendByteAndChecksum(0x00);  // type: temperature
  rdm_sender.SendByteAndChecksum(1);  // unit: C
  rdm_sender.SendByteAndChecksum(1);  // prefix: deci
  rdm_sender.SendIntAndChecksum(0);  // range min
  rdm_sender.SendIntAndChecksum(1500);  // range max
  rdm_sender.SendIntAndChecksum(100);  // normal min
  rdm_sender.SendIntAndChecksum(400);  // normal max
  rdm_sender.SendByteAndChecksum(1);  // recorded value support
  for (unsigned int i = 0; i < sizeof(TEMPERATURE_SENSOR_DESCRIPTION) - 1; ++i)
    rdm_sender.SendByteAndChecksum(TEMPERATURE_SENSOR_DESCRIPTION[i]);
  rdm_sender.EndRDMResponse();
}


/**
 * Handle a GET SENSOR_VALUE request
 */
void HandleGetSensorValue(const byte *received_message) {
  if (received_message[24]) {
    rdm_sender.SendNack(received_message, NR_DATA_OUT_OF_RANGE);
    return;
  }

  SendSensorResponse(received_message);
}


/**
 * Handle a GET DEVICE_POWER_CYCLES request
 */
void HandleGetDevicePowerCycles(const byte *received_message) {
  unsigned long power_cycles = WidgetSettings.DevicePowerCycles();
  rdm_sender.StartRDMAckResponse(received_message, sizeof(power_cycles));
  rdm_sender.SendLongAndChecksum(power_cycles);
  rdm_sender.EndRDMResponse();
}


/**
 * Handle a GET IDENTIFY_DEVICE request
 */
void HandleGetIdentifyDevice(const byte *received_message) {
  rdm_sender.StartRDMAckResponse(received_message, 1);
  rdm_sender.SendByteAndChecksum(identify_mode_enabled);
  rdm_sender.EndRDMResponse();
}


/**
 * Handle a SET DMX_START_ADDRESS request
 */
void HandleSetLanguage(bool was_broadcast,
                       int sub_device,
                       const byte *received_message) {
  // check for invalid size or value
  if (received_message[23] != 2) {
    rdm_sender.NackOrBroadcast(was_broadcast,
                               received_message,
                               NR_FORMAT_ERROR);
    return;
  }

  bool ok = true;
  for (byte i = 0; i < sizeof(SUPPORTED_LANGUAGE) - 1; ++i) {
    ok &= SUPPORTED_LANGUAGE[i] == received_message[24 + i];
  }

  if (!ok) {
    rdm_sender.NackOrBroadcast(was_broadcast,
                               received_message,
                               NR_DATA_OUT_OF_RANGE);
    return;
  }

  if (was_broadcast) {
    rdm_sender.ReturnRDMErrorResponse(RDM_STATUS_BROADCAST);
  } else {
    rdm_sender.SendEmptyAck(received_message);
  }
}


/**
 * Handle a SET DMX_START_ADDRESS request
 */
void HandleSetDeviceLabel(bool was_broadcast,
                          int sub_device,
                          const byte *received_message) {
  // check for invalid size or value
  if (received_message[23] > MAX_LABEL_SIZE) {
    rdm_sender.NackOrBroadcast(was_broadcast,
                               received_message,
                               NR_FORMAT_ERROR);
    return;
  }

  WidgetSettings.SetDeviceLabel((char*) received_message + 24,
                                received_message[23]);

  if (was_broadcast) {
    rdm_sender.ReturnRDMErrorResponse(RDM_STATUS_BROADCAST);
  } else {
    rdm_sender.SendEmptyAck(received_message);
  }
}


/**
 * Handle a SET DMX_START_ADDRESS request
 */
void HandleSetStartAddress(bool was_broadcast,
                           int sub_device,
                           const byte *received_message) {
  // check for invalid size or value
  if (received_message[23] != 2) {
    rdm_sender.NackOrBroadcast(was_broadcast,
                               received_message,
                               NR_FORMAT_ERROR);
    return;
  }

  int new_start_address = (((int) received_message[24] << 8) +
                           received_message[25]);

  if (new_start_address == 0 || new_start_address > MAX_DMX_ADDRESS) {
    rdm_sender.NackOrBroadcast(was_broadcast,
                               received_message,
                               NR_DATA_OUT_OF_RANGE);
    return;
  }

  WidgetSettings.SetStartAddress(new_start_address);

  if (was_broadcast) {
    rdm_sender.ReturnRDMErrorResponse(RDM_STATUS_BROADCAST);
  } else {
    rdm_sender.SendEmptyAck(received_message);
  }
}


/**
 * Handle a SET SENSOR_VALUE request
 */
void HandleSetSensorValue(bool was_broadcast,
                          int sub_device,
                          const byte *received_message) {
  // check for invalid size or value
  if (received_message[23] != 1) {
    rdm_sender.NackOrBroadcast(was_broadcast,
                               received_message,
                               NR_FORMAT_ERROR);
    return;
  }

  if (received_message[24] && received_message[24] != 0xff) {
    rdm_sender.SendNack(received_message, NR_DATA_OUT_OF_RANGE);
    return;
  }

  WidgetSettings.SaveSensorValue(0);
  SendSensorResponse(received_message);
}


/*
 * Handle a SET RECORD_SENSORS request
 */
void HandleRecordSensor(bool was_broadcast,
                        int sub_device,
                        const byte *received_message) {
  // check for invalid size or value
  if (received_message[23] != 1) {
    rdm_sender.NackOrBroadcast(was_broadcast,
                               received_message,
                               NR_FORMAT_ERROR);
    return;
  }

  if (received_message[24] && received_message[24] != 0xff) {
    rdm_sender.NackOrBroadcast(was_broadcast, received_message,
                               NR_DATA_OUT_OF_RANGE);
    return;
  }

  WidgetSettings.SaveSensorValue(ReadTemperatureSensor());

  if (was_broadcast) {
    rdm_sender.ReturnRDMErrorResponse(RDM_STATUS_BROADCAST);
  } else {
    rdm_sender.SendEmptyAck(received_message);
  }
}


/**
 * Handle a SET IDENTIFY_DEVICE request
 */
void HandleSetIdentifyDevice(bool was_broadcast,
                             int sub_device,
                             const byte *received_message) {
  // check for invalid size or value
  if (received_message[23] != 1 ||
      (received_message[24] != 0 && received_message[24] != 1)) {
    rdm_sender.NackOrBroadcast(was_broadcast,
                               received_message,
                               NR_FORMAT_ERROR);
    return;
  }

  identify_mode_enabled = received_message[24];
  digitalWrite(IDENTIFY_LED_PIN, identify_mode_enabled);

  if (was_broadcast) {
    rdm_sender.ReturnRDMErrorResponse(RDM_STATUS_BROADCAST);
  } else {
    rdm_sender.SendEmptyAck(received_message);
  }
}


/**
 * Handle a SET SERIAL_NUMBER request
 */
void HandleSetSerial(bool was_broadcast,
                     int sub_device,
                     const byte *received_message) {
  if (received_message[23] != 4) {
    rdm_sender.NackOrBroadcast(was_broadcast,
                               received_message,
                               NR_FORMAT_ERROR);
    return;
  }

  unsigned long new_serial_number = 0;
  for (byte i = 0; i < 4; ++i) {
    new_serial_number = new_serial_number << 8;
    new_serial_number += received_message[24 + i];
  }

  if (new_serial_number == 0xffffffff) {
    rdm_sender.NackOrBroadcast(was_broadcast,
                               received_message,
                               NR_DATA_OUT_OF_RANGE);
    return;
  }

  WidgetSettings.SetSerialNumber(new_serial_number);

  if (was_broadcast) {
    rdm_sender.ReturnRDMErrorResponse(RDM_STATUS_BROADCAST);
  } else {
    rdm_sender.SendEmptyAck(received_message);
  }
}


/*
 * Handle an RDM message
 * @param message pointer to a RDM message where the first byte is the sub star
 * code.
 * @param size the size of the message data.
 */
void HandleRDMMessage(const byte *message, int size) {
  // check for a packet that is too small, an invalid start / sub start code
  // or a mismatched message length.
  if (size < MINIMUM_RDM_PACKET_SIZE || message[0] != START_CODE ||
      message[1] != SUB_START_CODE || message[2] != size - 2) {
    rdm_sender.ReturnRDMErrorResponse(RDM_STATUS_FAILED);
    return;
  }

  if (!VerifyChecksum(message, size)) {
    rdm_sender.ReturnRDMErrorResponse(RDM_STATUS_FAILED_CHECKSUM);
    return;
  }

  // true if this is broadcast or vendorcast, in which case we don't return a
  // RDM message
  bool is_broadcast = true;
  for (int i = 5; i <= 8; ++i) {
    is_broadcast &= (message[i] == 0xff);
  }

  int expected_esta_id = message[3];
  expected_esta_id  = expected_esta_id << 8;
  expected_esta_id += message[4];

  bool to_us = is_broadcast || (
    WidgetSettings.MatchesEstaId(message + 3) &&
    WidgetSettings.MatchesSerialNumber(message + 5));

  if (!to_us) {
    rdm_sender.ReturnRDMErrorResponse(RDM_STATUS_INVALID_DESTINATION);
    return;
  }

  // check the command class
  byte command_class = message[20];
  if (command_class != GET_COMMAND && command_class != SET_COMMAND) {
    rdm_sender.ReturnRDMErrorResponse(RDM_STATUS_INVALID_COMMAND);
  }

  // check sub devices
  unsigned int sub_device = (message[18] << 8) + message[19];
  if (sub_device != 0 && sub_device != 0xffff) {
    // respond with nack
    rdm_sender.NackOrBroadcast(is_broadcast,
                               message,
                               NR_SUB_DEVICE_OUT_OF_RANGE);
    return;
  }

  unsigned int param_id = (message[21] << 8) + message[22];

  pid_definition *pid_handler = NULL;
  for (byte i = 0; i < sizeof(PID_DEFINITIONS) / sizeof(pid_definition); ++i) {
    if (PID_DEFINITIONS[i].pid == param_id)
      pid_handler = &PID_DEFINITIONS[i];
  }

  if (!pid_handler) {
    rdm_sender.NackOrBroadcast(is_broadcast, message, NR_UNKNOWN_PID);
    return;
  }

  if (command_class == GET_COMMAND) {
    if (!pid_handler->get_handler) {
      rdm_sender.NackOrBroadcast(is_broadcast, message,
                                 NR_UNSUPPORTED_COMMAND_CLASS);
      return;
    }

    if (is_broadcast) {
      rdm_sender.ReturnRDMErrorResponse(RDM_STATUS_BROADCAST);
      return;
    }

    if (sub_device) {
      rdm_sender.SendNack(message, NR_SUB_DEVICE_OUT_OF_RANGE);
      return;
    }

    if (message[23] != pid_handler->get_argument_size) {
      rdm_sender.SendNack(message, NR_FORMAT_ERROR);
      return;
    }

    pid_handler->get_handler(message);

  } else  {
    if (!pid_handler->set_handler) {
      rdm_sender.NackOrBroadcast(is_broadcast, message,
                                 NR_UNSUPPORTED_COMMAND_CLASS);
      return;
    }

    pid_handler->set_handler(is_broadcast, sub_device, message);
  }
}
