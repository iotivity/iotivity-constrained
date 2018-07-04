/****************************************************************************
 *
 * Copyright 2018 Samsung Electronics All Rights Reserved.
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

#include <gtest/gtest.h>
#include <cstdlib>

extern "C"{
    #include "st_data_manager.h"
    #include "st_resource_manager.h"
    #include "oc_api.h"
    #include "oc_ri.h"
    #include "st_port.h"
    #include "sttestcommon.h"
    #include "sc_easysetup.h"
}

static int device_index = 0;
static bool
resource_handler(oc_request_t *request)
{
    (void)request;
    return true;
}

class TestSTResourceManager: public testing::Test
{
    protected:
        virtual void SetUp()
        {

        }

        virtual void TearDown()
        {

        }
};

TEST_F(TestSTResourceManager, st_register_resources)
{
    char *uri = "/capability/switch/main/0";
    oc_resource_t *resource = NULL;
    st_data_mgr_info_load();
    int ret = st_register_resources(device_index);
    resource = oc_ri_get_app_resource_by_uri(uri, strlen(uri), device_index);
    EXPECT_EQ(0, ret);
    EXPECT_STREQ(uri, oc_string(resource->uri));
    st_data_mgr_info_free();
    oc_delete_resource(resource);
}

TEST_F(TestSTResourceManager, st_register_resources_fail)
{
    int ret = st_register_resources(device_index);
    EXPECT_EQ(-1, ret);
}

TEST_F(TestSTResourceManager, st_register_resource_handler)
{
    int ret = st_register_resource_handler(resource_handler, resource_handler);
    EXPECT_EQ(0, ret);
}

TEST_F(TestSTResourceManager, st_register_resource_handler_fail)
{
    int ret = st_register_resource_handler(NULL, NULL);
    EXPECT_EQ(-1, ret);
}

TEST_F(TestSTResourceManager, st_notify_back)
{
    // Given
    char *uri = "/capability/switch/main/0";
    oc_resource_t *resource = oc_new_resource(NULL, uri, 1, 0);
    oc_resource_bind_resource_type(resource, "core.light");
    oc_add_resource(resource);

    // When
    st_notify_back(uri);
    oc_delete_resource(resource);

    // Then
    // EXPECT_EQ(0, ret);
}

TEST_F(TestSTResourceManager, st_notify_back_fail_null)
{
    // Given
    char *uri = NULL;

    // When
    int ret = st_notify_back(uri);

    // Then
    EXPECT_EQ(-1, ret);
}

TEST_F(TestSTResourceManager, st_notify_back_fail)
{
    // Given
    char *uri = "/capability/switch/main/1";

    // When
    int ret = st_notify_back(uri);

    // Then
    EXPECT_EQ(-1, ret);
}

#define MAX_WAIT_TIME 10
#define RESOURCE_URI "/capability/switch/main/0"
#define DEVICE_URI "/oic/d"
#define DEVICE_TYPE "oic.d.light"
#define MANUFACTURER_NAME "Samsung"
#define DEVICE_NAME "Table Lamp"
#define DEVICE_NUM 0
#define OCF_SPEC_VERSION "ocf.1.0.0"
#define OCF_DATA_MODEL_VERSION "ocf.res.1.0.0"

class TestSTResourceManagerHandler: public testing::Test
{
    public:
        static oc_handler_t handler;
        static pthread_mutex_t mutex;
        static pthread_cond_t cv;
        static oc_endpoint_t *endpoint;
        static bool isServerStarted;
        static bool isResourceDiscovered;
        static bool isCallbackReceived;
        static oc_endpoint_t *LightEndpoint;

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
                PRINT("Switch Resource Discovered...\n");
                LightEndpoint = endpoint;
                isResourceDiscovered = true;
                return OC_STOP_DISCOVERY;
            }

            oc_free_server_endpoints(endpoint);
            return OC_CONTINUE_DISCOVERY;
        }

        static void onGetResponse(oc_client_response_t *data)
        {
            (void)data;
            isCallbackReceived = true;
        }

        static int appInit(void)
        {
            int result = oc_init_platform(MANUFACTURER_NAME, NULL, NULL);
            result |= oc_add_device(DEVICE_URI, DEVICE_TYPE, DEVICE_NAME,
                                    OCF_SPEC_VERSION, OCF_DATA_MODEL_VERSION, NULL, NULL);
            return result;
        }

        static void signalEventLoop(void)
        {
            pthread_mutex_lock(&mutex);
            pthread_cond_signal(&cv);
            pthread_mutex_unlock(&mutex);
        }

        static void registerResources(void)
        {
            st_register_resources(DEVICE_NUM);
        }

        static void requestsEntry(void)
        {
        }

        static void waitForEvent(int waitTime)
        {
            oc_clock_time_t next_event;
            (void)next_event;
            while (waitTime && !isCallbackReceived)
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
            deinit_provisioning_info_resource();
        }

        static void SetUpTestCase()
        {
            handler.init = &appInit;
            handler.signal_event_loop = &signalEventLoop;
            handler.register_resources = &registerResources;
            handler.requests_entry = &requestsEntry;
#ifdef OC_SECURITY
            oc_storage_config("./st_things_creds");
#endif /* OC_SECURITY */
            st_data_mgr_info_load();

            int initResult = oc_main_init(&handler);
            if ( initResult < 0)
            {
                FAIL() << "Initialization of main server failed";
                isServerStarted = false;
            }
            else
            {
                isServerStarted = true;
            }

            get_wildcard_acl_policy();

            ASSERT_TRUE(oc_do_ip_discovery(NULL, &onResourceDiscovered, NULL)) << "oc_do_ip_discovery() returned failure.";

            waitForEvent(MAX_WAIT_TIME);
            ASSERT_TRUE(isResourceDiscovered) << " Unable to discover Switch Resource";
        }

        static void TearDownTestCase()
        {
            if (isServerStarted)
                oc_main_shutdown();

            st_data_mgr_info_free();
        }
};

bool TestSTResourceManagerHandler::isServerStarted = false;
bool TestSTResourceManagerHandler::isCallbackReceived = false;
bool TestSTResourceManagerHandler::isResourceDiscovered = false;
oc_endpoint_t *TestSTResourceManagerHandler::LightEndpoint = nullptr;
oc_handler_t TestSTResourceManagerHandler::handler;
pthread_mutex_t TestSTResourceManagerHandler::mutex;
pthread_cond_t TestSTResourceManagerHandler::cv;

TEST_F(TestSTResourceManagerHandler, Get_Request)
{
    bool isSuccess = false;
    isCallbackReceived = false;

    isSuccess = oc_do_get(RESOURCE_URI, LightEndpoint, NULL, onGetResponse, HIGH_QOS, NULL);

    EXPECT_TRUE(isSuccess);

    waitForEvent(MAX_WAIT_TIME);
    EXPECT_TRUE(isCallbackReceived);
}

TEST_F(TestSTResourceManagerHandler, Post_Request)
{
    bool init_success, post_success = false;
    isCallbackReceived = false;

    init_success = oc_init_post(RESOURCE_URI, LightEndpoint, NULL, onGetResponse, LOW_QOS, NULL);
    post_success = oc_do_post();

    EXPECT_TRUE(init_success);
    EXPECT_TRUE(post_success);

    waitForEvent(MAX_WAIT_TIME);
    EXPECT_TRUE(isCallbackReceived);
}