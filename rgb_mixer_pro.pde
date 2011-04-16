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
 * RGB Color Mixer
 * Copyright (C) 2010 Simon Newton
 * A Simple RGB mixer that behaves like a DMX USB Pro.
 * http://opendmx.net/index.php/Arduino_RGB_Mixer
 *
 * 25/3/2010
 *  Changed to LSB order for the device & esta id
 *
 * 16/3/2010:
 *  Add support for the USB Pro Protocol Extensions
 *
 * 13/2/2010:
 *  Support for additional PWM pins
 *
 * 7/2/2010:
 *   Initial Release.
 */

#include "MessageLabels.h"
#include "RDMEnums.h"
#include "RDMSender.h"
#include "UsbProReceiver.h"
#include "UsbProSender.h"
#include "WidgetSettings.h"

// Pin constants
const byte LED_PIN = 13;
const byte IDENTIFY_LED_PIN = 12;
const byte PWM_PINS[] = {3, 5, 6, 9, 10, 11};


byte DEVICE_PARAMS[] = {0, 1, 0, 0, 40};
byte DEVICE_ID[] = {1, 0};
char DEVICE_NAME[] = "Arduino RGB Mixer";
char MANUFACTURER_NAME[] = "Open Lighting";
char SUPPORTED_LANGUAGE[] = "en";
unsigned long SOFTWARE_VERSION = 1;
char SOFTWARE_VERSION_STRING[] = "1.0";
unsigned int SUPPORTED_PARAMETERS[] = {0x0080, 0x0081, 0x0082, 0x00a0, 0x00b0};
const int MAX_DMX_ADDRESS = 512;
enum { MAX_LABEL_SIZE = 32 };

byte led_state = LOW;  // flash the led when we get data.

bool identify_mode_enabled = false;

void TakeAction(byte label, byte *message, unsigned int message_size);
void SetPWM(byte data[], unsigned int size);
void HandleRDMMessage(byte *message, int size);
void SendManufacturerResponse();
void SendDeviceResponse();
void SendSerialNumberResponse();
void HandleStringRequest(byte *received_message,
                         char *label,
                         byte label_size);


// global objects
UsbProSender sender;
RDMSender rdm_sender(&sender);

void setup() {
  for(byte i = 0; i < sizeof(PWM_PINS); i++) {
    pinMode(PWM_PINS[i], OUTPUT);
    analogWrite(PWM_PINS[i], 0);
  }
  pinMode(LED_PIN, OUTPUT);
  pinMode(IDENTIFY_LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, led_state);
  digitalWrite(IDENTIFY_LED_PIN, identify_mode_enabled);

  WidgetSettings.Init();
}


void loop() {
  UsbProReceiver receiver(TakeAction);
  receiver.Read();
}


/*
 * Called when a full message is recieved from the host
 */
void TakeAction(byte label, byte *message, unsigned int message_size) {
  switch (label) {
    case PARAMETERS_LABEL:
      // Widget Parameters request
      sender.WriteMessage(PARAMETERS_LABEL,
                          sizeof(DEVICE_PARAMS),
                          DEVICE_PARAMS);
      break;
    case DMX_DATA_LABEL:
      // Dmx Data
      if (message[0] == 0) {
        // 0 start code
        led_state = ! led_state;
        digitalWrite(LED_PIN, led_state);
        SetPWM(&message[1], message_size);
       }
      break;
    case SERIAL_NUMBER_LABEL:
      SendSerialNumberResponse();
      break;
    case NAME_LABEL:
      SendDeviceResponse();
      break;
    case MANUFACTURER_LABEL:
      SendManufacturerResponse();
      break;
     case RDM_LABEL:
      HandleRDMMessage(message, message_size);
      break;
  }
}


void SendSerialNumberResponse() {
  long serial = WidgetSettings.SerialNumber();
  sender.SendMessageHeader(SERIAL_NUMBER_LABEL, sizeof(serial));
  sender.Write((byte*) &serial, sizeof(serial));
  sender.SendMessageFooter();
}


void SendDeviceResponse() {
  sender.SendMessageHeader(NAME_LABEL,
                           sizeof(DEVICE_ID) + sizeof(DEVICE_NAME));
  sender.Write(DEVICE_ID, sizeof(DEVICE_ID));
  sender.Write((byte*) DEVICE_NAME, sizeof(DEVICE_NAME));
  sender.SendMessageFooter();
}


void SendManufacturerResponse() {
  int esta_id = WidgetSettings.EstaId();
  sender.SendMessageHeader(MANUFACTURER_LABEL,
                           sizeof(esta_id) + sizeof(MANUFACTURER_NAME));
  // ESTA ID is sent in little endian format
  sender.Write(esta_id);
  sender.Write(esta_id >> 8);
  sender.Write((byte*) MANUFACTURER_NAME, sizeof(MANUFACTURER_NAME));
  sender.SendMessageFooter();
}


/*
 * Write the DMX values to the PWM pins
 * @param data the dmx data buffer
 * @param size the size of the dmx buffer
 */
