#include <cstdlib>
#include "gtest/gtest.h"

extern "C" {
#include "engine.h"
#include "oc_api.h"
#include "oc_endpoint.h"
#include "oc_signal_event_loop.h"
#define delete pseudo_delete
#include "oc_core_res.h"
#undef delete
}

class TestEngine: public testing::Test
{
    protected:
        virtual void SetUp()
        {

        }

        virtual void TearDown()
        {

        }
};

TEST_F(TestEngine, CoapInitEngineTest_P)
{
    coap_init_engine();
}

TEST_F(TestEngine, coapReceiveTest_P)
{
    coap_init_engine();
    oc_message_t *message = oc_internal_allocate_outgoing_message();
    int ret = coap_receive(message);
    EXPECT_TRUE(ret) << "Failed to get ret of receive message";

}
