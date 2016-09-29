/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Copyright (c) 2013, Institute for Pervasive Computing, ETH Zurich
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 */

#include "engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* OIC Stack headers */
#include "api/oc_events.h"
#include "oc_buffer.h"
#include "oc_ri.h"

#ifdef OC_CLIENT
#include "oc_client_state.h"
#endif

OC_PROCESS(coap_engine, "CoAP Engine");

extern bool oc_ri_invoke_coap_entity_handler(void *request, void *response,
                                             uint8_t *buffer,
                                             uint16_t buffer_size,
                                             int32_t *offset,
                                             oc_endpoint_t *endpoint);

/*---------------------------------------------------------------------------*/
/*- Internal API ------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
int
coap_receive(oc_message_t *msg)
{
  erbium_status_code = NO_ERROR;

  LOG("\n\nCoAP Engine: received datalen=%u \n", msg->length);

  /* static declaration reduces stack peaks and program code size */
  static coap_packet_t
    message[1]; /* this way the packet can be treated as pointer as usual */
  static coap_packet_t response[1];
  static coap_transaction_t *transaction = NULL;

  erbium_status_code = coap_parse_message(message, msg->data, msg->length);

  if (erbium_status_code == NO_ERROR) {

/*TODO duplicates suppression, if required by application */

#if OC_DEBUG
    LOG("  Parsed: CoAP version: %u, token: 0x%02X%02X, mid: %u\n",
        message->version, message->token[0], message->token[1], message->mid);
    switch (message->type) {
    case COAP_TYPE_CON:
      LOG("  type: CON\n");
      break;
    case COAP_TYPE_NON:
      LOG("  type: NON\n");
      break;
    case COAP_TYPE_ACK:
      LOG("  type: ACK\n");
      break;
    case COAP_TYPE_RST:
      LOG("  type: RST\n");
      break;
    default:
      break;
    }
#endif

    /* handle requests */
    if (message->code >= COAP_GET && message->code <= COAP_DELETE) {

#if OC_DEBUG
      switch (message->code) {
      case COAP_GET:
        LOG("  method: GET\n");
        break;
      case COAP_PUT:
        LOG("  method: PUT\n");
        break;
      case COAP_POST:
        LOG("  method: POST\n");
        break;
      case COAP_DELETE:
        LOG("  method: DELETE\n");
        break;
      }
      LOG("  URL: %.*s\n", message->uri_path_len, message->uri_path);
      LOG("  Payload: %.*s\n", message->payload_len, message->payload);
#endif
      /* use transaction buffer for response to confirmable request */
      if ((transaction = coap_new_transaction(message->mid, &msg->endpoint))) {
        uint32_t block_num = 0;
        uint16_t block_size = COAP_MAX_BLOCK_SIZE;
        uint32_t block_offset = 0;
        int32_t new_offset = 0;

        /* prepare response */
        if (message->type == COAP_TYPE_CON) {
          /* reliable CON requests are answered with an ACK */
          coap_init_message(response, COAP_TYPE_ACK, CONTENT_2_05,
                            message->mid);
        } else {
          /* unreliable NON requests are answered with a NON as well */
          coap_init_message(response, COAP_TYPE_NON, CONTENT_2_05,
                            coap_get_mid());
          /* mirror token */
        }
        if (message->token_len) {
          coap_set_token(response, message->token, message->token_len);
          /* get offset for blockwise transfers */
        }
        if (coap_get_header_block2(message, &block_num, NULL, &block_size,
                                   &block_offset)) {
          LOG("\tBlockwise: block request %u (%u/%u) @ %u bytes\n", block_num,
              block_size, COAP_MAX_BLOCK_SIZE, block_offset);
          block_size = MIN(block_size, COAP_MAX_BLOCK_SIZE);
          new_offset = block_offset;
        }

        /* invoke resource handler in RI layer */
        if (oc_ri_invoke_coap_entity_handler(
              message, response,
              transaction->message->data + COAP_MAX_HEADER_SIZE, block_size,
              &new_offset, &msg->endpoint)) {

          if (erbium_status_code == NO_ERROR) {

            /* TODO coap_handle_blockwise(request, response, start_offset,
             * end_offset); */

            /* resource is unaware of Block1 */
            if (IS_OPTION(message, COAP_OPTION_BLOCK1) &&
                response->code < BAD_REQUEST_4_00 &&
                !IS_OPTION(response, COAP_OPTION_BLOCK1)) {
              LOG("\tBlock1 option NOT IMPLEMENTED\n");

              erbium_status_code = NOT_IMPLEMENTED_5_01;
              coap_error_message = "NoBlock1Support";

              /* client requested Block2 transfer */
            } else if (IS_OPTION(message, COAP_OPTION_BLOCK2)) {

              /* unchanged new_offset indicates that resource is unaware of
               * blockwise transfer */
              if (new_offset == block_offset) {
                LOG("\tBlockwise: unaware resource with payload length %u/%u\n",
                    response->payload_len, block_size);
                if (block_offset >= response->payload_len) {
                  LOG("\t\t: block_offset >= response->payload_len\n");

                  response->code = BAD_OPTION_4_02;
                  coap_set_payload(response, "BlockOutOfScope",
                                   15); /* a const char str[] and sizeof(str)
                                           produces larger code size */
                } else {
                  coap_set_header_block2(response, block_num,
                                         response->payload_len - block_offset >
                                           block_size,
                                         block_size);
                  coap_set_payload(
                    response, response->payload + block_offset,
                    MIN(response->payload_len - block_offset, block_size));
                } /* if(valid offset) */

                /* resource provides chunk-wise data */
              } else {
                LOG("\tBlockwise: blockwise resource, new offset %d\n",
                    new_offset);
                coap_set_header_block2(response, block_num,
                                       new_offset != -1 ||
                                         response->payload_len > block_size,
                                       block_size);

                if (response->payload_len > block_size) {
                  coap_set_payload(response, response->payload, block_size);
                }
              } /* if(resource aware of blockwise) */

              /* Resource requested Block2 transfer */
            } else if (new_offset != 0) {
              LOG("\tBlockwise: no block option for blockwise resource, using "
                  "block size %u\n",
                  COAP_MAX_BLOCK_SIZE);

              coap_set_header_block2(response, 0, new_offset != -1,
                                     COAP_MAX_BLOCK_SIZE);
              coap_set_payload(response, response->payload,
                               MIN(response->payload_len, COAP_MAX_BLOCK_SIZE));
            } /* blockwise transfer handling */
          }   /* no errors/hooks */
          /* successful service callback */
          /* serialize response */
        }
        if (erbium_status_code == NO_ERROR) {
          if ((transaction->message->length = coap_serialize_message(
                 response, transaction->message->data)) == 0) {
            erbium_status_code = PACKET_SERIALIZATION_ERROR;
          }
        }
      } else {
        erbium_status_code = SERVICE_UNAVAILABLE_5_03;
        coap_error_message = "NoFreeTraBuffer";
      } /* if(transaction buffer) */

      /* handle responses */
    } else { // Fix this
      if (message->type == COAP_TYPE_CON) {
        erbium_status_code = EMPTY_ACK_RESPONSE;
      } else if (message->type == COAP_TYPE_ACK) {
        /* transactions are closed through lookup below */
      } else if (message->type == COAP_TYPE_RST) {
#ifdef OC_SERVER
        /* cancel possible subscriptions */
        coap_remove_observer_by_mid(&msg->endpoint, message->mid);
#endif
      }

      /* Open transaction now cleared for ACK since mid matches */
      if ((transaction = coap_get_transaction_by_mid(message->mid))) {
        coap_clear_transaction(transaction);
      }
      /* if(ACKed transaction) */
      transaction = NULL;

#ifdef OC_CLIENT // ACKs and RSTs sent to oc_ri.. RSTs cleared, ACKs sent to
                 // client
      oc_ri_invoke_client_cb(message, &msg->endpoint);
#endif

    } /* request or response */
  }   /* parsed correctly */

  /* if(parsed correctly) */
  if (erbium_status_code == NO_ERROR) {
    if (transaction) { // Server transactions sent from here
      coap_send_transaction(transaction);
    }
  } else if (erbium_status_code == CLEAR_TRANSACTION) {
    LOG("Clearing transaction for manual response");
    coap_clear_transaction(transaction); // used in server for separate response
  }
