/******************************************************************
 *
 * Copyright 2018 Samsung Electronics All Rights Reserved.
 *
 *
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************/

#include <cstdlib>
#include <string>
#include <gtest/gtest.h>

extern "C" {
#include "oc_api.h"
#include "port/oc_clock.h"
}

#define MAX_WAIT_TIME 10
#define RESOURCE_URI "/LightResourceURI"
#define DEVICE_URI "/oic/d"
#define RESOURCE_TYPE "oic.r.light"
#define DEVICE_TYPE "oic.d.light"
#define RESOURCE_INTERFACE "oic.if.baseline"
#define MANUFACTURER_NAME "Samsung"
#define DEVICE_NAME "Table Lamp"
#define OCF_SPEC_VERSION "ocf.1.0.0"
#define OCF_DATA_MODEL_VERSION "ocf.res.1.0.0"

static int appInit(void)
{
    return -1;
}

static void registerResources(void)
{
}

static void signalEventLoop(void)
{
}

static void requestsEntry(void)
{
}

class TestServerClient: public testing::Test
{
    protected:
        virtual void SetUp()
        {

        }

        virtual void TearDown()
        {

        }
};

class TestUnicastRequest: public testing::Test
{
    public:
        static oc_handler_t s_handler;
        static bool s_isServerStarted;
        static bool s_isResourceDiscovered;
        static bool s_isCallbackReceived;
        static oc_resource_t *s_pResource;
        static pthread_mutex_t s_mutex;
        static pthread_mutex_t s_waitingMutex;
        static pthread_cond_t s_cv;
        static oc_endpoint_t *s_pLightEndpoint;

        static oc_discovery_flags_t onResourceDiscovered(const char *di,
                const char *uri, oc_string_array_t types,
                oc_interface_mask_t interfaces, oc_endpoint_t *endpoint,
                oc_resource_properties_t bm, void *user_data)
        {
            PRINT("onResourceDiscovered....\n");
            (void) di;
            (void) types;
            (void) interfaces;
            (void) bm;
            (void) user_data;
            std::string discoveredResourceUri = std::string(uri);
            if (discoveredResourceUri.compare(RESOURCE_URI) == 0)
            {
                PRINT("Light Resource Discovered....\n");
                s_pLightEndpoint = endpoint;
                s_isResourceDiscovered = true;
                s_isCallbackReceived = true;
                // pthread_mutex_lock(&s_mutex);
                // pthread_cond_signal(&s_cv);
                // pthread_mutex_unlock(&s_mutex);
                return OC_STOP_DISCOVERY;
            }

            oc_free_server_endpoints(endpoint);
            return OC_CONTINUE_DISCOVERY;
        }

        static void onGetResponse(oc_client_response_t *data)
        {
            PRINT("onGetResponse....\n");
            (void) data;
            s_isCallbackReceived = true;
        }

        static void onPutResponse(oc_client_response_t *data)
        {
            PRINT("onPutResponse....\n");
            (void) data;
            s_isCallbackReceived = true;
        }

        static void onPostResponse(oc_client_response_t *data)
        {
            PRINT("onPostResponse....\n");
            (void) data;
            s_isCallbackReceived = true;
        }

        static void onDeleteResponse(oc_client_response_t *data)
        {
            PRINT("onDeleteResponse....\n");
            (void) data;
            s_isCallbackReceived = true;
        }

        static void onGetRequest(oc_request_t *request,
                                 oc_interface_mask_t interface, void *user_data)
        {
            PRINT("onGetRequest....\n");
            (void) request;
            (void) interface;
            (void) user_data;
        }

        static void onPutRequest(oc_request_t *request,
                                 oc_interface_mask_t interface, void *user_data)
        {
            PRINT("onPutRequest....\n");
            (void) request;
            (void) interface;
            (void) user_data;
        }

        static void onPostRequest(oc_request_t *request,
                                  oc_interface_mask_t interface, void *user_data)
        {
            PRINT("onPostRequest....\n");
            (void) request;
            (void) interface;
            (void) user_data;
        }

        static void onDeleteRequest(oc_request_t *request,
                                    oc_interface_mask_t interface, void *user_data)
        {
            PRINT("onDeleteRequest....\n");
            (void) request;
            (void) interface;
            (void) user_data;
        }

        static int appInit(void)
        {
            PRINT("appInit....\n");
            int result = oc_init_platform(MANUFACTURER_NAME, NULL, NULL);
            result |= oc_add_device(DEVICE_URI, DEVICE_TYPE, DEVICE_NAME,
                                    OCF_SPEC_VERSION, OCF_DATA_MODEL_VERSION, NULL, NULL);
            return result;
        }

