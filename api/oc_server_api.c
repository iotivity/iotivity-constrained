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
#include "messaging/coap/oc_coap.h"
#include "messaging/coap/separate.h"
#include "oc_api.h"

#if defined(OC_COLLECTIONS) && defined(OC_SERVER)
#include "oc_collection.h"
#endif /* OC_COLLECTIONS && OC_SERVER */

#include "oc_core_res.h"

int
oc_add_device(const char *uri, const char *rt, const char *name,
              const char *spec_version, const char *data_model_version,
              oc_add_device_cb_t add_device_cb, void *data)
{
  if (!oc_core_add_new_device(uri, rt, name, spec_version, data_model_version,
                              add_device_cb, data))
    return -1;
  return 0;
}

int
oc_init_platform(const char *mfg_name, oc_init_platform_cb_t init_platform_cb,
                 void *data)
{
  if (!oc_core_init_platform(mfg_name, init_platform_cb, data))
    return -1;
  return 0;
}

int
oc_get_query_value(oc_request_t *request, const char *key, char **value)
{
  return oc_ri_get_query_value(request->query, request->query_len, key, value);
}

static int
response_length(void)
{
  int size = oc_rep_finalize();
  return (size <= 2) ? 0 : size;
}

void
oc_send_response(oc_request_t *request, oc_status_t response_code)
{
  request->response->response_buffer->response_length = response_length();
  request->response->response_buffer->code = oc_status_code(response_code);
}

void
oc_ignore_request(oc_request_t *request)
{
  request->response->response_buffer->code = OC_IGNORE;
}

void
oc_set_delayed_callback(void *cb_data, oc_trigger_t callback, uint16_t seconds)
{
  oc_ri_add_timed_event_callback_seconds(cb_data, callback, seconds);
}

void
oc_remove_delayed_callback(void *cb_data, oc_trigger_t callback)
{
  oc_ri_remove_timed_event_callback(cb_data, callback);
}

void
oc_process_baseline_interface(oc_resource_t *resource)
{
  oc_rep_set_string_array(root, rt, resource->types);
  oc_core_encode_interfaces_mask(oc_rep_object(root), resource->interfaces);
}

#ifdef OC_SERVER
static int query_iterator;

static void
oc_populate_resource_object(oc_resource_t *resource, const char *uri,
                            uint8_t num_resource_types, int device)
{
  oc_store_uri(uri, &resource->uri);
  oc_new_string_array(&resource->types, num_resource_types);
  resource->properties = 0;
  resource->device = device;
}

oc_resource_t *
oc_new_resource(const char *uri, uint8_t num_resource_types, int device)
{
  oc_resource_t *resource = oc_ri_alloc_resource();
  if (resource) {
    resource->interfaces = OC_IF_BASELINE;
    resource->default_interface = OC_IF_BASELINE;
    resource->observe_period_seconds = 0;
    resource->num_observers = 0;
    oc_populate_resource_object(resource, uri, num_resource_types, device);
  }
  return resource;
}

#if defined(OC_COLLECTIONS)
oc_resource_t *
oc_new_collection(const char *uri, uint8_t num_resource_types, int device)
{
  oc_collection_t *collection = oc_collection_alloc();
  if (collection) {
    collection->interfaces = OC_IF_BASELINE | OC_IF_LL | OC_IF_B;
    collection->default_interface = OC_IF_LL;
    oc_populate_resource_object((oc_resource_t *)collection, uri,
                                num_resource_types, device);
  }
  return (oc_resource_t *)collection;
}

bool
oc_add_collection(oc_resource_t *collection)
{
  oc_resource_set_observable(collection, false);
  return oc_collection_add((oc_collection_t *)collection);
}
#endif /* OC_COLLECTIONS */

void
oc_resource_bind_resource_interface(oc_resource_t *resource, uint8_t interface)
{
  resource->interfaces |= interface;
}

void
oc_resource_set_default_interface(oc_resource_t *resource,
                                  oc_interface_mask_t interface)
{
  resource->default_interface = interface;
}

void
oc_resource_bind_resource_type(oc_resource_t *resource, const char *type)
{
  oc_string_array_add_item(resource->types, (char *)type);
}

#ifdef OC_SECURITY
void
oc_resource_make_secure(oc_resource_t *resource)
{
  resource->properties |= OC_SECURE;
}
#endif /* OC_SECURITY */

void
oc_resource_set_discoverable(oc_resource_t *resource, bool state)
{
  if (state)
    resource->properties |= OC_DISCOVERABLE;
  else
    resource->properties &= ~OC_DISCOVERABLE;
}

void
oc_resource_set_observable(oc_resource_t *resource, bool state)
{
  if (state)
    resource->properties |= OC_OBSERVABLE;
  else
    resource->properties &= ~(OC_OBSERVABLE | OC_PERIODIC);
}

void
oc_resource_set_periodic_observable(oc_resource_t *resource, uint16_t seconds)
{
  resource->properties |= OC_OBSERVABLE | OC_PERIODIC;
  resource->observe_period_seconds = seconds;
}

