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

#define __USE_GNU

#include "tcpadapter.h"
#include "ipcontext.h"
#include "messaging/coap/coap.h"
#include "oc_endpoint.h"
#include "util/oc_memb.h"
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef OC_TCP

#define OC_TCP_LISTEN_BACKLOG 3

#define TLS_HEADER_SIZE 5

typedef struct tcp_session
{
  struct tcp_session *next;
  oc_endpoint_t endpoint;
  int sock;
} tcp_session_t;

OC_LIST(session_list);
OC_MEMB(tcp_session_s, tcp_session_t, OC_MAX_TCP_PEERS);

static int
configure_tcp_socket(int sock, struct sockaddr_storage *sock_info)
{
  int reuse = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
    OC_ERR("setting reuseaddr option %d", errno);
    return -1;
  }
  if (bind(sock, (struct sockaddr *)sock_info, sizeof(*sock_info)) == -1) {
    OC_ERR("binding socket %d", errno);
    return -1;
  }
  if (listen(sock, OC_TCP_LISTEN_BACKLOG) == -1) {
    OC_ERR("listening socket %d", errno);
    return -1;
  }

  return 0;
}

static int
get_assigned_tcp_port(int sock, struct sockaddr_storage *sock_info)
{

  socklen_t socklen = sizeof(*sock_info);
  if (getsockname(sock, (struct sockaddr *)sock_info, &socklen) == -1) {
    OC_ERR("obtaining socket information %d", errno);
    return -1;
  }

  return 0;
}

void
oc_tcp_add_socks_to_fd_set(ip_context_t *dev)
{
  FD_SET(dev->tcp.server_sock, &dev->rfds);
#ifdef OC_SECURITY
  FD_SET(dev->tcp.secure_sock, &dev->rfds);
#endif /* OC_SECURITY */

#ifdef OC_IPV4
  FD_SET(dev->tcp.server4_sock, &dev->rfds);
#ifdef OC_SECURITY
  FD_SET(dev->tcp.secure4_sock, &dev->rfds);
#endif /* OC_SECURITY */
#endif /* OC_IPV4 */
  FD_SET(dev->tcp.connect_pipe[0], &dev->rfds);
}

static int
add_new_session(int sock, const oc_endpoint_t *endpoint)
{
  tcp_session_t *session = oc_memb_alloc(&tcp_session_s);
  if (!session) {
    OC_ERR("session alloc failed");
    return -1;
  }

  memcpy(&session->endpoint, endpoint, sizeof(oc_endpoint_t));
  session->sock = sock;

  oc_list_add(session_list, session);

  return 0;
}

static int
accecpt_new_session(ip_context_t *dev, int fd, fd_set *setfds,
                    oc_endpoint_t *endpoint)
{
  struct sockaddr_storage receive_from;
  socklen_t receive_len = sizeof(receive_from);

  int new_socket = accept(fd, (struct sockaddr *)&receive_from, &receive_len);
  if (new_socket < 0) {
    OC_ERR("failed to accept incoming TCP connection");
    return -1;
  }
  OC_DBG("accepted incomming TCP connection");

  if (endpoint->flags & IPV6) {
    struct sockaddr_in6 *r = (struct sockaddr_in6 *)&receive_from;
    memcpy(endpoint->addr.ipv6.address, r->sin6_addr.s6_addr,
           sizeof(r->sin6_addr.s6_addr));
    endpoint->addr.ipv6.scope = r->sin6_scope_id;
    endpoint->addr.ipv6.port = ntohs(r->sin6_port);
#ifdef OC_IPV4
  } else if (endpoint->flags & IPV4) {
    struct sockaddr_in *r = (struct sockaddr_in *)&receive_from;
    memcpy(endpoint->addr.ipv4.address, &r->sin_addr.s_addr,
           sizeof(r->sin_addr.s_addr));
    endpoint->addr.ipv4.port = ntohs(r->sin_port);
#endif /* !OC_IPV4 */
  }

  FD_CLR(fd, setfds);
  if (add_new_session(new_socket, endpoint) < 0) {
    OC_ERR("recorded new TCP session");
    close(new_socket);
    return -1;
  }
  FD_SET(new_socket, &dev->rfds);

  return 0;
}

static tcp_session_t *
find_session_by_endpoint(oc_endpoint_t *endpoint)
{
  tcp_session_t *session = oc_list_head(session_list);
  while (session != NULL &&
         oc_endpoint_compare(&session->endpoint, endpoint) != 0) {
    session = session->next;
  }
  if (!session) {
    OC_DBG("could not find ongoing TCP session for endpoint:");
    OC_LOGipaddr(*endpoint);
    return NULL;
  }
  OC_DBG("found TCP session for endpoint:");
  OC_LOGipaddr(*endpoint);
  return session;
}

