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

#include "st_process.h"

static st_process_data_t g_process_data;

int
st_process_init(void)
{
  g_process_data.mutex = st_mutex_init();
  if (!g_process_data.mutex) {
    st_print_log("[St_Proc] st_mutex_init failed!\n");
    return -1;
  }

  g_process_data.app_mutex = st_mutex_init();
  if (!g_process_data.app_mutex) {
    st_print_log("[St_Proc] st_mutex_init failed!\n");
    st_mutex_destroy(g_process_data.mutex);
    return -1;
  }

  g_process_data.cv = st_cond_init();
  if (!g_process_data.cv) {
    st_print_log("[St_Proc] st_cond_init failed!\n");
    st_mutex_destroy(g_process_data.mutex);
    st_mutex_destroy(g_process_data.app_mutex);
    return -1;
  }

  g_process_data.quit = 0;
  return 0;
}

int
st_process_start(void)
{
  g_process_data.thread =
    st_thread_create(st_process_func, "MAIN", &g_process_data);
  if (!g_process_data.thread) {
    st_print_log("[St_Proc] Failed to create main thread\n");
    return -1;
  }
  return 0;
}

int
st_process_stop(void)
{
  g_process_data.quit = 1;
  if (st_thread_destroy(g_process_data.thread) != 0) {
    st_print_log("[St_Proc] st_thread_dstroy failed!\n");
    return -1;
  }
  g_process_data.thread = NULL;
  st_print_log("[St_Proc] st_thread_destroy finish!\n");
  return 0;
}

int
st_process_destroy(void)
{
  if (g_process_data.quit != 1) {
    st_print_log("[St_Proc] please stop process first.\n");
    return -1;
  }

  if (g_process_data.cv) {
    st_cond_destroy(g_process_data.cv);
    g_process_data.cv = NULL;
  }
  if (g_process_data.app_mutex) {
    st_mutex_destroy(g_process_data.app_mutex);
    g_process_data.app_mutex = NULL;
  }
  if (g_process_data.mutex) {
    st_mutex_destroy(g_process_data.mutex);
    g_process_data.mutex = NULL;
  }
  return 0;
}

void
st_process_signal(void)
{
  st_mutex_lock(g_process_data.mutex);
  st_cond_signal(g_process_data.cv);
  st_mutex_unlock(g_process_data.mutex);
}

void
st_process_app_sync_lock(void)
{
  st_mutex_lock(g_process_data.app_mutex);
}

void
st_process_app_sync_unlock(void)
{
  st_mutex_unlock(g_process_data.app_mutex);
}