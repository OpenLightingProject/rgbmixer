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
 * UsbProReceiver.h
 * Copyright (C) 2011 Simon Newton
 * A class which unpacks messages in the Usb Pro format.
 */

#include "WProgram.h"

#ifndef USBPRO_RECEIVER_H_
#define USBPRO_RECEIVER_H_

/**
 * Recieves a message over the serial link
 */
class UsbProReceiver {
  public:
    UsbProReceiver(void (*callback)(byte label, byte *message,
                   unsigned int size));
    void Read();

  private:
    void (*m_callback)(byte label, byte *message, unsigned int size);

    // The receiving state
    typedef enum {
      PRE_SOM = 0,
      GOT_SOM = 1,
      GOT_LABEL = 2,
      GOT_DATA_LSB = 3,
      IN_DATA = 4,
      WAITING_FOR_EOM = 5,
    } recieving_state;
};

#endif  // USBPRO_RECEIVER_H_
