#include <cstdlib>
#include "gtest/gtest.h"

#include "coap.h"
#ifdef OC_TCP
#include "coap_signal.h"
#endif /* OC_TCP */
#include "oc_api.h"
#include "oc_endpoint.h"

class TestCoap: public testing::Test
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
* @API: coap_get_mid()
* @Description: Tries to get mid value
* @PassCondition: Should return mid value
* @PreCondition: N/A
* @PostCondition: N/A
*/
TEST_F(TestCoap, CoapGetMidTest_P)
{
    uint16_t mid = coap_get_mid();
    EXPECT_TRUE(mid) << "Failed to get mid transaction";
}

/*
 * @API: coap_udp_init_message()
 * @Description: Tries to initialize udp message
 * @PassCondition: should not throw exception
 * @PreCondition: Get mid value
 * @PostCondition: N/A
 */
TEST_F(TestCoap, CoapUDPInitMessageTest_P)
{
    coap_packet_t ack[1];
    uint16_t mid = coap_get_mid();
    ASSERT_NO_THROW(coap_udp_init_message(ack, COAP_TYPE_ACK, 0, mid));
}

/*
 * @API: coap_serialize_message()
 * @Description: Tries to serialize message
 * @PassCondition: Should serialize message
 * @PreCondition: Get mid value, initialize udp message and getting internal outgoing message
 * @PostCondition: N/A
 */
TEST_F(TestCoap, CoapSerializeMessageTest_P)
{
    coap_packet_t ack[1];
    uint16_t mid = coap_get_mid();
    coap_udp_init_message(ack, COAP_TYPE_ACK, 0, mid);
    oc_message_t *message = oc_internal_allocate_outgoing_message();
    size_t size = coap_serialize_message(ack, message->data);
    oc_message_unref(message);
    EXPECT_TRUE(size) << "Failed to get mid transaction";
}

/*
 * @API: coap_udp_parse_message()
 * @Description: Tries to parse udp message
 * @PassCondition: Should get status of udp parsing message
 * @PreCondition: N/A
 * @PostCondition: N/A
 */
TEST_F(TestCoap, UDPParseMessageTest_P)
{
    uint8_t data[] = "1234567890";
    coap_packet_t packet[1];
    coap_status_t status = coap_udp_parse_message(packet, data, 10);
    ASSERT_NE(status, 0);
}

/*
 * @API: coap_set_header_max_age()
 * @Description: Tries to set header maximum age
 * @PassCondition: Should get status of setup maximum age
 * @PreCondition: N/A
 * @PostCondition: N/A
 */
TEST_F(TestCoap, CoapSetHeaderMaxAgeTest_P)
{
    coap_packet_t packet[1];
    int isSuccess = coap_set_header_max_age(packet, 10);
    ASSERT_EQ(isSuccess, 1);

}

/*
 * @API: coap_get_header_etag()
 * @Description: Tries to get header etag
 * @PassCondition: Should get status result of getting header etag
 * @PreCondition: N/A
 * @PostCondition: N/A
 */
TEST_F(TestCoap, CoapGetHeaderEtagTest_P)
{
    coap_packet_t packet[1];
    uint8_t *etag = "1234";
    int ret = coap_get_header_etag(packet, &etag);
    ASSERT_NE(ret, -1);
}

/*
 * @API: coap_set_header_location_query()
 * @Description: Tries to set header location query
 * @PassCondition: Should get status of setup header location query
 * @PreCondition: N/A
 * @PostCondition: N/A
 */
TEST_F(TestCoap, CoapSetHeaderQueryTest_P)
{
    coap_packet_t packet[1];
    char query[] = "1234";
    int isSuccess = coap_set_header_location_query(packet, query);
    ASSERT_NE(isSuccess, 0);
}

/*
 * @API: coap_set_header_block1()
 * @Description: Tries to set header block
 * @PassCondition: Should get failure status of setup header block
 * @PreCondition: N/A
 * @PostCondition: N/A
 */
TEST_F(TestCoap, CoapSetHeaderBlock1Test_N)
{
    coap_packet_t packet[1];
    int isFailure = coap_set_header_block1(packet, 10, 1, 5);
    ASSERT_EQ(isFailure, 0);
}

/*
 * @API: coap_set_header_block1()
 * @Description: Tries to set header block
 * @PassCondition: Should get success status of setup header block
 * @PreCondition: N/A
 * @PostCondition: N/A
 */