static tcp_session_t *
get_ready_to_read_session(fd_set *setfds)
{
  tcp_session_t *session = oc_list_head(session_list);
  while (session != NULL && !FD_ISSET(session->sock, setfds)) {
    session = session->next;
  }
  if (!session) {
    OC_DBG("No exist TCP session");
    return NULL;
  }
  OC_DBG("The session is found");
  return session;
}

static size_t
get_total_length_from_header(oc_message_t *message, oc_endpoint_t *endpoint)
{
  size_t total_length = 0;
  if (endpoint->flags & SECURED) {
    //[3][4] bytes in tls header are tls payload length
    total_length =
      TLS_HEADER_SIZE + (size_t)((message->data[3] << 8) | message->data[4]);
  } else {
    total_length = coap_tcp_get_packet_size(message->data);
  }

  return total_length;
}

tcp_receive_state_t
oc_tcp_receive_message(ip_context_t *dev, fd_set *fds, oc_message_t *message)
{
  message->endpoint.device = dev->device;

  if (FD_ISSET(dev->tcp.server_sock, fds)) {
    message->endpoint.flags = IPV6 | TCP;
    if (accecpt_new_session(dev, dev->tcp.server_sock, fds,
                            &message->endpoint) < 0) {
      OC_ERR("accept new session fail");
      return TCP_STATUS_ERROR;
    }
    return TCP_STATUS_ACCEPT;
#ifdef OC_SECURITY
  } else if (FD_ISSET(dev->tcp.secure_sock, fds)) {
    message->endpoint.flags = IPV6 | SECURED | TCP;
    if (accecpt_new_session(dev, dev->tcp.secure_sock, fds,
                            &message->endpoint) < 0) {
      OC_ERR("accept new session fail");
      return TCP_STATUS_ERROR;
    }
    return TCP_STATUS_ACCEPT;
#endif /* OC_SECURITY */
#ifdef OC_IPV4
  } else if (FD_ISSET(dev->tcp.server4_sock, fds)) {
    message->endpoint.flags = IPV4 | TCP;
    if (accecpt_new_session(dev, dev->tcp.server4_sock, fds,
                            &message->endpoint) < 0) {
      OC_ERR("accept new session fail");
      return TCP_STATUS_ERROR;
    }
    return TCP_STATUS_ACCEPT;
#ifdef OC_SECURITY
  } else if (FD_ISSET(dev->tcp.secure4_sock, fds)) {
    message->endpoint.flags = IPV4 | SECURED | TCP;
    if (accecpt_new_session(dev, dev->tcp.secure4_sock, fds,
                            &message->endpoint) < 0) {
      OC_ERR("accept new session fail");
      return TCP_STATUS_ERROR;
    }
    return TCP_STATUS_ACCEPT;
#endif /* OC_SECURITY */
#endif /* OC_IPV4 */
  } else if (FD_ISSET(dev->tcp.connect_pipe[0], fds)) {
    ssize_t len = read(dev->tcp.connect_pipe[0], message->data, OC_PDU_SIZE);
    if (len < 0) {
      OC_ERR("read error! %d", errno);
      return TCP_STATUS_ERROR;
    }
    OC_DBG("received new connection event [%.*s]", len,
           (char *)message->data);
    return TCP_STATUS_NONE;
  }

  // find session.
  tcp_session_t *session = get_ready_to_read_session(fds);
  if (!session) {
    OC_DBG("could not find TCP session socket in fd set");
    return TCP_STATUS_NONE;
  }

  // receive message.
  size_t total_length = 0;
  size_t want_read = OC_PDU_SIZE;
  message->length = 0;
  do {
    int count =
      recv(session->sock, message->data + message->length, want_read, 0);
    if (count < 0) {
      OC_ERR("recv error! %d", errno);
      FD_CLR(session->sock, &dev->rfds);
      close(session->sock);
      oc_list_remove(session_list, session);
      oc_memb_free(&tcp_session_s, session);
      return TCP_STATUS_ERROR;
    } else if (count == 0) {
      OC_DBG("peer closed TCP session");
      FD_CLR(session->sock, &dev->rfds);
      close(session->sock);
      oc_list_remove(session_list, session);
      oc_memb_free(&tcp_session_s, session);
      return TCP_STATUS_NONE;
    }

    OC_DBG("recv(): %d bytes.", count);
    message->length += (size_t)count;
    want_read -= (size_t)count;

    if (total_length == 0) {
      total_length = get_total_length_from_header(message, &session->endpoint);
      if (total_length >
          (unsigned)(OC_MAX_APP_DATA_SIZE + COAP_MAX_HEADER_SIZE)) {
        OC_ERR("total receive length(%ld) is bigger than max pdu size(%ld)",
               total_length, (OC_MAX_APP_DATA_SIZE + COAP_MAX_HEADER_SIZE));
        OC_ERR("It may occur buffer overflow.");
        return TCP_STATUS_ERROR;
      }
      OC_DBG("tcp packet total length : %ld bytes.", total_length);

      want_read = total_length - (size_t)count;
    }
  } while (total_length > message->length);

  memcpy(&message->endpoint, &session->endpoint, sizeof(oc_endpoint_t));

  FD_CLR(session->sock, fds);
  return TCP_STATUS_RECEIVE;
}

