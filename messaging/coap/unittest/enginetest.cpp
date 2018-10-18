#include <cstdlib>
#include "gtest/gtest.h"

#include "engine.h"
#include "oc_api.h"
#include "oc_endpoint.h"
#include "oc_signal_event_loop.h"
#define delete pseudo_delete
#include "oc_core_res.h"
#undef delete

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

/*
 * @API: coap_init_engine()
 * @Description: Tries to initialize engine
 * @PassCondition: should not throw exception
 * @PreCondition: N/A
 * @PostCondition: N/A
 */
TEST_F(TestEngine, CoapInitEngineTest_P)
{
    ASSERT_NO_THROW(coap_init_engine());
}

/*
 * @API: coap_receive()
 * @Description: Tries to receive message
 * @PassCondition: should not throw exception
 * @PreCondition: getting internal outgoing message
 * @PostCondition: N/A
 */
TEST_F(TestEngine, CoapReceiveTest_P)
{
    coap_init_engine();
    oc_message_t *message = oc_internal_allocate_outgoing_message();
    ASSERT_NO_THROW(coap_receive(message));
    oc_message_unref(message);
    
}