TEST_F(TestCoap, CoapSetHeaderBlock1Test_P)
{
    coap_packet_t packet[1];
    int isSuccess = coap_set_header_block1(packet, 10, 1, 32);
    ASSERT_NE(isSuccess, 0);
}

/*
 * @API: coap_get_header_size2()
 * @Description: Tries to get header size2
 * @PassCondition: Should get failure status of getting header size2
 * @PreCondition: N/A
 * @PostCondition: N/A
 */
TEST_F(TestCoap, CoapGetHeaderSize2Test_N)
{
    coap_packet_t packet[1];
    uint32_t size;
    int isSuccess = coap_get_header_size2(packet, &size);
    // checking will be added later after verification
}

/*
 * @API: coap_get_header_size1()
 * @Description: Tries to get header size1
 * @PassCondition: Should get failure status of getting header size1
 * @PreCondition: N/A
 * @PostCondition: N/A
 */
TEST_F(TestCoap, CoapGetHeaderSize1Test_N)
{
    coap_packet_t packet[1];
    uint32_t size;
    int isSuccess = coap_get_header_size1(packet, &size);
    // checking will be added later after verification
}

/*
 * @API: coap_set_header_size1()
 * @Description: Tries to set header size
 * @PassCondition: Should get success status of getting header size1
 * @PreCondition: N/A
 * @PostCondition: N/A
 */
TEST_F(TestCoap, CoapSetHeaderSize1Test_P)
{
    coap_packet_t packet[1];
    int isSuccess = coap_set_header_size1(packet, 10);
    ASSERT_EQ(isSuccess, 1);
}

/*
 * @API: coap_tcp_init_message()
 * @Description: Tries to initialize tcp message
 * @PassCondition: Should should not throw exception
 * @PreCondition: Get mid value
 * @PostCondition: N/A
 */
#ifdef OC_TCP
TEST_F(TestCoap, CoapTCPInitMessageTest_P)
{
    coap_packet_t packet[1];
    uint16_t mid = coap_get_mid();
    ASSERT_NO_THROW(coap_tcp_init_message(packet, 0));
}

/*
 * @API: coap_tcp_get_packet_size()
 * @Description: Tries to get tcp packet size
 * @PassCondition: Should get tcp packet size
 * @PreCondition: N/A
 * @PostCondition: N/A
 */
TEST_F(TestCoap, TCPPacketSizeTest_P)
{
    uint8_t data[] = "1234567890";
    size_t size = coap_tcp_get_packet_size(data);
    ASSERT_NE(size, 0);

}

/*
 * @API: coap_tcp_parse_message()
 * @Description: Tries to get status tcp parse message
 * @PassCondition: Should get status tcp parsing message
 * @PreCondition: N/A
 * @PostCondition: N/A
 */
TEST_F(TestCoap, TCPParseMessageTest_P)
{
    uint8_t data[] = "1234567890";
    coap_packet_t packet[1];
    coap_status_t status = coap_tcp_parse_message(packet, data, 10);
    ASSERT_NE(status, 0);

}

/*
 * @API: coap_signal_get_max_msg_size()
 * @Description: Tries to get max message size for signal packet
 * @PassCondition: Should get max message size
 * @PreCondition: set max message size option
 * @PostCondition: N/A
 */
TEST_F(TestCoap, SignalGetMaxMsgSizeTest_P)
{
    coap_packet_t packet[1];
    uint32_t answer = 1152;
    coap_tcp_init_message(packet, CSM_7_01);
    coap_signal_set_max_msg_size(packet, answer);

    uint32_t size = 0;
    int status = coap_signal_get_max_msg_size(packet, &size);

    ASSERT_NE(0, status);
    ASSERT_EQ(answer, size);
}

/*
 * @API: coap_signal_get_max_msg_size()
 * @Description: Tries to get max message size for signal packet
 * @PassCondition: Should get failure status
 * @PreCondition: N/A
 * @PostCondition: N/A
 */
TEST_F(TestCoap, SignalGetMaxMsgSizeTest_N)
{
    coap_packet_t packet[1];

    uint32_t size = 0;
    int isFailure = coap_signal_get_max_msg_size(packet, &size);

    ASSERT_EQ(isFailure, 0);
}

