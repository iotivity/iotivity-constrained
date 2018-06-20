/******************************************************************
 *
 * Copyright 2018 GRANITE RIVER LABS All Rights Reserved.
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
#include <stdio.h>
#include <gtest/gtest.h>

extern "C" {
    #include "port/oc_storage.h"
    #include <config.h>
    }
#define STORAGE_CONFIG "./simpleserver_creds"
//./simple_len_sert folder is not present
//used to check the negative scenario
#define STORAGE_CONFIG_N "./simple_len_sert"
uint8_t buf[OC_MAX_APP_DATA_SIZE] = "uuuuuu";
static buf2[OC_MAX_APP_DATA_SIZE] = "";

class TestStorage: public testing::Test
{
    protected:
        virtual void SetUp()
        {
            oc_storage_config(STORAGE_CONFIG);
        }
        virtual void TearDown()
        {
        }
};

TEST(TestStorage, StorageConfigTest_P)
{
    int ret = oc_storage_config(STORAGE_CONFIG);
    EXPECT_EQ(0, ret);
}

TEST(TestStorage, StorageConfigTest_N)
{
    int ret = oc_storage_config("IOTIVITY_CONSTRAINED_LITE_Sample_Test_devices_IOTIVITY_CONSTRAINED_LITE_Sample_Test_devices_IOTIVITY");
    ASSERT_EQ(-ENOENT, ret);
}

TEST_F(TestStorage, StorageReadWrite_P)
{
    long ret_write = oc_storage_write(STORAGE_CONFIG, buf, OC_MAX_APP_DATA_SIZE);
    long ret_read = oc_storage_read(STORAGE_CONFIG, buf2, OC_MAX_APP_DATA_SIZE);
    EXPECT_EQ(buf, buf2);
}

TEST_F(TestStorage, StorageReadWrite_N)
{
    long ret = oc_storage_write(STORAGE_CONFIG_N, buf, OC_MAX_APP_DATA_SIZE);
    EXPECT_EQ(-EINVAL, ret);
    long ret1 = oc_storage_read(STORAGE_CONFIG_N, buf2, OC_MAX_APP_DATA_SIZE);
    EXPECT_EQ(-EINVAL, ret1)<< "Invalid Folder able to read and write";
}

TEST_F(TestStorage, NewResource_P)
{
    oc_resource_t s_pResource = oc_new_resource(NULL, "/a/light", 2, 0);
    EXPECT_NE(0, s_pResource);
}

   TEST_F(TestStorage, NewCollectionTest_P)
{
    oc_resource_t *collection =  oc_new_collection("roomlights","/lights",1,0);
    EXPECT_NE(0, collection);
}