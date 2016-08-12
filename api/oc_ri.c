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

#include <stddef.h>
#include <strings.h>
#include <stdbool.h>

#include "util/oc_process.h"
#include "util/oc_etimer.h"
#include "util/oc_list.h"
#include "util/oc_memb.h"

#include "messaging/coap/engine.h"
#include "messaging/coap/constants.h"
#include "messaging/coap/oc_coap.h"

#include "port/oc_random.h"

#include "oc_buffer.h"
#include "oc_events.h"
#include "oc_ri.h"
#include "oc_uuid.h"
#include "oc_core_res.h"
#include "oc_discovery.h"
#include "oc_network_events.h"

#ifdef OC_SECURITY
#include "security/oc_dtls.h"
#include "security/oc_acl.h"
#endif /* OC_SECURITY */

#ifdef OC_SERVER
OC_LIST(app_resources);
OC_LIST(observe_callbacks);
OC_MEMB(app_resources_s, oc_resource_t, MAX_APP_RESOURCES);
#endif /* OC_SERVER */

#ifdef OC_CLIENT
#include "oc_client_state.h"
OC_LIST(client_cbs);
OC_MEMB(client_cbs_s, oc_client_cb_t, MAX_NUM_CONCURRENT_REQUESTS);
#endif /* OC_CLIENT */

OC_LIST(timed_callbacks);
OC_MEMB(event_callbacks_s, oc_event_callback_t, 3 * MAX_APP_RESOURCES);
OC_PROCESS(timed_callback_events, "OC timed callbacks");

//TODO: Define and use a  complete set of error codes.
int oc_stack_errno;

static unsigned int oc_coap_status_codes[__NUM_OC_STATUS_CODES__ ];

static void
set_mpro_status_codes()
{
  /* OK_200 */
  oc_coap_status_codes[OK] = CONTENT_2_05;
  /* CREATED_201 */
  oc_coap_status_codes[CREATED] = CREATED_2_01;
  /* NO_CONTENT_204 */
  oc_coap_status_codes[CHANGED] = CHANGED_2_04;
  /* NO_CONTENT_204 */
  oc_coap_status_codes[DELETED] = DELETED_2_02;
  /* NOT_MODIFIED_304 */
  oc_coap_status_codes[NOT_MODIFIED] = VALID_2_03;
  /* BAD_REQUEST_400 */
  oc_coap_status_codes[BAD_REQUEST] = BAD_REQUEST_4_00;
  /* UNAUTHORIZED_401 */
  oc_coap_status_codes[UNAUTHORIZED] = UNAUTHORIZED_4_01;
  /* BAD_REQUEST_400 */
  oc_coap_status_codes[BAD_OPTION] = BAD_OPTION_4_02;
  /* FORBIDDEN_403 */
  oc_coap_status_codes[FORBIDDEN] = FORBIDDEN_4_03;
  /* NOT_FOUND_404 */
  oc_coap_status_codes[NOT_FOUND] = NOT_FOUND_4_04;
  /* METHOD_NOT_ALLOWED_405 */
  oc_coap_status_codes[METHOD_NOT_ALLOWED] = METHOD_NOT_ALLOWED_4_05;
  /* NOT_ACCEPTABLE_406 */
  oc_coap_status_codes[NOT_ACCEPTABLE] = NOT_ACCEPTABLE_4_06;
  /* REQUEST_ENTITY_TOO_LARGE_413 */
  oc_coap_status_codes[REQUEST_ENTITY_TOO_LARGE] =
    REQUEST_ENTITY_TOO_LARGE_4_13;
  /* UNSUPPORTED_MEDIA_TYPE_415 */
  oc_coap_status_codes[UNSUPPORTED_MEDIA_TYPE] =
    UNSUPPORTED_MEDIA_TYPE_4_15;
  /* INTERNAL_SERVER_ERROR_500 */
  oc_coap_status_codes[INTERNAL_SERVER_ERROR] =
    INTERNAL_SERVER_ERROR_5_00;
  /* NOT_IMPLEMENTED_501 */
  oc_coap_status_codes[NOT_IMPLEMENTED] = NOT_IMPLEMENTED_5_01;
  /* BAD_GATEWAY_502 */
  oc_coap_status_codes[BAD_GATEWAY] = BAD_GATEWAY_5_02;
  /* SERVICE_UNAVAILABLE_503 */
  oc_coap_status_codes[SERVICE_UNAVAILABLE] = SERVICE_UNAVAILABLE_5_03;
  /* GATEWAY_TIMEOUT_504 */
  oc_coap_status_codes[GATEWAY_TIMEOUT] = GATEWAY_TIMEOUT_5_04;
  /* INTERNAL_SERVER_ERROR_500 */
  oc_coap_status_codes[PROXYING_NOT_SUPPORTED] =
    PROXYING_NOT_SUPPORTED_5_05;
}