        static void registerResources(void)
        {
            PRINT("registerResources....\n");
            s_pResource = oc_new_resource(NULL, RESOURCE_URI, 1, 0);
            oc_resource_bind_resource_type(s_pResource, RESOURCE_TYPE);
            oc_resource_bind_resource_interface(s_pResource, OC_IF_RW);
            oc_resource_set_default_interface(s_pResource, OC_IF_RW);
            oc_resource_set_discoverable(s_pResource, true);
            oc_resource_set_periodic_observable(s_pResource, 1);
            oc_resource_set_request_handler(s_pResource, OC_GET, onGetRequest,
                                            NULL);
            oc_resource_set_request_handler(s_pResource, OC_PUT, onPutRequest,
                                            NULL);
            oc_resource_set_request_handler(s_pResource, OC_POST, onPostRequest,
                                            NULL);
            oc_resource_set_request_handler(s_pResource, OC_DELETE, onDeleteRequest,
                                            NULL);
            oc_process_baseline_interface(s_pResource);
            oc_add_resource(s_pResource);
        }

        static void signalEventLoop(void)
        {
            pthread_mutex_lock(&s_mutex);
            pthread_cond_signal(&s_cv);
            pthread_mutex_unlock(&s_mutex);
        }

        static void requestsEntry(void)
        {
            PRINT("requestsEntry....\n");
        }

        static void waitForEvent(int waitTime)
        {
            struct timespec ts;
            oc_clock_time_t nextEvent, prevEvent = 0;

            pthread_mutex_lock(&s_waitingMutex);
            PRINT("Waiting for callback....\n");
            while (!s_isCallbackReceived && waitTime) {
                nextEvent = oc_main_poll();
                pthread_mutex_lock(&s_mutex);
                if (nextEvent == 0) {
                    pthread_cond_wait(&s_cv, &s_mutex);
                } else {
                    nextEvent /= OC_CLOCK_SECOND;
                    ts.tv_sec = nextEvent;
                    pthread_cond_timedwait(&s_cv, &s_mutex, &ts);
                    if (prevEvent > 0)
                    {
                        waitTime -= (nextEvent - prevEvent);
                    }                    
                    prevEvent = nextEvent;
                }
                pthread_mutex_unlock(&s_mutex);
            }

            pthread_mutex_unlock(&s_waitingMutex);
        }

    protected:
        virtual void SetUp()
        {
            s_isCallbackReceived = false;
        }

        virtual void TearDown()
        {
        }

        static void SetUpTestCase()
        {
            s_handler.init = &appInit;
            s_handler.signal_event_loop = &signalEventLoop;
            s_handler.register_resources = &registerResources;
            s_handler.requests_entry = &requestsEntry;

            oc_set_con_res_announced(false);

            int initResult = oc_main_init(&s_handler);
            if (initResult < 0)
            {

                FAIL() << "Initialization of main server failed";
                s_isServerStarted = false;
            }
            else
            {
                s_isServerStarted = true;
            }

            s_isResourceDiscovered = false;
            if (pthread_mutex_init(&s_mutex, NULL) < 0) {
                printf("pthread_mutex_init failed!\n");
                return ;
            }

            if (pthread_mutex_init(&s_waitingMutex, NULL) < 0) {
                printf("pthread_mutex_init failed!\n");
                pthread_mutex_destroy(&s_mutex);
                return ;
            }
        }

        static void TearDownTestCase()
        {
            if (s_pResource)
            {
                oc_delete_resource(s_pResource);
            }

            if (s_isServerStarted)
            {
                oc_main_shutdown();
            }

            pthread_mutex_destroy(&s_mutex);
            pthread_mutex_destroy(&s_waitingMutex);
        }
};

bool TestUnicastRequest::s_isServerStarted = false;
bool TestUnicastRequest::s_isResourceDiscovered = false;
bool TestUnicastRequest::s_isCallbackReceived = false;
oc_resource_t *TestUnicastRequest::s_pResource = nullptr;
oc_endpoint_t *TestUnicastRequest::s_pLightEndpoint = nullptr;
oc_handler_t TestUnicastRequest::s_handler;
pthread_mutex_t TestUnicastRequest::s_mutex;
pthread_mutex_t TestUnicastRequest::s_waitingMutex;
pthread_cond_t TestUnicastRequest::s_cv;

TEST(TestServerClient, ServerStartTest_P)
{
    static const oc_handler_t handler = { .init = appInit, .signal_event_loop =
            signalEventLoop, .register_resources = registerResources,
                                          .requests_entry = requestsEntry
                                        };

    int result = oc_main_init(&handler);
    EXPECT_LT(result, 0);

    oc_main_shutdown();
}