void
oc_resource_set_request_handler(oc_resource_t *resource, oc_method_t method,
                                oc_request_callback_t callback, void *user_data)
{
  oc_request_handler_t *handler = NULL;
  switch (method) {
  case OC_GET:
    handler = &resource->get_handler;
    break;
  case OC_POST:
    handler = &resource->post_handler;
    break;
  case OC_PUT:
    handler = &resource->put_handler;
    break;
  case OC_DELETE:
    handler = &resource->delete_handler;
    break;
  default:
    break;
  }

  handler->cb = callback;
  handler->user_data = user_data;
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
oc_init_query_iterator(void)
{
  query_iterator = 0;
}

int
oc_interate_query(oc_request_t *request, char **key, int *key_len, char **value,
                  int *value_len)
{
  if (query_iterator >= request->query_len)
    return -1;
  query_iterator = oc_ri_get_query_nth_key_value(
    request->query + query_iterator, request->query_len - query_iterator, key,
    key_len, value, value_len, 1);
  return 1;
}

void
oc_indicate_separate_response(oc_request_t *request,
                              oc_separate_response_t *response)
{
  request->response->separate_response = response;
  oc_send_response(request, OC_STATUS_OK);
}

void
oc_set_separate_response_buffer(oc_separate_response_t *handle)
{
#ifdef OC_BLOCK_WISE_SET_MTU
  oc_rep_new(handle->buffer, OC_BLOCK_WISE_BUFFER_SIZE);
#else  /* OC_BLOCK_WISE_SET_MTU */
  oc_rep_new(handle->buffer, OC_BLOCK_SIZE);
#endif /* !OC_BLOCK_WISE_SET_MTU */
}

void
oc_send_separate_response(oc_separate_response_t *handle,
                          oc_status_t response_code)
{
  oc_response_buffer_t response_buffer;
  response_buffer.buffer = handle->buffer;
  response_buffer.response_length = response_length();
  response_buffer.code = oc_status_code(response_code);

  coap_separate_t *cur = oc_list_head(handle->requests), *next = NULL;
  coap_packet_t response[1];

  while (cur != NULL) {
    next = cur->next;
    if (cur->observe > 0) {
      coap_transaction_t *t =
        coap_new_transaction(coap_get_mid(), &cur->endpoint);
      if (t) {
        coap_separate_resume(response, cur, oc_status_code(response_code),
                             t->mid);
        coap_set_header_content_format(response, APPLICATION_CBOR);
        if (cur->observe == 1) {
          coap_set_header_observe(response, 1);
        }

#ifdef OC_BLOCK_WISE_SET_MTU
        oc_blockwise_state_t *response_state = 0;
        if (response_buffer.response_length > cur->block2_size) {
          response_state = oc_blockwise_find_response_buffer(
            oc_string(cur->uri), oc_string_len(cur->uri), &cur->endpoint,
            cur->method);
          if (response_state) {
            goto clear_separate_store;
          }
          response_state = oc_blockwise_alloc_response_buffer(
            oc_string(cur->uri), oc_string_len(cur->uri), &cur->endpoint,
            cur->method);
          if (!response_state) {
            goto clear_separate_store;
          }

          memcpy(response_state->buffer, response_buffer.buffer,
                 response_buffer.response_length);
          response_state->payload_size = response_buffer.response_length;

          uint16_t payload_size = 0;
          const void *payload = oc_blockwise_dispatch_block(
            response_state, 0, cur->block2_size, &payload_size);
          if (payload) {
            coap_set_payload(response, payload, payload_size);
            coap_set_header_block2(response, 0, 1, cur->block2_size);
            coap_set_header_size2(response, response_state->payload_size);
            oc_blockwise_response_state_t *bwt_res_state =
              (oc_blockwise_response_state_t *)response_state;
            coap_set_header_etag(response, bwt_res_state->etag, COAP_ETAG_LEN);
          }
        } else
#endif /* OC_BLOCK_WISE_SET_MTU */
          if (response_buffer.response_length > 0) {
          coap_set_payload(response, handle->buffer,
                           response_buffer.response_length);
        }
        coap_set_status_code(response, response_buffer.code);
        t->message->length = coap_serialize_message(response, t->message->data);
        coap_send_transaction(t);
      }
#ifdef OC_BLOCK_WISE_SET_MTU
    clear_separate_store:
#endif /* OC_BLOCK_WISE_SET_MTU */
      coap_separate_clear(handle, cur);
    } else {
      if (coap_notify_observers(NULL, &response_buffer, &cur->endpoint) == 0) {
        coap_separate_clear(handle, cur);
      }
    }
    cur = next;
  }
  if (oc_list_length(handle->requests) == 0) {
    handle->active = 0;
  }
}

int
oc_notify_observers(oc_resource_t *resource)
{
  return coap_notify_observers(resource, NULL, NULL);
}
#endif /* OC_SERVER */
