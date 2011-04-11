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

// Pin constants
const byte LED_PIN = 13;
const byte IDENTIFY_LED_PIN = 12;
const byte PWM_PINS[] = {3, 5, 6, 9, 10, 11};

// The receiving state
typedef enum {
  PRE_SOM = 0,
  GOT_SOM = 1,
  GOT_LABEL = 2,
  GOT_DATA_LSB = 3,
  IN_DATA = 4,
  WAITING_FOR_EOM = 5,
} recieving_state;

// Use this to set the 'serial' number for the device.
// This is used by OLA to store patching information.
// TODO(simon): Set this with dip switches?
byte ESTA_ID[] = "pz";
byte SERIAL_NUMBER[] = {0, 0, 0, 1};
byte DEVICE_PARAMS[] = {0, 1, 0, 0, 40};
byte DEVICE_ID[] = {1, 0};
char DEVICE_NAME[] = "Arduino RGB Mixer";
char MANUFACTURER_NAME[] = "Open Lighting";
unsigned long SOFTWARE_VERSION = 1;
char SOFTWARE_VERSION_STRING[] = "1.0";
unsigned int SUPPORTED_PARAMETERS[] = {0x0080, 0x0081};

// Message Codes
const byte SERIAL_NUMBER_MESSAGE = 10;
const byte MANUFACTURER_MESSAGE = 77;
const byte NAME_MESSAGE = 78;
const byte RDM_MESSAGE = 82;

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
  /*
  PID_PARAMETER_DESCRIPTION = 0x0051,
  // production information
  */
  PID_DEVICE_INFO = 0x0060,
  /*
  PID_PRODUCT_DETAIL_ID_LIST = 0x0070,
  */
  PID_DEVICE_MODEL_DESCRIPTION = 0x0080,
  PID_MANUFACTURER_LABEL = 0x0081,
  PID_DEVICE_LABEL = 0x0082,
  /*
  PID_FACTORY_DEFAULTS = 0x0090,
  PID_LANGUAGE_CAPABILITIES = 0x00A0,
  PID_LANGUAGE = 0x00B0,
  */
  PID_SOFTWARE_VERSION_LABEL = 0x00C0,
  /*
  PID_BOOT_SOFTWARE_VERSION_ID = 0x00C1,
  PID_BOOT_SOFTWARE_VERSION_LABEL = 0x00C2,
  // dmx512
  PID_DMX_PERSONALITY = 0x00E0,
  PID_DMX_PERSONALITY_DESCRIPTION = 0x00E1,
  PID_DMX_START_ADDRESS = 0x00F0,
  PID_SLOT_INFO = 0x0120,
  PID_SLOT_DESCRIPTION = 0x0121,
  PID_DEFAULT_SLOT_VALUE = 0x0122,
  // sensors
  PID_SENSOR_DEFINITION = 0x0200,
  PID_SENSOR_VALUE = 0x0201,
  PID_RECORD_SENSORS = 0x0202,
  // power/lamp settings
  PID_DEVICE_HOURS = 0x0400,
  PID_LAMP_HOURS = 0x0401,
  PID_LAMP_STRIKES = 0x0402,
  PID_LAMP_STATE =  0x0403,
  PID_LAMP_ON_MODE = 0x0404,
  PID_DEVICE_POWER_CYCLES = 0x0405,
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
} rdm_pid;


// RDM globals
unsigned int current_checksum;

recieving_state recv_mode = PRE_SOM;
byte label = 0;
unsigned short expected_size = 0;
unsigned short data_offset = 0;
byte msg[600];
byte led_state = LOW;  // flash the led when we get data.

int dmx_start_address = 1;
bool identify_mode_enabled = false;


void DoRead();
void TakeAction();
void WriteMessage(byte label, int size, byte data[]);
void SetPWM(byte data[], unsigned int size);
void HandleRDMMessage(byte *message, int size);
void SendMessageHeader(byte label, int size);
void SendMessageFooter();
void SendManufacturerResponse();
void SendDeviceResponse();


void setup() {
  for(byte i = 0; i < sizeof(PWM_PINS); i++) {
    pinMode(PWM_PINS[i], OUTPUT);
  }
  pinMode(LED_PIN, OUTPUT);
  pinMode(IDENTIFY_LED_PIN, OUTPUT);
  Serial.begin(115200);  // fast baud rate, 9600 is too slow
  digitalWrite(LED_PIN, led_state);
  digitalWrite(IDENTIFY_LED_PIN, identify_mode_enabled);
}


void loop() {
  while (Serial.available()) {
    DoRead();
  }
}


/*
 * Read bytes from host
 */
