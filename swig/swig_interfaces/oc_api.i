/* File oc_api.i */
%module OCMain
%include <oc_clock.i>
%include "stdint.i"
%include <oc_ri.i>
/*%include <oc_collection.i>*/
%{
#include "oc_api.h"
#include "oc_rep.h"
#include <assert.h>

struct callback_data {
  JNIEnv *env;
  jobject obj;
};

void init_platform_java_callback(void *ptr) {
/* TODO still a work in progress
  struct callback_data *data = ptr;
  const jclass callbackInterfaceClass = (*data->env)->FindClass(data->env, "InitPlatformHandler");
  assert(callbackInterfaceClass);
  const jmethodID mid_handle = (*data->env)->GetMethodID(data->env, callbackInterfaceClass, "handle", "()V");
  assert(mid_handle);
  (*data->env)->CallVoidMethod(data->env, data->obj, mid_handle);
*/
}

/* Callback handlers for oc_main_init */
static JavaVM *jvm;
static jobject init_obj;
static jclass cid_MainInitHandler;

int oc_handler_init_callback(void)
{
  printf("JNI: %s\n", __FUNCTION__);
  JNIEnv *jenv = 0;
  int getEnvResult = 0;
  int attachCurrentThreadResult = 0;
  getEnvResult = jvm->GetEnv((void**)&jenv, JNI_VERSION_1_6);
  if (JNI_EDETACHED == getEnvResult) {
      attachCurrentThreadResult = jvm->AttachCurrentThread((void**)&jenv, NULL);
      assert(JNI_OK == attachCurrentThreadResult);
  }
  assert(jenv);
  assert(cid_MainInitHandler);
  const jmethodID mid_initilize = jenv->GetMethodID(cid_MainInitHandler, "initilize", "()I");
  assert(mid_initilize);
  jint ret_value = jenv->CallIntMethod(init_obj, mid_initilize);
  if (JNI_EDETACHED == getEnvResult) {
      jvm->DetachCurrentThread();
  }
  return (int)ret_value;
}

void oc_handler_signal_event_loop_callback(void) 
{
  printf("JNI: %s\n", __FUNCTION__);
  JNIEnv *jenv = 0;
  int getEnvResult = 0;
  int attachCurrentThreadResult = 0;
  getEnvResult = jvm->GetEnv((void**)&jenv, JNI_VERSION_1_6);
  if (JNI_EDETACHED == getEnvResult) {
      attachCurrentThreadResult = jvm->AttachCurrentThread((void**)&jenv, NULL);
      assert(JNI_OK == attachCurrentThreadResult);
  }
  assert(jenv);
  assert(cid_MainInitHandler);
  const jmethodID mid_signalEventLoop = jenv->GetMethodID(cid_MainInitHandler, "signalEventLoop", "()V");
  assert(mid_signalEventLoop);
  jenv->CallIntMethod(init_obj, mid_signalEventLoop);
  if (JNI_EDETACHED == getEnvResult) {
      jvm->DetachCurrentThread();
  }
}

void oc_handler_register_resource_callback(void)
{
  printf("JNI: %s\n", __FUNCTION__);
  JNIEnv *jenv = 0;
  int getEnvResult = 0;
  int attachCurrentThreadResult = 0;
  getEnvResult = jvm->GetEnv((void**)&jenv, JNI_VERSION_1_6);
  if (JNI_EDETACHED == getEnvResult) {
      attachCurrentThreadResult = jvm->AttachCurrentThread((void**)&jenv, NULL);
      assert(JNI_OK == attachCurrentThreadResult);
  }
  assert(jenv);
  assert(cid_MainInitHandler);
  const jmethodID mid_registerResources = jenv->GetMethodID(cid_MainInitHandler, "registerResources", "()V");
  assert(mid_registerResources);
  jenv->CallVoidMethod(init_obj, mid_registerResources);
  if (JNI_EDETACHED == getEnvResult) {
      jvm->DetachCurrentThread();
  }
}

void oc_handler_requests_entry_callback(void)
{
  printf("JNI: %s\n", __FUNCTION__);
  JNIEnv *jenv = 0;
  int getEnvResult = 0;
  int attachCurrentThreadResult = 0;
  getEnvResult = jvm->GetEnv((void**)&jenv, JNI_VERSION_1_6);
  if (JNI_EDETACHED == getEnvResult) {
      attachCurrentThreadResult = jvm->AttachCurrentThread((void**)&jenv, NULL);
      assert(JNI_OK == attachCurrentThreadResult);
  }
  assert(jenv);
  assert(cid_MainInitHandler);
  const jmethodID mid_requestEntry_method = jenv->GetMethodID(cid_MainInitHandler, "requestEntry", "()V");
  assert(mid_requestEntry_method);
  jenv->CallVoidMethod(init_obj, mid_requestEntry_method);
  if (JNI_EDETACHED == getEnvResult) {
      jvm->DetachCurrentThread();
  }
}

static oc_handler_t java_handler = {
    oc_handler_init_callback,              // init 
    oc_handler_signal_event_loop_callback, // signal_event_loop
    oc_handler_register_resource_callback, // register_resources
    oc_handler_requests_entry_callback     // requests_entry
    };
    
/* Mapping oc_resource functions into a Resource class */
class Resource {
  public:
    Resource(const char *name, const char *url, uint8_t num_resource_types, int device) {
      res = oc_new_resource(name, url, num_resource_types, device);
    }
    
    void bindResourceInterface(uint8_t interface) {
      oc_resource_bind_resource_interface(res, interface);
    }

    void setDefaultInterface(oc_interface_mask_t interface) {
      oc_resource_set_default_interface(res, interface);
    }

    void bindResourceType(const char *type) {
      oc_resource_bind_resource_type(res, type);
    }

    void makePublic() {
#ifdef OC_SECURITY
      oc_resource_make_public(res);
#endif /* OC_SECURITY */
    }

    void setDiscoverable(bool state) {
      oc_resource_set_discoverable(res, state);
    }
    
    void setObservable(bool state) {
      oc_resource_set_observable(res, state);
    }

    void setPeriodicObservable(uint16_t seconds) {
      oc_resource_set_periodic_observable(res, seconds);
    }

    void setRequestHandler(oc_method_t method, oc_request_callback_t callback, void *user_data) {
      oc_resource_set_request_handler(res, method, callback, user_data);
    }

    oc_resource_t* getResourceHandle() {
        return res;
    }
  private:
    oc_resource_t * res;
};

void addResource(Resource r) {
  oc_add_resource(r.getResourceHandle());
}

void java_oc_request_callback(oc_request_t *request, oc_interface_mask_t interfaces, void *user_data) {
  printf("JNI: %s\n", __FUNCTION__);
  struct callback_data *data = (callback_data *)user_data;
  const jclass callbackInterfaceClass = (data->env)->FindClass("org/iotivity/RequestHandler");
  assert(callbackInterfaceClass);
  const jmethodID mid_handler = (data->env)->GetMethodID(callbackInterfaceClass, "handler", "(Lorg/iotivity/OCRequest;ILjava/lang/Object;)V");
  assert(mid_handler);

  const jclass cid_OCRequest = (data->env)->FindClass("org/iotivity/OCRequest");
  assert(cid_OCRequest);
  const jmethodID mid_OCRequest_init = (data->env)->GetMethodID(cid_OCRequest, "<init>", "(JZ)V");
  assert(mid_OCRequest_init);
  (data->env)->CallVoidMethod(data->obj, mid_handler, (data->env)->NewObject(cid_OCRequest, mid_OCRequest_init, (jlong)request, false), (jint)interfaces, NULL/* user_data */);
}

int java_oc_init_platform(const char *mfg_name) {
    return oc_init_platform(mfg_name, NULL, NULL);
}

int java_oc_add_device(const char *uri, const char *rt, const char *name,
                       const char *spec_version, const char *data_model_version) {
    return oc_add_device(uri, rt, name, spec_version, data_model_version, NULL, NULL);
}

void java_oc_resource_make_public() {
#ifdef OC_SECURITY
      oc_resource_make_public(res);
#endif /* OC_SECURITY */
    }


/* from oc_rep.h */
void rep_start_root_object() {
    oc_rep_start_root_object();
}

void rep_end_root_object() {
    oc_rep_end_root_object();
}

int java_get_rep_error() {
    return g_err;
}

void java_rep_set_double(const char* key, double value) {
    g_err |= cbor_encode_text_string(&root_map, key, strlen(key));
    g_err |= cbor_encode_double(&root_map, value); 
}

void java_rep_set_int(const char* key, int value) {
    g_err |= cbor_encode_text_string(&root_map, key, strlen(key));
    g_err |= cbor_encode_int(&root_map, value);
}

void java_rep_set_uint(const char* key, unsigned int value) {
    g_err |= cbor_encode_text_string(&root_map, key, strlen(key));
    g_err |= cbor_encode_uint(&root_map, value);
}

void java_rep_set_boolean(const char* key, bool value) {
    g_err |= cbor_encode_text_string(&root_map, key, strlen(key));
    g_err |= cbor_encode_boolean(&root_map, value);
}

void java_rep_set_text_string(const char* key, const char* value) {
    g_err |= cbor_encode_text_string(&root_map, key, strlen(key));
    g_err |= cbor_encode_text_string(&root_map, value, strlen(value));
}

%}

