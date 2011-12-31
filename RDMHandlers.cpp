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


// The list of all pids that we support
const RDMHandler::pid_definition RDMHandler::PID_DEFINITIONS[] = {
  {PID_QUEUED_MESSAGE, &RDMHandler::HandleGetQueuedMessage, NULL, 1, true},
  {PID_SUPPORTED_PARAMETERS, &RDMHandler::HandleGetSupportedParameters, NULL,
    0, false},
  {PID_PARAMETER_DESCRIPTION, &RDMHandler::HandleGetParameterDescription, NULL,
    2, false},
  {PID_DEVICE_INFO, &RDMHandler::HandleGetDeviceInfo, NULL, 0, false},
  {PID_PRODUCT_DETAIL_ID_LIST, &RDMHandler::HandleGetProductDetailId, NULL, 0,
    true},
  {PID_DEVICE_MODEL_DESCRIPTION, &RDMHandler::HandleGetDeviceModelDescription,
    NULL, 0, true},
  {PID_MANUFACTURER_LABEL, &RDMHandler::HandleGetManufacturerLabel, NULL, 0,
    true},
  {PID_DEVICE_LABEL, &RDMHandler::HandleGetDeviceLabel,
    &RDMHandler::HandleSetDeviceLabel, 0, true},
  {PID_LANGUAGE_CAPABILITIES, &RDMHandler::HandleGetLanguage, NULL, 0, true},
  {PID_LANGUAGE, &RDMHandler::HandleGetLanguage,
    &RDMHandler::HandleSetLanguage, 0, true},
  {PID_SOFTWARE_VERSION_LABEL, &RDMHandler::HandleGetSoftwareVersion, NULL, 0,
    false},
  {PID_DMX_PERSONALITY, &RDMHandler::HandleGetPersonality,
    &RDMHandler::HandleSetPersonality, 0, true},
  {PID_DMX_PERSONALITY_DESCRIPTION,
    &RDMHandler::HandleGetPersonalityDescription, NULL, 1, true},
  {PID_DMX_START_ADDRESS, &RDMHandler::HandleGetStartAddress,
    &RDMHandler::HandleSetStartAddress, 0, false},
  {PID_SENSOR_DEFINITION, &RDMHandler::HandleGetSensorDefinition, NULL, 1,
    true},
  {PID_SENSOR_VALUE, &RDMHandler::HandleGetSensorValue,
    &RDMHandler::HandleSetSensorValue, 1, true},
  {PID_RECORD_SENSORS, NULL, &RDMHandler::HandleRecordSensor, 0, true},
  {PID_DEVICE_POWER_CYCLES, &RDMHandler::HandleGetDevicePowerCycles,
    &RDMHandler::HandleSetDevicePowerCycles, 0, true},
  {PID_IDENTIFY_DEVICE, &RDMHandler::HandleGetIdentifyDevice,
    &RDMHandler::HandleSetIdentifyDevice, 0, false},
  {PID_MANUFACTURER_SET_SERIAL, NULL, &RDMHandler::HandleSetSerial, 4, true},
};


const RDMHandler::rdm_personality RDMHandler::rdm_personalities[] = {
  {1, 6, "6x PWM"},
  {2, 6, "3x inverted PWM, 3x PWM"},
  {3, 6, "6x inverted PWM"},
};

// Various constants used in RDM messages
const char RDMHandler::SUPPORTED_LANGUAGE[] = "en";
const char RDMHandler::SOFTWARE_VERSION_STRING[] = "1.0";
const char RDMHandler::SET_SERIAL_PID_DESCRIPTION[] = "Set Serial Number";
const char RDMHandler::TEMPERATURE_SENSOR_DESCRIPTION[] = "Case Temperature";


/**
 * Verify a RDM checksum
 * @param message a pointer to an RDM message starting with the SUB_START_CODE
 * @param size the size of the message data
 * @return true if the checksum is ok, false otherwise
 */