#ifdef OC_SERVER
oc_resource_t *
oc_ri_get_app_resources()
{
  return oc_list_head(app_resources);
}
#endif

int
oc_status_code(oc_status_t key)
{
  return oc_coap_status_codes[key];
}

int
oc_ri_get_query_nth_key_value(const char *query, int query_len,
			      char **key, int *key_len, char **value,
			      int *value_len, int n)
{
  int next_pos = -1;
  int i = 0;
  char *start = (char*)query, *current, *end = (char*)query + query_len;
  current = start;

  while (i < (n - 1) && current != NULL)
    {
      current = memchr(start, '&', end - start);
      i++;
      start = current + 1;
    }

  current = memchr(start, '=', end - start);
  if (current != NULL)
    {
      *key_len = current - start;
      *key = start;
      *value = current + 1;
      current = memchr(*value, '&', end - *value);
      if (current == NULL) {
	*value_len = end - *value;
      }
      else {
	*value_len = current - *value;
      }
      next_pos = *value + *value_len - query + 1;
    }
  return next_pos;
}

int
oc_ri_get_query_value(const char *query, int query_len,
		      const char *key,  char **value)
{
  int next_pos = 0, found = -1, kl, vl;
  char *k;
  while (next_pos < query_len) {
    next_pos = oc_ri_get_query_nth_key_value(query + next_pos,
					     query_len - next_pos, &k, &kl,
					     value, &vl, 1);
    if (kl == strlen(key) && strncasecmp(key, k, kl) == 0) {
      found = vl;
      break;
    }
  }
  return found;
}

static void
allocate_events()
{
  int i = 0;
  for(i = 0; i < __NUM_OC_EVENT_TYPES__; i++) {
    oc_events[i] = oc_process_alloc_event();
  }
}

static void
start_processes()
{
  allocate_events();
  oc_process_start(&oc_etimer_process, NULL);
  oc_process_start(&timed_callback_events, NULL);
  oc_process_start(&coap_engine, NULL);
  oc_process_start(&message_buffer_handler, NULL);

#ifdef OC_SECURITY
  oc_process_start(&oc_dtls_handler, NULL);
#endif

  oc_process_start(&oc_network_events, NULL);
}

static void
stop_processes()
{
  oc_process_exit(&oc_etimer_process);
  oc_process_exit(&timed_callback_events);
  oc_process_exit(&coap_engine);

#ifdef OC_SECURITY
  oc_process_exit(&oc_dtls_handler);
#endif

  oc_process_exit(&message_buffer_handler);
}

#ifdef OC_SERVER
oc_resource_t *
oc_ri_get_app_resource_by_uri(const char *uri)
{
  oc_resource_t *res = oc_ri_get_app_resources();
  while (res != NULL) {
    if (oc_string_len(res->uri) == strlen(uri) &&
	strncmp(uri, oc_string(res->uri), strlen(uri)) == 0)
      return res;
    res = res->next;
  }
  return res;
}
#endif

void
oc_ri_init()
{
  oc_random_init(0); //Fix: allow user to seed RNG.
  oc_clock_init();
  set_mpro_status_codes();

#ifdef OC_SERVER
  oc_list_init(app_resources);
  oc_list_init(observe_callbacks);
#endif

#ifdef OC_CLIENT
  oc_list_init(client_cbs);
#endif

  oc_list_init(timed_callbacks);
  start_processes();
  oc_create_discovery_resource();
}

void
oc_ri_shutdown()
{
  oc_random_destroy();
  stop_processes();
}

#ifdef OC_SERVER
oc_resource_t *
oc_ri_alloc_resource()
{
  return oc_memb_alloc(&app_resources_s);
}

void
oc_ri_delete_resource(oc_resource_t *resource)
{
  oc_memb_free(&app_resources_s, resource);
}