%typemap(jni)    oc_init_platform_cb_t init_platform_cb "jobject";
%typemap(jtype)  oc_init_platform_cb_t init_platform_cb "InitPlatformHandler";
%typemap(jstype) oc_init_platform_cb_t init_platform_cb "InitPlatformHandler";
%typemap(javain) oc_init_platform_cb_t init_platform_cb "$javainput";

%typemap(in,numinputs=1) (oc_init_platform_cb_t init_platform_cb, void *data) {
  struct callback_data *data = (callback_data *)malloc(sizeof *data);
  data->env = jenv;
  data->obj = JCALL1(NewGlobalRef, jenv, $input);
  JCALL1(DeleteLocalRef, jenv, $input);
  $1 = init_platform_java_callback;
  $2 = data;
}

%typemap(jni)    const oc_handler_t *handler "jobject";
%typemap(jtype)  const oc_handler_t *handler "MainInitHandler";
%typemap(jstype) const oc_handler_t *handler "MainInitHandler";
%typemap(javain) const oc_handler_t *handler "$javainput";
%typemap(in)     const oc_handler_t *handler {
  JCALL1(GetJavaVM, jenv, &jvm);
  init_obj = JCALL1(NewGlobalRef, jenv, $input);
  JCALL1(DeleteLocalRef, jenv, $input);
  $1 = &java_handler;
  
  const jclass callback_interface = jenv->FindClass("org/iotivity/MainInitHandler");
  assert(callback_interface);
  cid_MainInitHandler = static_cast<jclass>(jenv->NewGlobalRef(callback_interface));
}

