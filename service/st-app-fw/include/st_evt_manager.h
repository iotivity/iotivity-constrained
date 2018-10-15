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
#ifndef ST_EVT_MANAGER_H
#define ST_EVT_MANAGER_H

#include "st_types.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  ST_EVT_INIT = 0,
  ST_EVT_START,
  ST_EVT_STOP,
  ST_EVT_DEINIT,
  ST_EVT_RUN,
  ST_EVT_START_WIFI_CONNECT,
  ST_EVT_RETRY_WIFI_CONNECT,
  ST_EVT_START_CLOUDMANAGER,
  ST_EVT_RESET,
  ST_EVT_MAX
} st_evt;

void st_evt_init(void);
void st_evt_deinit(void);

bool st_evt_is_in_queue(void);
st_evt st_evt_pop(void);
void st_evt_push(st_evt evt);

#ifdef __cplusplus
}
#endif

#endif