bool RDMHandler::VerifyChecksum(const byte *message, int size) {
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
int RDMHandler::ReadTemperatureSensor() {
  // v = input / 1024 * 5 V
  // t = 100 * v
  // we multiple the result by 10
  return 10 * 5.0 * analogRead(TEMP_SENSOR_PIN) * 100.0 / 1024.0;
}


/**
 * Send a sensor response, this is used for both PID_SENSOR_VALUE &
 * PID_RECORD_SENSORS.
 */
void RDMHandler::SendSensorResponse(const byte *received_message) {
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
void RDMHandler::HandleStringRequest(const byte *received_message,
                                     const char *label,
                                     byte label_size) {
  rdm_sender.StartRDMResponse(received_message, RDM_RESPONSE_ACK, label_size);
  for (unsigned int i = 0; i < label_size; ++i)
    rdm_sender.SendByteAndChecksum(label[i]);
  rdm_sender.EndRDMResponse();
}


/**
 * Handle a GET QUEUED_MESSAGE request
 */
void RDMHandler::HandleGetQueuedMessage(const byte *received_message) {
  if (m_device_label_pending) {
    rdm_sender.DecrementMessageCount();
    m_device_label_pending = false;
    m_sent_device_label = true;
    rdm_sender.StartCustomResponse(received_message, RDM_RESPONSE_ACK,
        0, SET_COMMAND_RESPONSE, PID_DEVICE_LABEL);
  } else if (m_sent_device_label && received_message[24] ==
             STATUS_GET_LAST_MESSAGE) {
    rdm_sender.StartCustomResponse(received_message, RDM_RESPONSE_ACK,
        0, SET_COMMAND_RESPONSE, PID_DEVICE_LABEL);
  } else {
    m_sent_device_label = false;
    rdm_sender.StartCustomResponse(received_message, RDM_RESPONSE_ACK,
        0, GET_COMMAND_RESPONSE, PID_STATUS_MESSAGES);
  }
  rdm_sender.EndRDMResponse();
}


/**
 * Handle a GET SUPPORTED_PARAMETERS request
 */
void RDMHandler::HandleGetSupportedParameters(const byte *received_message) {
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
void RDMHandler::HandleGetParameterDescription(const byte *received_message) {
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
void RDMHandler::HandleGetDeviceInfo(const byte *received_message) {
  rdm_sender.StartRDMAckResponse(received_message, 19);
  rdm_sender.SendIntAndChecksum(256);  // protocol version
  rdm_sender.SendIntAndChecksum(2);  // device model
  rdm_sender.SendIntAndChecksum(0x0508);  // product category
  rdm_sender.SendLongAndChecksum(SOFTWARE_VERSION);  // software version

  byte personality = WidgetSettings.Personality();
  rdm_sender.SendIntAndChecksum(rdm_personalities[personality - 1].slots);
  // current personality
  rdm_sender.SendByteAndChecksum(personality);
  rdm_sender.SendByteAndChecksum(sizeof(rdm_personalities) /
                                 sizeof(rdm_personality));
  // DMX Start Address
  rdm_sender.SendIntAndChecksum(WidgetSettings.StartAddress());
  rdm_sender.SendIntAndChecksum(0);  // Sub device count
  rdm_sender.SendByteAndChecksum(1);  // Sensor Count
  rdm_sender.EndRDMResponse();
}


/**
 * Handle a GET PRODUCT_DETAIL_ID request
 */
void RDMHandler::HandleGetProductDetailId(const byte *received_message) {
  rdm_sender.StartRDMAckResponse(received_message, 2);
  rdm_sender.SendIntAndChecksum(0x0403);  // PWM dimmer
  rdm_sender.EndRDMResponse();
}


/**
 * Handle a GET PID_DEVICE_MODEL_DESCRIPTION request
 */
void RDMHandler::HandleGetDeviceModelDescription(
    const byte *received_message) {
  HandleStringRequest(received_message, DEVICE_NAME, DEVICE_NAME_SIZE);
}


/**
 * Handle a GET MANUFACTURER_NAME request
 */
void RDMHandler::HandleGetManufacturerLabel(const byte *received_message) {
  HandleStringRequest(received_message, MANUFACTURER_NAME,
                      MANUFACTURER_NAME_SIZE);
}


/**
 * Handle a GET DEVICE_LABEL request
 */
void RDMHandler::HandleGetDeviceLabel(const byte *received_message) {
  char device_label[MAX_LABEL_SIZE];
  byte size = WidgetSettings.DeviceLabel(device_label, sizeof(device_label));
  HandleStringRequest(received_message, device_label, size);
}


/**
 * Handle a GET LANGUAGE / LANGUAGE_CAPABILITIES request
 */
void RDMHandler::HandleGetLanguage(const byte *received_message) {
  HandleStringRequest(received_message,
                      SUPPORTED_LANGUAGE,
                      sizeof(SUPPORTED_LANGUAGE) - 1);
}


/**
 * Handle a GET SOFTWARE_VERSION_LABEL request
 */
void RDMHandler::HandleGetSoftwareVersion(const byte *received_message) {
  rdm_sender.StartRDMAckResponse(received_message,
                                 sizeof(SOFTWARE_VERSION_STRING));
  for (unsigned int i = 0; i < sizeof(SOFTWARE_VERSION_STRING); ++i)
    rdm_sender.SendByteAndChecksum(SOFTWARE_VERSION_STRING[i]);
  rdm_sender.EndRDMResponse();
}


/**
 * Handle a GET DMX_PERSONALITY request
 */
void RDMHandler::HandleGetPersonality(const byte *received_message) {
  rdm_sender.StartRDMAckResponse(received_message, 2);
  rdm_sender.SendByteAndChecksum(WidgetSettings.Personality());
  rdm_sender.SendByteAndChecksum(sizeof(rdm_personalities) /
                                 sizeof(rdm_personality));
  rdm_sender.EndRDMResponse();
}


/**
 * Handle a GET DMX_PERSONALITY_DESCRIPTION request
 */
void RDMHandler::HandleGetPersonalityDescription(
    const byte *received_message) {
  byte max_personalities = sizeof(rdm_personalities) / sizeof(rdm_personality);
  byte personality_number = received_message[24];

  if (personality_number == 0 || personality_number > max_personalities) {
    rdm_sender.SendNack(received_message, NR_DATA_OUT_OF_RANGE);
    return;
  }

  rdm_personality const *personality =
    &rdm_personalities[personality_number - 1];
  unsigned int description_length = strlen(personality->description);

  rdm_sender.StartRDMAckResponse(received_message, 3 + description_length);
  rdm_sender.SendByteAndChecksum(personality_number);
  rdm_sender.SendIntAndChecksum(personality->slots);
  for (unsigned int i = 0; i < description_length; ++i)
    rdm_sender.SendByteAndChecksum(personality->description[i]);
  rdm_sender.EndRDMResponse();
}


/**
 * Handle a GET DMX_START_ADDRESS request
 */
void RDMHandler::HandleGetStartAddress(const byte *received_message) {
  int start_address = WidgetSettings.StartAddress();
  rdm_sender.StartRDMAckResponse(received_message, sizeof(start_address));
  rdm_sender.SendIntAndChecksum(start_address);
  rdm_sender.EndRDMResponse();
}


/**
 * Handle a GET SENSOR_DEFINITION request
 */
void RDMHandler::HandleGetSensorDefinition(const byte *received_message) {
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
void RDMHandler::HandleGetSensorValue(const byte *received_message) {
  if (received_message[24]) {
    rdm_sender.SendNack(received_message, NR_DATA_OUT_OF_RANGE);
    return;
  }

  SendSensorResponse(received_message);
}


/**
 * Handle a GET DEVICE_POWER_CYCLES request
 */
void RDMHandler::HandleGetDevicePowerCycles(const byte *received_message) {
  unsigned long power_cycles = WidgetSettings.DevicePowerCycles();
  rdm_sender.StartRDMAckResponse(received_message, sizeof(power_cycles));
  rdm_sender.SendLongAndChecksum(power_cycles);
  rdm_sender.EndRDMResponse();
}


/**
 * Handle a GET IDENTIFY_DEVICE request
 */
void RDMHandler::HandleGetIdentifyDevice(const byte *received_message) {
  rdm_sender.StartRDMAckResponse(received_message, 1);
  rdm_sender.SendByteAndChecksum(m_identify_mode_enabled);
  rdm_sender.EndRDMResponse();
}


/**
 * Handle a SET DMX_START_ADDRESS request
 */
void RDMHandler::HandleSetLanguage(bool was_broadcast,
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
void RDMHandler::HandleSetDeviceLabel(bool was_broadcast,
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
    // 400ms should be more than enough time
    rdm_sender.SendAckTimer(received_message, 4);
    m_device_label_pending = true;
    rdm_sender.IncrementMessageCount();
  }
}


/**
 * Handle a SET DMX_PERSONALITY request
 */
void RDMHandler::HandleSetPersonality(bool was_broadcast,
                                      int sub_device,
                                      const byte *received_message) {
  // check for invalid size or value
  if (received_message[23] != 1) {
    rdm_sender.NackOrBroadcast(was_broadcast,
                               received_message,
                               NR_FORMAT_ERROR);
    return;
  }

  if (received_message[24] == 0 ||
      received_message[24] >
      sizeof(rdm_personalities) / sizeof(rdm_personality)) {
    rdm_sender.NackOrBroadcast(was_broadcast,
                               received_message,
                               NR_DATA_OUT_OF_RANGE);
    return;
  }

  WidgetSettings.SetPersonality(received_message[24]);
  if (was_broadcast) {
    rdm_sender.ReturnRDMErrorResponse(RDM_STATUS_BROADCAST);
  } else {
    rdm_sender.SendEmptyAck(received_message);
  }
}


/**
 * Handle a SET DMX_START_ADDRESS request
 */
void RDMHandler::HandleSetStartAddress(bool was_broadcast,
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
void RDMHandler::HandleSetSensorValue(bool was_broadcast,
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
void RDMHandler::HandleRecordSensor(bool was_broadcast,
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
 * Handle a SET DEVICE_POWER_CYCLES request
 */
void RDMHandler::HandleSetDevicePowerCycles(bool was_broadcast,
                                            int sub_device,
                                            const byte *received_message) {
  // check for invalid size or value
  if (received_message[23] != 4) {
    rdm_sender.NackOrBroadcast(was_broadcast,
                               received_message,
                               NR_FORMAT_ERROR);
    return;
  }

  unsigned long power_cycles = 0;
  for (byte i = 0; i < 4; ++i) {
    power_cycles = power_cycles << 8;
    power_cycles += received_message[24 + i];
  }

  WidgetSettings.SetDevicePowerCycles(power_cycles);

  if (was_broadcast) {
    rdm_sender.ReturnRDMErrorResponse(RDM_STATUS_BROADCAST);
  } else {
    rdm_sender.SendEmptyAck(received_message);
  }
}

/**
 * Handle a SET IDENTIFY_DEVICE request
 */
void RDMHandler::HandleSetIdentifyDevice(bool was_broadcast,
                                         int sub_device,
                                         const byte *received_message) {
  // check for invalid size or value
  if (received_message[23] != 1) {
    rdm_sender.NackOrBroadcast(was_broadcast,
                               received_message,
                               NR_FORMAT_ERROR);
    return;
  }
  if (received_message[24] != 0 && received_message[24] != 1) {
    rdm_sender.NackOrBroadcast(was_broadcast,
                               received_message,
                               NR_DATA_OUT_OF_RANGE);
    return;
  }

  m_identify_mode_enabled = received_message[24];
  digitalWrite(IDENTIFY_LED_PIN, m_identify_mode_enabled);

  if (was_broadcast) {
    rdm_sender.ReturnRDMErrorResponse(RDM_STATUS_BROADCAST);
  } else {
    rdm_sender.SendEmptyAck(received_message);
  }
}


/**
 * Handle a SET SERIAL_NUMBER request
 */
void RDMHandler::HandleSetSerial(bool was_broadcast,
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
void RDMHandler::HandleRDMMessage(const byte *message, int size) {
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

  pid_definition const *pid_handler = NULL;
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

    (this->*(pid_handler->get_handler))(message);

  } else  {
    if (!pid_handler->set_handler) {
      rdm_sender.NackOrBroadcast(is_broadcast, message,
                                 NR_UNSUPPORTED_COMMAND_CLASS);
      return;
    }

    (this->*(pid_handler->set_handler))(is_broadcast, sub_device, message);
  }
}
