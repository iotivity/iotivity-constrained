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

#include "oc_server.h"

static bool light_state = false;

void
app_init(void)
{
  oc_new_platform("Intel");
  oc_add_platform();
  
  oc_new_device("/oic/d", "oic.d.light", "Kishen's light", "1.0", "1.0"); 
  oc_set_custom_device_property(purpose, "desk lamp");
  oc_add_device();
}

#ifdef OC_SECURITY
void
fetch_credentials(void)
{
  oc_storage_config("./creds");
}
#endif

static void
get_light(oc_request_t *request, oc_interface_mask_t interface)
{
  PRINT("GET_light:\n");
  oc_rep_start_root_object();  
  switch (interface) {
  case OC_IF_BASELINE:
    oc_process_baseline_interface(request->resource);
  case OC_IF_DEFAULT:
  case OC_IF_RW:
    oc_rep_set_boolean(root, state, light_state);
    break;
  default:
    break;
  }
  oc_rep_end_root_object();
  oc_send_response(request, OK);
  PRINT("Light state %d\n", light_state);
}

static void
put_light(oc_request_t *request, oc_interface_mask_t interface)
{
  PRINT("PUT_light:\n");
  bool state = false;
  oc_rep_t *rep = request->request_payload;
  while(rep != NULL) {
    PRINT("key: %s ", oc_string(rep->name));
    switch(rep->type) {
    case BOOL:
      state = rep->value_boolean;
      PRINT("value: %d\n", state);
      break;
    default:
      oc_send_response(request, BAD_REQUEST);
      return;
      break;
    }
    rep = rep->next;
  }
  oc_send_response(request, OK);
  light_state = state;
}

void
register_resources(void)
{
  oc_resource_t *res = oc_new_resource("/light/1", 1, 0);
  oc_resource_bind_resource_type(res, "oic.r.light");
  oc_resource_bind_resource_interface(res, OC_IF_RW);
  
#ifdef OC_SECURITY  
  oc_resource_make_secure(res);
#endif

  oc_resource_set_discoverable(res);
  oc_resource_set_periodic_observable(res, 1);
  oc_resource_set_request_handler(res, OC_GET, get_light);
  oc_resource_set_request_handler(res, OC_PUT, put_light);
  oc_add_resource(res);
}
