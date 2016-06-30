/* debug.c -- debug utilities
 *
 * Copyright (C) 2011--2012 Olaf Bergmann <bergmann@tzi.org>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "tinydtls.h"
#include "dtls_config.h"

#if defined(HAVE_ASSERT_H) && !defined(assert)
#include <assert.h>
#endif

#include <stdarg.h>
#include <stdio.h>

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifdef HAVE_TIME_H
#include <time.h>
#endif

#ifdef __ANDROID__
#include <android/log.h>
#endif

#include "global.h"
#include "debug.h"

static int maxlog = DTLS_LOG_WARN;	/* default maximum log level */

const char *dtls_package_name() {
  return PACKAGE_NAME;
}

const char *dtls_package_version() {
  return PACKAGE_VERSION;
}

log_t 
dtls_get_log_level() {
  return maxlog;
}

void
dtls_set_log_level(log_t level) {
  maxlog = level;
}

/* this array has the same order as the type log_t */
#ifdef __ANDROID__
static android_LogPriority loglevels_android[] = {
  ANDROID_LOG_FATAL,
  ANDROID_LOG_ERROR,
  ANDROID_LOG_ERROR,
  ANDROID_LOG_WARN,
  ANDROID_LOG_INFO,
  ANDROID_LOG_INFO,
  ANDROID_LOG_DEBUG
};
#else
static char *loglevels[] = {
  "EMRG", "ALRT", "CRIT", "WARN", "NOTE", "INFO", "DEBG" 
};
#endif

#ifdef HAVE_TIME_H

static inline size_t
print_timestamp(char *s, size_t len, time_t t) {
  struct tm *tmp;
  tmp = localtime(&t);
  return strftime(s, len, "%b %d %H:%M:%S", tmp);
}

#else /* alternative implementation: just print the timestamp */
typedef oc_clock_time_t clock_time_t;
#define CLOCK_SECOND OC_CLOCK_SECOND
static inline size_t
print_timestamp(char *s, size_t len, clock_time_t t) {
#ifdef HAVE_SNPRINTF
  return snprintf(s, len, "%u.%03u", 
		  (unsigned int)(t / CLOCK_SECOND), 
		  (unsigned int)(t % CLOCK_SECOND));
#else /* HAVE_SNPRINTF */
  /* @todo do manual conversion of timestamp */
  return 0;
#endif /* HAVE_SNPRINTF */
}

#endif /* HAVE_TIME_H */

/** 
 * A length-safe strlen() fake. 
 * 
 * @param s      The string to count characters != 0.
 * @param maxlen The maximum length of @p s.
 * 
 * @return The length of @p s.
 */
static inline size_t
dtls_strnlen(const char *s, size_t maxlen) {
  size_t n = 0;
  while(*s++ && n < maxlen)
    ++n;
  return n;
}

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

static size_t
dsrv_print_addr(const session_t *addr, char *buf, size_t len) {
#ifdef HAVE_ARPA_INET_H
  const void *addrptr = NULL;
#ifdef __ANDROID__
  unsigned short int port;
#else
  in_port_t port;
#endif
  char *p = buf;

  switch (addr->addr.sa.sa_family) {
  case AF_INET: 
    if (len < INET_ADDRSTRLEN)
      return 0;
  
    addrptr = &addr->addr.sin.sin_addr;
    port = ntohs(addr->addr.sin.sin_port);
    break;
  case AF_INET6:
    if (len < INET6_ADDRSTRLEN + 2)
      return 0;

    *p++ = '[';

    addrptr = &addr->addr.sin6.sin6_addr;
    port = ntohs(addr->addr.sin6.sin6_port);

    break;
  default:
    memcpy(buf, "(unknown address type)", min(22, len));
    return min(22, len);
  }

  if (inet_ntop(addr->addr.sa.sa_family, addrptr, p, len) == 0) {
    perror("dsrv_print_addr");
    return 0;
  }

  p += dtls_strnlen(p, len);

  if (addr->addr.sa.sa_family == AF_INET6) {
    if (p < buf + len) {
      *p++ = ']';
    } else 
      return 0;
  }

  p += snprintf(p, buf + len - p + 1, ":%d", port);

  return p - buf;
#else /* HAVE_ARPA_INET_H */
# if WITH_CONTIKI
  char *p = buf;
#  ifdef UIP_CONF_IPV6
  uint8_t i;
  const char hex[] = "0123456789ABCDEF";

  if (len < 41)
    return 0;

  *p++ = '[';

  for (i=0; i < 16; i += 2) {
    if (i) {
      *p++ = ':';
    }
    *p++ = hex[(addr->addr.u8[i] & 0xf0) >> 4];
    *p++ = hex[(addr->addr.u8[i] & 0x0f)];
    *p++ = hex[(addr->addr.u8[i+1] & 0xf0) >> 4];
    *p++ = hex[(addr->addr.u8[i+1] & 0x0f)];
  }
  *p++ = ']';
#  else /* UIP_CONF_IPV6 */
#   warning "IPv4 network addresses will not be included in debug output"

  if (len < 21)
    return 0;
#  endif /* UIP_CONF_IPV6 */
  if (buf + len - p < 6)
    return 0;

  p += sprintf(p, ":%d", uip_htons(addr->port));

  return p - buf;
# else /* WITH_CONTIKI */
  /* TODO: output addresses manually */
#   warning "inet_ntop() not available, network addresses will not be included in debug output"
# endif /* WITH_CONTIKI */
  return 0;
#endif
}

