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

#define RESOURCE_URI "/LightResourceURI"
#define RESOURCE_TYPE "oic.r.light"
#define RESOURCE_COLLECTION_NAME_ROOM "roomlights"
#define RESOURCE_COLLECTION_TYPE_LIGHT "/lights"
#define RESOURCE_COLLECTION_RELATION "room"

class TestCollectionRequest: public testing::Test
{
    public:
        static oc_resource_t *s_pResource;
        static oc_resource_t *s_pCol;
        static oc_link_t *s_pLink;

    protected:
        virtual void SetUp()
        {
        }

        virtual void TearDown()
        {
        }
};


oc_resource_t *TestCollectionRequest::s_pResource = nullptr;
oc_resource_t *TestCollectionRequest::s_pCol = nullptr;
oc_link_t *TestCollectionRequest::s_pLink = nullptr;

TEST_F(TestCollectionRequest, AddCollectionTest_P)
{
    s_pCol = oc_new_collection(RESOURCE_COLLECTION_NAME_ROOM,
                               RESOURCE_COLLECTION_TYPE_LIGHT, 1, 0);
    EXPECT_TRUE(TestCollectionRequest::s_pCol != NULL) << "Failed to make new collection";
}

TEST_F(TestCollectionRequest, DeleteCollectionTest_P)
{
    oc_delete_collection(TestCollectionRequest::s_pCol);
}

TEST_F(TestCollectionRequest, AddLinkTest_P)
{

	s_pResource = oc_new_resource(NULL, RESOURCE_URI, 1, 0);
	oc_resource_bind_resource_type(s_pResource, RESOURCE_TYPE);
	oc_resource_bind_resource_interface(s_pResource, OC_IF_RW);
	oc_resource_set_default_interface(s_pResource, OC_IF_RW);
	oc_resource_set_discoverable(s_pResource, true);
	oc_resource_set_periodic_observable(s_pResource, 1);
	oc_process_baseline_interface(s_pResource);
	oc_add_resource(s_pResource);
            
    s_pLink = oc_new_link(s_pResource);
    EXPECT_TRUE(s_pLink != NULL) << "Failed to make new link";
}

TEST_F(TestCollectionRequest, DeleteLinkTest_P)
{
    oc_delete_link(s_pLink);
}

TEST_F(TestCollectionRequest, AddLinkRelationTest_P)
{
           
    s_pLink = oc_new_link(s_pResource);
    oc_link_add_rel(s_pLink, RESOURCE_COLLECTION_RELATION);
}

TEST_F(TestCollectionRequest, SetLinkInstanceTest_P)
{
    oc_link_set_ins(s_pLink, RESOURCE_COLLECTION_RELATION);
}

TEST_F(TestCollectionRequest, AddCollectionLinkTest_P)
{
    s_pCol = oc_new_collection(RESOURCE_COLLECTION_NAME_ROOM,
                               RESOURCE_COLLECTION_TYPE_LIGHT, 1, 0);
    oc_resource_set_discoverable(s_pCol, true);
    s_pLink = oc_new_link(s_pResource);
    oc_collection_add_link(s_pCol, s_pLink);
}

TEST_F(TestCollectionRequest, RemoveCollectionLinkTest_P)
{
    oc_collection_remove_link(s_pCol, s_pLink);
    oc_delete_link(s_pLink);
    oc_delete_collection(TestCollectionRequest::s_pCol);
}

TEST_F(TestCollectionRequest, GetCollectionFromLinkTest_P)
{
    s_pCol = oc_new_collection(RESOURCE_COLLECTION_NAME_ROOM,
                               RESOURCE_COLLECTION_TYPE_LIGHT, 1, 0);
    s_pLink = oc_new_link(s_pResource);
    oc_collection_add_link(s_pCol, s_pLink);
    oc_link_t *link =  oc_collection_get_links(s_pCol);
    EXPECT_TRUE(link != NULL) << "Failed to get collection links ";
}


