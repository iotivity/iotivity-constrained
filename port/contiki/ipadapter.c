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

#include "oc_buffer.h"
#include "port/oc_connectivity.h"

#include "contiki.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/rpl/rpl.h"
#include "simple-udp.h"

#define OCF_MCAST_PORT_UNSECURED (5683)
#define OCF_SERVER_PORT_UNSECURED (56789)

static struct simple_udp_connection server, mcast;
PROCESS(ip_adapter_process, "IP Adapter");

void
handle_incoming_message(uint8_t *buffer, int size, uint8_t *addr, uint16_t port)
{
  oc_message_t *message = oc_allocate_message();

  if (message) {
    memcpy(message->data, buffer, size);
    message->length = size;
    message->endpoint.flags = IP;
    memcpy(message->endpoint.ipv6_addr.address, addr, 16);
    message->endpoint.ipv6_addr.port = port;

    PRINT("Incoming message from ");
    PRINTipaddr(message->endpoint);
    PRINT("\n");

    oc_network_event(message);
  }
}

static void
receive(struct simple_udp_connection *c, const uip_ipaddr_t *sender_addr,
        uint16_t sender_port, const uip_ipaddr_t *receiver_addr,
        uint16_t receiver_port, const uint8_t *data, uint16_t datalen)
{
  handle_incoming_message((uint8_t *)data, datalen, (uint8_t *)sender_addr,
                          sender_port);
}

static uip_ipaddr_t *
set_global_address(void)
{
  static uip_ipaddr_t ipaddr, mcast;

  uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

  /*
   * Joining the OCF multicast group at ff02::fd
   */
  uip_ip6addr(&mcast, 0xff02, 0, 0, 0, 0, 0, 0, 0x00fd);
  uip_ds6_maddr_t *rv = uip_ds6_maddr_add(&mcast);
  if (rv)
    LOG("Joined OCF multicast group\n");
  else
    LOG("Failed to join OCF multicast group\n");

  return &ipaddr;
}

static void
create_rpl_dag(uip_ipaddr_t *ipaddr)
{
  struct uip_ds6_addr *root_if;
  root_if = uip_ds6_addr_lookup(ipaddr);
  if (root_if != NULL) {
    rpl_dag_t *dag;
    uip_ipaddr_t prefix;
    rpl_set_root(RPL_DEFAULT_INSTANCE, ipaddr);
    dag = rpl_get_any_dag();
    uip_ip6addr(&prefix, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);
    rpl_set_prefix(dag, &prefix, 64);
    LOG("Created new RPL DAG\n");
  } else {
    LOG("Failed to create new RPL DAG\n");
  }
}

PROCESS_THREAD(ip_adapter_process, ev, data)
{
  static uip_ipaddr_t *ipaddr;

  PROCESS_BEGIN();

  ipaddr = set_global_address();

  create_rpl_dag(ipaddr);

  simple_udp_register(&mcast, OCF_MCAST_PORT_UNSECURED, NULL, 0, receive);

  simple_udp_register(&server, OCF_SERVER_PORT_UNSECURED, NULL, 0, receive);

  while (ev != PROCESS_EVENT_EXIT) {
    PROCESS_WAIT_EVENT();
  }
  PROCESS_END();
}

void
oc_send_buffer(oc_message_t *message)
{
  PRINT("Outgoing message to ");
  PRINTipaddr(message->endpoint);
  PRINT("\n");

  simple_udp_sendto_port(
    &server, message->data, message->length,
    (const uip_ipaddr_t *)message->endpoint.ipv6_addr.address,
    message->endpoint.ipv6_addr.port);
}

int
oc_connectivity_init(void)
{
  process_start(&ip_adapter_process, NULL);
  return 0;
}

void
oc_connectivity_shutdown(void)
{
  process_exit(&ip_adapter_process);
}

#ifdef OC_CLIENT
void
oc_send_multicast_message(oc_message_t *message)
{
  oc_send_buffer(message);
}
#endif /* OC_CLIENT */

// TODO:
#ifdef OC_SECURITY
uint16_t oc_connectivity_get_dtls_port(void);
#endif /* OC_SECURITY */

/*
 * oc_network_event_handler_mutex_* are defined only to comply with the
 * connectivity interface, but are not used since the adapter process does
 * not preempt the process running the event loop.
*/
void
oc_network_event_handler_mutex_init(void)
{
}

void
oc_network_event_handler_mutex_lock(void)
{
}

void
oc_network_event_handler_mutex_unlock(void)
{
}