%typemap(jni)    oc_request_callback_t callback "jobject";
%typemap(jtype)  oc_request_callback_t callback "RequestHandler";
%typemap(jstype) oc_request_callback_t callback "RequestHandler";
%typemap(javain) oc_request_callback_t callback "$javainput";
%typemap(in,numinputs=1) (oc_request_callback_t callback, void *user_data) {
  struct callback_data *user_data = (callback_data *)malloc(sizeof *user_data);
  user_data->env = jenv;
  user_data->obj = JCALL1(NewGlobalRef, jenv, $input);
  JCALL1(DeleteLocalRef, jenv, $input);
  $1 = java_oc_request_callback;
  $2 = user_data;
}

%rename(mainInit) oc_main_init;
%rename(mainPoll) oc_main_poll;
%rename(mainShutdown) oc_main_shutdown;
/* TODO The oc_add_device without the callback or data pointer */
%rename(addDevice) java_oc_add_device;
int java_oc_add_device(const char *uri, const char *rt, const char *name,
                       const char *spec_version, const char *data_model_version);
%ignore oc_add_device;
/* TODO Need to figure out how to handle callback and data ctx pointer
%rename(addDevice) oc_add_device;
int oc_add_device(const char *uri, const char *rt, const char *name,
                  const char *spec_version, const char *data_model_version,
                  oc_add_device_cb_t add_device_cb, void *data);
*/
/* the oc_init_platform without the callback or data pointer */
%rename(initPlatform) java_oc_init_platform;
int java_oc_init_platform(const char *mfg_name);
%ignore oc_init_platform;
/* TODO Need to figure out how to handle callback and data ctx pointer
%rename(initPlatform) oc_init_platform;
int oc_init_platform(const char *mfg_name, oc_init_platform_cb_t init_platform_cb, void *data);
*/
%rename(getConResAnnounced) oc_get_con_res_announced;
%rename(setConResAnnounce) oc_set_con_res_announced;