#ifdef __ANDROID__
void
dsrv_log(log_t level, char *format, ...) {
  va_list ap;

  if (maxlog < level)
    return;

  va_start(ap, format);
  __android_log_vprint(loglevels_android[level], PACKAGE_NAME, format, ap);
  va_end(ap);
}
#elif !defined (WITH_CONTIKI) && !defined (WITH_OCF)
void 
dsrv_log(log_t level, char *format, ...) {
  static char timebuf[32];
  va_list ap;
  FILE *log_fd;

  if (maxlog < level)
    return;

  log_fd = level <= DTLS_LOG_CRIT ? stderr : stdout;

  if (print_timestamp(timebuf,sizeof(timebuf), time(NULL)))
    fprintf(log_fd, "%s ", timebuf);

  if (level <= DTLS_LOG_DEBUG) 
    fprintf(log_fd, "%s ", loglevels[level]);

  va_start(ap, format);
  vfprintf(log_fd, format, ap);
  va_end(ap);
  fflush(log_fd);
}
#elif defined (HAVE_VPRINTF) /* WITH_CONTIKI */
void 
dsrv_log(log_t level, char *format, ...) {
  static char timebuf[32];
  va_list ap;

  if (maxlog < level)
    return;

  if (print_timestamp(timebuf,sizeof(timebuf), clock_time()))
    PRINTF("%s ", timebuf);

  if (level <= DTLS_LOG_DEBUG) 
    PRINTF("%s ", loglevels[level]);

  va_start(ap, format);
  vprintf(format, ap);
  va_end(ap);
}
#endif /* WITH_CONTIKI */

#ifndef NDEBUG
/** dumps packets in usual hexdump format */
void hexdump(const unsigned char *packet, int length) {
  int n = 0;

  while (length--) { 
    if (n % 16 == 0)
      printf("%08X ",n);

    printf("%02X ", *packet++);
    
    n++;
    if (n % 8 == 0) {
      if (n % 16 == 0)
	printf("\n");
      else
	printf(" ");
    }
  }
}

/** dump as narrow string of hex digits */
void dump(unsigned char *buf, size_t len) {
  while (len--) 
    printf("%02x", *buf++);
}

void dtls_dsrv_log_addr(log_t level, const char *name, const session_t *addr)
{
  char addrbuf[73];
  int len;

  len = dsrv_print_addr(addr, addrbuf, sizeof(addrbuf));
  if (!len)
    return;
  dsrv_log(level, "%s: %s\n", name, addrbuf);
}

#ifdef __ANDROID__
void
dtls_dsrv_hexdump_log(log_t level, const char *name, const unsigned char *buf, size_t length, int extend) {
  char *hex_dump_text;
  char *p;
  int ret;
  int size;

  if (maxlog < level)
    return;

  size = length * 3 + strlen(name) + 22;
  hex_dump_text = malloc(size);
  if (!hex_dump_text)
    return;

  p = hex_dump_text;

  ret = snprintf(p, size, "%s: (%zu bytes): ", name, length);
  if (ret >= size)
    goto print;
  p += ret;
  size -= ret;
  while (length--) {
    ret = snprintf(p, size, "%02X ", *buf++);
    if (ret >= size)
      goto print;
    p += ret;
    size -= ret;
  }
print:
  __android_log_print(loglevels_android[level], PACKAGE_NAME, "%s\n", hex_dump_text);
  free(hex_dump_text);
}
#elif !defined (WITH_CONTIKI)
void 
dtls_dsrv_hexdump_log(log_t level, const char *name, const unsigned char *buf, size_t length, int extend) {
  static char timebuf[32];
  FILE *log_fd;
  int n = 0;

  if (maxlog < level)
    return;

  log_fd = level <= DTLS_LOG_CRIT ? stderr : stdout;

  if (print_timestamp(timebuf, sizeof(timebuf), time(NULL)))
    fprintf(log_fd, "%s ", timebuf);

  if (level <= DTLS_LOG_DEBUG) 
    fprintf(log_fd, "%s ", loglevels[level]);

  if (extend) {
    fprintf(log_fd, "%s: (%zu bytes):\n", name, length);

    while (length--) {
      if (n % 16 == 0)
	fprintf(log_fd, "%08X ", n);

      fprintf(log_fd, "%02X ", *buf++);

      n++;
      if (n % 8 == 0) {
	if (n % 16 == 0)
	  fprintf(log_fd, "\n");
	else
	  fprintf(log_fd, " ");
      }
    }
  } else {
    fprintf(log_fd, "%s: (%zu bytes): ", name, length);
    while (length--) 
      fprintf(log_fd, "%02X", *buf++);
  }
  fprintf(log_fd, "\n");

  fflush(log_fd);
}
#else /* WITH_CONTIKI */
void 
dtls_dsrv_hexdump_log(log_t level, const char *name, const unsigned char *buf, size_t length, int extend) {
  static char timebuf[32];
  int n = 0;

  if (maxlog < level)
    return;

  if (print_timestamp(timebuf,sizeof(timebuf), clock_time()))
    PRINTF("%s ", timebuf);

  if (level >= 0 && level <= DTLS_LOG_DEBUG) 
    PRINTF("%s ", loglevels[level]);

  if (extend) {
    PRINTF("%s: (%zu bytes):\n", name, length);

    while (length--) {
      if (n % 16 == 0)
	PRINTF("%08X ", n);

      PRINTF("%02X ", *buf++);

      n++;
      if (n % 8 == 0) {
	if (n % 16 == 0)
	  PRINTF("\n");
	else
	  PRINTF(" ");
      }
    }
  } else {
    PRINTF("%s: (%zu bytes): ", name, length);
    while (length--) 
      PRINTF("%02X", *buf++);
  }
  PRINTF("\n");
}
#endif /* WITH_CONTIKI */

#endif /* NDEBUG */
