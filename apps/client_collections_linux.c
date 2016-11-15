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

#include "oc_api.h"
#include "port/oc_clock.h"

#include <pthread.h>
#include <signal.h>
#include <stdio.h>

static pthread_mutex_t mutex;
static pthread_cond_t cv;
static struct timespec ts;
static int quit = 0;

static void
set_device_custom_property(void *data)
{
  oc_set_custom_device_property(purpose, "operate lamp");
}

static void
app_init(void)
{
  oc_init_platform("Apple", NULL, NULL);
  oc_add_device("/oic/d", "oic.d.phone", "Kishen's IPhone", "1.0", "1.0",
                set_device_custom_property, NULL);
}

#define MAX_URI_LENGTH (30)
static char lights[MAX_URI_LENGTH];
static oc_server_handle_t lights_server;
static bool do_once = true;
static void get_lights_oic_if_b(oc_client_response_t *data);

static void
post_lights_oic_if_b(oc_client_response_t *data)
{
  PRINT("\nPOST_lights_oic_if_b:\n");
  if (data->code == OC_STATUS_CHANGED)
    PRINT("POST response OK\n");
  else
    PRINT("POST response code %d\n", data->code);

  oc_rep_t *ll = data->payload;

  while (ll != NULL) {
    PRINT("\tLink:\n");
    oc_rep_t *link = ll->value_object;
    while (link != NULL) {
      switch (link->type) {
      case STRING:
        PRINT("\t\tkey: %s value: %s\n", oc_string(link->name),
              oc_string(link->value_string));
        break;
      case OBJECT: {
        PRINT("\t\tkey: %s value: { ", oc_string(link->name));
        oc_rep_t *rep = link->value_object;
        while (rep != NULL) {
          switch (rep->type) {
          case BOOL:
            PRINT(" %s : %d ", oc_string(rep->name), rep->value_int);
            break;
          case INT:
            PRINT(" %s : %d ", oc_string(rep->name), rep->value_boolean);
            break;
          default:
            break;
          }
          rep = rep->next;
        }
        PRINT(" }\n\n");
      } break;
      default:
        break;
      }
      link = link->next;
    }
    ll = ll->next;
  }

  PRINT("\nSending GET %s?if=oic.if.b\n\n", lights);

  oc_do_get(lights, &lights_server, "if=oic.if.b", &get_lights_oic_if_b,
            LOW_QOS, NULL);
}

static void
get_lights_oic_if_b(oc_client_response_t *data)
{
  PRINT("\nGET_lights_oic_if_b:\n");
  oc_rep_t *ll = data->payload;

  while (ll != NULL) {
    PRINT("\tLink:\n");
    oc_rep_t *link = ll->value_object;
    while (link != NULL) {
      switch (link->type) {
      case STRING:
        PRINT("\t\tkey: %s value: %s\n", oc_string(link->name),
              oc_string(link->value_string));
        break;
      case OBJECT: {
        PRINT("\t\tkey: %s value: { ", oc_string(link->name));
        oc_rep_t *rep = link->value_object;
        while (rep != NULL) {
          switch (rep->type) {
          case BOOL:
            PRINT(" %s : %d ", oc_string(rep->name), rep->value_int);
            break;
          case INT:
            PRINT(" %s : %d ", oc_string(rep->name), rep->value_boolean);
            break;
          default:
            break;
          }
          rep = rep->next;
        }
        PRINT(" }\n\n");
      } break;
      default:
        break;
      }
      link = link->next;
    }
    ll = ll->next;
  }

  if (!do_once)
    return;

  PRINT("\nSending POST %s?if=oic.if.ll { state : true, counter : 200 }\n",
        lights);

  if (oc_init_post(lights, &lights_server, "if=oic.if.b", &post_lights_oic_if_b,
                   LOW_QOS, NULL)) {
    oc_rep_start_root_object();
    oc_rep_set_boolean(root, state, true);
    oc_rep_set_int(root, count, 200);
    oc_rep_end_root_object();
    if (oc_do_post())
      PRINT("Sent POST request\n\n");
    else
      PRINT("Could not send POST\n\n");
  } else
    PRINT("Could not init POST\n\n");

  do_once = false;
}

