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

#ifndef ST_DATA_MANAGER_H
#define ST_DATA_MANAGER_H

#include "oc_helpers.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct st_device_info
{
  oc_string_t device_type;
  oc_string_t device_name;
  oc_string_t spec_version;
  oc_string_t data_model_version;
} st_device_info_t;

typedef struct st_platform_info
{
  oc_string_t manufacturer_name;
  oc_string_t manufacturer_uri;
  oc_string_t manufacturing_date;
  oc_string_t model_number;
  oc_string_t platform_version;
  oc_string_t os_version;
  oc_string_t hardware_version;
  oc_string_t firmware_version;
  oc_string_t vendor_id;
} st_platform_info_t;

typedef struct st_specification
{
  struct st_specification *next;
  st_device_info_t device;
  st_platform_info_t platform;
  size_t device_idx;
} st_specification_t;

typedef struct st_resource
{
  struct st_resource *next;
  oc_string_t uri;
  oc_string_array_t types;
  uint8_t interfaces;
  uint8_t default_interface;
  uint8_t policy;
  size_t device_idx;
} st_resource_info_t;

typedef enum {
  ST_PROP_TYPE_BOOL,
  ST_PROP_TYPE_INT,
  ST_PROP_TYPE_DOUBLE,
  ST_PROP_TYPE_STRING,
  ST_PROP_TYPE_OBJECT,
  ST_PROP_TYPE_BYTE,
  ST_PROP_TYPE_INT_ARRAY,
  ST_PROP_TYPE_DOUBLE_ARRAY,
  ST_PROP_TYPE_STRING_ARRAY,
  ST_PROP_TYPE_OBJECT_ARRAY,
} st_property_type_t;

typedef struct st_property
{
  struct st_property *next;
  oc_string_t key;
  int type;
  bool mandatory;
  int rw;
} st_property_t;

#define MAX_NUM_PROPERTIES 3

typedef struct st_resource_type
{
  struct st_resource_type *next;
  oc_string_t type;

#ifdef OC_DYNAMIC_ALLOCATION
  OC_LIST_STRUCT(properties);
#else  /* OC_DYNAMIC_ALLOCATION */
  st_property_t properties[MAX_NUM_PROPERTIES];
  int properties_cnt;
#endif /* !OC_DYNAMIC_ALLOCATION */
} st_resource_type_t;

/**
   If you want to enable other configuration feature,
   please remove this comments to define ST_CONF_ENABLED
 */
// #define ST_CONF_ENABLED

typedef struct
{
  struct st_easy_setup_info_t
  {
    struct st_conn_info_t
    {
      int type;
      struct st_soft_ap_info_t
      {
        oc_string_t setup_id;
        bool artik;
      } soft_ap;
    } connectivity;
    int ownership_transfer_method;
  } easy_setup;
#ifdef ST_CONF_ENABLED
  struct st_wifi_info_t
  {
    int interfaces;
    int frequency;
  } wifi;
  struct st_file_path_info_t
  {
    oc_string_t svrdb;
    oc_string_t provisioning;
    oc_string_t certificate;
    oc_string_t private_key;
  } file_path;
#endif /* ST_CONF_ENABLED */
} st_configuration_t;

int st_data_mgr_info_load(void);
void st_data_mgr_info_free(void);
void st_free_device_profile(void);

st_specification_t *st_data_mgr_get_spec_info(void);
st_resource_info_t *st_data_mgr_get_resource_info(void);
st_resource_type_t *st_data_mgr_get_rsc_type_info(const char *rt);
st_configuration_t *st_data_mgr_get_config_info(void);


#ifdef OC_RPK

#define ST_SECURE_KEY_LEN  (32)

/**
  @brief A data structure of RPK profile data.
*/
typedef struct st_rpk_profile {
  char sign_seckey[ST_SECURE_KEY_LEN]; /* ecc based DSA private key */
  char sign_pubkey[ST_SECURE_KEY_LEN]; /* ecc based DSA public key */
  char seckey[ST_SECURE_KEY_LEN];      /* ecc based key-exchange private key */
  char pubkey[ST_SECURE_KEY_LEN];      /* ecc based key-exchange public key */
  char sn[64];                         /* serial number of device */
  int  sn_len;
  unsigned char otmsupf;               /* Things's own supported features */
} st_rpk_profile_t;

/**
  @brief A function for getting RPK profile data.
  @return st_rpk_profile_t The pointer of RPK profile data which is declared in the framework
*/
st_rpk_profile_t *st_data_get_rpk_profile(void);

/**
  @brief A function for setting RPK profile data
    RPK profile data is expected to be retrived from the private section from the device.
    This function will copy the data into the framework area.
    No need to maintain this data from the application after this function call
  @return error_code 0 is succss, -1 is error case in alloc
  @param profile RPK profile data
*/
int st_data_load_rpk_profile(st_rpk_profile_t *profile);

/**
  @brief A free function for RPK profile data
*/
void st_free_rpk_profile(void);

#endif /*OC_RPK*/


#ifdef __cplusplus
}
#endif

#endif /* ST_DATA_MANAGER_H */