/*
 * @API: coap_signal_set_max_msg_size()
 * @Description: Tries to set max message size for signal packet
 * @PassCondition: Should set max message size
 * @PreCondition: N/A
 * @PostCondition: N/A
 */
TEST_F(TestCoap, SignalSetMaxMsgSizeTest_P)
{
    coap_packet_t packet[1];
    coap_tcp_init_message(packet, CSM_7_01);

    uint32_t size = 1152;
    int status = coap_signal_set_max_msg_size(packet, size);

    ASSERT_NE(0, status);

    uint32_t actual = 0;
    coap_signal_get_max_msg_size(packet, &actual);
    ASSERT_EQ(size, actual);
}

/*
 * @API: coap_signal_get_max_msg_size()
 * @Description: Tries to set max message size for signal packet
 * @PassCondition: Should get failure status
 * @PreCondition: N/A
 * @PostCondition: N/A
 */
TEST_F(TestCoap, SignalSetMaxMsgSizeTest_N)
{
    coap_packet_t packet[1];

    uint32_t size = 1152;
    int isFailure = coap_signal_set_max_msg_size(packet, size);

    ASSERT_EQ(isFailure, 0);
}

/*
 * @API: coap_signal_get_bert()
 * @Description: Tries to get bert flag for signal packet
 * @PassCondition: Should get bert flag
 * @PreCondition: set bert option
 * @PostCondition: N/A
 */
TEST_F(TestCoap, SignalGetBertTest_P)
{
    coap_packet_t packet[1];
    uint8_t bert = 1;
    coap_tcp_init_message(packet, CSM_7_01);
    coap_signal_set_bert(packet, bert);

    uint8_t flag = 0;
    int status = coap_signal_get_bert(packet, &flag);

    ASSERT_NE(0, status);
    ASSERT_EQ(bert, flag);
}

/*
 * @API: coap_signal_get_bert()
 * @Description: Tries to get gert flag for signal packet
 * @PassCondition: Should get failure status
 * @PreCondition: N/A
 * @PostCondition: N/A
 */
TEST_F(TestCoap, SignalGetBertTest_N)
{
    coap_packet_t packet[1];

    uint8_t flag = 0;
    int isFailure = coap_signal_get_bert(packet, &flag);

    ASSERT_EQ(isFailure, 0);
}

/*
 * @API: coap_signal_set_bert()
 * @Description: Tries to set bert flag for signal packet
 * @PassCondition: Should set bert flag
 * @PreCondition: N/A
 * @PostCondition: N/A
 */
TEST_F(TestCoap, SignalSetBertTest_P)
{
    coap_packet_t packet[1];
    coap_tcp_init_message(packet, CSM_7_01);

    uint8_t bert = 1;
    int status = coap_signal_set_bert(packet, bert);

    ASSERT_NE(0, status);

    uint8_t actual = 0;
    coap_signal_get_bert(packet, &actual);
    ASSERT_EQ(bert, actual);
}

/*
 * @API: coap_signal_set_bert()
 * @Description: Tries to set bert flag for signal packet
 * @PassCondition: Should get failure status
 * @PreCondition: N/A
 * @PostCondition: N/A
 */
TEST_F(TestCoap, SignalSetBertTest_N)
{
    coap_packet_t packet[1];

    uint8_t bert = 1;
    int isFailure = coap_signal_set_bert(packet, bert);

    ASSERT_EQ(isFailure, 0);
}

/*
 * @API: coap_signal_get_custody()
 * @Description: Tries to get custody flag for signal packet
 * @PassCondition: Should get custody flag
 * @PreCondition: set custody option
 * @PostCondition: N/A
 */
TEST_F(TestCoap, SignalGetCustodyTest_P)
{
    coap_packet_t packet[1];
    uint8_t custody = 1;
    coap_tcp_init_message(packet, PING_7_02);
    coap_signal_set_custody(packet, custody);

    uint8_t flag = 0;
    int status = coap_signal_get_custody(packet, &flag);

    ASSERT_NE(0, status);
    ASSERT_EQ(custody, flag);
}

/*
 * @API: coap_signal_get_custody()
 * @Description: Tries to get custody flag for signal packet
 * @PassCondition: Should get failure status
 * @PreCondition: N/A
 * @PostCondition: N/A
 */
TEST_F(TestCoap, SignalGetCustodyTest_N)
{
    coap_packet_t packet[1];

    uint8_t flag = 0;
    int isFailure = coap_signal_get_custody(packet, &flag);

    ASSERT_EQ(isFailure, 0);
}

