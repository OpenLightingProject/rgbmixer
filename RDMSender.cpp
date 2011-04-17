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
 * RDMSender.cpp
 * Copyright (C) 2011 Simon Newton
 */

#include "RDMSender.h"
#include "MessageLabels.h"
#include "WidgetSettings.h"


void RDMSender::ReturnRDMErrorResponse(byte error_code) const {
  m_sender->SendMessageHeader(RDM_LABEL, 1);
  m_sender->Write(error_code);
  m_sender->SendMessageFooter();
}


void RDMSender::SendByteAndChecksum(byte b) const {
  m_current_checksum += b;
  m_sender->Write(b);
}

void RDMSender::SendIntAndChecksum(int i) const {
  SendByteAndChecksum(i >> 8);
  SendByteAndChecksum(i);
}

void RDMSender::SendLongAndChecksum(long l) const {
  SendIntAndChecksum(l >> 16);
  SendIntAndChecksum(l);
}

/**
 * Send the RDM header
 */
void RDMSender::StartRDMResponse(const byte *received_message,
                                 rdm_response_type response_type,
                                 unsigned int param_data_size) const {
  // set the global checksum to 0
  m_current_checksum = 0;
  // size is the rdm status code, the rdm header + the param_data_size
  m_sender->SendMessageHeader(RDM_LABEL,
                              1 + MINIMUM_RDM_PACKET_SIZE + param_data_size);
  SendByteAndChecksum(RDM_STATUS_OK);
  SendByteAndChecksum(START_CODE);
  SendByteAndChecksum(SUB_START_CODE);
  SendByteAndChecksum(MINIMUM_RDM_PACKET_SIZE - 2 + param_data_size);

  // copy the src uid into the dst uid field
  SendByteAndChecksum(received_message[9]);
  SendByteAndChecksum(received_message[10]);
  SendByteAndChecksum(received_message[11]);
  SendByteAndChecksum(received_message[12]);
  SendByteAndChecksum(received_message[13]);
  SendByteAndChecksum(received_message[14]);

  // add our UID as the src, the ESTA_ID & fields are reversed
  SendIntAndChecksum(WidgetSettings.EstaId());
  SendLongAndChecksum(WidgetSettings.SerialNumber());

  SendByteAndChecksum(received_message[15]);  // transaction #
  SendByteAndChecksum(response_type);  // response type
  SendByteAndChecksum(0);  // message count

  // sub device
  SendByteAndChecksum(received_message[18]);
  SendByteAndChecksum(received_message[19]);

  // command class
  if (received_message[20] == GET_COMMAND) {
    SendByteAndChecksum(GET_COMMAND_RESPONSE);
  } else {
    SendByteAndChecksum(SET_COMMAND_RESPONSE);
  }

  // param id, we don't use queued messages so this always matches the request
  SendByteAndChecksum(received_message[21]);
  SendByteAndChecksum(received_message[22]);
  SendByteAndChecksum(param_data_size);
}


void RDMSender::StartRDMAckResponse(const byte *received_message,
                                    unsigned int param_data_size) const {
  StartRDMResponse(received_message, RDM_RESPONSE_ACK, param_data_size);
}


void RDMSender::EndRDMResponse() const {
  m_sender->Write(m_current_checksum >> 8);
  m_sender->Write(m_current_checksum);
  m_sender->SendMessageFooter();
}


void RDMSender::SendEmptyAck(const byte *received_message) const {
  StartRDMAckResponse(received_message, 0);
  EndRDMResponse();
}


/**
 * Send a Nack response
 * @param received_message a pointer to the received RDM message
 * @param nack_reason the NACK reasons
 */
void RDMSender::SendNack(const byte *received_message,
                        rdm_nack_reason nack_reason) const {
  StartRDMResponse(received_message, RDM_RESPONSE_NACK, 2);
  SendIntAndChecksum(nack_reason);
  EndRDMResponse();
}


void RDMSender::NackOrBroadcast(bool was_broadcast,
                                const byte *received_message,
                                rdm_nack_reason nack_reason) const {
  if (was_broadcast)
    ReturnRDMErrorResponse(RDM_STATUS_BROADCAST);
  else
    SendNack(received_message, nack_reason);
}
