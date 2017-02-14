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

#ifdef OC_SECURITY

#include "oc_cred.h"
#include "config.h"
#include "oc_api.h"
#include "oc_base64.h"
#include "oc_core_res.h"
#include "oc_doxm.h"
#include "oc_dtls.h"
#include "port/oc_log.h"
#include "util/oc_list.h"
#include "util/oc_memb.h"

OC_LIST(creds_l);
OC_MEMB(creds, oc_sec_cred_t, OC_MAX_NUM_SUBJECTS + 1);
#define OXM_JUST_WORKS "oic.sec.doxm.jw"

oc_sec_cred_t *
oc_sec_find_cred(oc_uuid_t *subjectuuid)
{
  oc_sec_cred_t *cred = oc_list_head(creds_l);
  while (cred != NULL) {
    if (memcmp(cred->subjectuuid.id, subjectuuid->id, 16) == 0) {
      return cred;
    }
    cred = cred->next;
  }
  return NULL;
}

oc_sec_cred_t *
oc_sec_get_cred(oc_uuid_t *subjectuuid)
{
  oc_sec_cred_t *cred = oc_sec_find_cred(subjectuuid);
  if (cred == NULL) {
    cred = oc_memb_alloc(&creds);
    memcpy(cred->subjectuuid.id, subjectuuid->id, 16);
    oc_list_add(creds_l, cred);
  }
  return cred;
}

void
oc_sec_encode_cred(void)
{
  oc_sec_cred_t *creds = oc_list_head(creds_l);
  char uuid[37];
  oc_rep_start_root_object();
  oc_process_baseline_interface(oc_core_get_resource_by_index(OCF_SEC_CRED));
  oc_rep_set_array(root, creds);
  if (creds == NULL) {
    oc_rep_object_array_start_item(creds);
    oc_rep_object_array_end_item(creds);
  }
  while (creds != NULL) {
    oc_rep_object_array_start_item(creds);
    oc_rep_set_int(creds, credid, creds->credid);
    oc_rep_set_int(creds, credtype, creds->credtype);
    oc_uuid_to_str(&creds->subjectuuid, uuid, 37);
    oc_rep_set_text_string(creds, subjectuuid, uuid);
    oc_rep_set_object(creds, privatedata);
    oc_rep_set_byte_string(privatedata, data,
        (const uint8_t *)creds->key, strlen((const char*)creds->key));
    oc_rep_set_text_string(privatedata, encoding, "oic.sec.encoding.raw");
    oc_rep_close_object(creds, privatedata);
    oc_rep_object_array_end_item(creds);
    creds = creds->next;
  }
  oc_rep_close_array(root, creds);
  oc_rep_end_root_object();
}

bool
oc_sec_decode_cred(oc_rep_t *rep, oc_sec_cred_t **owner)
{
  oc_sec_doxm_t *doxm = oc_sec_get_doxm();
  int credid = 0, credtype = 0;
  char subjectuuid[37] = { 0 };
  oc_uuid_t subject;
  oc_sec_cred_t *credobj;
  bool got_key = false, base64_key = false;
  int len = 0;
  uint8_t key[24];
  while (rep != NULL) {
    len = oc_string_len(rep->name);
    switch (rep->type) {
    case STRING:
      if (len == 10 && memcmp(oc_string(rep->name), "rowneruuid", 10) == 0) {
        oc_str_to_uuid(oc_string(rep->value.string), &doxm->rowneruuid);
      }
      break;
    case OBJECT_ARRAY: {
      oc_rep_t *creds_array = rep->value.object_array;
      while (creds_array != NULL) {
        oc_rep_t *cred = creds_array->value.object;
        bool valid_cred = false;
        while (cred != NULL) {
          len = oc_string_len(cred->name);
          valid_cred = true;
          switch (cred->type) {
          case INT:
            if (len == 6 && memcmp(oc_string(cred->name), "credid", 6) == 0)
              credid = cred->value.integer;
            else if (len == 8 &&
                     memcmp(oc_string(cred->name), "credtype", 8) == 0)
              credtype = cred->value.integer;
            break;
          case STRING:
            if (len == 11 &&
                memcmp(oc_string(cred->name), "subjectuuid", 11) == 0) {
              memcpy(subjectuuid, oc_string(cred->value.string),
                     oc_string_len(cred->value.string) + 1);
            }
            break;
          case OBJECT: {
            oc_rep_t *data = cred->value.object;
            while (data != NULL) {
              switch (data->type) {
              case STRING: {
                if (oc_string_len(data->name) == 8 &&
                    memcmp("encoding", oc_string(data->name), 8) == 0) {
                  if (oc_string_len(data->value.string) == 23 &&
                      memcmp("oic.sec.encoding.base64",
                             oc_string(data->value.string), 23) == 0) {
                    base64_key = true;
                  }
                } else if (oc_string_len(data->name) == 4 &&
                           memcmp(oc_string(data->name), "data", 4) == 0) {
                  uint8_t *p = oc_cast(data->value.string, uint8_t);
                  int size = oc_string_len(data->value.string);
                  if (size == 0)
                    goto next_item;
                  if (size != 24)
                    return false;
                  got_key = true;
                  memcpy(key, p, size);
                }
              } break;
              case BYTE_STRING: {
                uint8_t *p = oc_cast(data->value.string, uint8_t);
                int size = oc_string_len(data->value.string);
                if (size == 0)
                  goto next_item;
                if (size != 16)
                  return false;
                got_key = true;
                memcpy(key, p, 16);
              } break;
              default:
                break;
              }
            next_item:
              data = data->next;
            }
            if (got_key && base64_key) {
              oc_base64_decode(key, 24);
            }
          } break;
          default:
            break;
          }
          cred = cred->next;
        }
        if (valid_cred) {
          oc_str_to_uuid(subjectuuid, &subject);
          credobj = oc_sec_get_cred(&subject);
          credobj->credid = credid;
          credobj->credtype = credtype;

          if (got_key) {
            memcpy(credobj->key, key, 16);
          } else {
            if (owner)
              *owner = credobj;
          }
        }
        creds_array = creds_array->next;
      }
    } break;
    default:
      break;
    }
    rep = rep->next;
  }
  return true;
}

void
post_cred(oc_request_t *request, oc_interface_mask_t interface, void *data)
{
  (void)interface;
  (void)data;
  oc_sec_doxm_t *doxm = oc_sec_get_doxm();
  oc_sec_cred_t *owner = NULL;
  bool success = oc_sec_decode_cred(request->request_payload, &owner);
  if (owner && memcmp(owner->subjectuuid.id, doxm->rowneruuid.id, 16) == 0) {
    oc_uuid_t *dev = oc_core_get_device_id(0);
    oc_sec_derive_owner_psk(request->origin, (const uint8_t *)OXM_JUST_WORKS,
                            strlen(OXM_JUST_WORKS), owner->subjectuuid.id, 16,
                            dev->id, 16, owner->key, 16);
  }
  if (!success) {
    oc_send_response(request, OC_STATUS_BAD_REQUEST);
  } else {
    oc_send_response(request, OC_STATUS_CHANGED);
  }
}

#endif /* OC_SECURITY */
