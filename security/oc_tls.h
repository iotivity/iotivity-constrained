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

#ifndef OC_TLS_H
#define OC_TLS_H
#ifdef OC_SECURITY

#include "mbedtls/ssl.h"
#include "oc_uuid.h"
#include "port/oc_connectivity.h"
#include "util/oc_etimer.h"
#include "util/oc_list.h"
#include "util/oc_process.h"
#include <stdbool.h>

OC_PROCESS_NAME(oc_tls_handler);

typedef void (*oc_sec_get_cpubkey_and_token)(uint8_t *cpubkey, int *cpubkey_len, uint8_t *token, int *token_len);
void oc_sec_set_cpubkey_and_token_load(oc_sec_get_cpubkey_and_token cpubkey_and_token_cb);

typedef void (*oc_sec_get_own_key)(uint8_t *priv_key, int *priv_key_len, uint8_t *pub_key, int *pub_key_len);
void oc_sec_set_own_key_load(oc_sec_get_own_key ownkey_cb);

int oc_tls_init_context(void);
bool oc_sec_load_certs(int device);
bool oc_sec_load_ca_cert(const unsigned char *ca_cert_buf,
                         size_t ca_cet_buf_len);
#ifdef OC_UNLOAD_CERT
void
oc_sec_unload_own_certs();
#endif
bool oc_sec_get_rpk_psk(int device, unsigned char *psk, int *psk_len);

void oc_tls_shutdown(void);
void oc_tls_close_connection(oc_endpoint_t *endpoint);

int oc_tls_update_psk_identity(int device);
bool oc_sec_derive_owner_psk(oc_endpoint_t *endpoint, const uint8_t *oxm,
                             const size_t oxm_len, const uint8_t *server_uuid,
                             const size_t server_uuid_len,
                             const uint8_t *obt_uuid, const size_t obt_uuid_len,
                             uint8_t *key, const size_t key_len);

void oc_tls_remove_peer(oc_endpoint_t *endpoint);
int oc_tls_send_message(oc_message_t *message);
oc_uuid_t *oc_tls_get_peer_uuid(oc_endpoint_t *endpoint);
bool oc_tls_connected(oc_endpoint_t *endpoint);

void oc_tls_elevate_anon_ciphersuite(void);
void oc_tls_demote_anon_ciphersuite(void);

typedef struct {
  struct oc_etimer fin_timer;
  oc_clock_time_t int_ticks;
} oc_tls_retr_timer_t;

typedef struct oc_tls_peer_s {
  struct oc_tls_peer_s *next;
  OC_LIST_STRUCT(recv_q);
  OC_LIST_STRUCT(send_q);
  mbedtls_ssl_context ssl_ctx;
  oc_endpoint_t endpoint;
  int role;
  oc_tls_retr_timer_t timer;
  uint8_t master_secret[48];
  uint8_t client_server_random[64];
  oc_uuid_t uuid;
  oc_clock_time_t timestamp;
} oc_tls_peer_t;

bool oc_sec_get_rpk_hmac(int device, unsigned char *hmac, int *hmac_len);

#endif /* OC_SECURITY */
#endif /* OC_TLS_H */
