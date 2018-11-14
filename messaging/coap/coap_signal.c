/****************************************************************************
 *
 * Copyright 2018 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/

#include "coap_signal.h"
#include "coap.h"
#include "transactions.h"
#include <string.h>

static void
coap_make_token(coap_packet_t *packet)
{
  packet->token_len = 8;
  int i = 0;
  uint32_t r;
  while (i < packet->token_len) {
    r = oc_random_value();
    memcpy(packet->token + i, &r, sizeof(r));
    i += sizeof(r);
  }
}

static int
coap_send_signal_message(oc_endpoint_t *endpoint, coap_packet_t *packet)
{
  oc_message_t *message = oc_internal_allocate_outgoing_message();
  if (!message) {
    OC_ERR("message alloc failed.");
    return 0;
  }

  memcpy(&message->endpoint, endpoint, sizeof(oc_endpoint_t));

  message->length = coap_serialize_message(packet, message->data);
  oc_send_message(message);

  return 1;
}

int
coap_send_csm_message(oc_endpoint_t *endpoint, uint8_t bert_enable)
{
  if (!endpoint)
    return 0;

  coap_packet_t csm_pkt[1];
  coap_tcp_init_message(csm_pkt, CSM_7_01);

  coap_make_token(csm_pkt);

  if (!coap_signal_set_max_msg_size(csm_pkt, OC_PDU_SIZE)) {
    OC_ERR("coap_signal_set_max_msg_size failed");
    return 0;
  }

  if (bert_enable) {
    if (!coap_signal_set_bert(csm_pkt, bert_enable)) {
      OC_ERR("coap_signal_set_bert failed");
      return 0;
    }
  }

  OC_DBG("send csm signal message.");
  return coap_send_signal_message(endpoint, csm_pkt);
}

int
coap_send_ping_message(oc_endpoint_t *endpoint, uint8_t custody_enable,
                       uint8_t *token, uint8_t token_len)
{
  if (!endpoint || !token || token_len == 0)
    return 0;

  coap_packet_t ping_pkt[1];
  coap_tcp_init_message(ping_pkt, PING_7_02);

  coap_set_token(ping_pkt, token, token_len);

  if (custody_enable) {
    if (!coap_signal_set_custody(ping_pkt, custody_enable)) {
      OC_ERR("coap_signal_set_custody failed");
      return 0;
    }
  }

  coap_transaction_t *t = coap_new_transaction(0, endpoint);
  if (!t) {
    return 0;
  }
  t->message->length = coap_serialize_message(ping_pkt, t->message->data);

  OC_DBG("send ping signal message.");
  coap_send_transaction(t);

  return 1;
}

int
coap_send_pong_message(oc_endpoint_t *endpoint, void *packet)
{
  if (!endpoint || !packet)
    return 0;

  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;
  coap_packet_t pong_pkt[1];
  coap_tcp_init_message(pong_pkt, PONG_7_03);

  coap_set_token(pong_pkt, coap_pkt->token, coap_pkt->token_len);

  if (coap_pkt->custody) {
    if (!coap_signal_set_custody(pong_pkt, coap_pkt->custody)) {
      OC_ERR("coap_signal_set_custody failed");
      return 0;
    }
  }

  OC_DBG("send pong signal message.");
  return coap_send_signal_message(endpoint, pong_pkt);
}

int
coap_send_release_message(oc_endpoint_t *endpoint, const char *alt_addr,
                          size_t alt_addr_len, uint32_t hold_off)
{
  if (!endpoint)
    return 0;

  coap_packet_t release_pkt[1];
  coap_tcp_init_message(release_pkt, RELEASE_7_04);

  coap_make_token(release_pkt);

  if (alt_addr && alt_addr_len > 0) {
    if (!coap_signal_set_alt_addr(release_pkt, alt_addr, alt_addr_len)) {
      OC_ERR("coap_signal_set_alt_addr failed");
      return 0;
    }
  }

  if (hold_off > 0) {
    if (!coap_signal_set_hold_off(release_pkt, hold_off)) {
      OC_ERR("coap_signal_set_hold_off failed");
      return 0;
    }
  }

  OC_DBG("send release signal message.");
  return coap_send_signal_message(endpoint, release_pkt);
}

int
coap_send_abort_message(oc_endpoint_t *endpoint, uint16_t opt,
                        const char *diagnostic, size_t diagnostic_len)
{
  if (!endpoint)
    return 0;

  coap_packet_t abort_pkt[1];
  coap_tcp_init_message(abort_pkt, ABORT_7_05);

  coap_make_token(abort_pkt);

  if (opt != 0) {
    if (!coap_signal_set_bad_csm(abort_pkt, opt)) {
      OC_ERR("coap_signal_set_bad_csm failed");
      return 0;
    }
  }

  if (diagnostic && diagnostic_len > 0) {
    if (!coap_set_payload(abort_pkt, (uint8_t *)diagnostic, diagnostic_len)) {
      OC_ERR("coap_set_payload failed");
      return 0;
    }
  }

  OC_DBG("send abort signal message.");
  return coap_send_signal_message(endpoint, abort_pkt);
}

int
coap_check_is_signal_message(void *packet)
{
  if (!packet)
    return 0;

  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;
  if (coap_pkt->code == CSM_7_01 || coap_pkt->code == PING_7_02 ||
      coap_pkt->code == PONG_7_03 || coap_pkt->code == RELEASE_7_04 ||
      coap_pkt->code == ABORT_7_05)
    return 1;

  return 0;
}

int
handle_coap_signal_message(void *packet, oc_endpoint_t *endpoint)
{
  coap_packet_t *const coap_pkt = (coap_packet_t *)packet;

  OC_DBG("Coap signal message received.(code: %d)", coap_pkt->code);
  if (coap_pkt->code == CSM_7_01) {
    tcp_csm_state_t state = oc_tcp_get_csm_state(endpoint);
    if (state == CSM_DONE) {
      // TODO: max-message-size, bert handling
      return COAP_NO_ERROR;
    } else if (state == CSM_NONE) {
      coap_send_csm_message(endpoint, 0);
    }
    oc_tcp_update_csm_state(endpoint, CSM_DONE);
  } else if (coap_pkt->code == PING_7_02) {
    coap_send_pong_message(endpoint, packet);
  } else if (coap_pkt->code == PONG_7_03) {
  } else if (coap_pkt->code == RELEASE_7_04) {
    // alternative address
    // hold off
    oc_connectivity_end_session(endpoint);
  } else if (coap_pkt->code == ABORT_7_05) {
    OC_WRN("Peer aborted! [code: %d(diagnostic: %*.s)]", coap_pkt->bad_csm_opt,
           coap_pkt->payload_len, (char *)coap_pkt->payload);
  }

  return COAP_NO_ERROR;
}