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

/*
 * Copyright (c) 2005, Swedish Institute of Computer Science
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

#include "port/oc_log.h"
#include "oc_mmem.h"
#include "oc_list.h"
#include <stdint.h>
#include <string.h>

//Fix add these sizes to config.h
#define INTS_POOL_SIZE 16
#define DOUBLES_POOL_SIZE 16
#define BYTES_POOL_SIZE 1000 
static double doubles[DOUBLES_POOL_SIZE];
static int64_t ints[INTS_POOL_SIZE];
static unsigned char bytes[BYTES_POOL_SIZE];
static unsigned int avail_bytes, avail_ints, avail_doubles;
OC_LIST(bytes_list);
OC_LIST(ints_list);
OC_LIST(doubles_list);

/*---------------------------------------------------------------------------*/
int
oc_mmem_alloc (struct oc_mmem *m, unsigned int size, pool pool_type)
{
  switch (pool_type)
    {
    case BYTE_POOL:
      if (avail_bytes < size)
	{
	  return 0;
	}
      oc_list_add (bytes_list, m);
      m->ptr = &bytes[BYTES_POOL_SIZE - avail_bytes];
      m->size = size;
      avail_bytes -= size;
      break;
    case INT_POOL:
      if (avail_ints < size)
	{
	  return 0;
	}
      oc_list_add (ints_list, m);
      m->ptr = &ints[INTS_POOL_SIZE - avail_ints];
      m->size = size;
      avail_ints -= size;
      break;
    case DOUBLE_POOL:
      if (avail_doubles < size)
	{
	  return 0;
	}
      oc_list_add (doubles_list, m);
      m->ptr = &doubles[DOUBLES_POOL_SIZE - avail_doubles];
      m->size = size;
      avail_doubles -= size;
      break;
    default:
      break;
    }
  return 1;
}

void
oc_mmem_free (struct oc_mmem *m, pool pool_type)
{
  struct oc_mmem *n;

  if (m->next != NULL)
    {
      switch (pool_type)
	{
	case BYTE_POOL:
	  memmove (
	      m->ptr,
	      m->next->ptr,
	      &bytes[BYTES_POOL_SIZE - avail_bytes]
		  - (unsigned char *) m->next->ptr);
	  break;
	case INT_POOL:
	  memmove (
	      m->ptr, m->next->ptr,
	      &ints[INTS_POOL_SIZE - avail_ints] - (int64_t *) m->next->ptr);
	  break;
	case DOUBLE_POOL:
	  memmove (
	      m->ptr,
	      m->next->ptr,
	      &doubles[DOUBLES_POOL_SIZE - avail_doubles]
		  - (double *) m->next->ptr);
	  break;
	default:
	  return;
	  break;
	}
      for (n = m->next; n != NULL; n = n->next)
	{
	  n->ptr = (void *) ((char *) n->ptr - m->size);
	}
    }

  switch (pool_type)
    {
    case BYTE_POOL:
      avail_bytes += m->size;
      oc_list_remove (bytes_list, m);
      break;
    case INT_POOL:
      avail_ints += m->size;
      oc_list_remove (ints_list, m);
      break;
    case DOUBLE_POOL:
      avail_doubles += m->size;
      oc_list_remove (doubles_list, m);
      break;
    }

  LOG("%d %d %d\n", avail_bytes, avail_ints, avail_doubles);
}

void
oc_mmem_init (void)
{
  static int inited = 0;
  if (inited)
    {
      return;
    }
  oc_list_init (bytes_list);
  oc_list_init (ints_list);
  oc_list_init (doubles_list);
  avail_bytes = BYTES_POOL_SIZE;
  avail_ints = INTS_POOL_SIZE;
  avail_doubles = DOUBLES_POOL_SIZE;
  inited = 1;
}
/*---------------------------------------------------------------------------*/
