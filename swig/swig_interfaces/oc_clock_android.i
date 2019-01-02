/* File oc_clock.i */
%module OCClock
%include "stdint.i"
#define OC_DYNAMIC_ALLOCATION
#define CLOCKS_PER_SEC (1000000)

%ignore oc_clock_time_t;
typedef long long oc_clock_time_t;

%include "../../port/android/oc_config.h"

%pragma(java) jniclasscode=%{
  static {
    try {
        System.loadLibrary("iotivity-lite-jni");
    } catch (UnsatisfiedLinkError e) {
      System.err.println("Native code library failed to load. \n" + e);
      System.exit(1);
    }
  }
%}

%{
#include "../../port/oc_clock.h"
%}

%rename(clockInit) oc_clock_init;
%rename(clockTime) oc_clock_time;
%rename(clockSeconds) oc_clock_seconds;
%rename(clockWait) oc_clock_wait;
%include "../../port/oc_clock.h"