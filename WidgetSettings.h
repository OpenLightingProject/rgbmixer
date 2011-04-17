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
 * WidgetSettings.h
 * Copyright (C) 2011 Simon Newton
 */

#include "WProgram.h"

#ifndef WIDGET_SETTINGS_H
#define WIDGET_SETTINGS_H

/**
 * Manages reading & writing settings from EEPROM.
 */
class WidgetSettingsClass {
  public:
    WidgetSettingsClass() {}
    void Init();

    unsigned int StartAddress() const { return m_start_address; };
    void SetStartAddress(unsigned int start_address);

    int EstaId() const;
    void SetEstaId(int esta_id);
    // helper method to compare an array of bytes against the esta id
    bool MatchesEstaId(byte *data) const;

    long SerialNumber() const;
    void SetSerialNumber(long serial_number);
    // helper method to compare an array of bytes against the serial #
    bool MatchesSerialNumber(byte *data) const;

    byte DeviceLabel(char *label, byte length) const;
    void SetDeviceLabel(const char *new_label, byte length);

    unsigned long DevicePowerCycles() const;
    void IncrementDevicePowerCycles();

    int SensorValue() const;
    void SaveSensorValue(int value);

  private:
    static const int MAGIC_NUMBER;
    static const long DEFAULT_SERIAL_NUMBER;
    static const char DEFAULT_LABEL[];
    static const byte MAX_LABEL_LENGTH;

    static const byte MAGIC_NUMBER_OFFSET;
    static const byte START_ADDRESS_OFFSET;
    static const byte ESTA_ID_OFFSET;
    static const byte SERIAL_NUMBER_OFFSET;
    static const byte DEVICE_LABEL_SIZE_OFFSET;
    static const byte DEVICE_LABEL_OFFSET;
    static const byte DEVICE_POWER_CYCLES_OFFSET;
    static const byte SENSOR_0_RECORDED_VALUE;

    unsigned int m_start_address;


    unsigned int ReadInt(unsigned int offset) const;
    void WriteInt(unsigned int offset, int data);

    unsigned long ReadLong(unsigned long offset) const;
    void WriteLong(unsigned long offset, long data);
};

extern WidgetSettingsClass WidgetSettings;
#endif  // USBPRO_SENDER_H
