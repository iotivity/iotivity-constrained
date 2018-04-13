/****************************************************************************
 *
 * Copyright 2018 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/

#include "esresources.h"
#include "oc_api.h"
#include "port/oc_clock.h"

#include "oc_core_res.h"

#include <pthread.h>
#include <signal.h>
#include <stdio.h>

#define OC_RSRVD_ES_RES_TYPE_PROV         "oic.r.easysetup"	 //"oic.wk.prov"
#define OC_RSRVD_ES_URI_PROV              "/EasySetupResURI"	 //"/ProvisioningResURI"
#define OC_RSRVD_ES_RES_TYPE_WIFI         "oic.r.wificonf"	 //"oic.wk.wifi"
#define OC_RSRVD_ES_URI_WIFI              "/WiFiConfResURI"	 //"/WiFiProvisioningResURI"
#define OC_RSRVD_ES_RES_TYPE_CLOUDSERVER  "oic.r.coapcloudconf"	 //"oic.wk.cloudserver"
#define OC_RSRVD_ES_URI_CLOUDSERVER       "/CoapCloudConfResURI" //"/CloudServerProvisioningResURI"
#define OC_RSRVD_ES_RES_TYPE_DEVCONF      "oic.r.devconf"	 //"oic.wk.devconf"
#define OC_RSRVD_ES_URI_DEVCONF           "/DevConfResURI"	 //"/DevConfProvisioningResURI"

es_connect_request_cb gConnectRequestEvtCb = NULL;
es_wifi_conf_cb gWifiConfRsrcEvtCb = NULL;
es_coap_cloud_conf_cb gCoapCloudConfRsrcEvtCb = NULL;
es_dev_conf_cb gDevConfRsrcEvtCb = NULL;

void resgister_wifi_rsrc_event_callback(es_wifi_conf_cb cb)
{
    gWifiConfRsrcEvtCb = cb;
}

void register_cloud_rsrc_event_callback(es_coap_cloud_conf_cb cb)
{
    gCoapCloudConfRsrcEvtCb = cb;
}

void register_devconf_rsrc_event_callback(es_dev_conf_cb cb)
{
    gDevConfRsrcEvtCb = cb;
}

void register_connect_request_event_callback(es_connect_request_cb cb)
{
    gConnectRequestEvtCb = cb;
}

void unregister_resource_event_callback(void)
{
    if (gWifiConfRsrcEvtCb) {
        gWifiConfRsrcEvtCb = NULL;
    }
    if (gCoapCloudConfRsrcEvtCb) {
        gCoapCloudConfRsrcEvtCb = NULL;
    }
    if (gDevConfRsrcEvtCb) {
        gDevConfRsrcEvtCb = NULL;
    }
    if (gConnectRequestEvtCb) {
        gConnectRequestEvtCb = NULL;
    }
}

static void get_devconf(oc_request_t *request, oc_interface_mask_t interface, void *user_data)
{
  (void)interface;
  (void)user_data;
  (void)request;
  PRINT("get_devconf:\n");

}

static void post_devconf(oc_request_t *request, oc_interface_mask_t interface, void *user_data)
{
  (void)interface;
  (void)user_data;
  (void)request;
  PRINT("POST_devconf:\n");
  
}

static void get_cloud(oc_request_t *request, oc_interface_mask_t interface, void *user_data)
{
  (void)interface;
  (void)user_data;
  (void)request;
  PRINT("get_cloud:\n");
  
}

static void post_cloud(oc_request_t *request, oc_interface_mask_t interface, void *user_data)
{
  (void)interface;
  (void)user_data;
  (void)request;
  PRINT("post_cloud:\n");
  
}

static void get_wifi(oc_request_t *request, oc_interface_mask_t interface, void *user_data)
{
  (void)interface;
  (void)user_data;
  (void)request;
  PRINT("get_wifi:\n");
  
}

static void post_wifi(oc_request_t *request, oc_interface_mask_t interface, void *user_data)
{
  (void)interface;
  (void)user_data;
  (void)request;
  PRINT("post_wifi:\n");
  
}

void create_easysetup_resources(void)
{
  oc_resource_t *wifi = oc_new_resource("wifi", OC_RSRVD_ES_URI_WIFI, 1, 0);
  oc_resource_bind_resource_type(wifi, OC_RSRVD_ES_RES_TYPE_WIFI);
  oc_resource_bind_resource_interface(wifi,OC_IF_BASELINE);
  oc_resource_set_default_interface(wifi, OC_IF_BASELINE);
  oc_resource_set_discoverable(wifi, true);
  oc_resource_set_periodic_observable(wifi, 1);
  oc_resource_set_request_handler(wifi, OC_GET, get_wifi, NULL);
  oc_resource_set_request_handler(wifi, OC_POST, post_wifi, NULL);
  oc_add_resource(wifi);

  oc_resource_t *cloud = oc_new_resource("cloud", OC_RSRVD_ES_URI_CLOUDSERVER, 1, 0);
  oc_resource_bind_resource_type(cloud, OC_RSRVD_ES_RES_TYPE_CLOUDSERVER);
  oc_resource_bind_resource_interface(cloud,OC_IF_BASELINE);
  oc_resource_set_default_interface(cloud, OC_IF_BASELINE);
  oc_resource_set_discoverable(cloud, true);
  oc_resource_set_periodic_observable(cloud, 1);
  oc_resource_set_request_handler(cloud, OC_GET, get_cloud, NULL);
  oc_resource_set_request_handler(cloud, OC_POST, post_cloud, NULL);
  oc_add_resource(cloud);

  oc_resource_t *devconf =
    oc_new_resource("devconf", OC_RSRVD_ES_URI_DEVCONF, 1, 0);
  oc_resource_bind_resource_type(devconf, OC_RSRVD_ES_RES_TYPE_DEVCONF);
  oc_resource_bind_resource_interface(devconf,OC_IF_BASELINE);
  oc_resource_set_default_interface(devconf, OC_IF_BASELINE);
  oc_resource_set_discoverable(devconf, true);
  oc_resource_set_periodic_observable(devconf, 1);
  oc_resource_set_request_handler(devconf, OC_GET, get_devconf, NULL);
  oc_resource_set_request_handler(devconf, OC_POST, post_devconf, NULL);
  oc_add_resource(devconf);

#ifdef OC_COLLECTIONS
  oc_resource_t *col = oc_new_collection("easysetup", OC_RSRVD_ES_URI_PROV, 2, 0);
  oc_resource_bind_resource_type(col, OC_RSRVD_ES_RES_TYPE_PROV);
  oc_resource_bind_resource_type(col, "oic.wk.col");
  oc_resource_bind_resource_interface(col,OC_IF_LL);
  oc_resource_bind_resource_interface(col,OC_IF_B);
  oc_resource_set_discoverable(col, true);
  oc_link_t *l1 = oc_new_link(wifi);
  oc_collection_add_link(col, l1);

  oc_link_t *l2 = oc_new_link(devconf);
  oc_collection_add_link(col, l2);

  oc_link_t *l3 = oc_new_link(cloud);
  oc_collection_add_link(col, l3);
  oc_add_collection(col);
#endif /* OC_COLLECTIONS */
}