// server side
%rename(newResource) oc_new_resource;
%rename(resourceBindResourceInterface) oc_resource_bind_resource_interface;
%rename(resourceSetDefaultInterface) oc_resource_set_default_interface;
%rename(resourceBindResourceType) oc_resource_bind_resource_type;
%rename(processBaselineInterface) oc_process_baseline_interface;
%rename(newCollection) oc_new_collection;
%rename(deleteCollection) oc_delete_collection;
%rename(newLink) oc_new_link;
%rename(deleteLink) oc_delete_link;
%rename(linkAddRelation) oc_link_add_rel;
%rename(linkSetInstance) oc_link_set_ins;
%rename(collectionAddLink) oc_collection_add_link;
%rename(collectionRemoveLink) oc_collection_remove_link;
%rename(collectionGetLinks) oc_collection_get_links;
%rename(addCollection) oc_add_collection;
%rename(collectionGetCollection) oc_collection_get_collections;
// custom instance of oc_resource_make_public to handle OC_SECURITY
%rename(resourceMakePublic) java_oc_resource_make_public;
%ignore oc_resource_make_public;
%rename(resourceSetDiscoverable) oc_resource_set_discoverable;
%rename(resourceSetObservable) oc_resource_set_observable;
%rename(resourceSetPeriodicObservable) oc_resource_set_periodic_observable;
%rename(resourceSetRequestHandler) oc_resource_set_request_handler;
%rename(addResource) oc_add_resource;
%rename(deleteResource) oc_delete_resource;
%rename(setConWriteCallback) oc_set_con_write_cb;
%rename(initQueryIterator) oc_init_query_iterator;
%rename(iterateQuery) oc_iterate_query;
%rename(iterateQueryGetValues) oc_iterate_query_get_values;
%rename(getQueryValue) oc_get_query_value;
%rename(sendResponce) oc_send_response;
%rename(ignoreRequest) oc_ignore_request;
%rename(indicateSeparateResponse) oc_indicate_separate_response;
%rename(setSeparateResponseBuffer) oc_set_separate_response_buffer;
%rename(sendSeparateResponse) oc_send_separate_response;
%rename(notifyObservers) oc_notify_observers;

// client side
%rename(doIPDiscovery) oc_do_ip_discovery;
%rename(doIPDiscoveryAtEndpoint) oc_do_ip_discovery_at_endpoint;
%rename(doGet) oc_do_get;
%rename(doDelete) oc_do_delete;
%rename(initPut) oc_init_put;
%rename(doPut) oc_do_put;
%rename(initPost) oc_init_post;
%rename(doPost) oc_do_post;
%rename(doObserve) oc_do_observe;
%rename(stopObserve) oc_stop_observe;
%rename(doIPMulticast) oc_do_ip_multicast;
%rename(stopMulticast) oc_stop_multicast;
%rename(freeServerEndpoints) oc_free_server_endpoints;
%rename(closeSession) oc_close_session;

// common operations
%rename(setDelayedCallback) oc_set_delayed_callback;
%rename(removeDelayedCallback) oc_remove_delayed_callback;
%include "oc_api.h"

%rename (OCRequestPayload) oc_rep_s;
%rename (OCType) oc_rep_value_type_t;
%rename (OCValue) oc_rep_value;
%ignore g_encoder;
%ignore root_map;
%ignore links_array;
%ignore g_err;
%ignore oc_rep_new;
%ignore oc_rep_finalize;
%ignore oc_rep_get_cbor_errno;
%ignore oc_rep_set_pool;
%ignore oc_parse_rep;
%ignore oc_free_rep;
%ignore oc_rep_get_int;
%ignore oc_rep_get_bool;
%ignore oc_rep_get_double;
%ignore oc_rep_get_byte_string;
%ignore oc_rep_get_string;
%ignore oc_rep_get_int_array;
%ignore oc_rep_get_bool_array;
%ignore oc_rep_get_double_array;
%ignore oc_rep_get_byte_string_array;
%ignore oc_rep_get_string_array;
%ignore oc_rep_get_object;
%ignore oc_rep_get_object_array;
%include "oc_rep.h"


/*
%typemap(in) oc_rep_t oc_rep_s;
%typemap(out) oc_rep_s oc_rep_t;

%typemap(jstype) oc_rep_s "OCRequestPayload";
%typemap(jtype) oc_rep_s "OCRequestPayload";
%typemap(jni) oc_rep_s "jobject";
%typemap(javain) oc_rep_s "$javainput";
%typemap(javaout)

typedef struct oc_rep_s
{
  oc_rep_value_type_t type;
  struct oc_rep_s *next;
  oc_string_t name;
  union oc_rep_value
  {
    int integer;
    bool boolean;
    double double_p;
    oc_string_t string;
    oc_array_t array;
    struct oc_rep_s *object;
    struct oc_rep_s *object_array;
  } value;
} oc_rep_t;
*/
%rename(repStartRootObject) rep_start_root_object;
void rep_start_root_object();

%rename(repEndRootObject) rep_end_root_object;
void rep_end_root_object();

%rename (getRepError) java_get_rep_error;
int java_get_rep_error();

%rename (repSetInt) java_rep_set_int;
void java_rep_set_int(const char* key, int value);

%rename (repSetBoolean) java_rep_set_boolean;
void java_rep_set_boolean(const char* key, bool value);

%rename (repSetTextString) java_rep_set_text_string;
void java_rep_set_text_string(const char* key, const char* value);