void DoRead() {
  byte data = Serial.read();
  switch (recv_mode) {
    case PRE_SOM:
      if (data == 0x7E) {
        recv_mode = GOT_SOM;
      }
      return;
    case GOT_SOM:
      label = data;
      recv_mode = GOT_LABEL;
      return;
    case GOT_LABEL:
      data_offset = 0;
      expected_size = data;
      recv_mode = GOT_DATA_LSB;
      return;
    case GOT_DATA_LSB:
      expected_size += (data << 8);
      if (expected_size == 0) {
        recv_mode = WAITING_FOR_EOM;
      } else {
        recv_mode = IN_DATA;
      }
      return;
    case IN_DATA:
      msg[data_offset] = data;
      data_offset++;
      if (data_offset == expected_size) {
        recv_mode = WAITING_FOR_EOM;
      }
      return;
    case WAITING_FOR_EOM:
      if (data == 0xE7) {
        // this was a valid packet, act on it
        TakeAction();
      }
      recv_mode = PRE_SOM;
  }
}


/*
 * Called when a full message is recieved from the host
 */
void TakeAction() {
  switch (label) {
    case 3:
      // Widget Parameters request
      WriteMessage(3, 5, DEVICE_PARAMS);
      break;
    case 6:
      // Dmx Data
      if (msg[0] == 0) {
        // 0 start code
        led_state = ! led_state;
        digitalWrite(LED_PIN, led_state);
        SetPWM(&msg[1], expected_size);
       }
      break;
    case SERIAL_NUMBER_MESSAGE:
      WriteMessage(SERIAL_NUMBER_MESSAGE,
                   sizeof(SERIAL_NUMBER),
                   SERIAL_NUMBER);
      break;
    case NAME_MESSAGE:
      SendDeviceResponse();
      break;
    case MANUFACTURER_MESSAGE:
      SendManufacturerResponse();
      break;
     case RDM_MESSAGE:
      HandleRDMMessage(msg, expected_size);
      break;
  }
}


/*
 * Send a message to the host
 * @param label the message label
 * @param size the length of the data
 * @param data the data buffer
 */
void WriteMessage(byte label, int size, byte data[]) {
  SendMessageHeader(label, size);
  Serial.write(data, size);
  SendMessageFooter();
}


void SendDeviceResponse() {
  SendMessageHeader(NAME_MESSAGE,
                    sizeof(DEVICE_ID) + sizeof(DEVICE_NAME));
  Serial.write(DEVICE_ID, sizeof(DEVICE_ID));
  Serial.write((byte*) DEVICE_NAME, sizeof(DEVICE_NAME));
  SendMessageFooter();
}


void SendManufacturerResponse() {
  SendMessageHeader(MANUFACTURER_MESSAGE,
                    sizeof(ESTA_ID) + sizeof(MANUFACTURER_NAME));
  Serial.write(ESTA_ID, sizeof(ESTA_ID));
  Serial.write((byte*) MANUFACTURER_NAME, sizeof(MANUFACTURER_NAME));
  SendMessageFooter();
}


/**
 * Sends the message header
 */
void SendMessageHeader(byte label, int size) {
  Serial.write(0x7E);
  Serial.write(label);
  Serial.write(size);
  Serial.write(size >> 8);
}

/**
 * Sends the message footer
 */
void SendMessageFooter() {
  Serial.write(0xE7);
}


/*
 * Write the DMX values to the PWM pins
 * @param data the dmx data buffer
 * @param size the size of the dmx buffer
 */