void SetPWM(byte data[], unsigned int size) {
  unsigned int start_address = WidgetSettings.StartAddress() - 1;
  for (byte i = 0; i < sizeof(PWM_PINS) && start_address + i < size; ++i) {
    analogWrite(PWM_PINS[i], data[start_address + i]);
  }
}


/**
 * Verify a RDM checksum
 * @param message a pointer to an RDM message starting with the SUB_START_CODE
 * @param size the size of the message data
 * @return true if the checksum is ok, false otherwise
 */
bool VerifyChecksum(byte *message, int size) {
  // don't checksum the checksum itself (last two bytes)
  unsigned int checksum = 0;
  for (int i = 0; i < size - 2; i++)
    checksum += message[i];

  byte checksum_offset = message[2];
  return (checksum >> 8 == message[checksum_offset] &&
          (checksum & 0xff) == message[checksum_offset + 1]);
}


/**
 * Handle a GET SUPPORTED_PARAMETERS request
 */
void HandleGetSupportedParameters(byte *received_message) {
  if (received_message[23]) {
    rdm_sender.SendNack(received_message, NR_FORMAT_ERROR);
    return;
  }

  rdm_sender.StartRDMResponse(received_message,
                              RDM_RESPONSE_ACK,
                              sizeof(SUPPORTED_PARAMETERS));
  for (byte i = 0; i < sizeof(SUPPORTED_PARAMETERS) / sizeof(int); ++i) {
    rdm_sender.SendIntAndChecksum(SUPPORTED_PARAMETERS[i]);
  }

  rdm_sender.EndRDMResponse();
}

/**
 * Handle a GET DEVICE_INFO request
 */
void HandleGetDeviceInfo(byte *received_message) {
  if (received_message[23]) {
    rdm_sender.SendNack(received_message, NR_FORMAT_ERROR);
    return;
  }

  rdm_sender.StartRDMResponse(received_message, RDM_RESPONSE_ACK, 19);
  rdm_sender.SendIntAndChecksum(256);  // protocol version
  rdm_sender.SendIntAndChecksum(2);  // device model
  rdm_sender.SendIntAndChecksum(0x0508);  // product category
  rdm_sender.SendLongAndChecksum(SOFTWARE_VERSION);  // software version
  rdm_sender.SendIntAndChecksum(3);  // DMX footprint
  rdm_sender.SendIntAndChecksum(0x0101);  // DMX Personality
  rdm_sender.SendIntAndChecksum(WidgetSettings.StartAddress());  // DMX Start Address
  rdm_sender.SendIntAndChecksum(0);  // Sub device count
  rdm_sender.SendByteAndChecksum(0);  // Sensor Count
  rdm_sender.EndRDMResponse();
}


/**
 * Handle a GET DEVICE_LABEL request
 */
void HandleGetDeviceLabel(byte *received_message) {
  char device_label[MAX_LABEL_SIZE];
  byte size = WidgetSettings.DeviceLabel(device_label, sizeof(device_label));

  HandleStringRequest(received_message, device_label, size);
}


/**
 * Handle a GET SOFTWARE_VERSION_LABEL request
 */
void HandleGetSoftwareVersion(byte *received_message) {
  if (received_message[23]) {
    rdm_sender.SendNack(received_message, NR_FORMAT_ERROR);
    return;
  }

  rdm_sender.StartRDMResponse(received_message,
                   RDM_RESPONSE_ACK,
                   sizeof(SOFTWARE_VERSION_STRING));
  for (unsigned int i = 0; i < sizeof(SOFTWARE_VERSION_STRING); ++i)
    rdm_sender.SendByteAndChecksum(SOFTWARE_VERSION_STRING[i]);
  rdm_sender.EndRDMResponse();
}


/**
 * Handle a GET DMX_START_ADDRESS request
 */
void HandleGetStartAddress(byte *received_message) {
  if (received_message[23]) {
    rdm_sender.SendNack(received_message, NR_FORMAT_ERROR);
    return;
  }

  int start_address = WidgetSettings.StartAddress();
  rdm_sender.StartRDMResponse(received_message,
                              RDM_RESPONSE_ACK,
                              sizeof(start_address));
  rdm_sender.SendIntAndChecksum(start_address);
  rdm_sender.EndRDMResponse();
}

/**
 * Handle a GET IDENTIFY_DEVICE request
 */
void HandleGetIdentifyDevice(byte *received_message) {
  if (received_message[23]) {
    rdm_sender.SendNack(received_message, NR_FORMAT_ERROR);
    return;
  }

  rdm_sender.StartRDMResponse(received_message, RDM_RESPONSE_ACK, 1);
  rdm_sender.SendByteAndChecksum(identify_mode_enabled);
  rdm_sender.EndRDMResponse();
}


/**
 * Handle a SET DMX_START_ADDRESS request
 */
