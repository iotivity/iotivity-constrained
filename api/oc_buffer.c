/*
// Copyright (c) 2016 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include "messaging/coap/engine.h"
#include "oc_signal_event_loop.h"
#include "port/oc_network_events_mutex.h"
#include "util/oc_memb.h"
#include <stdint.h>
#include <stdio.h>
#ifdef OC_DYNAMIC_ALLOCATION
#include "util/oc_mem.h"

#define MIN_DATA_SIZE (1 << 5) // 0b100000

#endif /* OC_DYNAMIC_ALLOCATION */

#ifdef OC_SECURITY
#include "security/oc_tls.h"
#endif /* OC_SECURITY */

#include "config.h"
#include "oc_buffer.h"
#include "oc_events.h"

OC_PROCESS(message_buffer_handler, "OC Message Buffer Handler");
OC_MEMB(oc_incoming_buffers, oc_message_t, OC_MAX_NUM_CONCURRENT_REQUESTS);
OC_MEMB(oc_outgoing_buffers, oc_message_t, OC_MAX_NUM_CONCURRENT_REQUESTS);

void
oc_data_unref(uint8_t *data)
{
#ifdef OC_DYNAMIC_ALLOCATION
  if (data)
    oc_mem_free(data);
#endif /* OC_DYNAMIC_ALLOCATION */
}

uint8_t *
oc_allocate_data(size_t size)
{
#ifdef OC_DYNAMIC_ALLOCATION
  // make 2^x  length
  size_t tunned_size = MIN_DATA_SIZE;

  if (size >= OC_PDU_SIZE) {
    tunned_size = OC_PDU_SIZE;
  } else {
    while (tunned_size < size) {
      tunned_size = tunned_size << 1;
      if (tunned_size > OC_PDU_SIZE) {
        tunned_size = OC_PDU_SIZE;
        break;
      }
    }
  }
  OC_DBG("oc_allocate_data:input size(%zu)-tunned_size(%zu)", size, tunned_size);

  return oc_mem_malloc(tunned_size);
#else
  return NULL;
#endif
}

static oc_message_t *
allocate_message(struct oc_memb *pool, bool flexible_data_flag)
{
  oc_network_event_handler_mutex_lock();
  oc_message_t *message = (oc_message_t *)oc_memb_alloc(pool);
  oc_network_event_handler_mutex_unlock();
  if (message) {
#ifdef OC_DYNAMIC_ALLOCATION
    if (!flexible_data_flag) {
      message->data = oc_mem_malloc(OC_PDU_SIZE);
      if (!message->data) {
        oc_memb_free(pool, message);
        return NULL;
      }
    } else {
      message->data = NULL;
    }
#endif /* OC_DYNAMIC_ALLOCATION */
    message->pool = pool;
    message->length = 0;
    message->next = 0;
    message->ref_count = 1;
    message->endpoint.interface_index = -1;
#ifndef OC_DYNAMIC_ALLOCATION
    OC_DBG("buffer: Allocated TX/RX buffer; num free: %d",
           oc_memb_numfree(pool));
#endif /* !OC_DYNAMIC_ALLOCATION */
  }
#ifndef OC_DYNAMIC_ALLOCATION
  else {
    OC_WRN("buffer: No free TX/RX buffers!");
  }
#endif /* !OC_DYNAMIC_ALLOCATION */
  return message;
}

oc_message_t *
oc_allocate_message_from_pool(struct oc_memb *pool)
{
  if (pool) {
    return allocate_message(pool, false);
  }
  return NULL;
}

void
oc_set_buffers_avail_cb(oc_memb_buffers_avail_callback_t cb)
{
  oc_memb_set_buffers_avail_cb(&oc_incoming_buffers, cb);
}

oc_message_t *
oc_allocate_message(void)
{
  return allocate_message(&oc_incoming_buffers, false);
}

oc_message_t *
oc_allocate_message_except_data(void)
{
  return allocate_message(&oc_incoming_buffers, true);
}

