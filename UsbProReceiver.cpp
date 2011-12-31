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
 * UsbProReceiver.cpp
 * Copyright (C) 2010 Simon Newton
 */

#include "UsbProReceiver.h"


UsbProReceiver::UsbProReceiver(void (*callback)(byte label,
                                                const byte *message,
                                                unsigned int size),
                              void (*idle_callback)()):
    m_callback(callback),
    m_idle_callback(idle_callback) {
  Serial.begin(115200);  // fast baud rate, 9600 is too slow
}


/*
 * Read bytes from host
 */
void UsbProReceiver::Read() {
  recieving_state recv_mode = PRE_SOM;
  byte label = 0;
  unsigned short expected_size = 0;
  unsigned short data_offset = 0;
  byte message[600];

  while (true) {
    while (!Serial.available()) {
      m_idle_callback();
    }

    byte data = Serial.read();
    switch (recv_mode) {
      case PRE_SOM:
        if (data == 0x7E) {
          recv_mode = GOT_SOM;
        }
        break;
      case GOT_SOM:
        label = data;
        recv_mode = GOT_LABEL;
        break;
      case GOT_LABEL:
        data_offset = 0;
        expected_size = data;
        recv_mode = GOT_DATA_LSB;
        break;
      case GOT_DATA_LSB:
        expected_size += (data << 8);
        if (expected_size == 0) {
          recv_mode = WAITING_FOR_EOM;
        } else {
          recv_mode = IN_DATA;
        }
        break;
      case IN_DATA:
        message[data_offset] = data;
        data_offset++;
        if (data_offset == expected_size) {
          recv_mode = WAITING_FOR_EOM;
        }
        break;
      case WAITING_FOR_EOM:
        if (data == 0xE7) {
          // this was a valid packet, act on it
          m_callback(label, message, expected_size);
        }
        recv_mode = PRE_SOM;
    }
  }
}