TEST(TestServerClient, ServerStopTest_P)
{

    static const oc_handler_t handler = { .init = appInit, .signal_event_loop =
            signalEventLoop, .register_resources = registerResources,
                                          .requests_entry = requestsEntry
                                        };

    int result = oc_main_init(&handler);
    ASSERT_LT(result, 0);

    EXPECT_NO_THROW(oc_main_shutdown());
}

TEST_F(TestUnicastRequest, DiscoverResourceTest_P)
{
    bool isSuccess = false;
    isSuccess = oc_do_ip_discovery(NULL, onResourceDiscovered, NULL);
    EXPECT_TRUE(isSuccess) << "Resource discovery failed!!";
    waitForEvent(MAX_WAIT_TIME);
    EXPECT_TRUE(s_isResourceDiscovered) << "Failed to discover light resource";
}

TEST_F(TestUnicastRequest, IPDiscoverResourceTest_P)
{
    bool isSuccess = false;
    isSuccess = oc_do_ip_discovery_at_endpoint(RESOURCE_URI, NULL,
                s_pLightEndpoint, NULL);
    EXPECT_TRUE(isSuccess) << "Failed to discover resource";
}

TEST_F(TestUnicastRequest, SendGetRequest_P)
{
    bool isSuccess = false;

    isSuccess = oc_do_get(RESOURCE_URI, s_pLightEndpoint, NULL, onGetResponse,
                          HIGH_QOS, NULL);
    ASSERT_TRUE(isSuccess) << "oc_do_get() returned failure";
}

TEST_F(TestUnicastRequest, SendGetRequestTwice_P)
{
    bool isSuccess = false;

    isSuccess = oc_do_get(RESOURCE_URI, s_pLightEndpoint, NULL, onGetResponse,
                          LOW_QOS, NULL);
    ASSERT_TRUE(isSuccess) << "oc_do_get() returned failure";

    isSuccess = oc_do_get(RESOURCE_URI, s_pLightEndpoint, NULL, onGetResponse,
                          LOW_QOS, NULL);
    ASSERT_TRUE(isSuccess) << "oc_do_get() returned failure during second call";
}

TEST_F(TestUnicastRequest, SendInitPutRequest_P)
{
    bool isSuccess = false;

    isSuccess = oc_init_put(RESOURCE_URI, s_pLightEndpoint, NULL, onPutResponse,
                            LOW_QOS, NULL);
    ASSERT_TRUE(isSuccess) << "oc_init_put() returned failure";
    oc_rep_start_root_object();
    oc_rep_set_boolean(root, state, true);
    oc_rep_end_root_object();
}

TEST_F(TestUnicastRequest, SendDoPutRequest_P)
{
    bool isSuccess = false;

    isSuccess = oc_do_put();
    ASSERT_TRUE(isSuccess) << "oc_do_put() returned failure";
}

TEST_F(TestUnicastRequest, SendInitPostRequest_P)
{
    bool isSuccess = false;

    isSuccess = oc_init_post(RESOURCE_URI, s_pLightEndpoint, NULL,
                             onPostResponse, LOW_QOS, NULL);
    ASSERT_TRUE(isSuccess) << "oc_init_put() returned failure";
    oc_rep_start_root_object();
    oc_rep_set_boolean(root, state, true);
    oc_rep_end_root_object();
}

TEST_F(TestUnicastRequest, SendDoPostRequest_P)
{
    bool isSuccess = false;

    isSuccess = oc_do_post();
    ASSERT_TRUE(isSuccess) << "oc_do_put() returned failure";
}

TEST_F(TestUnicastRequest, SendObserveRequest_P)
{
    bool isSuccess = false;

    isSuccess = oc_do_observe(RESOURCE_URI, s_pLightEndpoint, NULL, NULL,
                              LOW_QOS, NULL);
    ASSERT_TRUE(isSuccess) << "oc_do_observe() returned failure";
}

TEST_F(TestUnicastRequest, SendStopObserveRequest_P)
{
    bool isSuccess = false;

    isSuccess = oc_stop_observe(RESOURCE_URI, s_pLightEndpoint);
    ASSERT_TRUE(isSuccess) << "oc_stop_observe() returned failure";
}

TEST_F(TestUnicastRequest, SendDeleteRequest_P)
{
    bool isSuccess = false;

    isSuccess = oc_do_delete(RESOURCE_URI, s_pLightEndpoint, NULL,
                             onGetResponse, LOW_QOS, NULL);
    ASSERT_TRUE(isSuccess) << "oc_do_delete() returned failure";
}

TEST_F(TestUnicastRequest, FreeServerEndpoint_P)
{
    oc_free_server_endpoints(s_pLightEndpoint);
}

TEST_F(TestUnicastRequest, FreeServerEndpoint_N)
{
    oc_free_server_endpoints(NULL);
}

TEST_F(TestUnicastRequest, CloseSession_P)
{
    oc_close_session(s_pLightEndpoint);
}