static int
get_session_socket(oc_endpoint_t *endpoint)
{
  int sock = -1;
  tcp_session_t *session = find_session_by_endpoint(endpoint);
  if (!session) {
    return -1;
  }

  sock = session->sock;
  return sock;
}

static int
initiate_new_session(ip_context_t *dev, oc_endpoint_t *endpoint,
                     const struct sockaddr_storage *receiver)
{
  int sock = -1;

  if (endpoint->flags & IPV6) {
    sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
#ifdef OC_IPV4
  } else if (endpoint->flags & IPV4) {
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#endif
  }

  if (sock < 0) {
    OC_ERR("could not create socket for new TCP session");
    return -1;
  }

  socklen_t receiver_size = sizeof(*receiver);
  if (connect(sock, (struct sockaddr *)receiver, receiver_size) < 0) {
    OC_ERR("could not initiate TCP connection");
    close(sock);
    return -1;
  }
  OC_DBG("successfully initiated TCP connection");

  if (add_new_session(sock, endpoint) < 0) {
    OC_ERR("could not add new TCP session");
    close(sock);
    return -1;
  }
  FD_SET(sock, &dev->rfds);

  oc_string_t ep_str;
  if (oc_endpoint_to_string(endpoint, &ep_str) == 0) {
    ssize_t len = 0;
    do {
      len = write(dev->tcp.connect_pipe[1], oc_string(ep_str),
                  oc_string_len(ep_str));
    } while (len == -1 && errno == EINTR);
    oc_free_string(&ep_str);
    OC_DBG("sent connection event to receive thread\n");
  } else {
    OC_ERR("could not create endpoint string");
    close(sock);
    return -1;
  }

  return sock;
}

void
oc_tcp_send_buffer(ip_context_t *dev, oc_message_t *message,
                   const struct sockaddr_storage *receiver)
{
  int send_sock = get_session_socket(&message->endpoint);

  if (send_sock < 0) {
    if ((send_sock = initiate_new_session(dev, &message->endpoint, receiver)) <
        0) {
      OC_ERR("could not initiate new TCP session");
      return;
    }
  }

  size_t bytes_sent = 0;
  do {
    ssize_t send_len = send(send_sock, message->data + bytes_sent,
                            message->length - bytes_sent, 0);
    if (send_len < 0) {
      OC_WRN("send() returned errno %d", errno);
      return;
    }
    bytes_sent += send_len;
  } while (bytes_sent < message->length);

  OC_DBG("Sent %d bytes", bytes_sent);
}

#ifdef OC_IPV4
static int
tcp_connectivity_ipv4_init(ip_context_t *dev)
{
  OC_DBG("Initializing TCP adapter IPv4 for device %d", dev->device);

  memset(&dev->tcp.server4, 0, sizeof(struct sockaddr_storage));
  struct sockaddr_in *l = (struct sockaddr_in *)&dev->tcp.server4;
  l->sin_family = AF_INET;
  l->sin_addr.s_addr = INADDR_ANY;
  l->sin_port = 0;

#ifdef OC_SECURITY
  memset(&dev->tcp.secure4, 0, sizeof(struct sockaddr_storage));
  struct sockaddr_in *sm = (struct sockaddr_in *)&dev->tcp.secure4;
  sm->sin_family = AF_INET;
  sm->sin_addr.s_addr = INADDR_ANY;
  sm->sin_port = 0;
#endif /* OC_SECURITY */

  dev->tcp.server4_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  if (dev->tcp.server4_sock < 0) {
    OC_ERR("creating TCP server socket");
    return -1;
  }

#ifdef OC_SECURITY
  dev->tcp.secure4_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (dev->tcp.secure4_sock < 0) {
    OC_ERR("creating TCP secure socket");
    return -1;
  }
#endif /* OC_SECURITY */

  if (configure_tcp_socket(dev->tcp.server4_sock, &dev->tcp.server4) < 0) {
    OC_ERR("set socket option in server socket");
    return -1;
  }

  if (get_assigned_tcp_port(dev->tcp.server4_sock, &dev->tcp.server4) < 0) {
    OC_ERR("get port for server socket");
    return -1;
  }
  dev->tcp.port4 = ntohs(((struct sockaddr_in *)&dev->tcp.server4)->sin_port);

#ifdef OC_SECURITY
  if (configure_tcp_socket(dev->tcp.secure4_sock, &dev->tcp.secure4) < 0) {
    OC_ERR("set socket option in secure socket");
    return -1;
  }

  if (get_assigned_tcp_port(dev->tcp.secure4_sock, &dev->tcp.secure4) < 0) {
    OC_ERR("get port for secure socket");
    return -1;
  }
  dev->tcp.tls4_port =
    ntohs(((struct sockaddr_in *)&dev->tcp.secure4)->sin_port);
#endif /* OC_SECURITY */

  OC_DBG("Successfully initialized TCP adapter IPv4 for device %d",
         dev->device);

  return 0;
}
#endif /* OC_IPV4 */

