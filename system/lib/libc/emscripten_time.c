/*
 * Copyright 2022 The Emscripten Authors.  All rights reserved.
 * Emscripten is available under two separate licenses, the MIT license and the
 * University of Illinois/NCSA Open Source License.  Both these licenses can be
 * found in the LICENSE file.
 */

#include <time.h>
#include <sys/timeb.h>
#include <sys/time.h>
#include <stdbool.h>
#include <emscripten/html5.h>
#include "libc.h"

// Replaces musl's __tz.c

__attribute__((__weak__)) long  timezone = 0;
__attribute__((__weak__)) int   daylight = 0;
__attribute__((__weak__)) char *tzname[2] = { 0, 0 };

void _tzset_js(long* timezone, int* daylight, char** tzname);
time_t _timegm_js(struct tm *tm);
time_t _mktime_js(struct tm *tm);
void _localtime_js(const time_t *restrict t, struct tm *restrict tm);
void _gmtime_js(const time_t *restrict t, struct tm *restrict tm);

__attribute__((__weak__))
void tzset() {
  _tzset_js(&timezone, &daylight, tzname);
}

__attribute__((__weak__))
time_t timegm(struct tm *tm) {
  tzset();
  return _timegm_js(tm);
}

__attribute__((__weak__))
time_t mktime(struct tm *tm) {
  tzset();
  return _mktime_js(tm);
}

__attribute__((__weak__))
struct tm *__localtime_r(const time_t *restrict t, struct tm *restrict tm) {
  tzset();
  _localtime_js(t, tm);
  // __localtime_js sets everything but the tmzone pointer
  tm->__tm_zone = tm->tm_isdst ? tzname[1] :tzname[0];
  return tm;
}

__attribute__((__weak__))
struct tm *__gmtime_r(const time_t *restrict t, struct tm *restrict tm) {
  tzset();
  _gmtime_js(t, tm);
  tm->tm_isdst = 0;
  tm->__tm_gmtoff = 0;
  tm->__tm_zone = "GMT";
  return tm;
}

weak_alias(__gmtime_r, gmtime_r);
weak_alias(__localtime_r, localtime_r);

time_t time(time_t *t) {
  time_t time = emscripten_date_now() / 1000;
  if (t) *t = time;
  return time;
}

// get the monotomic clock time/res, or return -1 if not available
int clock_gettime_monotomic_js(double *ret);
int clock_getres_monotomic_js(int *ret);

int __clock_gettime(clockid_t clk_id, struct timespec *ts) {
  double time_double;

  if (clk_id == CLOCK_REALTIME) {
    time_double = emscripten_date_now();
  } else if (!((clk_id == CLOCK_MONOTONIC || clk_id == CLOCK_MONOTONIC_RAW) && clock_gettime_monotomic_js(&time_double) >= 0)) {
    errno = EINVAL;
    return -1;
  }

  // we use a 64bit integer here because the time in millis is greater than 32bit MAX_INT
  long long time = time_double;
  ts->tv_sec = time / 1000;
  ts->tv_nsec = (time % 1000)*1000*1000;
  return 0;
}

weak_alias(__clock_gettime, clock_gettime);

int clock_getres(clockid_t clk_id, struct timespec *res) {
  int nsec;

  if (clk_id == CLOCK_REALTIME) {
    nsec = 1000 * 1000;
  } else if (!((clk_id == CLOCK_MONOTONIC || clk_id == CLOCK_MONOTONIC_RAW) && clock_getres_monotomic_js(&nsec) >= 0)) {
    errno = EINVAL;
    return -1;
  }

  res->tv_sec = nsec / 1000000000;
  res->tv_nsec = nsec; // resolution is nanoseconds
  return 0;
}


int gettimeofday(struct timeval *restrict tv, void *restrict tz) {
  long long now = emscripten_date_now();
  tv->tv_sec = now / 1000;
  tv->tv_usec = (now % 1000) * 1000;
  return 0;
}

int ftime(struct timeb *tp) {
  long long now = emscripten_date_now();
  tp->time = now / 1000;
  tp->millitm = now % 1000;
  tp->timezone = 0;
  tp->dstflag = 0;
  return 0;
}
