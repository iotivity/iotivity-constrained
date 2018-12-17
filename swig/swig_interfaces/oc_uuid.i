/* File oc_obt.i */
%module OCUuidUtil
%include "typemaps.i"
%include "iotivity.swg"

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
#include "oc_uuid.h"

%}

%rename(OCUuid) oc_uuid_t;
%ignore oc_str_to_uuid;
%rename(stringToUuid) jni_str_to_uuid;
%newobject jni_str_to_uuid;
%inline %{
oc_uuid_t * jni_str_to_uuid(const char *str)
{
  oc_uuid_t *value = (oc_uuid_t *)malloc(sizeof(oc_uuid_t));
  oc_str_to_uuid(str, value);
  return value;
}
%}

%ignore oc_uuid_to_str;
%rename(uuidToString) jni_uuid_to_str;
%newobject jni_uuid_to_str;
%inline %{
char * jni_uuid_to_str(const oc_uuid_t *uuid)
{
  char *retValue = (char *)malloc(sizeof(char) * OC_UUID_LEN);
  oc_uuid_to_str(uuid, retValue, OC_UUID_LEN);
  return retValue;
}
%}


%ignore oc_gen_uuid;
%rename(generateUuid) jni_gen_uuid;
%newobject jni_gen_uuid;
%inline %{
oc_uuid_t * jni_gen_uuid()
{
  oc_uuid_t *value = (oc_uuid_t *)malloc(sizeof(oc_uuid_t));
  oc_gen_uuid(value);
  return value;
}
%}

%include oc_uuid.h