bool
oc_ri_add_resource(oc_resource_t *resource)
{
  bool valid = true;

  if(!resource->get_handler && !resource->put_handler
     && !resource->post_handler && !resource->delete_handler)
    valid = false;

  if(resource->properties & OC_PERIODIC
     && resource->observe_period_seconds == 0)
    valid = false;

  if(valid) {
    oc_list_add(app_resources, resource);
  }

  return valid;
}
#endif /* OC_SERVER */

void
oc_ri_add_timed_event_callback_ticks(void *cb_data,
				     oc_trigger_t event_callback,
				     uint16_t ticks)
{
  oc_event_callback_t *event_cb =
    (oc_event_callback_t*)oc_memb_alloc(&event_callbacks_s);

  if(event_cb) {
    event_cb->data = cb_data;
    event_cb->callback = event_callback;
    OC_PROCESS_CONTEXT_BEGIN(&timed_callback_events);
    oc_etimer_set(&event_cb->timer, ticks);
    OC_PROCESS_CONTEXT_END(&timed_callback_events);
    oc_list_add(timed_callbacks, event_cb);
  }
}

static void
poll_event_callback_timers(oc_list_t list,
			   struct oc_memb *cb_pool)
{
  oc_event_callback_t *event_cb =
    (oc_event_callback_t*)oc_list_head(list), *next;

  while(event_cb != NULL) {
    next = event_cb->next;

    if(oc_etimer_expired(&event_cb->timer)) {
      if(event_cb->callback(event_cb->data) == DONE) {
	oc_list_remove(list, event_cb);
	oc_memb_free(cb_pool, event_cb);
      } else {
	OC_PROCESS_CONTEXT_BEGIN(&timed_callback_events);
	oc_etimer_restart(&event_cb->timer);
	OC_PROCESS_CONTEXT_END(&timed_callback_events);
      }
    }

    event_cb = next;
  }
}

static void
check_event_callbacks()
{
#ifdef OC_SERVER
  poll_event_callback_timers(observe_callbacks, &event_callbacks_s);
#endif /* OC_SERVER */
  poll_event_callback_timers(timed_callbacks, &event_callbacks_s);
}

#ifdef OC_SERVER
static oc_event_callback_retval_t
periodic_observe_handler(void *data)
{
  oc_resource_t *resource = (oc_resource_t*)data;

  if(coap_notify_observers(resource, NULL, NULL)) {
    return CONTINUE;
  }

  return DONE;
}

static oc_event_callback_t*
get_periodic_observe_callback(oc_resource_t *resource)
{
  oc_event_callback_t *event_cb;
  bool found = false;

  for(event_cb = (oc_event_callback_t*)oc_list_head(observe_callbacks);
      event_cb; event_cb = event_cb->next) {
    if(resource == event_cb->data) {
      found = true;
      break;
    }
  }

  if(found) {
    return event_cb;
  }

  return NULL;
}

static void
remove_periodic_observe_callback(oc_resource_t *resource)
{
  oc_event_callback_t *event_cb = get_periodic_observe_callback(resource);

  if(event_cb) {
    oc_etimer_stop(&event_cb->timer);
    oc_list_remove(observe_callbacks, event_cb);
    oc_memb_free(&event_callbacks_s, event_cb);
  }
}

static void
add_periodic_observe_callback(oc_resource_t *resource)
{
  oc_event_callback_t *event_cb = get_periodic_observe_callback(resource);

  if(!event_cb)
    {
      event_cb = (oc_event_callback_t*)oc_memb_alloc(&event_callbacks_s);
      event_cb->data = resource;
      event_cb->callback = periodic_observe_handler;
      OC_PROCESS_CONTEXT_BEGIN(&timed_callback_events);
      oc_etimer_set(&event_cb->timer,
		    resource->observe_period_seconds * OC_CLOCK_SECOND);
      OC_PROCESS_CONTEXT_END(&timed_callback_events);
      oc_list_add(observe_callbacks, event_cb);
    }
}
#endif

