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

#ifndef CONFIG_H
#define CONFIG_H

/* Time resolution */
#include <stdint.h>
typedef uint32_t oc_clock_time_t;
#include <zephyr.h>
#define OC_CLOCK_CONF_SECOND (sys_clock_ticks_per_sec)

#define OC_BYTES_POOL_SIZE (1024)
#define OC_INTS_POOL_SIZE (16)
#define OC_DOUBLES_POOL_SIZE (16)

/* Server-side parameters */
/* Maximum number of server resources */
#define MAX_APP_RESOURCES (1)

/* Client-side parameters */

/* Common paramters */
/* Maximum number of concurrent requests */
#define MAX_NUM_CONCURRENT_REQUESTS (3)

#define EST_NUM_REP_OBJECTS (70)

/* Maximum size of request/response PDUs */
#define MAX_PAYLOAD_SIZE (512)

/* Number of send/receive buffers */
#define NUM_TX_RX_BUFFERS (MAX_NUM_CONCURRENT_REQUESTS + 1)

/* Number of devices on the OCF platform */
#define MAX_NUM_DEVICES (1)

/* Platform payload size */
#define MAX_PLATFORM_PAYLOAD_SIZE (256)

/* Device payload size */
#define MAX_DEVICE_PAYLOAD_SIZE (256)

/* Security layer */
#define MAX_NUM_SUBJECTS (2)

#define MAX_DTLS_PEERS (1)

#define DTLS_INACTIVITY_TIMEOUT (30)

#endif /* CONFIG_H */