void SetPWM(byte data[], unsigned int size) {
  for (byte i = 0; i < sizeof(PWM_PINS) && i < size; i++) {
    analogWrite(PWM_PINS[i], data[i]);
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


void ReturnRDMErrorResponse(byte error_code) {
  SendMessageHeader(RDM_MESSAGE, 1);
  Serial.write(error_code);
  SendMessageFooter();
}


void SendByteAndChecksum(byte b) {
  current_checksum += b;
  Serial.write(b);
}

void SendIntAndChecksum(int i) {
  SendByteAndChecksum(i >> 8);
  SendByteAndChecksum(i);
}

void SendLongAndChecksum(long l) {
  SendIntAndChecksum(l >> 16);
  SendIntAndChecksum(l);
}

/**
 * Send the RDM header
 */
void StartRDMResponse(byte *received_message,
                      rdm_response_type response_type,
                      unsigned int param_data_size) {
  // set the global checksum to 0
  current_checksum = 0;
  // size is the rdm status code, the rdm header + the param_data_size
  SendMessageHeader(RDM_MESSAGE,
                    1 + MINIMUM_RDM_PACKET_SIZE + param_data_size);
  SendByteAndChecksum(RDM_STATUS_OK);
  SendByteAndChecksum(START_CODE);
  SendByteAndChecksum(SUB_START_CODE);
  SendByteAndChecksum(MINIMUM_RDM_PACKET_SIZE - 2 + param_data_size);

  // copy the src uid into the dst uid field
  SendByteAndChecksum(received_message[9]);
  SendByteAndChecksum(received_message[10]);
  SendByteAndChecksum(received_message[11]);
  SendByteAndChecksum(received_message[12]);
  SendByteAndChecksum(received_message[13]);
  SendByteAndChecksum(received_message[14]);

  // add our UID as the src, the ESTA_ID & SERIAL_NUMBER fields are reversed
  SendByteAndChecksum(ESTA_ID[1]);
  SendByteAndChecksum(ESTA_ID[0]);
  SendByteAndChecksum(SERIAL_NUMBER[3]);
  SendByteAndChecksum(SERIAL_NUMBER[2]);
  SendByteAndChecksum(SERIAL_NUMBER[1]);
  SendByteAndChecksum(SERIAL_NUMBER[0]);

  SendByteAndChecksum(received_message[15]);  // transaction #
  SendByteAndChecksum(response_type);  // response type
  SendByteAndChecksum(0);  // message count

  // sub device
  SendByteAndChecksum(received_message[18]);
  SendByteAndChecksum(received_message[19]);

  // command class
  if (received_message[20] == GET_COMMAND) {
    SendByteAndChecksum(GET_COMMAND_RESPONSE);
  } else {
    SendByteAndChecksum(SET_COMMAND_RESPONSE);
  }

  // param id, we don't use queued messages so this always matches the request
  SendByteAndChecksum(received_message[21]);
  SendByteAndChecksum(received_message[22]);
  SendByteAndChecksum(param_data_size);
}


void EndRDMResponse() {
  Serial.write(current_checksum >> 8);
  Serial.write(current_checksum);
  SendMessageFooter();
}


/**
 * Send a Nack response
 * @param received_message a pointer to the received RDM message
 * @param nack_reason the NACK reasons
 */
void SendNack(byte *received_message, rdm_nack_reason nack_reason) {
  StartRDMResponse(received_message, RDM_RESPONSE_NACK, 2);
  SendIntAndChecksum(nack_reason);
  EndRDMResponse();
}


void NackOrBroadcast(bool was_broadcast,
                     byte *received_message,
                     rdm_nack_reason nack_reason) {
  if (was_broadcast)
    ReturnRDMErrorResponse(RDM_STATUS_BROADCAST);
  else
    SendNack(received_message, nack_reason);
}


/**
 * Handle a GET SUPPORTED_PARAMETERS request
 */
void HandleGetSupportedParameters(bool was_broadcast,
                                  int sub_device,
                                  byte *received_message) {
  if (was_broadcast) {
    ReturnRDMErrorResponse(RDM_STATUS_BROADCAST);
    return;
  }

  if (sub_device) {
    SendNack(received_message, NR_SUB_DEVICE_OUT_OF_RANGE);
    return;
  }

  if (received_message[23]) {
    SendNack(received_message, NR_FORMAT_ERROR);
    return;
  }

  StartRDMResponse(received_message,
                   RDM_RESPONSE_ACK,
                   sizeof(SUPPORTED_PARAMETERS));
  for (byte i = 0; i < sizeof(SUPPORTED_PARAMETERS) / sizeof(int); ++i) {
    SendIntAndChecksum(SUPPORTED_PARAMETERS[i]);
  }

  EndRDMResponse();
}

/**
 * Handle a GET DEVICE_INFO request
 */
void HandleGetDeviceInfo(bool was_broadcast,
                         int sub_device,
                         byte *received_message) {
  if (was_broadcast) {
    ReturnRDMErrorResponse(RDM_STATUS_BROADCAST);
    return;
  }

  if (sub_device) {
    SendNack(received_message, NR_SUB_DEVICE_OUT_OF_RANGE);
    return;
  }

  if (received_message[23]) {
    SendNack(received_message, NR_FORMAT_ERROR);
    return;
  }

  StartRDMResponse(received_message, RDM_RESPONSE_ACK, 19);
  SendIntAndChecksum(256);  // protocol version
  SendIntAndChecksum(2);  // device model
  SendIntAndChecksum(0x0508);  // product category
  SendLongAndChecksum(SOFTWARE_VERSION);  // software version
  //SendIntAndChecksum(3);  // DMX footprint
  SendIntAndChecksum(0);  // DMX footprint
  SendIntAndChecksum(0x0101);  // DMX Personality
  //SendIntAndChecksum(dmx_start_address);  // DMX Start Address
  SendIntAndChecksum(0xffff);  // DMX Start Address
  SendIntAndChecksum(0);  // Sub device count
  SendByteAndChecksum(0);  // Sensor Count
  EndRDMResponse();
}


/**
 * Handle a GET SOFTWARE_VERSION_LABEL request
 */
void HandleGetSoftwareVersion(bool was_broadcast,
                              int sub_device,
                              byte *received_message) {
  if (was_broadcast) {
    ReturnRDMErrorResponse(RDM_STATUS_BROADCAST);
    return;
  }

  if (sub_device) {
    SendNack(received_message, NR_SUB_DEVICE_OUT_OF_RANGE);
    return;
  }

  if (received_message[23]) {
    SendNack(received_message, NR_FORMAT_ERROR);
    return;
  }

  StartRDMResponse(received_message,
                   RDM_RESPONSE_ACK,
                   sizeof(SOFTWARE_VERSION_STRING));
  for (unsigned int i = 0; i < sizeof(SOFTWARE_VERSION_STRING); ++i)
    SendByteAndChecksum(SOFTWARE_VERSION_STRING[i]);
  EndRDMResponse();
}


/**
 * Handle a GET IDENTIFY_DEVICE request
 */
void HandleGetIdentifyDevice(bool was_broadcast,
                             int sub_device,
                             byte *received_message) {
  if (was_broadcast) {
    ReturnRDMErrorResponse(RDM_STATUS_BROADCAST);
    return;
  }

  if (sub_device) {
    SendNack(received_message, NR_SUB_DEVICE_OUT_OF_RANGE);
    return;
  }

  if (received_message[23]) {
    SendNack(received_message, NR_FORMAT_ERROR);
    return;
  }

  StartRDMResponse(received_message, RDM_RESPONSE_ACK, 1);
  SendByteAndChecksum(identify_mode_enabled);
  EndRDMResponse();
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
    NackOrBroadcast(was_broadcast, received_message, NR_FORMAT_ERROR);
    return;
  }

  identify_mode_enabled = received_message[24];
  digitalWrite(IDENTIFY_LED_PIN, identify_mode_enabled);

  if (was_broadcast) {
    ReturnRDMErrorResponse(RDM_STATUS_BROADCAST);
  } else {
    StartRDMResponse(received_message, RDM_RESPONSE_ACK, 0);
    EndRDMResponse();
  }
}


/**
 * Handle a GET request for a PID that returns a string
 *
 */
void HandleStringRequest(bool was_broadcast,
                         int sub_device,
                         byte *received_message,
                         char *label,
                         byte label_size) {
  if (was_broadcast) {
    ReturnRDMErrorResponse(RDM_STATUS_BROADCAST);
    return;
  }

  if (sub_device) {
    SendNack(received_message, NR_SUB_DEVICE_OUT_OF_RANGE);
    return;
  }

  if (received_message[23]) {
    SendNack(received_message, NR_FORMAT_ERROR);
    return;
  }


  StartRDMResponse(received_message, RDM_RESPONSE_ACK, label_size);
  for (unsigned int i = 0; i < label_size; ++i)
    SendByteAndChecksum(label[i]);
  EndRDMResponse();
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
    ReturnRDMErrorResponse(RDM_STATUS_FAILED);
    return;
  }

  if (!VerifyChecksum(message, size)) {
    ReturnRDMErrorResponse(RDM_STATUS_FAILED_CHECKSUM);
    return;
  }

  // true if this is broadcast or vendorcast, in which case we don't return a
  // RDM message
  bool is_broadcast = true;
  for (int i = 5; i <= 8; ++i) {
    is_broadcast &= (message[i] == 0xff);
  }

  // the serial number & esta id we store is inverted
  bool to_us = is_broadcast || (
    message[3] == ESTA_ID[1] &&
    message[4] == ESTA_ID[0] &&
    message[5] == SERIAL_NUMBER[3] &&
    message[6] == SERIAL_NUMBER[2] &&
    message[7] == SERIAL_NUMBER[1] &&
    message[8] == SERIAL_NUMBER[0]);

  if (!to_us) {
    ReturnRDMErrorResponse(RDM_STATUS_INVALID_DESTINATION);
    return;
  }

  // check the command class
  byte command_class = message[20];
  if (command_class != GET_COMMAND && command_class != SET_COMMAND) {
    ReturnRDMErrorResponse(RDM_STATUS_INVALID_COMMAND);
  }

  // check sub devices
  unsigned int sub_device = (message[18] << 8) + message[19];
  if (sub_device != 0 && sub_device != 0xffff) {
    // respond with nack
    NackOrBroadcast(is_broadcast, message, NR_SUB_DEVICE_OUT_OF_RANGE);
    return;
  }

  unsigned int param_id = (message[21] << 8) + message[22];


  byte data[] = {RDM_STATUS_OK};
  if (is_broadcast) {
    data[0] = RDM_STATUS_BROADCAST;
  }

  switch (param_id) {
    case PID_SUPPORTED_PARAMETERS:
      if (command_class == GET_COMMAND)
        HandleGetSupportedParameters(is_broadcast, sub_device, message);
      else
        NackOrBroadcast(is_broadcast, message, NR_UNSUPPORTED_COMMAND_CLASS);
      break;
    case PID_DEVICE_INFO:
      if (command_class == GET_COMMAND)
        HandleGetDeviceInfo(is_broadcast, sub_device, message);
      else
        NackOrBroadcast(is_broadcast, message, NR_UNSUPPORTED_COMMAND_CLASS);
      break;
    case PID_SOFTWARE_VERSION_LABEL:
      if (command_class == GET_COMMAND)
        HandleGetSoftwareVersion(is_broadcast, sub_device, message);
      else
        NackOrBroadcast(is_broadcast, message, NR_UNSUPPORTED_COMMAND_CLASS);
      break;
    case PID_DEVICE_MODEL_DESCRIPTION:
      if (command_class == GET_COMMAND)
        HandleStringRequest(is_broadcast,
                            sub_device,
                            message,
                            DEVICE_NAME,
                            sizeof(DEVICE_NAME));
      else
        NackOrBroadcast(is_broadcast, message, NR_UNSUPPORTED_COMMAND_CLASS);
      break;
    case PID_MANUFACTURER_LABEL:
      if (command_class == GET_COMMAND)
        HandleStringRequest(is_broadcast,
                            sub_device,
                            message,
                            MANUFACTURER_NAME,
                            sizeof(MANUFACTURER_NAME));
      else
        NackOrBroadcast(is_broadcast, message, NR_UNSUPPORTED_COMMAND_CLASS);
      break;
    case PID_IDENTIFY_DEVICE:
      if (command_class == GET_COMMAND)
        HandleGetIdentifyDevice(is_broadcast, sub_device, message);
      else
        HandleSetIdentifyDevice(is_broadcast, sub_device, message);
      break;
    default:
      NackOrBroadcast(is_broadcast, message, NR_UNKNOWN_PID);
  }
}
