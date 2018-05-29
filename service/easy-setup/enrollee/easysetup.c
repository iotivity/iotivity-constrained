/* ****************************************************************
 *
 * Copyright 2018 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************/

/**
 * @file
 *
 * This file contains the implementation for EasySetup Enrollee device.
 */

#include "easysetup.h"
#include "resourcehandler.h"
#include "oc_log.h"

es_result_e
es_init_enrollee(bool is_secured, es_resource_mask_e resource_mask,
                 es_provisioning_callbacks_s callbacks)
{
  return create_easysetup_resources(is_secured, resource_mask, callbacks);
}

es_result_e
es_set_device_property(es_device_property *device_property)
{
  return set_device_property(device_property);
}

es_result_e
es_set_state(es_enrollee_state es_state)
{
  return set_enrollee_state(es_state);
}

es_enrollee_state
es_get_state(void)
{
  return get_enrollee_state();
}

es_result_e
es_set_error_code(es_error_code es_err_code)
{
  return set_enrollee_err_code(es_err_code);
}

es_result_e
es_terminate_enrollee()
{
  delete_easysetup_resources();
  return ES_OK;
}

es_result_e
es_set_callback_for_userdata(es_read_userdata_cb readcb,
                             es_write_userdata_cb writecb)
{
  if (!readcb && !writecb) {
    OC_ERR("Both of callbacks for user data are Null!");
    return ES_ERROR;
  }

  set_callback_for_userdata(readcb, writecb);
  return ES_OK;
}

void
es_notify_connection_change(void)
{
  notify_connection_change();
}