oc_interface_mask_t
get_interface_mask(char *iface, int if_len)
{
  oc_interface_mask_t interface = 0;
  if (OC_BASELINE_IF_LEN == if_len &&
      strncmp(iface, OC_RSRVD_IF_BASELINE, if_len) == 0)
    interface |= OC_IF_BASELINE;
  if (OC_LL_IF_LEN == if_len &&
      strncmp(iface, OC_RSRVD_IF_LL, if_len) == 0)
    interface |= OC_IF_LL;
  if (OC_B_IF_LEN == if_len &&
      strncmp(iface, OC_RSRVD_IF_B, if_len) == 0)
    interface |= OC_IF_B;
  if (OC_R_IF_LEN == if_len &&
      strncmp(iface, OC_RSRVD_IF_R, if_len) == 0)
    interface |= OC_IF_R;
  if (OC_RW_IF_LEN == if_len &&
      strncmp(iface, OC_RSRVD_IF_RW, if_len) == 0)
    interface |= OC_IF_RW;
  if (OC_A_IF_LEN == if_len &&
      strncmp(iface, OC_RSRVD_IF_A, if_len) == 0)
    interface |= OC_IF_A;
  if (OC_S_IF_LEN == if_len &&
      strncmp(iface, OC_RSRVD_IF_S, if_len) == 0)
    interface |= OC_IF_S;
  return interface;
}

bool
oc_ri_invoke_coap_entity_handler(void *request, void *response,
				 uint8_t *buffer,
				 uint16_t buffer_size, int32_t *offset,
				 oc_endpoint_t *endpoint)
{
  bool method_impl = true, bad_request = false, success = true;

#ifdef OC_SECURITY
  bool granted = true;
#endif
  coap_packet_t * const pkt = (coap_packet_t*)request;
  oc_request_t request_obj;
  oc_response_buffer_t response_buffer;
  oc_response_t response_obj;
  oc_method_t method = pkt->code;
  response_buffer.buffer = buffer;
  response_buffer.buffer_size = buffer_size;
  response_buffer.block_offset = offset;
  response_buffer.code = 0;
  response_buffer.response_length = 0;
  response_obj.slow_response = 0;
  response_obj.response_buffer = &response_buffer;
  request_obj.response = &response_obj;
  request_obj.request_payload = 0;
  request_obj.query_len = 0;
  request_obj.resource = 0;
  request_obj.origin = endpoint;
  oc_interface_mask_t interface = 0;
  const char *uri_path;
  int uri_path_len = coap_get_header_uri_path(request, &uri_path);
  const char *uri_query;
  int uri_query_len = coap_get_header_uri_query(request, &uri_query);
  if(uri_query_len) {
    request_obj.query = uri_query;
    request_obj.query_len = uri_query_len;
    char *iface;
    int if_len = oc_ri_get_query_value(uri_query, uri_query_len,
				       "if", &iface);
    if (if_len) {
      interface |= get_interface_mask(iface, if_len);
    }
  }

  if (interface == 0)
    interface = OC_IF_DEFAULT;

  const uint8_t *payload;
  int payload_len = coap_get_payload(request, &payload);
  if(payload_len) {
    if (oc_parse_rep(payload, payload_len, &request_obj.request_payload)
	!= 0)
      bad_request = true;
  }

  oc_resource_t *resource, *cur_resource = NULL;

  if (!bad_request) {
    int i;
    for(i = 0; i < NUM_OC_CORE_RESOURCES; i++) {
      resource = oc_core_get_resource_by_index(i);
      if(oc_string_len(resource->uri) == (uri_path_len + 1) &&
	 strncmp((const char*)oc_string(resource->uri) + 1, uri_path,
		 uri_path_len) == 0) {
	request_obj.resource = cur_resource = resource;
	break;
      }
    }
  }

#ifdef OC_SERVER
  if(!cur_resource & !bad_request) {
    for(resource = oc_ri_get_app_resources();
	resource;
	resource = resource->next) {
      if (oc_string_len(resource->uri) == (uri_path_len + 1) &&
	  strncmp((const char*)oc_string(resource->uri) + 1, uri_path,
		  uri_path_len) == 0)
	{
	  request_obj.resource = cur_resource = resource;
	  break;
	}
    }
  }
#endif

  if (cur_resource) {
    oc_rep_new(buffer, buffer_size);
    if ((interface & ~(cur_resource->interfaces | OC_IF_DEFAULT)) > 0) {
      bad_request = true;
    }
#ifdef OC_SECURITY
    else if ((cur_resource->properties & OC_SECURE) &&
	     !oc_sec_check_acl(method, cur_resource, endpoint)) {
      granted = false;
    }
#endif
    else {
      if(method == OC_GET && cur_resource->get_handler) {
	cur_resource->get_handler(&request_obj, interface);
      }
      else if(method == OC_POST && cur_resource->post_handler) {
	cur_resource->post_handler(&request_obj, interface);
      }
      else if(method == OC_PUT && cur_resource->put_handler) {
	cur_resource->put_handler(&request_obj, interface);
      }
      else if(method == OC_DELETE && cur_resource->delete_handler) {
	cur_resource->delete_handler(&request_obj, interface);
      }
      else {
	method_impl = false;
      }
    }
  }

  if(payload_len) {
    oc_free_rep(request_obj.request_payload);
  }

  if(!cur_resource) {
    LOG("ocri: Could not find resource\n");
    response_buffer.response_length = 0;
    response_buffer.code = oc_status_code(NOT_FOUND);
    success = false;
  }
  else if (bad_request) {
    LOG("ocri: Bad Request\n");
    response_buffer.code = oc_status_code(BAD_REQUEST);
    success = false;
  }
  else if(!method_impl) {
    LOG("ocri: Could not find method\n");
    response_buffer.response_length = 0;
    response_buffer.code = oc_status_code(METHOD_NOT_ALLOWED);
    success = false;
  }
#ifdef OC_SECURITY
  else if (!granted) {
    LOG("ocri: Forbidden request\n");
    response_buffer.response_length = 0;
    response_buffer.code = oc_status_code(FORBIDDEN);
    success = false;
  }
#endif

#ifdef OC_SERVER
  uint32_t observe = 2;
  if(success && coap_get_header_observe(request, &observe)) {
    if(cur_resource->properties & OC_OBSERVABLE) {
      if(observe == 0) {
	if(coap_observe_handler(request, response,
				cur_resource, endpoint) == 0) {
	  if(cur_resource->properties
	     & OC_PERIODIC) {
	    add_periodic_observe_callback(cur_resource);
	  }
	}
      }
      else if(observe == 1) {
	if(coap_observe_handler(request, response,
				cur_resource, endpoint) > 0) {
	  if(cur_resource->properties & OC_PERIODIC) {
	    remove_periodic_observe_callback(cur_resource);
	  }
	}
      }
    }
  }
#endif

#ifdef OC_SERVER
  if (response_obj.slow_response != NULL) {
    if(coap_separate_accept(request,
			    response_obj.slow_response, endpoint,
			    observe))
      response_obj.slow_response->in_process = 1;
  }
  else
#endif
  if(response_buffer.code == IGNORE) {
    erbium_status_code = CLEAR_TRANSACTION;
  }
  else {
#ifdef OC_SERVER
    if ((method == OC_PUT || method == OC_POST) &&
	response_buffer.code == oc_status_code(OK))
      coap_notify_observers(cur_resource, NULL, NULL);
#endif
    if (response_buffer.response_length) {
      coap_set_payload(response, response_buffer.buffer,
		       response_buffer.response_length);
      coap_set_header_content_format(response, APPLICATION_CBOR);
    }
    coap_set_status_code(response, response_buffer.code);
  }
  return success;
}