int
oc_tcp_connectivity_init(ip_context_t *dev)
{
  OC_DBG("Initializing TCP adapter for device %d", dev->device);

  memset(&dev->tcp.server, 0, sizeof(struct sockaddr_storage));
  struct sockaddr_in6 *l = (struct sockaddr_in6 *)&dev->tcp.server;
  l->sin6_family = AF_INET6;
  l->sin6_addr = in6addr_any;
  l->sin6_port = 0;

#ifdef OC_SECURITY
  memset(&dev->tcp.secure, 0, sizeof(struct sockaddr_storage));
  struct sockaddr_in6 *sm = (struct sockaddr_in6 *)&dev->tcp.secure;
  sm->sin6_family = AF_INET6;
  sm->sin6_addr = in6addr_any;
  sm->sin6_port = 0;
#endif /* OC_SECURITY */

  dev->tcp.server_sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);

  if (dev->tcp.server_sock < 0) {
    OC_ERR("creating TCP server socket");
    return -1;
  }

#ifdef OC_SECURITY
  dev->tcp.secure_sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
  if (dev->tcp.secure_sock < 0) {
    OC_ERR("creating TCP secure socket");
    return -1;
  }
#endif /* OC_SECURITY */

  if (configure_tcp_socket(dev->tcp.server_sock, &dev->tcp.server) < 0) {
    OC_ERR("set socket option in server socket");
    return -1;
  }

  if (get_assigned_tcp_port(dev->tcp.server_sock, &dev->tcp.server) < 0) {
    OC_ERR("get port for server socket");
    return -1;
  }
  dev->tcp.port = ntohs(((struct sockaddr_in *)&dev->tcp.server)->sin_port);

#ifdef OC_SECURITY
  if (configure_tcp_socket(dev->tcp.secure_sock, &dev->tcp.secure) < 0) {
    OC_ERR("set socket option in secure socket");
    return -1;
  }

  if (get_assigned_tcp_port(dev->tcp.secure_sock, &dev->tcp.secure) < 0) {
    OC_ERR("get port for secure socket");
    return -1;
  }
  dev->tcp.tls_port = ntohs(((struct sockaddr_in *)&dev->tcp.secure)->sin_port);
#endif /* OC_SECURITY */

#ifdef OC_IPV4
  if (tcp_connectivity_ipv4_init(dev) != 0) {
    OC_ERR("Could not initialize IPv4 for TCP");
  }
#endif /* OC_IPV4 */

  if (pipe(dev->tcp.connect_pipe) < 0) {
    OC_ERR("Could not initialize connection pipe");
  }

  OC_DBG("=======tcp port info.========");
  OC_DBG("  ipv6 port   : %u", dev->tcp.port);
#ifdef OC_SECURITY
  OC_DBG("  ipv6 secure : %u", dev->tcp.tls_port);
#endif
#ifdef OC_IPV4
  OC_DBG("  ipv4 port   : %u", dev->tcp.port4);
#ifdef OC_SECURITY
  OC_DBG("  ipv4 secure : %u", dev->tcp.tls4_port);
#endif
#endif

  OC_DBG("Successfully initialized TCP adapter for device %d", dev->device);

  return 0;
}

void
oc_tcp_connectivity_shutdown(ip_context_t *dev)
{
  close(dev->tcp.server_sock);

#ifdef OC_IPV4
  close(dev->tcp.server4_sock);
#endif /* OC_IPV4 */

#ifdef OC_SECURITY
  close(dev->tcp.secure_sock);
#ifdef OC_IPV4
  close(dev->tcp.secure4_sock);
#endif /* OC_IPV4 */
#endif /* OC_SECURITY */

  close(dev->tcp.connect_pipe[0]);
  close(dev->tcp.connect_pipe[1]);

  tcp_session_t *session = (tcp_session_t *)oc_list_head(session_list), *next;
  while (session != NULL) {
    next = session->next;
    if (session->endpoint.device == dev->device) {
      oc_list_remove(session_list, session);
      oc_memb_free(&tcp_session_s, session);
    }
    session = next;
  }

  OC_DBG("oc_tcp_connectivity_shutdown for device %d", dev->device);
}

#endif /* OC_TCP */
