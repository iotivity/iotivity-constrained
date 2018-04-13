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

#ifndef ES_ES_RESOURCES_H
#define ES_ES_RESOURCES_H

#include "estypes.h"
#include "enrolleecommon.h"
#include "oc_rep.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*es_connect_request_cb) (es_result_e, es_connect_request *);
typedef void (*es_wifi_conf_cb) (es_result_e, es_wifi_conf_data *);
typedef void (*es_coap_cloud_conf_cb) (es_result_e, es_coap_cloud_conf_data *);
typedef void (*es_dev_conf_cb) (es_result_e, es_dev_conf_data *);

typedef void (*es_write_userdata_cb) (oc_rep_t* payload, char* resource_type);
typedef void (*es_read_userdata_cb) (oc_rep_t* payload, char* resource_type, void** userdata);

void create_easysetup_resources(void);
void delete_easysetup_resources(void);

es_result_e set_device_property (es_device_property *device_property);
es_result_e set_enrollee_state (es_enrollee_state es_state);
es_result_e set_enrollee_err_code (es_error_code es_err_code);

void resgister_wifi_rsrc_event_callback (es_wifi_conf_cb);
void register_cloud_rsrc_event_callback (es_coap_cloud_conf_cb);
void register_devconf_rsrc_event_callback (es_dev_conf_cb);
void register_connect_request_event_callback (es_connect_request_cb cb);
void unregister_resource_event_callback (void);

#ifdef __cplusplus
}
#endif


#endif //ES_ES_RESOURCES_H