#ifdef OC_CLIENT

static void
free_client_cb(oc_client_cb_t *cb)
{
  oc_free_string(&cb->uri);
  oc_list_remove(client_cbs, cb);
  oc_memb_free(&client_cbs_s, cb);
}

void
oc_ri_remove_client_cb_by_mid(uint16_t mid)
{
  oc_client_cb_t *cb = (oc_client_cb_t*)oc_list_head(client_cbs);
  while (cb != NULL) {
    if (cb->mid == mid)
      break;
    cb = cb->next;
  }
  if (cb)
    free_client_cb(cb);
}

oc_event_callback_retval_t
oc_ri_remove_client_cb(void* data)
{
  free_client_cb(data);
  return DONE;
}

bool
oc_ri_send_rst(oc_endpoint_t *endpoint, uint8_t *token, uint8_t token_len,
	       uint16_t mid)
{
  coap_packet_t rst[1];
  coap_init_message(rst, COAP_TYPE_RST, 0, mid);
  coap_set_header_observe(rst, 1);
  coap_set_token(rst, token, token_len);
  oc_message_t *message = oc_allocate_message();
  if (message) {
    message->length = coap_serialize_message(rst, message->data);
    coap_send_message(message);
    return true;
  }
  return false;
}

bool
oc_ri_invoke_client_cb(void *response, oc_endpoint_t *endpoint)
{
  uint8_t *payload;
  int payload_len;
  coap_packet_t * const pkt = (coap_packet_t*)response;
  oc_client_cb_t *cb = oc_list_head(client_cbs);
  int i;
  /*
        if con then send ack and process as above
            -empty ack sent from below by engine
        if ack with piggyback then process as above
            -processed below
        if ack and empty then it is slow response, and keep cb
            -handled by separate flag
        if ack is for block then store data and pass to client
  */

  unsigned int content_format = APPLICATION_CBOR;
  coap_get_header_content_format(pkt, &content_format);

  while (cb != NULL) {
    if (cb->token_len == pkt->token_len &&
	memcmp(cb->token, pkt->token, pkt->token_len) == 0) {

      /* If content format is not CBOR, then reject response
         and clear callback
	 If incoming response type is RST, then clear callback
      */
      if (content_format != APPLICATION_CBOR || pkt->type == COAP_TYPE_RST) {
	free_client_cb(cb);
	break;
      }

      /* Check code, translate to oc_status_code, store
       Check observe option:
         if no observe option, set to -1, else store observe seq
      */
      oc_client_response_t client_response;
      client_response.observe_option = -1;
      client_response.payload = 0;

      for (i = 0; i < __NUM_OC_STATUS_CODES__; i++) {
	if (oc_coap_status_codes[i] == pkt->code) {
	  client_response.code = i;
	  break;
	}
      }
      coap_get_header_observe(pkt, &client_response.observe_option);

      bool separate = false;
      /*
	if payload exists, process payload and save in client response
	send client response to callback and return
      */
      payload_len = coap_get_payload(response, (const uint8_t**)&payload);
      if (payload_len) {
	if (cb->discovery) {
	  if (oc_ri_process_discovery_payload(payload,
					      payload_len, cb->handler,
					      endpoint) ==
	      OC_STOP_DISCOVERY) {
	    oc_ri_remove_client_cb(cb);
	  }
	}
	else {
	  uint16_t err = oc_parse_rep(payload, payload_len,
				      &client_response.payload);
	  if (err == 0) {
	    oc_response_handler_t handler =
	      (oc_response_handler_t)cb->handler;
	    handler(&client_response);
	  }
	  oc_free_rep(client_response.payload);
	}
      }
      else { //no payload
	if (pkt->type == COAP_TYPE_ACK && pkt->code == 0) {
	  separate = true;
	}
	else if (!cb->discovery) {
	  oc_response_handler_t handler =
	    (oc_response_handler_t)cb->handler;
	  handler(&client_response);
	}
      }

      /* check observe sequence number:
	 if -1 or 1 then remove cb, else keep cb
      */
      if (client_response.observe_option != 0 &&
	  client_response.observe_option < 2 &&
	  !separate &&
	  !cb->discovery) {
	free_client_cb(cb);
      }
      else
	cb->observe_seq = client_response.observe_option;

      break;
    }
    cb = cb->next;
  }

  return true;
}

