/*
// Copyright (c) 2017 Intel Corporation
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

#include "oc_api.h"
#include "oc_pki.h"
#include "port/oc_clock.h"
#include <pthread.h>
#include <signal.h>
#include <stdio.h>

static pthread_mutex_t mutex;
static pthread_cond_t cv;
static struct timespec ts;
static int quit = 0;

static double temp_C = 5.0, min_C = 0.0, max_C = 100.0, min_K = 273.15,
              max_K = 373.15, min_F = 32, max_F = 212;
typedef enum { C = 100, F, K } units_t;
static bool switch_state, pswitch_state;

static int
app_init(void)
{
  int err = oc_init_platform("Intel", NULL, NULL);

  err |= oc_add_device("/oic/d", "oic.d.switch", "Temp_sensor", "ocf.1.0.0",
                       "ocf.res.1.0.0", NULL, NULL);
  return err;
}

static void
get_temp(oc_request_t *request, oc_interface_mask_t interface, void *user_data)
{
  (void)user_data;
  PRINT("GET_temp:\n");
  bool invalid_query = false;
  double temp = temp_C;
  units_t temp_units = C;
  char *units;
  int units_len = oc_get_query_value(request, "units", &units);
  if (units_len != -1) {
    if (units[0] == 'K') {
      temp = temp_C + 273.15;
      temp_units = K;
    } else if (units[0] == 'F') {
      temp = (temp_C / 100) * 180 + 32;
      temp_units = F;
    } else if (units[0] != 'C')
      invalid_query = true;
  }

  oc_rep_start_root_object();
  switch (interface) {
  case OC_IF_BASELINE:
    oc_process_baseline_interface(request->resource);
    oc_rep_set_text_string(root, id, "home_thermostat");
  /* fall through */
  case OC_IF_A:
  case OC_IF_S:
    oc_rep_set_double(root, temperature, temp);
    switch (temp_units) {
    case C:
      oc_rep_set_text_string(root, units, "C");
      break;
    case F:
      oc_rep_set_text_string(root, units, "F");
      break;
    case K:
      oc_rep_set_text_string(root, units, "K");
      break;
    }
    break;
  default:
    break;
  }

  if (!invalid_query) {
    oc_rep_set_array(root, range);
    switch (temp_units) {
    case C:
      oc_rep_add_double(range, min_C);
      oc_rep_add_double(range, max_C);
      break;
    case K:
      oc_rep_add_double(range, min_K);
      oc_rep_add_double(range, max_K);
      break;
    case F:
      oc_rep_add_double(range, min_F);
      oc_rep_add_double(range, max_F);
      break;
    }
    oc_rep_close_array(root, range);
  }

  oc_rep_end_root_object();

  if (invalid_query)
    oc_send_response(request, OC_STATUS_FORBIDDEN);
  else
    oc_send_response(request, OC_STATUS_OK);
}

static void
post_temp(oc_request_t *request, oc_interface_mask_t interface, void *user_data)
{
  (void)interface;
  (void)user_data;
  PRINT("POST_temp:\n");
  bool out_of_range = false;
  double temp = -1;

  oc_rep_t *rep = request->request_payload;
  while (rep != NULL) {
    switch (rep->type) {
    case OC_REP_DOUBLE:
      temp = rep->value.double_p;
      break;
    default:
      break;
    }
    rep = rep->next;
  }

  if (temp < min_C || temp > max_C)
    out_of_range = true;

  temp_C = temp;

  oc_rep_start_root_object();
  oc_rep_set_text_string(root, id, "home_thermostat");
  oc_rep_set_double(root, temperature, temp_C);
  oc_rep_set_text_string(root, units, "C");
  oc_rep_set_array(root, range);
  oc_rep_add_double(range, min_C);
  oc_rep_add_double(range, max_C);
  oc_rep_close_array(root, range);
  oc_rep_end_root_object();

  if (out_of_range)
    oc_send_response(request, OC_STATUS_FORBIDDEN);
  else
    oc_send_response(request, OC_STATUS_CHANGED);
}

static void
get_pswitch(oc_request_t *request, oc_interface_mask_t interface,
            void *user_data)
{
  (void)user_data;
  PRINT("GET_pswitch:\n");
  oc_rep_start_root_object();
  switch (interface) {
  case OC_IF_BASELINE:
    oc_process_baseline_interface(request->resource);
    oc_rep_set_text_string(root, id, "purifier_switch");
  /* fall through */
  case OC_IF_A:
    oc_rep_set_boolean(root, value, pswitch_state);
    break;
  default:
    break;
  }
  oc_rep_end_root_object();

  oc_send_response(request, OC_STATUS_OK);
}

static void
post_pswitch(oc_request_t *request, oc_interface_mask_t interface,
             void *user_data)
{
  (void)interface;
  (void)user_data;
  PRINT("POST_pswitch:\n");
  bool state = false, bad_request = false;
  oc_rep_t *rep = request->request_payload;
  while (rep != NULL) {
    switch (rep->type) {
    case OC_REP_BOOL:
      state = rep->value.boolean;
      break;
    default:
      if (oc_string_len(rep->name) > 2) {
        if (strncmp(oc_string(rep->name), "x.", 2) == 0) {
          break;
        }
      }
      bad_request = true;
      break;
    }
    rep = rep->next;
  }

  if (!bad_request) {
    pswitch_state = state;
  }

  oc_rep_start_root_object();
  oc_rep_set_boolean(root, value, pswitch_state);
  oc_rep_end_root_object();

  if (!bad_request) {
    oc_send_response(request, OC_STATUS_CHANGED);
  } else {
    oc_send_response(request, OC_STATUS_BAD_REQUEST);
  }
}