/*
 * @API: coap_signal_set_custody()
 * @Description: Tries to set custody flag for signal packet
 * @PassCondition: Should set custody flag
 * @PreCondition: N/A
 * @PostCondition: N/A
 */
TEST_F(TestCoap, SignalSetCustodyTest_P)
{
    coap_packet_t packet[1];
    coap_tcp_init_message(packet, PING_7_02);

    uint8_t custody = 1;
    int status = coap_signal_set_custody(packet, custody);

    ASSERT_NE(0, status);

    uint8_t actual = 0;
    coap_signal_get_custody(packet, &actual);
    ASSERT_EQ(custody, actual);
}

/*
 * @API: coap_signal_set_custody()
 * @Description: Tries to set custody flag for signal packet
 * @PassCondition: Should get failure status
 * @PreCondition: N/A
 * @PostCondition: N/A
 */
TEST_F(TestCoap, SignalSetCustodyTest_N)
{
    coap_packet_t packet[1];

    uint8_t custody = 1;
    int isFailure = coap_signal_set_custody(packet, custody);

    ASSERT_EQ(isFailure, 0);
}

/*
 * @API: coap_signal_get_alt_addr()
 * @Description: Tries to get alternative address for signal packet
 * @PassCondition: Should get alternative address
 * @PreCondition: set alternative address option
 * @PostCondition: N/A
 */
TEST_F(TestCoap, SignalGetAltAddrTest_P)
{
    coap_packet_t packet[1];
    const char *addr = "coap+tcp://127.0.0.1:5683";
    size_t addr_len = strlen(addr) + 1;
    coap_tcp_init_message(packet, RELEASE_7_04);
    coap_signal_set_alt_addr(packet, addr, addr_len);

    const char *actual = NULL;
    size_t actual_len = coap_signal_get_alt_addr(packet, &actual);

    ASSERT_EQ(addr, actual);
    ASSERT_EQ(addr_len, actual_len);
}

/*
 * @API: coap_signal_get_alt_addr()
 * @Description: Tries to get alternative address for signal packet
 * @PassCondition: Should get failure status
 * @PreCondition: N/A
 * @PostCondition: N/A
 */
TEST_F(TestCoap, SignalGetAltAddrTest_N)
{
    coap_packet_t packet[1];

    size_t isFailure = coap_signal_get_alt_addr(packet, NULL);

    ASSERT_EQ(isFailure, 0);
}

/*
 * @API: coap_signal_set_alt_addr()
 * @Description: Tries to set alternative address for signal packet
 * @PassCondition: Should set alternative address
 * @PreCondition: N/A
 * @PostCondition: N/A
 */
TEST_F(TestCoap, SignalSetAltAddrTest_P)
{
    coap_packet_t packet[1];
    coap_tcp_init_message(packet, RELEASE_7_04);

    const char *addr = "coap+tcp://127.0.0.1:5683";
    size_t addr_len = strlen(addr) + 1;
    size_t length = coap_signal_set_alt_addr(packet, addr, addr_len);

    ASSERT_EQ(addr_len, length);

    const char *actual = NULL;
    length = coap_signal_get_alt_addr(packet, &actual);
    ASSERT_EQ(addr_len, length);
    ASSERT_EQ(addr, actual);
}

/*
 * @API: coap_signal_set_alt_addr()
 * @Description: Tries to set alternative address for signal packet
 * @PassCondition: Should get failure status
 * @PreCondition: N/A
 * @PostCondition: N/A
 */
TEST_F(TestCoap, SignalSetAltAddrTest_N)
{
    coap_packet_t packet[1];

    size_t isFailure = coap_signal_set_alt_addr(packet, NULL, 0);

    ASSERT_EQ(isFailure, 0);
}

/*
 * @API: coap_signal_get_hold_off()
 * @Description: Tries to get hold off seconds for signal packet
 * @PassCondition: Should get hold off seconds
 * @PreCondition: set hold off option
 * @PostCondition: N/A
 */
TEST_F(TestCoap, SignalGetHoldOffTest_P)
{
    coap_packet_t packet[1];
    coap_tcp_init_message(packet, RELEASE_7_04);
    uint32_t time_seconds = 10;
    coap_signal_set_hold_off(packet, time_seconds);

    uint32_t actual = 0;
    int status = coap_signal_get_hold_off(packet, &actual);

    ASSERT_NE(0, status);
    ASSERT_EQ(time_seconds, actual);
}

