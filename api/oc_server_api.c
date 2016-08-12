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
#include "messaging/coap/separate.h"
#include "messaging/coap/oc_coap.h"
#include "oc_core_res.h"
#include "oc_api.h"
#include "oc_constants.h"

static oc_string_t *payload;

extern int oc_stack_errno;
//TODO:
//0x01: Couldnt add platform
//0x02: Couldnt add device
//0x03: CBOR error

void
oc_new_device(const char *uri, const char *rt,
	      const char *name, const char *spec_version,
	      const char *data_model_version)
{
  payload = oc_core_add_new_device(uri, rt, name,
				   spec_version,
				   data_model_version);
  if (!payload)
    oc_stack_errno |= 0x02;
}

void
oc_add_device()
{
  if (oc_stack_errno)
    return;
  extern oc_string_t temp_buffer;
  oc_rep_end_root_object();
  int size = oc_rep_finalize();
  if (size != -1) {
    oc_alloc_string(payload, size);
    memcpy(oc_cast(*payload, uint8_t), oc_cast(temp_buffer, uint8_t), size);
  }
  else
    oc_stack_errno |= 0x03;
  oc_free_string(&temp_buffer); 
}

void
oc_new_platform(const char *mfg_name)
{
  payload = oc_core_add_new_platform(mfg_name);
  if (!payload)
    oc_stack_errno |= 0x01;  
}

void
oc_add_platform()
{
   oc_add_device();
}

int
oc_get_query_value(oc_request_t *request, const char *key, char **value)
{
  return oc_ri_get_query_value(request->query, request->query_len,
			       key, value);
}

static int
response_length()
{
  int size = oc_rep_finalize();
  return  (size >= 0 && size <= 2)?0:size;
}

void
oc_send_response(oc_request_t *request,
		 oc_status_t response_code)
{
  //FIX:: set errno if CBOR encoding failed.
  request->response->response_buffer->response_length = response_length();
  request->response->response_buffer->code = oc_status_code(response_code);
}

void
oc_ignore_request(oc_request_t *request)
{
  request->response->response_buffer->code = IGNORE;
}

void
oc_set_delayed_callback(void *cb_data,
			oc_trigger_t callback, uint16_t seconds)
{
  oc_ri_add_timed_event_callback_seconds(cb_data, callback, seconds);
}

void
oc_process_baseline_interface(oc_resource_t *resource)
{
  oc_rep_set_string_array(root, rt, resource->types);
  oc_core_encode_interfaces_mask(oc_rep_object(root), resource->interfaces);
  oc_rep_set_uint(root, p, resource->properties & ~OC_PERIODIC);
}

#ifdef OC_SERVER
static int query_iterator;

//FIX: validate uri
oc_resource_t *
oc_new_resource(const char *uri, uint8_t num_resource_types,
		int device)
{
  oc_resource_t *resource = oc_ri_alloc_resource();
  const char *start = uri;
  size_t end = strlen(uri);
  oc_alloc_string(&resource->uri, end + 1);
  strncpy((char*)oc_string(resource->uri), start, end);
  strcpy((char*)oc_string(resource->uri) + end, (const char*)"");
  oc_new_string_array(&resource->types, num_resource_types);
  resource->interfaces = OC_IF_BASELINE;
  resource->observe_period_seconds = 0;
  resource->properties = OC_ACTIVE;
  resource->num_observers = 0;
  resource->device = device;
  return resource;
}

void
oc_resource_bind_resource_interface(oc_resource_t *resource,
				    uint8_t interface)
{
  resource->interfaces |= interface;
}

void
oc_resource_bind_resource_type(oc_resource_t *resource,
			       const char *type)
{
  oc_string_array_add_item(resource->types, (char* )type);
}

#ifdef OC_SECURITY
void
oc_resource_make_secure(oc_resource_t *resource)
{
  resource->properties |= OC_SECURE;
}
#endif /* OC_SECURITY */

void
oc_resource_set_discoverable(oc_resource_t *resource)
{
  resource->properties |= OC_DISCOVERABLE;
}

void
oc_resource_set_observable(oc_resource_t *resource)
{
  resource->properties |= OC_OBSERVABLE;
}

void
oc_resource_set_periodic_observable(oc_resource_t *resource,
				    uint16_t seconds)
{
  resource->properties |= OC_OBSERVABLE | OC_PERIODIC;
  resource->observe_period_seconds = seconds;
}

void
oc_deactivate_resource(oc_resource_t *resource)
{
  resource->properties ^= OC_ACTIVE;
}

void
oc_resource_set_request_handler(oc_resource_t *resource,
				oc_method_t method,
				oc_request_handler_t handler)
{
  switch(method) {
  case OC_GET:
    resource->get_handler = handler;
    break;
  case OC_POST:
    resource->post_handler = handler;
    break;
  case OC_PUT:
    resource->put_handler = handler;
    break;
  case OC_DELETE:
    resource->delete_handler = handler;
    break;
  default:
    break;
  }
}

bool
oc_add_resource(oc_resource_t *resource)
{
  return oc_ri_add_resource(resource);
}

void
oc_delete_resource(oc_resource_t *resource)
{
  oc_ri_delete_resource(resource);
}

void
oc_init_query_iterator(oc_request_t *request)
{
  query_iterator = 0;
}

int
oc_interate_query(oc_request_t *request, char **key, int *key_len,
		  char **value, int *value_len)
{
  if (query_iterator >= request->query_len)
    return -1;
  query_iterator =
    oc_ri_get_query_nth_key_value(request->query + query_iterator,
				  request->query_len - query_iterator,
				  key, key_len, value, value_len, 1);
  return 1;
}

void
oc_indicate_slow_response(oc_request_t *request,
			  oc_slow_response_t *response)
{
  request->response->slow_response = response;
  oc_send_response(request, OK);
}

void
oc_set_slow_response_buffer(oc_slow_response_t *handle)
{
  oc_rep_new(handle->buffer, COAP_MAX_BLOCK_SIZE); // check
}

void
oc_send_slow_response(oc_slow_response_t *handle,
		      oc_status_t response_code)
{
  oc_response_buffer_t response_buffer;
  response_buffer.buffer = handle->buffer;
  response_buffer.response_length = response_length();
  response_buffer.code = oc_status_code(response_code);
  coap_separate_t *cur = oc_list_head(handle->requests), *next = NULL;
  coap_packet_t response[1];
  while(cur != NULL) {
    next = cur->next;
    if(cur->observe > 0) {
      coap_transaction_t *t = coap_new_transaction(coap_get_mid(),
						   &cur->endpoint);
      if(t) {
	coap_separate_resume(response, cur,
			     oc_status_code(response_code),
			     t->mid);
	coap_set_header_content_format(response, APPLICATION_CBOR);
	if(cur->observe == 1) {
	  coap_set_header_observe(response, 1);
	}
	coap_set_payload(response, handle->buffer,
			 response_buffer.response_length);
	t->message->length = coap_serialize_message(response,
						    t->message->data);
	coap_send_transaction(t);
      }
    } else {
      coap_notify_observers(NULL, &response_buffer,
			    &cur->endpoint);
    }
    coap_separate_clear(handle, cur);
    cur = next;
  }
  handle->in_process = 0;
}

int
oc_notify_observers(oc_resource_t *resource)
{
  return coap_notify_observers(resource, NULL, NULL);
}

#endif /* OC_SERVER */