void HandleSetLanguage(bool was_broadcast,
                       int sub_device,
                       byte *received_message) {
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
    rdm_sender.StartRDMResponse(received_message, RDM_RESPONSE_ACK, 0);
    rdm_sender.EndRDMResponse();
  }
}


/**
 * Handle a SET DMX_START_ADDRESS request
 */
void HandleSetDeviceLabel(bool was_broadcast,
                          int sub_device,
                          byte *received_message) {
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
    rdm_sender.StartRDMResponse(received_message, RDM_RESPONSE_ACK, 0);
    rdm_sender.EndRDMResponse();
  }
}

/**
 * Handle a SET DMX_START_ADDRESS request
 */
void HandleSetStartAddress(bool was_broadcast,
                           int sub_device,
                           byte *received_message) {
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
    rdm_sender.StartRDMResponse(received_message, RDM_RESPONSE_ACK, 0);
    rdm_sender.EndRDMResponse();
  }
}


/**
 * Handle a SET IDENTIFY_DEVICE request
 */
void HandleSetIdentifyDevice(bool was_broadcast,
                             int sub_device,
                             byte *received_message) {
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
    rdm_sender.StartRDMResponse(received_message, RDM_RESPONSE_ACK, 0);
    rdm_sender.EndRDMResponse();
  }
}


/**
 * Handle a GET request for a PID that returns a string
 *
 */
void HandleStringRequest(byte *received_message,
                         char *label,
                         byte label_size) {
  if (received_message[23]) {
    rdm_sender.SendNack(received_message, NR_FORMAT_ERROR);
    return;
  }

  rdm_sender.StartRDMResponse(received_message, RDM_RESPONSE_ACK, label_size);
  for (unsigned int i = 0; i < label_size; ++i)
    rdm_sender.SendByteAndChecksum(label[i]);
  rdm_sender.EndRDMResponse();
}


/**
 * Handle an RDM GET request
 */
void HandleRDMGet(int param_id, bool is_broadcast, int sub_device,
                  byte *message) {
  if (is_broadcast) {
    rdm_sender.ReturnRDMErrorResponse(RDM_STATUS_BROADCAST);
    return;
  }

  if (sub_device) {
    rdm_sender.SendNack(message, NR_SUB_DEVICE_OUT_OF_RANGE);
    return;
  }

  switch (param_id) {
    case PID_SUPPORTED_PARAMETERS:
      HandleGetSupportedParameters(message);
      break;
    case PID_DEVICE_INFO:
      HandleGetDeviceInfo(message);
      break;
    case PID_DEVICE_MODEL_DESCRIPTION:
      HandleStringRequest(message,
                          DEVICE_NAME,
                          sizeof(DEVICE_NAME));
      break;
    case PID_MANUFACTURER_LABEL:
      HandleStringRequest(message,
                          MANUFACTURER_NAME,
                          sizeof(MANUFACTURER_NAME));
      break;
    case PID_DEVICE_LABEL:
      HandleGetDeviceLabel(message);
      break;
    case PID_LANGUAGE_CAPABILITIES:
    case PID_LANGUAGE:
      // these strings aren't null terminated
      HandleStringRequest(message,
                          SUPPORTED_LANGUAGE,
                          sizeof(SUPPORTED_LANGUAGE) - 1);
      break;
    case PID_SOFTWARE_VERSION_LABEL:
      HandleGetSoftwareVersion(message);
      break;
    case PID_DMX_START_ADDRESS:
      HandleGetStartAddress(message);
      break;
    case PID_IDENTIFY_DEVICE:
      HandleGetIdentifyDevice(message);
      break;
    default:
      rdm_sender.NackOrBroadcast(is_broadcast, message, NR_UNKNOWN_PID);
  }
}


/**
 * Handle an RDM SET request
 */
void HandleRDMSet(int param_id, bool is_broadcast, int sub_device,
                  byte *message) {

  switch (param_id) {
    case PID_LANGUAGE:
      HandleSetLanguage(is_broadcast, sub_device, message);
      break;
    case PID_DEVICE_LABEL:
      HandleSetDeviceLabel(is_broadcast, sub_device, message);
      break;
    case PID_DMX_START_ADDRESS:
      HandleSetStartAddress(is_broadcast, sub_device, message);
      break;
    case PID_IDENTIFY_DEVICE:
      HandleSetIdentifyDevice(is_broadcast, sub_device, message);
      break;
    default:
      rdm_sender.NackOrBroadcast(is_broadcast, message, NR_UNKNOWN_PID);
  }
}


/*
 * Handle an RDM message
 * @param message pointer to a RDM message where the first byte is the sub star
 * code.
 * @param size the size of the message data.
 */
void HandleRDMMessage(byte *message, int size) {
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


  byte data[] = {RDM_STATUS_OK};
  if (is_broadcast) {
    data[0] = RDM_STATUS_BROADCAST;
  }

  if (command_class == GET_COMMAND) {
    HandleRDMGet(param_id, is_broadcast, sub_device, message);
    return;
  } else  {
    HandleRDMSet(param_id, is_broadcast, sub_device, message);
  }
}