oc_message_t *
oc_internal_allocate_outgoing_message(void)
{
  return allocate_message(&oc_outgoing_buffers, false);
}

oc_message_t *
oc_internal_allocate_outgoing_message_except_data(void)
{
  return allocate_message(&oc_outgoing_buffers, true);
}

void
oc_message_add_ref(oc_message_t *message)
{
  if (message)
    message->ref_count++;
}

void
oc_message_unref(oc_message_t *message)
{
  if (message) {
    message->ref_count--;
    if (message->ref_count <= 0) {
#ifdef OC_DYNAMIC_ALLOCATION
      oc_mem_free(message->data);
#endif /* OC_DYNAMIC_ALLOCATION */
      struct oc_memb *pool = message->pool;
      oc_memb_free(pool, message);
#ifndef OC_DYNAMIC_ALLOCATION
      OC_DBG("buffer: freed TX/RX buffer; num free: %d", oc_memb_numfree(pool));
#endif /* !OC_DYNAMIC_ALLOCATION */
    }
  }
}

void
oc_recv_message(oc_message_t *message)
{
  if (oc_process_post(&message_buffer_handler, oc_events[INBOUND_NETWORK_EVENT],
                      message) == OC_PROCESS_ERR_FULL)
    oc_message_unref(message);
}

void
oc_send_message(oc_message_t *message)
{
  if (oc_process_post(&message_buffer_handler,
                      oc_events[OUTBOUND_NETWORK_EVENT],
                      message) == OC_PROCESS_ERR_FULL)
    message->ref_count--;

  _oc_signal_event_loop();
}

OC_PROCESS_THREAD(message_buffer_handler, ev, data)
{
  OC_PROCESS_BEGIN();
  OC_DBG("Started buffer handler process");
  while (1) {
    OC_PROCESS_YIELD();

    if (ev == oc_events[INBOUND_NETWORK_EVENT]) {
#ifdef OC_SECURITY
      uint8_t b = (uint8_t)((oc_message_t *)data)->data[0];
      if (b > 19 && b < 64) {
        OC_DBG("Inbound network event: encrypted request");
        oc_process_post(&oc_tls_handler, oc_events[UDP_TO_TLS_EVENT], data);
      } else {
        OC_DBG("Inbound network event: decrypted request");
        oc_process_post(&coap_engine, oc_events[INBOUND_RI_EVENT], data);
      }
#else  /* OC_SECURITY */
      OC_DBG("Inbound network event: decrypted request");
      oc_process_post(&coap_engine, oc_events[INBOUND_RI_EVENT], data);
#endif /* !OC_SECURITY */
    } else if (ev == oc_events[OUTBOUND_NETWORK_EVENT]) {
      oc_message_t *message = (oc_message_t *)data;
#ifndef ST_APP_OPTIMIZATION
#ifdef OC_CLIENT
      if (message->endpoint.flags & DISCOVERY) {
        OC_DBG("Outbound network event: multicast request");
        oc_send_discovery_request(message);
        oc_message_unref(message);
      } else
#endif /* OC_CLIENT */
#endif/*.ST_APP_OPTIMIZATION */
#ifdef OC_SECURITY
          if (message->endpoint.flags & SECURED) {
        OC_DBG("Outbound network event: forwarding to TLS");

#ifdef OC_CLIENT
        if (!oc_tls_connected(&message->endpoint)) {
          OC_DBG("Posting INIT_TLS_CONN_EVENT");
          oc_process_post(&oc_tls_handler, oc_events[INIT_TLS_CONN_EVENT],
                          data);
        } else
#endif /* OC_CLIENT */
        {
          OC_DBG("Posting RI_TO_TLS_EVENT");
          oc_process_post(&oc_tls_handler, oc_events[RI_TO_TLS_EVENT], data);
        }
      } else
#endif /* OC_SECURITY */
      {
        OC_DBG("Outbound network event: unicast message");
        oc_send_buffer(message);
        oc_message_unref(message);
      }
    }
  }
  OC_PROCESS_END();
}
