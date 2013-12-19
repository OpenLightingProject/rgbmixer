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
 * RDMSender.h
 * Copyright (C) 2011 Simon Newton
 */

#ifndef RDM_SENDER_H
#define RDM_SENDER_H

#include "Arduino.h"
#include "RDMEnums.h"
#include "UsbProSender.h"

/**
 * Sends a properly framed RDM message over the serial link
 */
class RDMSender {
  public:
    explicit RDMSender(const UsbProSender *sender)
      : m_sender(sender),
        m_message_count(0),
        m_current_checksum(0) {}

    void ReturnRDMErrorResponse(byte error_code) const;

    void StartRDMResponse(const byte *received_message,
                          rdm_response_type response_type,
                          unsigned int param_data_size) const;
    void StartCustomResponse(const byte *received_message,
                             rdm_response_type response_type,
                             unsigned int param_data_size,
                             byte command_class,
                             int pid) const;
    void StartRDMAckResponse(const byte *received_message,
                             unsigned int param_data_size) const;
    void SendByteAndChecksum(byte b) const;
    void SendIntAndChecksum(int i) const;
    void SendLongAndChecksum(long l) const;
    void EndRDMResponse() const;

    // helper method to send acks
    void SendEmptyAck(const byte *received_message) const;

    // helper method to send ack timers
    void SendAckTimer(const byte *received_message,
                      int response_time) const;

    // helper method to send nacks
    void SendNack(const byte *received_message,
                  rdm_nack_reason nack_reason) const;
    // helper method to either send a nack, or a broadcast response code
    void NackOrBroadcast(bool was_broadcast,
                         const byte *received_message,
                         rdm_nack_reason nack_reason) const;

    void IncrementMessageCount();
    void DecrementMessageCount();

  private:
    const UsbProSender *m_sender;
    byte m_message_count;
    mutable unsigned int m_current_checksum;
};
#endif  // RDM_SENDER_H