static void
get_switch(oc_request_t *request, oc_interface_mask_t interface,
           void *user_data)
{
  (void)user_data;
  PRINT("GET_switch:\n");
  oc_rep_start_root_object();
  switch (interface) {
  case OC_IF_BASELINE:
    oc_process_baseline_interface(request->resource);
    oc_rep_set_text_string(root, id, "thermostat_switch");
  /* fall through */
  case OC_IF_A:
    oc_rep_set_boolean(root, value, switch_state);
    break;
  default:
    break;
  }
  oc_rep_end_root_object();

  oc_send_response(request, OC_STATUS_OK);
}

static void
post_switch(oc_request_t *request, oc_interface_mask_t interface,
            void *user_data)
{
  (void)interface;
  (void)user_data;
  PRINT("POST_switch:\n");
  bool state = false, bad_request = false;
  oc_rep_t *rep = request->request_payload;
  while (rep != NULL) {
    switch (rep->type) {
    case OC_REP_BOOL:
      state = rep->value.boolean;
      break;
    default:
      if (oc_string_len(rep->name) > 2) {
        if (strncmp(oc_string(rep->name), "x.", 2) == 0) {
          break;
        }
      }
      bad_request = true;
      break;
    }
    rep = rep->next;
  }

  if (!bad_request) {
    switch_state = state;
  }

  oc_rep_start_root_object();
  oc_rep_set_boolean(root, value, switch_state);
  oc_rep_end_root_object();

  if (!bad_request) {
    oc_send_response(request, OC_STATUS_CHANGED);
  } else {
    oc_send_response(request, OC_STATUS_BAD_REQUEST);
  }
}

static void
register_resources(void)
{
  oc_resource_t *temp = oc_new_resource("tempsensor", "/temp", 1, 0);
  oc_resource_bind_resource_type(temp, "oic.r.temperature");
  oc_resource_bind_resource_interface(temp, OC_IF_A);
  oc_resource_bind_resource_interface(temp, OC_IF_S);
  oc_resource_set_default_interface(temp, OC_IF_A);
  oc_resource_set_discoverable(temp, true);
  oc_resource_set_periodic_observable(temp, 1);
  oc_resource_set_request_handler(temp, OC_GET, get_temp, NULL);
  oc_resource_set_request_handler(temp, OC_POST, post_temp, NULL);
  oc_add_resource(temp);

  oc_resource_t *bswitch = oc_new_resource("smartbutton", "/switch", 1, 0);
  oc_resource_bind_resource_type(bswitch, "oic.r.switch.binary");
  oc_resource_bind_resource_interface(bswitch, OC_IF_A);
  oc_resource_set_default_interface(bswitch, OC_IF_A);
  oc_resource_set_discoverable(bswitch, true);
  oc_resource_set_request_handler(bswitch, OC_GET, get_switch, NULL);
  oc_resource_set_request_handler(bswitch, OC_POST, post_switch, NULL);
  oc_add_resource(bswitch);

  oc_resource_t *pbswitch =
    oc_new_resource("purifierswitch", "/purifier_switch", 1, 0);
  oc_resource_bind_resource_type(pbswitch, "oic.r.switch.binary");
  oc_resource_bind_resource_interface(pbswitch, OC_IF_A);
  oc_resource_set_default_interface(pbswitch, OC_IF_A);
  oc_resource_set_discoverable(pbswitch, true);
  oc_resource_set_request_handler(pbswitch, OC_GET, get_pswitch, NULL);
  oc_resource_set_request_handler(pbswitch, OC_POST, post_pswitch, NULL);
  oc_add_resource(pbswitch);

#ifdef OC_COLLECTIONS
  oc_resource_t *col = oc_new_collection("dashboard", "/platform", 1, 2, 2, 0);
  oc_resource_bind_resource_type(col, "oic.wk.col");
  oc_resource_set_discoverable(col, true);
  oc_collection_add_supported_rt(col, "oic.r.temperature");
  oc_collection_add_supported_rt(col, "oic.r.switch.binary");
  oc_collection_add_mandatory_rt(col, "oic.r.temperature");
  oc_collection_add_mandatory_rt(col, "oic.r.switch.binary");
  oc_link_t *l1 = oc_new_link(temp);
  oc_collection_add_link(col, l1);

  oc_link_t *l2 = oc_new_link(bswitch);
  oc_collection_add_link(col, l2);

  oc_link_t *l3 = oc_new_link(pbswitch);
  oc_collection_add_link(col, l3);
  oc_add_collection(col);
#endif /* OC_COLLECTIONS */
}

static void
signal_event_loop(void)
{
  pthread_mutex_lock(&mutex);
  pthread_cond_signal(&cv);
  pthread_mutex_unlock(&mutex);
}

