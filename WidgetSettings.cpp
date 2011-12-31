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
 *   device label size (2)
 *   device label (32)
 *   device power cycles (4)
 *   sensor 0 recorded value (2)
 *   dmx personality (1)
 */

#include "EEPROM.h"
#include "WidgetSettings.h"


const int WidgetSettingsClass::MAGIC_NUMBER = 0x4f4d;
const long WidgetSettingsClass::DEFAULT_SERIAL_NUMBER = 1;
const char WidgetSettingsClass::DEFAULT_LABEL[] = "Default Label";

const byte WidgetSettingsClass::MAGIC_NUMBER_OFFSET = 0;
const byte WidgetSettingsClass::START_ADDRESS_OFFSET = 2;
const byte WidgetSettingsClass::ESTA_ID_OFFSET = 4;
const byte WidgetSettingsClass::SERIAL_NUMBER_OFFSET = 6;
const byte WidgetSettingsClass::DEVICE_LABEL_SIZE_OFFSET = 10;
const byte WidgetSettingsClass::DEVICE_LABEL_OFFSET = 12;
const byte WidgetSettingsClass::DEVICE_POWER_CYCLES_OFFSET = 44;
const byte WidgetSettingsClass::SENSOR_0_RECORDED_VALUE = 46;
const byte WidgetSettingsClass::DMX_PERSONALITY_VALUE = 48;

/**
 * Check if the settings are valid and if not initialize them
 */
void WidgetSettingsClass::Init() {
  int magic_number = ReadInt(MAGIC_NUMBER_OFFSET);

  if (magic_number != MAGIC_NUMBER) {
    // init the settings
    WriteInt(MAGIC_NUMBER_OFFSET, MAGIC_NUMBER);
    SetStartAddress(1);
    SetEstaId(0x7a70);
    SetSerialNumber(DEFAULT_SERIAL_NUMBER);
    SetDeviceLabel(DEFAULT_LABEL, sizeof(DEFAULT_LABEL));
    SetDevicePowerCycles(0);
    SaveSensorValue(0);
    SetPersonality(1);
  } else {
    m_start_address = ReadInt(START_ADDRESS_OFFSET);
    m_personality = EEPROM.read(DMX_PERSONALITY_VALUE);
  }
  IncrementDevicePowerCycles();
}

void WidgetSettingsClass::SetStartAddress(unsigned int start_address) {
  WriteInt(START_ADDRESS_OFFSET, start_address);
  m_start_address = start_address;
}


int WidgetSettingsClass::EstaId() const {
  return ReadInt(ESTA_ID_OFFSET);
}

void WidgetSettingsClass::SetEstaId(int esta_id) {
  WriteInt(ESTA_ID_OFFSET, esta_id);
}


bool WidgetSettingsClass::MatchesEstaId(const byte *data) const {
  bool match = true;
  for (byte i = 0; i < sizeof(long); ++i) {
    match &= EEPROM.read(ESTA_ID_OFFSET + i) == data[i];
  }
  return match;
}


long WidgetSettingsClass::SerialNumber() const {
  return ReadLong(SERIAL_NUMBER_OFFSET);
}


void WidgetSettingsClass::SetSerialNumber(long serial_number) {
  WriteLong(SERIAL_NUMBER_OFFSET, serial_number);
}


bool WidgetSettingsClass::MatchesSerialNumber(const byte *data) const {
  bool match = true;
  for (byte i = 0; i < sizeof(long); ++i) {
    match &= EEPROM.read(6 + i) == data[i];
  }
  return match;
}


byte WidgetSettingsClass::DeviceLabel(char *label, byte length) const {
  byte size = min(ReadInt(DEVICE_LABEL_SIZE_OFFSET), length);
  byte i = 0;
  for (; i < size; ++i) {
    label[i] = EEPROM.read(DEVICE_LABEL_OFFSET + i);
    if (!label[i])
      break;
  }
  return i;
}


void WidgetSettingsClass::SetDeviceLabel(const char *new_label,
                                         byte length) {
  byte size = min(MAX_LABEL_LENGTH, length);
  memcpy(m_label_buffer, new_label, size);
  m_label_size = size;
  m_label_pending = true;
}


unsigned long WidgetSettingsClass::DevicePowerCycles() const {
  return ReadLong(DEVICE_POWER_CYCLES_OFFSET);
}


void WidgetSettingsClass::SetDevicePowerCycles(unsigned long count) {
  WriteLong(DEVICE_POWER_CYCLES_OFFSET, count);
}


int WidgetSettingsClass::SensorValue() const {
  return ReadInt(SENSOR_0_RECORDED_VALUE);
}


void WidgetSettingsClass::SaveSensorValue(int value) {
  WriteInt(SENSOR_0_RECORDED_VALUE, value);
}


void WidgetSettingsClass::SetPersonality(byte value) {
  EEPROM.write(DMX_PERSONALITY_VALUE, value);
  m_personality = value;
}


bool WidgetSettingsClass::PerformWrite() {
  if (!m_label_pending)
    return false;

  for (byte i = 0; i < m_label_size; ++i) {
    EEPROM.write(DEVICE_LABEL_OFFSET + i, m_label_buffer[i]);
  }
  WriteInt(DEVICE_LABEL_SIZE_OFFSET, m_label_size);
  m_label_pending = false;
  return true;
}


void WidgetSettingsClass::IncrementDevicePowerCycles() {
  SetDevicePowerCycles(DevicePowerCycles() + 1);
}


unsigned int WidgetSettingsClass::ReadInt(unsigned int offset) const {
  return (EEPROM.read(offset) << 8) + EEPROM.read(offset + 1);
}


void WidgetSettingsClass::WriteInt(unsigned int offset, int data) {
  EEPROM.write(offset, data >> 8);
  EEPROM.write(offset + 1, data);
}

unsigned long WidgetSettingsClass::ReadLong(unsigned long offset) const {
  unsigned long l = ReadInt(offset);
  l = l << 16;
  l += ReadInt(offset + 2);
  return l;
}


void WidgetSettingsClass::WriteLong(unsigned long offset, long data) {
  WriteInt(offset, data >> 16);
  WriteInt(offset + 2, data);
}

WidgetSettingsClass WidgetSettings;