static void
get_lights_oic_if_ll(oc_client_response_t *data)
{
  PRINT("\nGET_lights_oic_if_ll:\n");
  oc_rep_t *ll = data->payload;

  while (ll != NULL) {
    PRINT("\tLink:\n");
    oc_rep_t *link = ll->value_object;
    while (link != NULL) {
      PRINT("\t\tkey: %s value: ", oc_string(link->name));
      switch (link->type) {
      case STRING:
        PRINT("%s\n", oc_string(link->value_string));
        break;
      case STRING_ARRAY: {
        PRINT("[ ");
        int i;
        for (i = 0; i < oc_string_array_get_allocated_size(link->value_array);
             i++) {
          PRINT(" %s ", oc_string_array_get_item(link->value_array, i));
        }
        PRINT(" ]\n");
      } break;
      case OBJECT: {
        PRINT("{ ");
        oc_rep_t *rep = link->value_object;
        while (rep != NULL) {
          PRINT(" %s : ", oc_string(rep->name));
          switch (rep->type) {
          case STRING:
            PRINT("%s ", oc_string(rep->value_string));
            break;
          default:
            break;
          }
          rep = rep->next;
        }
        PRINT(" }\n\n");
      } break;
      default:
        break;
      }
      link = link->next;
    }
    ll = ll->next;
  }

  PRINT("\nSending GET %s?if=oic.if.b\n\n", lights);

  oc_do_get(lights, &lights_server, "if=oic.if.b", &get_lights_oic_if_b,
            LOW_QOS, NULL);
}

static oc_discovery_flags_t
discovery(const char *di, const char *uri, oc_string_array_t types,
          oc_interface_mask_t interfaces, oc_server_handle_t *server,
          void *user_data)
{
  int i;
  int uri_len = strlen(uri);
  uri_len = (uri_len >= MAX_URI_LENGTH) ? MAX_URI_LENGTH - 1 : uri_len;

  for (i = 0; i < oc_string_array_get_allocated_size(types); i++) {
    char *t = oc_string_array_get_item(types, i);
    if (strlen(t) == 10 && strncmp(t, "oic.wk.col", 10) == 0) {
      memcpy(&lights_server, server, sizeof(oc_server_handle_t));

      strncpy(lights, uri, uri_len);
      lights[uri_len] = '\0';

      PRINT("\nSending GET %s?if=oic.if.ll\n\n", lights);

      oc_do_get(lights, &lights_server, "if=oic.if.ll", &get_lights_oic_if_ll,
                LOW_QOS, NULL);

      return OC_STOP_DISCOVERY;
    }
  }
  return OC_CONTINUE_DISCOVERY;
}

static void
issue_requests(void)
{
  oc_do_ip_discovery("oic.wk.col", &discovery, NULL);
}

static void
signal_event_loop(void)
{
  pthread_mutex_lock(&mutex);
  pthread_cond_signal(&cv);
  pthread_mutex_unlock(&mutex);
}

static void
handle_signal(int signal)
{
  signal_event_loop();
  quit = 1;
}

int
main(void)
{
  int init;
  struct sigaction sa;
  sigfillset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = handle_signal;
  sigaction(SIGINT, &sa, NULL);

  static const oc_handler_t handler = {.init = app_init,
                                       .signal_event_loop = signal_event_loop,
                                       .requests_entry = issue_requests };

  oc_clock_time_t next_event;

#ifdef OC_SECURITY
  oc_storage_config("./creds");
#endif /* OC_SECURITY */

  init = oc_main_init(&handler);
  if (init < 0)
    return init;

  while (quit != 1) {
    next_event = oc_main_poll();
    pthread_mutex_lock(&mutex);
    if (next_event == 0) {
      pthread_cond_wait(&cv, &mutex);
    } else {
      ts.tv_sec = (next_event / OC_CLOCK_SECOND);
      ts.tv_nsec = (next_event % OC_CLOCK_SECOND) * 1.e09 / OC_CLOCK_SECOND;
      pthread_cond_timedwait(&cv, &mutex, &ts);
    }
    pthread_mutex_unlock(&mutex);
  }

  oc_main_shutdown();
  return 0;
}
