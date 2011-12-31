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
 * RDMHandlers.h
 * Copyright (C) 2011 Simon Newton
 */


#ifndef RDM_HANDLERS_H
#define RDM_HANDLERS_H

#include "WProgram.h"
#include "RDMSender.h"

/**
 * Sends a properly framed RDM message over the serial link
 */
class RDMHandler {
  public:
    explicit RDMHandler(const UsbProSender *sender)
      : m_identify_mode_enabled(false),
        m_device_label_pending(false),
        m_sent_device_label(false),
        rdm_sender(sender) {
      pinMode(IDENTIFY_LED_PIN, OUTPUT);
      digitalWrite(IDENTIFY_LED_PIN, m_identify_mode_enabled);
    }

    /*
     * Handle an RDM message
     * @param message pointer to a RDM message where the first byte is the sub
     * start code.
     * @param size the size of the message data.
     */
    void HandleRDMMessage(const byte *message, int size);

    void QueueSetDeviceLabel() {
      m_device_label_pending = true;
    }

  private:
    // The definition for a PID, this includes which functions to call to
    // handle GET/SET requests and if we should include this PID in the list of
    // supported parameters.
    typedef struct {
      unsigned int pid;
      void (RDMHandler::*get_handler)(const byte *message);
      void (RDMHandler::*set_handler)(bool was_broadcast,
                                      int sub_device,
                                      const byte *received_message);
      byte get_argument_size;
      bool include_in_supported_params;
    } pid_definition;

    // personalities
    typedef struct {
      byte personality_number;
      byte slots;
      const char *description;
    } rdm_personality;

    bool m_identify_mode_enabled;
    bool m_device_label_pending;
    bool m_sent_device_label;
    RDMSender rdm_sender;


    bool VerifyChecksum(const byte *message, int size);
    int ReadTemperatureSensor();
    void SendSensorResponse(const byte *received_message);
    void HandleStringRequest(const byte *received_message,
                             const char *label,
                             byte label_size);

    // GET Handlers
    void HandleGetQueuedMessage(const byte *received_message);
    void HandleGetSupportedParameters(const byte *received_message);
    void HandleGetParameterDescription(const byte *received_message);
    void HandleGetDeviceInfo(const byte *received_message);
    void HandleGetProductDetailId(const byte *received_message);
    void HandleGetDeviceModelDescription(const byte *received_message);
    void HandleGetManufacturerLabel(const byte *received_message);
    void HandleGetDeviceLabel(const byte *received_message);
    void HandleGetLanguage(const byte *received_message);
    void HandleGetSoftwareVersion(const byte *received_message);
    void HandleGetPersonality(const byte *received_message);
    void HandleGetPersonalityDescription(const byte *received_message);
    void HandleGetStartAddress(const byte *received_message);
    void HandleGetSensorDefinition(const byte *received_message);
    void HandleGetSensorValue(const byte *received_message);
    void HandleGetDevicePowerCycles(const byte *received_message);
    void HandleGetIdentifyDevice(const byte *received_message);

    // SET Handlers
    void HandleSetLanguage(bool was_broadcast, int sub_device,
                           const byte *received_message);
    void HandleSetDeviceLabel(bool was_broadcast, int sub_device,
                              const byte *received_message);
    void HandleSetPersonality(bool was_broadcast, int sub_device,
                              const byte *received_message);
    void HandleSetStartAddress(bool was_broadcast,
                               int sub_device,
                               const byte *received_message);
    void HandleSetSensorValue(bool was_broadcast, int sub_device,
                              const byte *received_message);
    void HandleRecordSensor(bool was_broadcast, int sub_device,
                            const byte *received_message);
    void HandleSetDevicePowerCycles(bool was_broadcast, int sub_device,
                                    const byte *received_message);
    void HandleSetIdentifyDevice(bool was_broadcast, int sub_device,
                                 const byte *received_message);
    void HandleSetSerial(bool was_broadcast, int sub_device,
                         const byte *received_message);


    // Pin constants
    static const byte IDENTIFY_LED_PIN = 12;
    static const byte TEMP_SENSOR_PIN = 0;

    // Various constants used in RDM messages
    static const unsigned long SOFTWARE_VERSION = 1;
    static const int MAX_DMX_ADDRESS = 512;
    enum { MAX_LABEL_SIZE = 32 };
    static const char SUPPORTED_LANGUAGE[];
    static const char SOFTWARE_VERSION_STRING[];
    static const char SET_SERIAL_PID_DESCRIPTION[];
    static const char TEMPERATURE_SENSOR_DESCRIPTION[];

    // our personalities
    static const rdm_personality rdm_personalities[];

    static const RDMHandler::pid_definition PID_DEFINITIONS[];
};

#endif  // RDM_HANDLERS_H