/*
 * @API: coap_signal_get_hold_off()
 * @Description: Tries to get hold off seconds for signal packet
 * @PassCondition: Should get failure status
 * @PreCondition: N/A
 * @PostCondition: N/A
 */
TEST_F(TestCoap, SignalGetHoldOffTest_N)
{
    coap_packet_t packet[1];

    uint32_t time_seconds;
    size_t isFailure = coap_signal_get_hold_off(packet, &time_seconds);

    ASSERT_EQ(isFailure, 0);
}

/*
 * @API: coap_signal_set_hold_off()
 * @Description: Tries to set hold off seconds for signal packet
 * @PassCondition: Should set hold off seconds
 * @PreCondition: N/A
 * @PostCondition: N/A
 */
TEST_F(TestCoap, SignalSetHoldOffTest_P)
{
    coap_packet_t packet[1];
    coap_tcp_init_message(packet, RELEASE_7_04);

    uint32_t time_seconds = 10;
    int status = coap_signal_set_hold_off(packet, time_seconds);

    ASSERT_NE(0, status);

    uint32_t actual = 0;
    coap_signal_get_hold_off(packet, &actual);
    ASSERT_EQ(time_seconds, actual);
}

/*
 * @API: coap_signal_set_hold_off()
 * @Description: Tries to set hold off seconds for signal packet
 * @PassCondition: Should get failure status
 * @PreCondition: N/A
 * @PostCondition: N/A
 */
TEST_F(TestCoap, SignalSetHoldOffTest_N)
{
    coap_packet_t packet[1];

    uint32_t time_seconds = 10;
    size_t isFailure = coap_signal_set_hold_off(packet, time_seconds);

    ASSERT_EQ(isFailure, 0);
}

/*
 * @API: coap_signal_get_bad_csm()
 * @Description: Tries to get bad csm option for signal packet
 * @PassCondition: Should get bad csm option
 * @PreCondition: set bad csm option
 * @PostCondition: N/A
 */
TEST_F(TestCoap, SignalGetBadCsmTest_P)
{
    coap_packet_t packet[1];
    coap_tcp_init_message(packet, ABORT_7_05);
    uint16_t opt = 10;
    coap_signal_set_bad_csm(packet, opt);

    uint16_t actual = 0;
    int status = coap_signal_get_bad_csm(packet, &actual);

    ASSERT_NE(0, status);
    ASSERT_EQ(opt, actual);
}

/*
 * @API: coap_signal_get_bad_csm()
 * @Description: Tries to get bad csm option for signal packet
 * @PassCondition: Should get failure status
 * @PreCondition: N/A
 * @PostCondition: N/A
 */
TEST_F(TestCoap, SignalGetBadCsmTest_N)
{
    coap_packet_t packet[1];

    uint16_t opt;
    size_t isFailure = coap_signal_get_bad_csm(packet, &opt);

    ASSERT_EQ(isFailure, 0);
}

/*
 * @API: coap_signal_set_bad_csm()
 * @Description: Tries to set bad csm option for signal packet
 * @PassCondition: Should set bad csm option
 * @PreCondition: N/A
 * @PostCondition: N/A
 */
TEST_F(TestCoap, SignalSetBadCsmTest_P)
{
    coap_packet_t packet[1];
    coap_tcp_init_message(packet, ABORT_7_05);

    uint16_t opt = 10;
    int status = coap_signal_set_bad_csm(packet, opt);

    ASSERT_NE(0, status);

    uint16_t actual = 0;
    coap_signal_get_bad_csm(packet, &actual);
    ASSERT_EQ(opt, actual);
}

/*
 * @API: coap_signal_set_bad_csm()
 * @Description: Tries to set bad csm option for signal packet
 * @PassCondition: Should get failure status
 * @PreCondition: N/A
 * @PostCondition: N/A
 */
TEST_F(TestCoap, SignalSetBadCsmTest_N)
{
    coap_packet_t packet[1];

    uint16_t opt = 10;
    size_t isFailure = coap_signal_set_bad_csm(packet, opt);

    ASSERT_EQ(isFailure, 0);
}