oc_client_cb_t *oc_ri_get_client_cb(const char *uri,
				    oc_server_handle_t *server,
				    oc_method_t method)
{
  oc_client_cb_t *cb = (oc_client_cb_t*)oc_list_head(client_cbs);

  while (cb != NULL) {
    if (oc_string_len(cb->uri) == strlen(uri) &&
	strncmp(oc_string(cb->uri), uri, strlen(uri)) == 0 &&
	memcmp(&cb->server.endpoint,
	       &server->endpoint,
	       sizeof(oc_endpoint_t)) == 0 &&
	cb->method == method)
      return cb;

    cb = cb->next;
  }

  return cb;
}

oc_client_cb_t *
oc_ri_alloc_client_cb(const char *uri,
		      oc_server_handle_t *server,
		      oc_method_t method,
		      void *handler,
		      oc_qos_t qos)
{
  oc_client_cb_t *cb = oc_memb_alloc(&client_cbs_s);
  if (!cb)
    return cb;

  cb->mid = coap_get_mid();
  oc_new_string(&cb->uri, uri);
  cb->method = method;
  cb->qos = qos;
  cb->handler = handler;
  cb->token_len = 8;
  int i = 0;
  uint16_t r;
  while (i < cb->token_len) {
    r = oc_random_rand();
    memcpy(cb->token + i, &r, sizeof(r));
    i += sizeof(r);
  }
  cb->discovery = false;
  cb->timestamp = oc_clock_time();
  cb->observe_seq = -1;
  memcpy(&cb->server, server, sizeof(oc_server_handle_t));

  oc_list_add(client_cbs, cb);
  return cb;
}

