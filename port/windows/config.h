#ifndef CONFIG_H
#define CONFIG_H

/* Time resolution */
#include <stdint.h>
typedef uint64_t oc_clock_time_t;
#define __attribute__(x) /* used in dtls.h, but visual studio does not support it, are we going to fix tinydtls instead? */
#define strncasecmp _strnicmp
/* Sets one clock tick to 1 ms */
#define OC_CLOCK_CONF_TICKS_PER_SECOND (1000)

/* Security Layer */
/* Max inactivity timeout before tearing down DTLS connection */
#define OC_DTLS_INACTIVITY_TIMEOUT (60)

/* If we selected support for dynamic memory allocation */
#ifdef OC_DYNAMIC_ALLOCATION
#define OC_COLLECTIONS
#define OC_BLOCK_WISE

#else /* OC_DYNAMIC_ALLOCATION */
/* List of constraints below for a build that does not employ dynamic
memory allocation
*/
/* Memory pool sizes */
#define OC_BYTES_POOL_SIZE (1800)
#define OC_INTS_POOL_SIZE (100)
#define OC_DOUBLES_POOL_SIZE (4)

/* Server-side parameters */
/* Maximum number of server resources */
#define OC_MAX_APP_RESOURCES (2)

#define OC_MAX_NUM_COLLECTIONS (1)

/* Common paramters */
/* Prescriptive lower layers MTU size, enable block-wise transfers */
#define OC_BLOCK_WISE_SET_MTU (400)

/* Maximum size of request/response payloads */
#define OC_MAX_APP_DATA_SIZE (1024)

/* Maximum number of concurrent requests */
#define OC_MAX_NUM_CONCURRENT_REQUESTS (4)

/* Maximum number of nodes in a payload tree structure */
#define OC_MAX_NUM_REP_OBJECTS (100)

/* Number of devices on the OCF platform */
#define OC_MAX_NUM_DEVICES (1)

/* Maximum number of endpoints */
#define OC_MAX_NUM_ENDPOINTS (10)

/* Security layer */
/* Maximum number of authorized clients */
#define OC_MAX_NUM_SUBJECTS (2)

/* Maximum number of concurrent DTLS sessions */
#define OC_MAX_DTLS_PEERS (1)

#endif /* !OC_DYNAMIC_ALLOCATION */

#endif /* CONFIG_H */
