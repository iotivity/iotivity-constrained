#include <gtest/gtest.h>
#include <cstdlib>

extern "C"{
    #include "st_manager.h"
    #include "st_process.h"
    #include "st_port.h"
    void st_manager_quit(void);
}

#ifdef OC_SECURITY
static bool otm_confirm_handler_test(void){}
#endif
static void st_status_handler_test(st_status_t status)
{
    (void)status;
}
static
void *st_manager_func(void *data)
{
    (void)data;
    int ret = st_manager_start();
    EXPECT_EQ(0, ret);

    return NULL;
}

class TestSTManager: public testing::Test
{
    protected:
        virtual void SetUp()
        {

        }

        virtual void TearDown()
        {

        }
};

TEST_F(TestSTManager, st_manager_initialize)
{
    int ret = st_manager_initialize();
    EXPECT_EQ(0, ret);
    st_manager_deinitialize();
}

TEST_F(TestSTManager, st_manager_start)
{
    int ret = st_manager_initialize();
    EXPECT_EQ(0, ret);

    st_thread_t t = st_thread_create(st_manager_func, "TEST", 0, NULL);
    st_sleep(1);
    st_manager_quit();
    st_thread_destroy(t);
    st_manager_stop();
    st_manager_deinitialize();
}

TEST_F(TestSTManager, st_manager_reset)
{
    int ret = st_manager_initialize();
    EXPECT_EQ(0, ret);

    st_thread_t t = st_thread_create(st_manager_func, "TEST", 0, NULL);
    st_sleep(1);
    st_manager_reset();
    st_sleep(1);
    st_manager_quit();
    st_thread_destroy(t);
    st_manager_stop();
    st_manager_deinitialize();
}

#ifdef OC_SECURITY
TEST_F(TestSTManager, st_register_otm_confirm_handler)
{
    bool ret = st_register_otm_confirm_handler(otm_confirm_handler_test);
    EXPECT_EQ(true, ret);
    st_unregister_otm_confirm_handler();
}
#endif

TEST_F(TestSTManager, st_register_status_handler)
{
    bool ret = st_register_status_handler(st_status_handler_test);
    EXPECT_EQ(true, ret);
    st_unregister_status_handler();
}