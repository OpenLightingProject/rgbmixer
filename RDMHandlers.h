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
 * Copyright (C) 2010 Simon Newton
 */


#ifndef RDM_HANDLERS_H
#define RDM_HANDLERS_H

#include "WProgram.h"


/**
 * Setup the RDM Handling
 */
void SetupRDMHandling();

/*
 * Handle an RDM message
 * @param message pointer to a RDM message where the first byte is the sub star
 * code.
 * @param size the size of the message data.
 */
void HandleRDMMessage(byte *message, int size);


/**
 * Handle an RDM GET request
 */
void HandleRDMGet(int param_id, bool is_broadcast, int sub_device,
                  byte *message);


/**
 * Handle an RDM SET request
 */
void HandleRDMSet(int param_id, bool is_broadcast, int sub_device,
                  byte *message);


/**
 * Verify a RDM checksum
 * @param message a pointer to an RDM message starting with the SUB_START_CODE
 * @param size the size of the message data
 * @return true if the checksum is ok, false otherwise
 */
bool VerifyChecksum(byte *message, int size);


/**
 * Handle a GET SUPPORTED_PARAMETERS request
 */
void HandleGetSupportedParameters(byte *received_message);


/**
 * Handle a GET PARAMETER_DESCRIPTION request
 */
void HandleGetParameterDescription(byte *received_message);


/**
 * Handle a GET DEVICE_INFO request
 */
void HandleGetDeviceInfo(byte *received_message);

/**
 * Handle a GET PRODUCT_DETAIL_ID request
 */
void HandleProductDetailId(byte *received_message);


/**
 * Handle a GET DEVICE_LABEL request
 */
void HandleGetDeviceLabel(byte *received_message);


/**
 * Handle a GET SOFTWARE_VERSION_LABEL request
 */
void HandleGetSoftwareVersion(byte *received_message);


/**
 * Handle a GET DMX_START_ADDRESS request
 */
void HandleGetStartAddress(byte *received_message);



/**
 * Handle a GET SENSOR_DEFINITION request
 */
void HandleGetSensorDefinition(byte *received_message);


/**
 * Read the current temperature value
 */
int ReadTemperatureSensor();

void SendSensorResponse(byte *received_message);


/**
 * Handle a GET SENSOR_VALUE request
 */
void HandleGetSensorValue(byte *received_message);


/**
 * Handle a GET DEVICE_POWER_CYCLES request
 */
void HandleGetDevicePowerCycles(byte *received_message);


/**
 * Handle a GET IDENTIFY_DEVICE request
 */
void HandleGetIdentifyDevice(byte *received_message);


/**
 * Handle a SET DMX_START_ADDRESS request
 */
void HandleSetLanguage(bool was_broadcast, int sub_device,
                       byte *received_message);


/**
 * Handle a SET DMX_START_ADDRESS request
 */
void HandleSetDeviceLabel(bool was_broadcast, int sub_device,
                          byte *received_message);


/**
 * Handle a SET DMX_START_ADDRESS request
 */
void HandleSetStartAddress(bool was_broadcast,
                           int sub_device,
                           byte *received_message);


/**
 * Handle a SET SENSOR_VALUE request
 */
void HandleSetSensorValue(bool was_broadcast, int sub_device,
                          byte *received_message);

/*
 * Handle a SET RECORD_SENSORS request
 */
void HandleRecordSensor(bool was_broadcast, int sub_device,
                        byte *received_message);


/**
 * Handle a SET IDENTIFY_DEVICE request
 */
void HandleSetIdentifyDevice(bool was_broadcast, int sub_device,
                             byte *received_message);

/**
 * Handle a SET SERIAL_NUMBER request
 */
void HandleSetSerial(bool was_broadcast, int sub_device,
                     byte *received_message);

/**
 * Handle a GET request for a PID that returns a string
 *
 */
void HandleStringRequest(byte *received_message, char *label,
                         byte label_size);

#endif  // RDM_HANDLERS_H
