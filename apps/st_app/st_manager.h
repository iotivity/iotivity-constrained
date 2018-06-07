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

#ifndef ST_MANAGER_H
#define ST_MANAGER_H

#include <stdbool.h>

typedef bool (*st_user_confirm_cb_t)(void);

int st_manager_initialize(void);
int st_manager_start(void);
void st_manager_reset(void);
void st_manager_stop(void);
void st_manager_deinitialize(void);

void st_register_user_confirm_handler(st_user_confirm_cb_t cb);

#endif /* ST_MANAGER_H */