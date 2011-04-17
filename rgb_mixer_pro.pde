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

#include "Common.h"
#include "MessageLabels.h"
#include "RDMHandlers.h"
#include "UsbProReceiver.h"
#include "UsbProSender.h"
#include "WidgetSettings.h"

// Define the variables from Common.h
char DEVICE_NAME[] = "Arduino RGB Mixer";
byte DEVICE_NAME_SIZE = sizeof(DEVICE_NAME);
char MANUFACTURER_NAME[] = "Open Lighting";
byte MANUFACTURER_NAME_SIZE = sizeof(MANUFACTURER_NAME);

UsbProSender sender;

// Pin constants
const byte LED_PIN = 13;
const byte PWM_PINS[] = {3, 5, 6, 9, 10, 11};

// device setting
const byte DEVICE_PARAMS[] = {0, 1, 0, 0, 40};
const byte DEVICE_ID[] = {1, 0};

// global state
byte led_state = LOW;  // flash the led when we get data.


/**
 * Send the Serial Number response
 */
void SendSerialNumberResponse() {
  long serial = WidgetSettings.SerialNumber();
  sender.SendMessageHeader(SERIAL_NUMBER_LABEL, sizeof(serial));
  sender.Write((byte*) &serial, sizeof(serial));
  sender.SendMessageFooter();
}


/**
 * Send the device id / name response
 */
void SendDeviceResponse() {
  sender.SendMessageHeader(NAME_LABEL,
                           sizeof(DEVICE_ID) + sizeof(DEVICE_NAME));
  sender.Write(DEVICE_ID, sizeof(DEVICE_ID));
  sender.Write((byte*) DEVICE_NAME, sizeof(DEVICE_NAME));
  sender.SendMessageFooter();
}


/**
 * Send the manufacturer id / name response
 */
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


/**
 * Write the DMX values to the PWM pins.
 * @param data the dmx data buffer.
 * @param size the size of the dmx buffer.
 */
void SetPWM(const byte data[], unsigned int size) {
  unsigned int start_address = WidgetSettings.StartAddress() - 1;
  for (byte i = 0; i < sizeof(PWM_PINS) && start_address + i < size; ++i) {
    analogWrite(PWM_PINS[i], data[start_address + i]);
  }
}


/*
 * Called when a full message is recieved from the host.
 * @param label the message label.
 * @param message the array of bytes that make up the message.
 * @param message_size the size of the message.
 */
void TakeAction(byte label, const byte *message, unsigned int message_size) {
  switch (label) {
    case PARAMETERS_LABEL:
      // Widget Parameters request
      sender.WriteMessage(PARAMETERS_LABEL,
                          sizeof(DEVICE_PARAMS),
                          DEVICE_PARAMS);
      break;
    case DMX_DATA_LABEL:
      if (message[0] == 0) {
        // 0 start code
        led_state = !led_state;
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


/**
 * Setup the i/o pins correctly
 */
void setup() {
  for (byte i = 0; i < sizeof(PWM_PINS); i++) {
    pinMode(PWM_PINS[i], OUTPUT);
    analogWrite(PWM_PINS[i], 0);
  }
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, led_state);
  SetupRDMHandling();
  WidgetSettings.Init();
}


void loop() {
  UsbProReceiver receiver(TakeAction);
  receiver.Read();
}