TEST_F(TestCoap, SignalSerializeParseTest_CSM)
{
    coap_packet_t packet[1];
    coap_tcp_init_message(packet, CSM_7_01);

    uint32_t size = 1152;
    coap_signal_set_max_msg_size(packet, size);
    coap_signal_set_bert(packet, 1);

    uint8_t buffer[OC_PDU_SIZE];
    size_t buffer_len = coap_serialize_message(packet, buffer);

    coap_packet_t parse_packet[1];
    coap_status_t ret = coap_tcp_parse_message(parse_packet, buffer, buffer_len);

    ASSERT_EQ(COAP_NO_ERROR, ret);
    ASSERT_EQ(packet->code, parse_packet->code);
    ASSERT_EQ(packet->max_msg_size, parse_packet->max_msg_size);
    ASSERT_EQ(packet->bert, parse_packet->bert);
}

TEST_F(TestCoap, SignalSerializeParseTest_PING)
{
    coap_packet_t packet[1];
    coap_tcp_init_message(packet, PING_7_02);
    coap_signal_set_custody(packet, 1);

    uint8_t buffer[OC_PDU_SIZE];
    size_t buffer_len = coap_serialize_message(packet, buffer);

    coap_packet_t parse_packet[1];
    coap_status_t ret = coap_tcp_parse_message(parse_packet, buffer, buffer_len);

    ASSERT_EQ(COAP_NO_ERROR, ret);
    ASSERT_EQ(packet->code, parse_packet->code);
    ASSERT_EQ(packet->custody, parse_packet->custody);
}

TEST_F(TestCoap, SignalSerializeParseTest_PONG)
{
    coap_packet_t packet[1];
    coap_tcp_init_message(packet, PONG_7_03);
    coap_signal_set_custody(packet, 1);

    uint8_t buffer[OC_PDU_SIZE];
    size_t buffer_len = coap_serialize_message(packet, buffer);

    coap_packet_t parse_packet[1];
    coap_status_t ret = coap_tcp_parse_message(parse_packet, buffer, buffer_len);

    ASSERT_EQ(COAP_NO_ERROR, ret);
    ASSERT_EQ(packet->code, parse_packet->code);
    ASSERT_EQ(packet->custody, parse_packet->custody);
}

TEST_F(TestCoap, SignalSerializeParseTest_RELEASE)
{
    coap_packet_t packet[1];
    coap_tcp_init_message(packet, RELEASE_7_04);

    const char *addr = "coap+tcp://127.0.0.1:5683";
    size_t addr_len = strlen(addr) + 1;
    coap_signal_set_alt_addr(packet, addr, addr_len);
    uint32_t hold_off = 10;
    coap_signal_set_hold_off(packet, hold_off);

    uint8_t buffer[OC_PDU_SIZE];
    size_t buffer_len = coap_serialize_message(packet, buffer);

    coap_packet_t parse_packet[1];
    coap_status_t ret = coap_tcp_parse_message(parse_packet, buffer, buffer_len);

    ASSERT_EQ(COAP_NO_ERROR, ret);
    ASSERT_EQ(packet->code, parse_packet->code);
    ASSERT_STREQ(packet->alt_addr, parse_packet->alt_addr);
    ASSERT_EQ(packet->alt_addr_len, parse_packet->alt_addr_len);
    ASSERT_EQ(packet->hold_off, parse_packet->hold_off);
}

TEST_F(TestCoap, SignalSerializeParseTest_ABORT)
{
    coap_packet_t packet[1];
    coap_tcp_init_message(packet, ABORT_7_05);

    uint16_t bad_csm_opt = 10;
    coap_signal_set_bad_csm(packet, bad_csm_opt);

    const char * diagnostic = "BAD CSM OPTION";
    size_t diagnostic_len = strlen(diagnostic) + 1;
    coap_set_payload(packet, diagnostic, diagnostic_len);

    uint8_t buffer[OC_PDU_SIZE];
    size_t buffer_len = coap_serialize_message(packet, buffer);

    coap_packet_t parse_packet[1];
    coap_status_t ret = coap_tcp_parse_message(parse_packet, buffer, buffer_len);

    ASSERT_EQ(COAP_NO_ERROR, ret);
    ASSERT_EQ(packet->code, parse_packet->code);
    ASSERT_EQ(packet->bad_csm_opt, parse_packet->bad_csm_opt);
    ASSERT_STREQ(diagnostic, (char *)parse_packet->payload);
}

#endif /* OC_TCP */
