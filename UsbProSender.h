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
 * UsbProSender.h
 * Copyright (C) 2011 Simon Newton
 */

#include "WProgram.h"

#ifndef USBPRO_SENDER_H
#define USBPRO_SENDER_H

/**
 * Sends a properly framed message over the serial link
 */
class UsbProSender {
  public:
    UsbProSender() {}

    void SendMessageHeader(byte label, int size) const;
    void SendMessageFooter() const;

    // helper message to send an array of bytes
    void WriteMessage(byte label, int size, byte data[]) const;

    void Write(byte b) const { Serial.write(b); }
    void Write(byte *b, unsigned int l) const { Serial.write(b, l); }
};

#endif  // USBPRO_SENDER_H
