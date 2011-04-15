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
 * WidgetSettings.cpp
 * Copyright (C) 2011 Simon Newton
 *
 *
 * EEPROM layout is as follows:
 *   magic number (2)
 *   dmx start address (2)
 *   esta ID (2)
 *   serial number (4)
 */

#include <EEPROM.h>
#include "WidgetSettings.h"


const byte WidgetSettingsClass::MAGIC_NUMBER[] = {'o', 'z'};
const long WidgetSettingsClass::DEFAULT_SERIAL_NUMBER = 1;

/**
 * Check if the settings are valid and if not initialize them
 */
void WidgetSettingsClass::Init() {
  bool ok = true;
  for (byte i = 0; i < sizeof(MAGIC_NUMBER); ++i) {
    byte value = EEPROM.read(i);
    ok &= (value == MAGIC_NUMBER[i]);
  }

  digitalWrite(12, ok);

  if (!ok) {
    // init the settings
    for (byte i = 0; i < sizeof(MAGIC_NUMBER); ++i) {
      EEPROM.write(i, MAGIC_NUMBER[i]);
    }
    SetStartAddress(1);
    SetEstaId(0x7a70);
    SetSerialNumber(DEFAULT_SERIAL_NUMBER);
  }
}

int WidgetSettingsClass::StartAddress() {
  return (EEPROM.read(2) << 8) + EEPROM.read(3);
}

void WidgetSettingsClass::SetStartAddress(int start_address) {
  EEPROM.write(2, start_address >> 8);
  EEPROM.write(3, start_address);
}


int WidgetSettingsClass::EstaId() {
  return (EEPROM.read(4) << 8) + EEPROM.read(5);
}

void WidgetSettingsClass::SetEstaId(int esta_id) {
  EEPROM.write(4, esta_id >> 8);
  EEPROM.write(5, esta_id);
}


bool WidgetSettingsClass::MatchesEstaId(byte *data) {
  bool match = true;
  for (byte i = 0; i < sizeof(long); ++i) {
    match &= EEPROM.read(4 + i) == data[i];
  }
  return match;
}


long WidgetSettingsClass::SerialNumber() {
  long serial = 0;
  for (byte i = 0; i < sizeof(serial); ++i) {
    serial = serial << 8;
    serial += EEPROM.read(6 + i);
  }
  return serial;
}


void WidgetSettingsClass::SetSerialNumber(long serial_number) {
  EEPROM.write(6, serial_number >> 24);
  EEPROM.write(7, serial_number >> 16);
  EEPROM.write(8, serial_number >> 8);
  EEPROM.write(9, serial_number);
}


bool WidgetSettingsClass::MatchesSerialNumber(byte *data) {
  bool match = true;
  for (byte i = 0; i < sizeof(long); ++i) {
    match &= EEPROM.read(6 + i) == data[i];
  }
  return match;
}

WidgetSettingsClass WidgetSettings;
