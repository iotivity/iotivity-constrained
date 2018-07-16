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


#include <stdlib.h>
#include "gtest/gtest.h"
extern "C" {
    #include "oc_rep.h"
    #include "oc_rep.h"
}
#define OC_MAX_APP_DATA_SIZE (2048)

TEST(TestRep, OCRepFinalizeTest_P)
{
    int repSize = oc_rep_finalize();
    EXPECT_NE(repSize, -1);
}

TEST(TestRep, RepNewTest_P)
{
    uint8_t *buf;  
    buf = malloc(OC_MAX_APP_DATA_SIZE);
    oc_rep_new(buf, OC_MAX_APP_DATA_SIZE);
    EXPECT_NE(buf, NULL);
}

TEST(TestRep, ParseRepTest_P)
{
    uint8_t *buf;
    oc_rep_t *rep;
    int ret;
    int parse;

    buf = malloc(OC_MAX_APP_DATA_SIZE);
    ret = oc_storage_read("obt_state", buf, OC_MAX_APP_DATA_SIZE);
    parse = oc_parse_rep(buf, ret, &rep);
    EXPECT_EQ(parse, 0);
}