oc_discovery_flags_t
oc_ri_process_discovery_payload(uint8_t *payload,
				int len,
				oc_discovery_cb_t *handler,
				oc_endpoint_t *endpoint)
{
  oc_discovery_flags_t ret = OC_CONTINUE_DISCOVERY;
  oc_string_t uri;
  uri.ptr = 0;
  oc_string_t di;
  di.ptr = 0;
  bool secure = false;
  uint16_t dtls_port = 0, default_port = endpoint->ipv6_addr.port;
  oc_string_array_t types;
  oc_interface_mask_t interfaces = 0;
  oc_server_handle_t handle;
  memcpy(&handle.endpoint, endpoint, sizeof(oc_endpoint_t));

  oc_rep_t *array = 0, *rep;
  int s = oc_parse_rep(payload, len, &rep);
  if (s == 0)
    array = rep;
  while (array != NULL) {
    oc_rep_t *device_map = array->value_object;
    while (device_map != NULL) {
      switch (device_map->type) {
      case STRING:
	if (oc_string_len(device_map->name) == 2 &&
	    strncmp(oc_string(device_map->name), "di", 2) == 0)
	  di = device_map->value_string;
	break;
      default:
	break;
      }
      device_map = device_map->next;
    }
    device_map = array->value_object;
    while (device_map != NULL) {
	switch (device_map->type) {
	case OBJECT_ARRAY: {
	  oc_rep_t *links = device_map->value_object_array;
	  while (links != NULL) {
	    switch (links->type) {
	    case OBJECT: {
	      oc_rep_t *resource_info = links->value_object;
	      while (resource_info != NULL) {
		switch (resource_info->type) {
		case STRING:
		  uri = resource_info->value_string;
		  break;
		case STRING_ARRAY:
		  if (oc_string_len(resource_info->name) == 2 &&
		      strncmp(oc_string(resource_info->name), "rt", 2)
		      == 0)
		    types = resource_info->value_array;
		  else {
		    interfaces = 0;
		    int i;
		    for (i = 0;
			 i < oc_string_array_get_allocated_size(resource_info->value_array);
			 i++) {
		      interfaces |= get_interface_mask(oc_string_array_get_item(resource_info->value_array, i), oc_string_array_get_item_size(resource_info->value_array, i));
		    }
		  }
		  break;
		case OBJECT: {
		  oc_rep_t *policy_info = resource_info->value_object;
		  while (policy_info != NULL) {
		    if (policy_info->type == INT &&
			oc_string_len(policy_info->name) == 4 &&
			strncmp(oc_string(policy_info->name), "port", 4) == 0)
		      dtls_port = policy_info->value_int;
		    if (policy_info->type == BOOL &&
			oc_string_len(policy_info->name) == 3 &&
			strncmp(oc_string(policy_info->name), "sec", 3) == 0)
		      secure = true;
		    policy_info = policy_info->next;
		  }
		}
		  break;
		default:
		  break;
		}
		resource_info = resource_info->next;
	      }
	      if (secure) {
		handle.endpoint.ipv6_addr.port = dtls_port;
		handle.endpoint.flags |= SECURED;
	      }
	      else {
		handle.endpoint.ipv6_addr.port = default_port;
		handle.endpoint.flags &= ~SECURED;
	      }

	      if (handler(oc_string(di),
			  oc_string(uri),
			  types,
			  interfaces,
			  &handle) ==
		  OC_STOP_DISCOVERY) {
		ret = OC_STOP_DISCOVERY;
		goto done;
	      }
	      dtls_port = 0;
	      secure = false;
	    }
	      break;
	    default:
	      break;
	    }
	    links = links->next;
	  }
	}
	  break;
	default:
	  break;
	}
	device_map = device_map->next;
    }
    array = array->next;
  }
 done:
  oc_free_rep(rep);
  return ret;
}

#endif /* OC_CLIENT */

OC_PROCESS_THREAD(timed_callback_events, ev, data)
{
  OC_PROCESS_BEGIN();
  while(1) {
    OC_PROCESS_YIELD();
    if(ev == OC_PROCESS_EVENT_TIMER) {
      check_event_callbacks();
    }
  }
  OC_PROCESS_END();
}

//TODO:
//resource collections
//if method accepted by interface selection
//resources for presence based discovery
