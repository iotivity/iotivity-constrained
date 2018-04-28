/****************************************************************************
 *
 * Copyright 2017 Samsung Electronics All Rights Reserved.
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
#include <tinyara/config.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/socket.h>
#include <netdb.h>
#include <net/lwip/netif.h>
#include "uuid/uuid.h"

#if CONFIG_NSOCKET_DESCRIPTORS > 0
extern struct netif *g_netdevices;
#endif

void uuid_generate_random(uuid_t out)
{
	int i = 0;

	srand(time(NULL));			/* TODO: Need to check time(NULL) */
	for (i = 0; i < sizeof(uuid_t); i++) {	/* 16 means UUID size */
		out[i] = rand() % 0xff;
	}

	out[6] = (out[6] & 0x0F) | 0x40;
	out[8] = (out[8] & 0x3F) | 0x80;
}

void uuid_generate(uuid_t out)
{
	uuid_generate_random(out);
}

void uuid_unparse_lower(const uuid_t uu, char *out)
{
	snprintf(out, 37 /* UUID_STRING_SIZE */ ,
			 "%02x%02x%02x%02x-" "%02x%02x-" "%02x%02x-" "%02x%02x-" "%02x%02x%02x%02x%02x%02x", uu[0], uu[1], uu[2], uu[3], uu[4], uu[5], uu[6], uu[7], uu[8], uu[9], uu[10], uu[11], uu[12], uu[13], uu[14], uu[15]);
}

int getifaddrs(struct ifaddrs **ifap)
{
	static struct ifaddrs ifa;
	static struct sockaddr_in addr, netmask;
	uint8_t flags;

	memset(&ifa, 0, sizeof(ifa));
	memset(&addr, 0, sizeof(addr));
	memset(&netmask, 0, sizeof(netmask));

	struct netif *curr = g_netdevices;

	netlib_get_ipv4addr(curr->d_ifname, &addr.sin_addr);
	netlib_get_dripv4addr(curr->d_ifname, &netmask.sin_addr);
	netlib_getifstatus(curr->d_ifname, &flags);

	ifa.ifa_next = NULL;
	ifa.ifa_name = curr->d_ifname;
	ifa.ifa_flags = flags | IFF_RUNNING;
	addr.sin_family = netmask.sin_family = AF_INET;
	ifa.ifa_addr = (struct sockaddr *)&addr;
	ifa.ifa_netmask = (struct sockaddr *)&netmask;

	*ifap = &ifa;

	return 0;
}

unsigned int if_nametoindex(const char *ifname)
{
	return 0;					// TODO: Now supports only 1 device
}

const char *gai_strerror(int errcode)
{
	static const char *n_str = "null";
	return n_str;
}

/*
 * Finds the first occurrence of the byte string s in byte string l.
 */

void * memmem(const void *l, size_t lLen, const void *s, size_t sLen)
{
    char *cur;
    char *last;
    const char *cl = (const char *)l;
    const char *cs = (const char *)s;

    if (lLen == 0 || sLen == 0)
    {
        return NULL;
    }
    if (lLen < sLen)
    {
        return NULL;
    }
    if (sLen == 1)
    {
        return (void *)memchr(l, (int)*cs, lLen);
    }

    last = (char *)cl + lLen - sLen;

    for (cur = (char *)cl; cur <= last; cur++)
    {
        if (cur[0] == cs[0] && memcmp(cur, cs, sLen) == 0)
        {
            return cur;
        }
    }
    return NULL;
}