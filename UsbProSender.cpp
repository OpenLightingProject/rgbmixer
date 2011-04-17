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
 * UsbProSender.cpp
 * Copyright (C) 2011 Simon Newton
 */

#include "UsbProSender.h"


/**
 * Sends the message header
 */
void UsbProSender::SendMessageHeader(byte label, int size) const {
  Serial.write(0x7E);
  Serial.write(label);
  Serial.write(size);
  Serial.write(size >> 8);
}

/**
 * Sends the message footer
 */
void UsbProSender::SendMessageFooter() const {
  Serial.write(0xE7);
}


/*
 * Send a message to the host
 * @param label the message label
 * @param size the length of the data
 * @param data the data buffer
 */
void UsbProSender::WriteMessage(byte label, int size,
                                const byte data[]) const {
  SendMessageHeader(label, size);
  Serial.write(data, size);
  SendMessageFooter();
}