#ifdef OC_CLIENT
  else if (erbium_status_code == EMPTY_ACK_RESPONSE) {
    coap_init_message(message, COAP_TYPE_ACK, 0, message->mid);
    oc_message_t *response = oc_allocate_message();
    if (response) {
      memcpy(&response->endpoint, &msg->endpoint, sizeof(msg->endpoint));
      response->length = coap_serialize_message(message, response->data);
      coap_send_message(response);
    }
  }
#endif /* OC_CLIENT */
#ifdef OC_SERVER
  else { // framework errors handled here
    coap_message_type_t reply_type = COAP_TYPE_RST;

    coap_clear_transaction(transaction);

    coap_init_message(message, reply_type, SERVICE_UNAVAILABLE_5_03,
                      message->mid);

    oc_message_t *response = oc_allocate_message();
    if (response) {
      memcpy(&response->endpoint, &msg->endpoint, sizeof(msg->endpoint));
      response->length = coap_serialize_message(message, response->data);
      coap_send_message(response);
    }
  }
#endif /* OC_SERVER */

  /* if(new data) */
  return erbium_status_code;
}
/*---------------------------------------------------------------------------*/
void
coap_init_engine(void)
{
  coap_register_as_transaction_handler();
}
/*---------------------------------------------------------------------------*/
OC_PROCESS_THREAD(coap_engine, ev, data)
{
  OC_PROCESS_BEGIN();

  coap_register_as_transaction_handler();
  coap_init_connection();

  while (1) {
    OC_PROCESS_YIELD();

    if (ev == oc_events[INBOUND_RI_EVENT]) {
      coap_receive(data);

      oc_message_unref(data);
    } else if (ev == OC_PROCESS_EVENT_TIMER) {
      coap_check_transactions();
    }
  }

  OC_PROCESS_END();
}

/*---------------------------------------------------------------------------*/
