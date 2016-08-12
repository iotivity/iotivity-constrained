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

/*
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

#include <string.h>
#include "util/oc_memb.h"
#include "util/oc_list.h"
#include "transactions.h"
#include "observe.h"
#include "oc_buffer.h"

#ifdef OC_CLIENT
#include "oc_client_state.h"
#endif /* OC_CLIENT */

#ifdef OC_SECURITY
#include "security/oc_dtls.h"
#endif

/*---------------------------------------------------------------------------*/
OC_MEMB(transactions_memb, coap_transaction_t, COAP_MAX_OPEN_TRANSACTIONS);
OC_LIST(transactions_list);

static struct oc_process *transaction_handler_process = NULL;

/*---------------------------------------------------------------------------*/
/*- Internal API ------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
void coap_register_as_transaction_handler()
{
  transaction_handler_process = OC_PROCESS_CURRENT();
}

coap_transaction_t *
coap_new_transaction(uint16_t mid, oc_endpoint_t *endpoint)
{
  coap_transaction_t *t = oc_memb_alloc(&transactions_memb);
  if(t) {
    oc_message_t *message = oc_allocate_message();
    if(message) {
      LOG("Created new transaction %d %d\n", mid,
	  message->length);
      t->mid = mid;
      t->retrans_counter = 0;

      t->message = message;

      /* save client address */
      memcpy(&t->message->endpoint, endpoint,
	     sizeof(oc_endpoint_t));

      oc_list_add(transactions_list, t); /* list itself makes sure same element is not added twice */
    } else {
      oc_memb_free(&transactions_memb, t);
      t = NULL;
    }
  }

  return t;
}

/*---------------------------------------------------------------------------*/
void coap_send_transaction(coap_transaction_t *t)
{
  LOG("Sending transaction %u\n", t->mid);
  bool confirmable = false;

  confirmable = (COAP_TYPE_CON
		 == ((COAP_HEADER_TYPE_MASK & t->message->data[0])
		     >> COAP_HEADER_TYPE_POSITION)) ?
    true : false;

  if(confirmable) {
    if(t->retrans_counter < COAP_MAX_RETRANSMIT) {
      /* not timed out yet */
      LOG("Keeping transaction %u\n", t->mid);

      if(t->retrans_counter == 0) {
	t->retrans_timer.timer.interval =
	  COAP_RESPONSE_TIMEOUT_TICKS
	  + (oc_random_rand()
	     % (oc_clock_time_t)
	     COAP_RESPONSE_TIMEOUT_BACKOFF_MASK );
	LOG("Initial interval %d\n", t->retrans_timer.timer.interval);
      } else {
	t->retrans_timer.timer.interval <<= 1; /* double */
	LOG("Doubled %d\n", t->retrans_timer.timer.interval);
      }

      OC_PROCESS_CONTEXT_BEGIN(transaction_handler_process);
      oc_etimer_restart(&t->retrans_timer); /* interval updated above */
      OC_PROCESS_CONTEXT_END(transaction_handler_process);

      coap_send_message(t->message);

      oc_message_add_ref(t->message);

      t = NULL;
    } else {
      /* timed out */
      LOG("Timeout\n");

#ifdef OC_SERVER
      LOG("timeout.. so removing observers\n");
      /* handle observers */
      coap_remove_observer_by_client(&t->message->endpoint);
#endif /* OC_SERVER */

#ifdef OC_SECURITY
      if (t->message->endpoint.flags & SECURED) {
	oc_sec_dtls_close_init(&t->message->endpoint);
      }
#endif /* OC_SECURITY */

#ifdef OC_CLIENT
      oc_ri_remove_client_cb_by_mid(t->mid);
#endif /* OC_CLIENT */

      coap_clear_transaction(t);
    }
  } else {
    coap_send_message(t->message);
    oc_message_add_ref(t->message);

    coap_clear_transaction(t);
  }
}
/*---------------------------------------------------------------------------*/
void coap_clear_transaction(coap_transaction_t *t)
{
  if(t) {
    LOG("Freeing transaction %u: %p\n", t->mid, t);

    oc_etimer_stop(&t->retrans_timer);
    oc_message_unref(t->message);
    oc_list_remove(transactions_list, t);
    oc_memb_free(&transactions_memb, t);
  }
}
coap_transaction_t *
coap_get_transaction_by_mid(uint16_t mid)
{
  coap_transaction_t *t = NULL;

  for(t = (coap_transaction_t *)oc_list_head(transactions_list); t;
      t = t->next) {
    if(t->mid == mid) {
      LOG("Found transaction for MID %u: %p\n", t->mid, t);
      return t;
    }
  }
  return NULL;
}

/*---------------------------------------------------------------------------*/
void coap_check_transactions()
{
  coap_transaction_t *t = NULL;

  for(t = (coap_transaction_t *)oc_list_head(transactions_list); t;
      t = t->next) {
    if(oc_etimer_expired(&t->retrans_timer)) {
      ++(t->retrans_counter);
      LOG("Retransmitting %u (%u)\n", t->mid, t->retrans_counter);
      coap_send_transaction(t);
    }
  }
}
/*---------------------------------------------------------------------------*/
