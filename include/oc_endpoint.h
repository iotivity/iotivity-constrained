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

#ifndef OC_ENDPOINT_H
#define OC_ENDPOINT_H

#include "oc_helpers.h"

typedef enum { OCF_VER_1_0_0 = 2048, OIC_VER_1_1_0 = 2112 } ocf_version_t;

typedef struct
{
  uint16_t port;
  uint8_t address[16];
  uint8_t scope;
} oc_ipv6_addr_t;

typedef struct
{
  uint16_t port;
  uint8_t address[4];
} oc_ipv4_addr_t;

typedef struct
{
  uint8_t type;
  uint8_t address[6];
} oc_le_addr_t;

typedef struct oc_endpoint_t
{
  struct oc_endpoint_t *next;
  int device;
  enum transport_flags
  {
    DISCOVERY = 1 << 0,
    SECURED = 1 << 1,
    IPV4 = 1 << 2,
    IPV6 = 1 << 3,
    TCP = 1 << 4,
    GATT = 1 << 5,
    MULTICAST = 1 << 6
  } flags;

  union dev_addr
  {
    oc_ipv6_addr_t ipv6;
    oc_ipv4_addr_t ipv4;
    oc_le_addr_t bt;
  } addr;
  uint8_t priority;
  ocf_version_t version;
} oc_endpoint_t;

#define oc_make_ipv4_endpoint(__name__, __flags__, __port__, ...)              \
  oc_endpoint_t __name__ = {.flags = __flags__,                                \
                            .addr.ipv4 = {.port = __port__,                    \
                                          .address = { __VA_ARGS__ } } }
#define oc_make_ipv6_endpoint(__name__, __flags__, __port__, ...)              \
  oc_endpoint_t __name__ = {.flags = __flags__,                                \
                            .addr.ipv6 = {.port = __port__,                    \
                                          .address = { __VA_ARGS__ } } }

void oc_init_endpoint_list(void);
int oc_add_endpoint_to_list(oc_endpoint_t *endpoint);
oc_endpoint_t *oc_get_endpoint_list(void);
void oc_free_endpoint_list(void);
oc_endpoint_t *oc_new_endpoint(void);
void oc_free_endpoint(oc_endpoint_t *endpoint);
int oc_endpoint_to_string(oc_endpoint_t *endpoint, oc_string_t *endpoint_str);
int oc_string_to_endpoint(oc_string_t *endpoint_str, oc_endpoint_t *endpoint,
                          oc_string_t *uri);
int oc_ipv6_endpoint_is_link_local(oc_endpoint_t *endpoint);
int oc_endpoint_compare(const oc_endpoint_t *ep1, const oc_endpoint_t *ep2);
int oc_endpoint_compare_address(oc_endpoint_t *ep1, oc_endpoint_t *ep2);

#endif /* OC_ENDPOINT_H */
