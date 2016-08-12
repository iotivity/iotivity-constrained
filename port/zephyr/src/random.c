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

#include <drivers/system_timer.h>
#include "port/oc_random.h"

void oc_random_init(unsigned short seed)
{
}

unsigned short oc_random_rand(void)
{
  return (unsigned short)sys_cycle_get_32();	
}

void oc_random_destroy()
{
}