static void
handle_signal(int signal)
{
  (void)signal;
  signal_event_loop();
  quit = 1;
}

int
main(void)
{
  int init;
  struct sigaction sa;
  sigfillset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = handle_signal;
  sigaction(SIGINT, &sa, NULL);

  static const oc_handler_t handler = {.init = app_init,
                                       .signal_event_loop = signal_event_loop,
                                       .register_resources =
                                         register_resources };

  oc_clock_time_t next_event;
  oc_set_con_res_announced(false);
  oc_set_mtu_size(16384);
  oc_set_max_app_data_size(16384);

#ifdef OC_SECURITY
  oc_storage_config("./smart_home_server_linux_creds");
#endif /* OC_SECURITY */

  init = oc_main_init(&handler);
  if (init < 0)
    return init;

#if defined(OC_SECURITY) && defined(OC_PKI)
  const unsigned char my_crt[] = {
    0x30, 0x82, 0x03, 0xf8, 0x30, 0x82, 0x03, 0x9e, 0xa0, 0x03, 0x02, 0x01,
    0x02, 0x02, 0x09, 0x00, 0x8d, 0x0a, 0xfb, 0x7b, 0x53, 0xb2, 0x4c, 0xb6,
    0x30, 0x0a, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x02,
    0x30, 0x5b, 0x31, 0x0c, 0x30, 0x0a, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x0c,
    0x03, 0x4f, 0x43, 0x46, 0x31, 0x22, 0x30, 0x20, 0x06, 0x03, 0x55, 0x04,
    0x0b, 0x0c, 0x19, 0x4b, 0x79, 0x72, 0x69, 0x6f, 0x20, 0x54, 0x65, 0x73,
    0x74, 0x20, 0x49, 0x6e, 0x66, 0x72, 0x61, 0x73, 0x74, 0x72, 0x75, 0x63,
    0x74, 0x75, 0x72, 0x65, 0x31, 0x27, 0x30, 0x25, 0x06, 0x03, 0x55, 0x04,
    0x03, 0x0c, 0x1e, 0x4b, 0x79, 0x72, 0x69, 0x6f, 0x20, 0x54, 0x45, 0x53,
    0x54, 0x20, 0x49, 0x6e, 0x74, 0x65, 0x72, 0x6d, 0x65, 0x64, 0x69, 0x61,
    0x74, 0x65, 0x20, 0x43, 0x41, 0x30, 0x30, 0x30, 0x32, 0x30, 0x1e, 0x17,
    0x0d, 0x31, 0x38, 0x31, 0x32, 0x31, 0x33, 0x31, 0x33, 0x33, 0x36, 0x33,
    0x30, 0x5a, 0x17, 0x0d, 0x31, 0x39, 0x30, 0x36, 0x31, 0x31, 0x31, 0x33,
    0x33, 0x36, 0x33, 0x30, 0x5a, 0x30, 0x61, 0x31, 0x0c, 0x30, 0x0a, 0x06,
    0x03, 0x55, 0x04, 0x0a, 0x0c, 0x03, 0x4f, 0x43, 0x46, 0x31, 0x22, 0x30,
    0x20, 0x06, 0x03, 0x55, 0x04, 0x0b, 0x0c, 0x19, 0x4b, 0x79, 0x72, 0x69,
    0x6f, 0x20, 0x54, 0x65, 0x73, 0x74, 0x20, 0x49, 0x6e, 0x66, 0x72, 0x61,
    0x73, 0x74, 0x72, 0x75, 0x63, 0x74, 0x75, 0x72, 0x65, 0x31, 0x2d, 0x30,
    0x2b, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0c, 0x24, 0x39, 0x39, 0x30, 0x30,
    0x30, 0x30, 0x31, 0x30, 0x2d, 0x31, 0x31, 0x31, 0x31, 0x2d, 0x31, 0x31,
    0x31, 0x31, 0x2d, 0x31, 0x31, 0x31, 0x31, 0x2d, 0x31, 0x31, 0x31, 0x31,
    0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x30, 0x30, 0x59, 0x30, 0x13,
    0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01, 0x06, 0x08, 0x2a,
    0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00, 0x04, 0xe4,
    0xbf, 0xcf, 0x9a, 0x29, 0x2d, 0x91, 0x54, 0x4b, 0x6d, 0x2c, 0x67, 0x38,
    0x7b, 0xe8, 0x7c, 0x58, 0x96, 0x60, 0x40, 0xc7, 0x72, 0xc2, 0x41, 0xb7,
    0x6f, 0xa6, 0x1a, 0x09, 0xe8, 0x85, 0x96, 0xad, 0x02, 0x4c, 0x95, 0xbf,
    0x67, 0x75, 0x24, 0x88, 0x98, 0x3c, 0x2a, 0x9a, 0xdb, 0x95, 0x96, 0x62,
    0x74, 0xce, 0x2d, 0x79, 0xe8, 0x30, 0xfa, 0x4c, 0x4a, 0x97, 0x5e, 0xaf,
    0xb9, 0xb3, 0x5c, 0xa3, 0x82, 0x02, 0x43, 0x30, 0x82, 0x02, 0x3f, 0x30,
    0x09, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x04, 0x02, 0x30, 0x00, 0x30, 0x0e,
    0x06, 0x03, 0x55, 0x1d, 0x0f, 0x01, 0x01, 0xff, 0x04, 0x04, 0x03, 0x02,
    0x03, 0x88, 0x30, 0x29, 0x06, 0x03, 0x55, 0x1d, 0x25, 0x04, 0x22, 0x30,
    0x20, 0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x02, 0x06,
    0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x01, 0x06, 0x0a, 0x2b,
    0x06, 0x01, 0x04, 0x01, 0x82, 0xde, 0x7c, 0x01, 0x06, 0x30, 0x1d, 0x06,
    0x03, 0x55, 0x1d, 0x0e, 0x04, 0x16, 0x04, 0x14, 0x46, 0x93, 0xfa, 0xa7,
    0x95, 0xbf, 0xbb, 0x0d, 0x1c, 0x38, 0xc4, 0xce, 0xe7, 0x49, 0x33, 0x16,
    0xe8, 0x59, 0xe9, 0xbe, 0x30, 0x1f, 0x06, 0x03, 0x55, 0x1d, 0x23, 0x04,
    0x18, 0x30, 0x16, 0x80, 0x14, 0x19, 0x73, 0x6a, 0x04, 0x1a, 0x0b, 0x07,
    0x70, 0x4f, 0x53, 0x79, 0x53, 0x36, 0x87, 0xfc, 0x0c, 0xba, 0x7c, 0xae,
    0x0b, 0x30, 0x81, 0x96, 0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07,
    0x01, 0x01, 0x04, 0x81, 0x89, 0x30, 0x81, 0x86, 0x30, 0x5d, 0x06, 0x08,
    0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x30, 0x02, 0x86, 0x51, 0x68, 0x74,
    0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x74, 0x65, 0x73, 0x74, 0x70, 0x6b, 0x69,
    0x2e, 0x6b, 0x79, 0x72, 0x69, 0x6f, 0x2e, 0x63, 0x6f, 0x6d, 0x2f, 0x6f,
    0x63, 0x66, 0x2f, 0x63, 0x61, 0x63, 0x65, 0x72, 0x74, 0x73, 0x2f, 0x42,
    0x42, 0x45, 0x36, 0x34, 0x46, 0x39, 0x41, 0x37, 0x45, 0x45, 0x33, 0x37,
    0x44, 0x32, 0x39, 0x41, 0x30, 0x35, 0x45, 0x34, 0x42, 0x42, 0x37, 0x37,
    0x35, 0x39, 0x35, 0x46, 0x33, 0x30, 0x38, 0x42, 0x45, 0x34, 0x31, 0x45,
    0x42, 0x30, 0x37, 0x2e, 0x63, 0x72, 0x74, 0x30, 0x25, 0x06, 0x08, 0x2b,
    0x06, 0x01, 0x05, 0x05, 0x07, 0x30, 0x01, 0x86, 0x19, 0x68, 0x74, 0x74,
    0x70, 0x3a, 0x2f, 0x2f, 0x74, 0x65, 0x73, 0x74, 0x6f, 0x63, 0x73, 0x70,
    0x2e, 0x6b, 0x79, 0x72, 0x69, 0x6f, 0x2e, 0x63, 0x6f, 0x6d, 0x30, 0x5f,
    0x06, 0x03, 0x55, 0x1d, 0x1f, 0x04, 0x58, 0x30, 0x56, 0x30, 0x54, 0xa0,
    0x52, 0xa0, 0x50, 0x86, 0x4e, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f,
    0x74, 0x65, 0x73, 0x74, 0x70, 0x6b, 0x69, 0x2e, 0x6b, 0x79, 0x72, 0x69,
    0x6f, 0x2e, 0x63, 0x6f, 0x6d, 0x2f, 0x6f, 0x63, 0x66, 0x2f, 0x63, 0x72,
    0x6c, 0x73, 0x2f, 0x42, 0x42, 0x45, 0x36, 0x34, 0x46, 0x39, 0x41, 0x37,
    0x45, 0x45, 0x33, 0x37, 0x44, 0x32, 0x39, 0x41, 0x30, 0x35, 0x45, 0x34,
    0x42, 0x42, 0x37, 0x37, 0x35, 0x39, 0x35, 0x46, 0x33, 0x30, 0x38, 0x42,
    0x45, 0x34, 0x31, 0x45, 0x42, 0x30, 0x37, 0x2e, 0x63, 0x72, 0x6c, 0x30,
    0x5f, 0x06, 0x0a, 0x2b, 0x06, 0x01, 0x04, 0x01, 0x83, 0x91, 0x56, 0x01,
    0x00, 0x04, 0x51, 0x30, 0x4f, 0x30, 0x09, 0x02, 0x01, 0x02, 0x02, 0x01,
    0x00, 0x02, 0x01, 0x00, 0x30, 0x36, 0x0c, 0x19, 0x31, 0x2e, 0x33, 0x2e,
    0x36, 0x2e, 0x31, 0x2e, 0x34, 0x2e, 0x31, 0x2e, 0x35, 0x31, 0x34, 0x31,
    0x34, 0x2e, 0x30, 0x2e, 0x30, 0x2e, 0x31, 0x2e, 0x30, 0x0c, 0x19, 0x31,
    0x2e, 0x33, 0x2e, 0x36, 0x2e, 0x31, 0x2e, 0x34, 0x2e, 0x31, 0x2e, 0x35,
    0x31, 0x34, 0x31, 0x34, 0x2e, 0x30, 0x2e, 0x30, 0x2e, 0x32, 0x2e, 0x30,
    0x0c, 0x03, 0x43, 0x54, 0x54, 0x0c, 0x05, 0x49, 0x6e, 0x74, 0x65, 0x6c,
    0x30, 0x2a, 0x06, 0x0a, 0x2b, 0x06, 0x01, 0x04, 0x01, 0x83, 0x91, 0x56,
    0x01, 0x01, 0x04, 0x1c, 0x30, 0x1a, 0x06, 0x0b, 0x2b, 0x06, 0x01, 0x04,
    0x01, 0x83, 0x91, 0x56, 0x01, 0x01, 0x00, 0x06, 0x0b, 0x2b, 0x06, 0x01,
    0x04, 0x01, 0x83, 0x91, 0x56, 0x01, 0x01, 0x01, 0x30, 0x30, 0x06, 0x0a,
    0x2b, 0x06, 0x01, 0x04, 0x01, 0x83, 0x91, 0x56, 0x01, 0x02, 0x04, 0x22,
    0x30, 0x20, 0x0c, 0x0e, 0x31, 0x2e, 0x33, 0x2e, 0x36, 0x2e, 0x31, 0x2e,
    0x34, 0x2e, 0x31, 0x2e, 0x37, 0x31, 0x0c, 0x09, 0x44, 0x69, 0x73, 0x63,
    0x6f, 0x76, 0x65, 0x72, 0x79, 0x0c, 0x03, 0x31, 0x2e, 0x30, 0x30, 0x0a,
    0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x02, 0x03, 0x48,
    0x00, 0x30, 0x45, 0x02, 0x20, 0x21, 0xac, 0x87, 0x42, 0x81, 0x04, 0x85,
    0x2f, 0x99, 0x38, 0xd1, 0xfb, 0x1f, 0x9c, 0x2e, 0xa7, 0x56, 0xca, 0x58,
    0xb5, 0x89, 0xd4, 0x02, 0x7f, 0x2f, 0x6a, 0x63, 0x91, 0x4e, 0xdf, 0xe8,
    0x5e, 0x02, 0x21, 0x00, 0xd7, 0x0c, 0xc3, 0x66, 0x90, 0xc2, 0x7f, 0xa7,
    0x27, 0x97, 0xc1, 0x0a, 0x24, 0x1b, 0xdc, 0xb8, 0xd4, 0x48, 0xc1, 0xb6,
    0x8f, 0xce, 0xaa, 0x82, 0x0f, 0xb0, 0x3a, 0xd7, 0x41, 0x06, 0x6e, 0x1d
  };

  unsigned char my_key[] = {
    0x30, 0x77, 0x02, 0x01, 0x01, 0x04, 0x20, 0x34, 0x23, 0xa2, 0xf0,
    0x44, 0x8a, 0xe4, 0x4c, 0x8b, 0x21, 0x7e, 0x4c, 0x0d, 0x68, 0x8a,
    0xdc, 0xea, 0x5e, 0xcf, 0xb8, 0x60, 0x0a, 0x97, 0xe3, 0x5a, 0x78,
    0x13, 0xfb, 0x12, 0x48, 0xad, 0x0d, 0xa0, 0x0a, 0x06, 0x08, 0x2a,
    0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07, 0xa1, 0x44, 0x03, 0x42,
    0x00, 0x04, 0xe4, 0xbf, 0xcf, 0x9a, 0x29, 0x2d, 0x91, 0x54, 0x4b,
    0x6d, 0x2c, 0x67, 0x38, 0x7b, 0xe8, 0x7c, 0x58, 0x96, 0x60, 0x40,
    0xc7, 0x72, 0xc2, 0x41, 0xb7, 0x6f, 0xa6, 0x1a, 0x09, 0xe8, 0x85,
    0x96, 0xad, 0x02, 0x4c, 0x95, 0xbf, 0x67, 0x75, 0x24, 0x88, 0x98,
    0x3c, 0x2a, 0x9a, 0xdb, 0x95, 0x96, 0x62, 0x74, 0xce, 0x2d, 0x79,
    0xe8, 0x30, 0xfa, 0x4c, 0x4a, 0x97, 0x5e, 0xaf, 0xb9, 0xb3, 0x5c
  };

  unsigned char int_ca[] = {
    0x30, 0x82, 0x02, 0xfa, 0x30, 0x82, 0x02, 0xa1, 0xa0, 0x03, 0x02, 0x01,
    0x02, 0x02, 0x09, 0x00, 0xf3, 0x9b, 0x8c, 0xc0, 0x57, 0x2a, 0x11, 0xb5,
    0x30, 0x0a, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x02,
    0x30, 0x53, 0x31, 0x0c, 0x30, 0x0a, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x0c,
    0x03, 0x4f, 0x43, 0x46, 0x31, 0x22, 0x30, 0x20, 0x06, 0x03, 0x55, 0x04,
    0x0b, 0x0c, 0x19, 0x4b, 0x79, 0x72, 0x69, 0x6f, 0x20, 0x54, 0x65, 0x73,
    0x74, 0x20, 0x49, 0x6e, 0x66, 0x72, 0x61, 0x73, 0x74, 0x72, 0x75, 0x63,
    0x74, 0x75, 0x72, 0x65, 0x31, 0x1f, 0x30, 0x1d, 0x06, 0x03, 0x55, 0x04,
    0x03, 0x0c, 0x16, 0x4b, 0x79, 0x72, 0x69, 0x6f, 0x20, 0x54, 0x45, 0x53,
    0x54, 0x20, 0x52, 0x4f, 0x4f, 0x54, 0x20, 0x43, 0x41, 0x30, 0x30, 0x30,
    0x32, 0x30, 0x1e, 0x17, 0x0d, 0x31, 0x38, 0x31, 0x31, 0x33, 0x30, 0x31,
    0x38, 0x31, 0x32, 0x31, 0x35, 0x5a, 0x17, 0x0d, 0x32, 0x38, 0x31, 0x31,
    0x32, 0x36, 0x31, 0x38, 0x31, 0x32, 0x31, 0x35, 0x5a, 0x30, 0x5b, 0x31,
    0x0c, 0x30, 0x0a, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x0c, 0x03, 0x4f, 0x43,
    0x46, 0x31, 0x22, 0x30, 0x20, 0x06, 0x03, 0x55, 0x04, 0x0b, 0x0c, 0x19,
    0x4b, 0x79, 0x72, 0x69, 0x6f, 0x20, 0x54, 0x65, 0x73, 0x74, 0x20, 0x49,
    0x6e, 0x66, 0x72, 0x61, 0x73, 0x74, 0x72, 0x75, 0x63, 0x74, 0x75, 0x72,
    0x65, 0x31, 0x27, 0x30, 0x25, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0c, 0x1e,
    0x4b, 0x79, 0x72, 0x69, 0x6f, 0x20, 0x54, 0x45, 0x53, 0x54, 0x20, 0x49,
    0x6e, 0x74, 0x65, 0x72, 0x6d, 0x65, 0x64, 0x69, 0x61, 0x74, 0x65, 0x20,
    0x43, 0x41, 0x30, 0x30, 0x30, 0x32, 0x30, 0x59, 0x30, 0x13, 0x06, 0x07,
    0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01, 0x06, 0x08, 0x2a, 0x86, 0x48,
    0xce, 0x3d, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00, 0x04, 0xbc, 0x0f, 0x86,
    0x9f, 0x7a, 0x1f, 0x46, 0x91, 0xf8, 0xd1, 0x7b, 0x95, 0xa6, 0x90, 0x51,
    0x7f, 0xbf, 0x26, 0x0e, 0xd7, 0xdc, 0x94, 0xe9, 0x01, 0x77, 0xbf, 0xf7,
    0xdb, 0x24, 0x1c, 0x98, 0xad, 0x8b, 0x43, 0x4c, 0x26, 0xfe, 0xec, 0xa5,
    0xd9, 0xcc, 0x9e, 0x00, 0x13, 0xee, 0x37, 0xa3, 0x45, 0x71, 0x1f, 0x7e,
    0x2d, 0x89, 0x17, 0x67, 0x93, 0xf8, 0x3a, 0xfc, 0xbd, 0x47, 0x8d, 0xd0,
    0xbe, 0xa3, 0x82, 0x01, 0x54, 0x30, 0x82, 0x01, 0x50, 0x30, 0x12, 0x06,
    0x03, 0x55, 0x1d, 0x13, 0x01, 0x01, 0xff, 0x04, 0x08, 0x30, 0x06, 0x01,
    0x01, 0xff, 0x02, 0x01, 0x00, 0x30, 0x0e, 0x06, 0x03, 0x55, 0x1d, 0x0f,
    0x01, 0x01, 0xff, 0x04, 0x04, 0x03, 0x02, 0x01, 0x86, 0x30, 0x1d, 0x06,
    0x03, 0x55, 0x1d, 0x0e, 0x04, 0x16, 0x04, 0x14, 0x19, 0x73, 0x6a, 0x04,
    0x1a, 0x0b, 0x07, 0x70, 0x4f, 0x53, 0x79, 0x53, 0x36, 0x87, 0xfc, 0x0c,
    0xba, 0x7c, 0xae, 0x0b, 0x30, 0x1f, 0x06, 0x03, 0x55, 0x1d, 0x23, 0x04,
    0x18, 0x30, 0x16, 0x80, 0x14, 0x28, 0x48, 0xe4, 0xe5, 0x27, 0x58, 0xd9,
    0x08, 0xee, 0x09, 0x34, 0xe4, 0xb1, 0xbb, 0x3d, 0x59, 0x66, 0x1f, 0xc8,
    0xf5, 0x30, 0x81, 0x8d, 0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07,
    0x01, 0x01, 0x04, 0x81, 0x80, 0x30, 0x7e, 0x30, 0x55, 0x06, 0x08, 0x2b,
    0x06, 0x01, 0x05, 0x05, 0x07, 0x30, 0x02, 0x86, 0x49, 0x68, 0x74, 0x74,
    0x70, 0x3a, 0x2f, 0x2f, 0x74, 0x65, 0x73, 0x74, 0x70, 0x6b, 0x69, 0x2e,
    0x6b, 0x79, 0x72, 0x69, 0x6f, 0x2e, 0x63, 0x6f, 0x6d, 0x2f, 0x6f, 0x63,
    0x66, 0x2f, 0x34, 0x45, 0x36, 0x38, 0x45, 0x33, 0x46, 0x43, 0x46, 0x30,
    0x46, 0x32, 0x45, 0x34, 0x46, 0x38, 0x30, 0x41, 0x38, 0x44, 0x31, 0x34,
    0x33, 0x38, 0x46, 0x36, 0x41, 0x31, 0x42, 0x41, 0x35, 0x36, 0x39, 0x35,
    0x37, 0x31, 0x33, 0x44, 0x36, 0x33, 0x2e, 0x63, 0x72, 0x74, 0x30, 0x25,
    0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x30, 0x01, 0x86, 0x19,
    0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x74, 0x65, 0x73, 0x74, 0x6f,
    0x63, 0x73, 0x70, 0x2e, 0x6b, 0x79, 0x72, 0x69, 0x6f, 0x2e, 0x63, 0x6f,
    0x6d, 0x30, 0x5a, 0x06, 0x03, 0x55, 0x1d, 0x1f, 0x04, 0x53, 0x30, 0x51,
    0x30, 0x4f, 0xa0, 0x4d, 0xa0, 0x4b, 0x86, 0x49, 0x68, 0x74, 0x74, 0x70,
    0x3a, 0x2f, 0x2f, 0x74, 0x65, 0x73, 0x74, 0x70, 0x6b, 0x69, 0x2e, 0x6b,
    0x79, 0x72, 0x69, 0x6f, 0x2e, 0x63, 0x6f, 0x6d, 0x2f, 0x6f, 0x63, 0x66,
    0x2f, 0x34, 0x45, 0x36, 0x38, 0x45, 0x33, 0x46, 0x43, 0x46, 0x30, 0x46,
    0x32, 0x45, 0x34, 0x46, 0x38, 0x30, 0x41, 0x38, 0x44, 0x31, 0x34, 0x33,
    0x38, 0x46, 0x36, 0x41, 0x31, 0x42, 0x41, 0x35, 0x36, 0x39, 0x35, 0x37,
    0x31, 0x33, 0x44, 0x36, 0x33, 0x2e, 0x63, 0x72, 0x6c, 0x30, 0x0a, 0x06,
    0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x02, 0x03, 0x47, 0x00,
    0x30, 0x44, 0x02, 0x1f, 0x05, 0xe4, 0x45, 0x87, 0x7e, 0xbb, 0x9a, 0x4e,
    0x3c, 0x7e, 0x78, 0xe3, 0x00, 0x66, 0x05, 0x12, 0x73, 0xfd, 0xbd, 0x23,
    0xa6, 0x9b, 0xd4, 0x20, 0x7c, 0x7c, 0x21, 0x41, 0xf4, 0x0a, 0x2a, 0x02,
    0x21, 0x00, 0xc2, 0xf0, 0x29, 0xcc, 0x55, 0x33, 0x82, 0xe5, 0xa2, 0x28,
    0xa3, 0x96, 0x20, 0xe2, 0x4e, 0xc1, 0x0c, 0x33, 0x71, 0x6d, 0x14, 0x28,
    0x3e, 0xe8, 0xd8, 0x7a, 0xcd, 0x0e, 0x4d, 0x51, 0xa0, 0x3c
  };

  unsigned char root_ca[] = {
    0x30, 0x82, 0x01, 0xdf, 0x30, 0x82, 0x01, 0x85, 0xa0, 0x03, 0x02, 0x01,
    0x02, 0x02, 0x09, 0x00, 0xf3, 0x9b, 0x8c, 0xc0, 0x57, 0x2a, 0x11, 0xb2,
    0x30, 0x0a, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x02,
    0x30, 0x53, 0x31, 0x0c, 0x30, 0x0a, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x0c,
    0x03, 0x4f, 0x43, 0x46, 0x31, 0x22, 0x30, 0x20, 0x06, 0x03, 0x55, 0x04,
    0x0b, 0x0c, 0x19, 0x4b, 0x79, 0x72, 0x69, 0x6f, 0x20, 0x54, 0x65, 0x73,
    0x74, 0x20, 0x49, 0x6e, 0x66, 0x72, 0x61, 0x73, 0x74, 0x72, 0x75, 0x63,
    0x74, 0x75, 0x72, 0x65, 0x31, 0x1f, 0x30, 0x1d, 0x06, 0x03, 0x55, 0x04,
    0x03, 0x0c, 0x16, 0x4b, 0x79, 0x72, 0x69, 0x6f, 0x20, 0x54, 0x45, 0x53,
    0x54, 0x20, 0x52, 0x4f, 0x4f, 0x54, 0x20, 0x43, 0x41, 0x30, 0x30, 0x30,
    0x32, 0x30, 0x1e, 0x17, 0x0d, 0x31, 0x38, 0x31, 0x31, 0x33, 0x30, 0x31,
    0x37, 0x33, 0x31, 0x30, 0x35, 0x5a, 0x17, 0x0d, 0x32, 0x38, 0x31, 0x31,
    0x32, 0x37, 0x31, 0x37, 0x33, 0x31, 0x30, 0x35, 0x5a, 0x30, 0x53, 0x31,
    0x0c, 0x30, 0x0a, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x0c, 0x03, 0x4f, 0x43,
    0x46, 0x31, 0x22, 0x30, 0x20, 0x06, 0x03, 0x55, 0x04, 0x0b, 0x0c, 0x19,
    0x4b, 0x79, 0x72, 0x69, 0x6f, 0x20, 0x54, 0x65, 0x73, 0x74, 0x20, 0x49,
    0x6e, 0x66, 0x72, 0x61, 0x73, 0x74, 0x72, 0x75, 0x63, 0x74, 0x75, 0x72,
    0x65, 0x31, 0x1f, 0x30, 0x1d, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0c, 0x16,
    0x4b, 0x79, 0x72, 0x69, 0x6f, 0x20, 0x54, 0x45, 0x53, 0x54, 0x20, 0x52,
    0x4f, 0x4f, 0x54, 0x20, 0x43, 0x41, 0x30, 0x30, 0x30, 0x32, 0x30, 0x59,
    0x30, 0x13, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01, 0x06,
    0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00,
    0x04, 0x6b, 0x75, 0xb1, 0x4d, 0x90, 0x85, 0x07, 0x0a, 0xfe, 0x47, 0xe5,
    0x29, 0x21, 0x7d, 0x4c, 0x2a, 0xef, 0x29, 0xa0, 0xdc, 0x90, 0xb5, 0x9d,
    0x66, 0x8c, 0xaf, 0x3f, 0xac, 0xf4, 0x3a, 0xba, 0x8d, 0x76, 0xd0, 0x6c,
    0x71, 0x98, 0x15, 0x62, 0xc4, 0x87, 0x31, 0x06, 0x75, 0x47, 0x5f, 0x70,
    0x5b, 0x1b, 0x1f, 0x96, 0xf3, 0x6b, 0xf1, 0xb3, 0x15, 0x5b, 0x52, 0xb7,
    0x1d, 0x63, 0x24, 0xa6, 0xc8, 0xa3, 0x42, 0x30, 0x40, 0x30, 0x0f, 0x06,
    0x03, 0x55, 0x1d, 0x13, 0x01, 0x01, 0xff, 0x04, 0x05, 0x30, 0x03, 0x01,
    0x01, 0xff, 0x30, 0x0e, 0x06, 0x03, 0x55, 0x1d, 0x0f, 0x01, 0x01, 0xff,
    0x04, 0x04, 0x03, 0x02, 0x01, 0x86, 0x30, 0x1d, 0x06, 0x03, 0x55, 0x1d,
    0x0e, 0x04, 0x16, 0x04, 0x14, 0x28, 0x48, 0xe4, 0xe5, 0x27, 0x58, 0xd9,
    0x08, 0xee, 0x09, 0x34, 0xe4, 0xb1, 0xbb, 0x3d, 0x59, 0x66, 0x1f, 0xc8,
    0xf5, 0x30, 0x0a, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03,
    0x02, 0x03, 0x48, 0x00, 0x30, 0x45, 0x02, 0x20, 0x25, 0x31, 0x4c, 0x20,
    0x55, 0xe2, 0xfc, 0x77, 0x95, 0xb8, 0x8d, 0x97, 0x45, 0x27, 0x96, 0x60,
    0x72, 0x59, 0x3b, 0x5d, 0x3e, 0xba, 0x2c, 0xd3, 0x1f, 0x1a, 0x41, 0x31,
    0x4a, 0x35, 0x35, 0x9e, 0x02, 0x21, 0x00, 0xd3, 0xaf, 0x4e, 0x67, 0x77,
    0xd8, 0x0d, 0x24, 0x12, 0xd2, 0x29, 0x1d, 0xb8, 0x8a, 0x03, 0xcf, 0x91,
    0x14, 0x30, 0x8f, 0x25, 0x68, 0xcd, 0xe2, 0x5a, 0x31, 0xac, 0x10, 0xbb,
    0xbf, 0x42, 0x44
  };

  int credid =
    oc_pki_add_mfg_cert(0, my_crt, sizeof(my_crt), my_key, sizeof(my_key));

  oc_pki_add_mfg_intermediate_cert(0, credid, int_ca, sizeof(int_ca));

  oc_pki_add_mfg_trust_anchor(0, root_ca, sizeof(root_ca));

  oc_pki_set_security_profile(0, OC_SP_BLACK, OC_SP_BLACK, credid);
#endif /* OC_SECURITY && OC_PKI */

  while (quit != 1) {
    next_event = oc_main_poll();
    pthread_mutex_lock(&mutex);
    if (next_event == 0) {
      pthread_cond_wait(&cv, &mutex);
    } else {
      ts.tv_sec = (next_event / OC_CLOCK_SECOND);
      ts.tv_nsec = (next_event % OC_CLOCK_SECOND) * 1.e09 / OC_CLOCK_SECOND;
      pthread_cond_timedwait(&cv, &mutex, &ts);
    }
    pthread_mutex_unlock(&mutex);
  }

  oc_main_shutdown();

  return 0;
}
