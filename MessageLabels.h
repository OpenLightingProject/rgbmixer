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
 * MessageLabels.h
 * Copyright (C) 2011 Simon Newton
 * Contains the message labels used to identify packets.
 */

#include "Arduino.h"

#ifndef MESSAGE_LABELS_H
#define MESSAGE_LABELS_H

// Message Label Codes
enum {
  PARAMETERS_LABEL = 3,
  DMX_DATA_LABEL = 6,
  SERIAL_NUMBER_LABEL = 10,
  MANUFACTURER_LABEL = 77,
  NAME_LABEL = 78,
  RDM_LABEL = 82,
};
#endif  // MESSAGE_LABELS_H
