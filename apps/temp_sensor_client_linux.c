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

static void
app_init(void)
{
  oc_init_platform("GE", NULL, NULL);
  oc_add_device("/oic/d", "oic.d.smarthub", "Smart home hub", "1.0", "1.0",
                NULL, NULL);
}

#define MAX_URI_LENGTH (30)
static char temp_1[MAX_URI_LENGTH];
static oc_server_handle_t temp_sensor;
static int temperature;

static oc_event_callback_retval_t
stop_observe(void *data)
{
  PRINT("Stopping OBSERVE\n");
  oc_stop_observe(temp_1, &temp_sensor);
  return DONE;
}

static void
get_temp(oc_client_response_t *data)
{
  oc_rep_t *rep = data->payload;
  while (rep != NULL) {
    PRINT("key %s, value ", oc_string(rep->name));
    switch (rep->type) {
    case INT:
      PRINT("%d\n", rep->value_int);
      temperature = rep->value_int;
      break;
    default:
      break;
    }
    rep = rep->next;
  }
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
    if (strlen(t) == 16 && strncmp(t, "oic.r.tempsensor", 16) == 0) {
      memcpy(&temp_sensor, server, sizeof(oc_server_handle_t));

      strncpy(temp_1, uri, uri_len);
      temp_1[uri_len] = '\0';

      oc_do_observe(temp_1, &temp_sensor, NULL, &get_temp, HIGH_QOS, NULL);
      oc_set_delayed_callback(NULL, &stop_observe, 30);

      return OC_STOP_DISCOVERY;
    }
  }
  return OC_CONTINUE_DISCOVERY;
}

static void
issue_requests(void)
{
  oc_do_ip_discovery("oic.r.tempsensor", &discovery, NULL);
}

#include "port/oc_clock.h"
#include <pthread.h>
#include <signal.h>
#include <stdio.h>

static pthread_mutex_t mutex;
static pthread_cond_t cv;
static struct timespec ts;
static int quit = 0;

static void
signal_event_loop(void)
{
  pthread_mutex_lock(&mutex);
  pthread_cond_signal(&cv);
  pthread_mutex_lock(&mutex);
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
  struct sigaction sa;
  sigfillset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = handle_signal;
  sigaction(SIGINT, &sa, NULL);

  static const oc_handler_t handler = {.init = app_init,
                                       .signal_event_loop = signal_event_loop,
                                       .requests_entry = issue_requests };

  oc_clock_time_t next_event;

  int init = oc_main_init(&handler);

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
