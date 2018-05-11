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

#ifndef IPCONTEXT_H
#define IPCONTEXT_H

#define WIN32_LEAN_AND_MEAN
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef OC_TCP
typedef struct tcp_context_t {
  struct sockaddr_storage server;
  int server_sock;
  uint16_t port;
#ifdef OC_SECURITY
  struct sockaddr_storage secure;
  int secure_sock;
  uint16_t tls_port;
#endif /* OC_SECURITY */
#ifdef OC_IPV4
  struct sockaddr_storage server4;
  int server4_sock;
  uint16_t port4;
#ifdef OC_SECURITY
  struct sockaddr_storage secure4;
  int secure4_sock;
  uint16_t tls4_port;
#endif /* OC_SECURITY */
#endif /* OC_IPV4 */
  int connect_pipe[2];
  pthread_mutex_t mutex;
} tcp_context_t;
#endif

typedef struct ip_context_t {
  struct ip_context_t *next;
  struct sockaddr_storage mcast;
  struct sockaddr_storage server;
  SOCKET mcast_sock;
  SOCKET server_sock;
  uint16_t port;
#ifdef OC_SECURITY
  struct sockaddr_storage secure;
  SOCKET secure_sock;
  uint16_t dtls_port;
#endif /* OC_SECURITY */
#ifdef OC_IPV4
  struct sockaddr_storage mcast4;
  struct sockaddr_storage server4;
  SOCKET mcast4_sock;
  SOCKET server4_sock;
  uint16_t port4;
#ifdef OC_SECURITY
  struct sockaddr_storage secure4;
  SOCKET secure4_sock;
  uint16_t dtls4_port;
#endif /* OC_SECURITY */
#endif /* OC_IPV4 */
#ifdef OC_TCP
  tcp_context_t tcp;
#endif
  HANDLE event_thread_handle;
  HANDLE event_server_handle;
  DWORD event_thread;
  BOOL terminate;
  int device;
} ip_context_t;

#endif /* IPCONTEXT_H */
