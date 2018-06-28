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
        static pthread_cond_t s_cv;
        static oc_endpoint_t *s_pLightEndpoint;

        static oc_discovery_flags_t onResourceDiscovered(const char *di, const char *uri,
                oc_string_array_t types, oc_interface_mask_t interfaces,
                oc_endpoint_t *endpoint, oc_resource_properties_t bm, void *user_data)
        {
            (void)di;
            (void)types;
            (void)interfaces;
            (void)bm;
            (void)user_data;
            std::string discoveredResourceUri = std::string(uri);
            if (discoveredResourceUri.compare(RESOURCE_URI) == 0)
            {
                PRINT("Light Resource Discovered....\n");
                s_pLightEndpoint = endpoint;
                s_isResourceDiscovered = true;
                return OC_STOP_DISCOVERY;
            }

            oc_free_server_endpoints(endpoint);
            return OC_CONTINUE_DISCOVERY;
        }



        static void onGetResponse(oc_client_response_t *data)
        {
            (void)data;
            s_isCallbackReceived = true;
        }

        static void onGetRequest(oc_request_t *request, oc_interface_mask_t interface, void *user_data)
        {
            (void)request;
            (void)interface;
            (void)user_data;
        }

        static int appInit(void)
        {
            int result = oc_init_platform(MANUFACTURER_NAME, NULL, NULL);
            result |= oc_add_device(DEVICE_URI, DEVICE_TYPE, DEVICE_NAME,
                                    OCF_SPEC_VERSION, OCF_DATA_MODEL_VERSION, NULL, NULL);
            return result;
        }

        static void registerResources(void)
        {
            s_pResource = oc_new_resource(NULL, RESOURCE_URI, 1, 0);
            oc_resource_bind_resource_type(s_pResource, RESOURCE_TYPE);
            oc_resource_bind_resource_interface(s_pResource, OC_IF_BASELINE);
            oc_resource_set_default_interface(s_pResource, OC_IF_BASELINE);
            oc_resource_set_discoverable(s_pResource, true);
            oc_resource_set_periodic_observable(s_pResource, 1);
            oc_resource_set_request_handler(s_pResource, OC_GET, onGetRequest, NULL);
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
        }

        static void waitForEvent(int waitTime)
        {
            oc_clock_time_t next_event;
            (void)next_event;
            while (waitTime && !s_isCallbackReceived)
            {
                PRINT("Waiting for callback....\n");
                next_event = oc_main_poll();
                sleep(1);
                waitTime--;
            }
        }

    protected:
        virtual void SetUp()
        {
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
            if ( initResult < 0)
            {

                FAIL() << "Initialization of main server failed";
                s_isServerStarted = false;
            }
            else
            {
                s_isServerStarted = true;
            }

            s_isResourceDiscovered = false;
            ASSERT_TRUE(oc_do_ip_discovery(NULL, &onResourceDiscovered, NULL)) << "oc_do_ip_discovery() returned failure.";

            waitForEvent(MAX_WAIT_TIME);
            ASSERT_TRUE(s_isResourceDiscovered) << " Unable to discover Light Resource";
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
        }
};

bool TestUnicastRequest::s_isServerStarted = false;
bool TestUnicastRequest::s_isResourceDiscovered = false;
bool TestUnicastRequest::s_isCallbackReceived = false;
oc_resource_t *TestUnicastRequest::s_pResource = nullptr;
oc_endpoint_t *TestUnicastRequest::s_pLightEndpoint = nullptr;
oc_handler_t TestUnicastRequest::s_handler;
pthread_mutex_t TestUnicastRequest::s_mutex;
pthread_cond_t TestUnicastRequest::s_cv;

TEST(TestServerClient, ServerStartTest_P)
{
    static const oc_handler_t handler = {.init = appInit,
                                         .signal_event_loop = signalEventLoop,
                                         .register_resources = registerResources,
                                         .requests_entry = requestsEntry
                                        };

    int result = oc_main_init(&handler);
    EXPECT_LT(result,  0);

    oc_main_shutdown();
}

TEST(TestServerClient, ServerStopTest_P)
{

    static const oc_handler_t handler = {.init = appInit,
                                         .signal_event_loop = signalEventLoop,
                                         .register_resources = registerResources,
                                         .requests_entry = requestsEntry
                                        };

    int result = oc_main_init(&handler);
    ASSERT_LT(result,  0);

    EXPECT_NO_THROW(oc_main_shutdown());
}

TEST_F(TestUnicastRequest, DiscoverResourceTest_P)
{
    EXPECT_TRUE(s_isResourceDiscovered) << "Failed to discover resource";
}

TEST_F(TestUnicastRequest, SendGetRequest_P)
{
    bool isSuccess = false;
    s_isCallbackReceived = false;

    isSuccess = oc_do_get(RESOURCE_URI, s_pLightEndpoint, NULL, onGetResponse, HIGH_QOS, NULL);
    ASSERT_TRUE(isSuccess) << "oc_do_get() returned failure";

    waitForEvent(MAX_WAIT_TIME);
    ASSERT_TRUE(s_isCallbackReceived) << "Callback for get was not called";
}

TEST_F(TestUnicastRequest, SendGetRequestTwice_P)
{
    bool isSuccess = false;
    s_isCallbackReceived = false;

    isSuccess = oc_do_get(RESOURCE_URI, s_pLightEndpoint, NULL, onGetResponse, LOW_QOS, NULL);
    ASSERT_TRUE(isSuccess) << "oc_do_get() returned failure";

    waitForEvent(MAX_WAIT_TIME);
    ASSERT_TRUE(s_isCallbackReceived) << "Callback for get was not called";

    s_isCallbackReceived = false;
    isSuccess = oc_do_get(RESOURCE_URI, s_pLightEndpoint, NULL, onGetResponse, LOW_QOS, NULL);
    ASSERT_TRUE(isSuccess) << "oc_do_get() returned failure during second call";

    waitForEvent(MAX_WAIT_TIME);
    ASSERT_TRUE(s_isCallbackReceived) << "Callback for get was not called during second call";
}