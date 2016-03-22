#include <cstring>

#include "frame/test_macros.h"

#ifdef max
#undef max
#endif

#include "mem_pool/lru_object_pool.h"


struct test_lru_data {};
static int g_stat_lru[4] = {0, 0, 0, 0};

struct test_lru_action : public util::mempool::lru_default_action<test_lru_data> {
    typedef util::mempool::lru_default_action<test_lru_data> base_type;
    void push(test_lru_data *obj) { ++g_stat_lru[0]; }

    void pull(test_lru_data *obj) { ++g_stat_lru[1]; }

    void reset(test_lru_data *obj) { ++g_stat_lru[2]; }

    void gc(test_lru_data *obj) {
        ++g_stat_lru[3];
        base_type::gc(obj);
    }
};

CASE_TEST(lru_object_pool_test, basic) {
    {
        typedef util::mempool::lru_pool<uint32_t, test_lru_data, test_lru_action> test_lru_pool_t;
        util::mempool::lru_pool_manager::ptr_t mgr = util::mempool::lru_pool_manager::create();
        test_lru_pool_t lru;
        test_lru_data *check_ptr;
        lru.init(mgr);

        memset(&g_stat_lru, 0, sizeof(g_stat_lru));

        CASE_EXPECT_TRUE(lru.push(123, new test_lru_data()));
        CASE_EXPECT_TRUE(lru.push(456, check_ptr = new test_lru_data()));
        CASE_EXPECT_TRUE(lru.push(123, new test_lru_data()));

        CASE_EXPECT_EQ(3, g_stat_lru[0]);
        CASE_EXPECT_EQ(0, g_stat_lru[1]);
        CASE_EXPECT_EQ(0, g_stat_lru[2]);
        CASE_EXPECT_EQ(0, g_stat_lru[3]);

        CASE_EXPECT_EQ(3, mgr->item_count().get());
        CASE_EXPECT_EQ(3, mgr->list_count().get());

        CASE_EXPECT_EQ(NULL, lru.pull(789));
        CASE_EXPECT_EQ(check_ptr, lru.pull(456));
        CASE_EXPECT_EQ(NULL, lru.pull(456));
        delete check_ptr;
        check_ptr = NULL;

        CASE_EXPECT_EQ(3, g_stat_lru[0]);
        CASE_EXPECT_EQ(1, g_stat_lru[1]);
        CASE_EXPECT_EQ(1, g_stat_lru[2]);
        CASE_EXPECT_EQ(0, g_stat_lru[3]);

        CASE_EXPECT_EQ(1, mgr->gc());

        CASE_EXPECT_EQ(3, g_stat_lru[0]);
        CASE_EXPECT_EQ(1, g_stat_lru[1]);
        CASE_EXPECT_EQ(1, g_stat_lru[2]);
        CASE_EXPECT_EQ(1, g_stat_lru[3]);
    }

    CASE_EXPECT_EQ(3, g_stat_lru[0]);
    CASE_EXPECT_EQ(1, g_stat_lru[1]);
    CASE_EXPECT_EQ(1, g_stat_lru[2]);
    CASE_EXPECT_EQ(2, g_stat_lru[3]);
}

CASE_TEST(lru_object_pool_test, inner_gc_proc) {
    typedef util::mempool::lru_pool<uint32_t, test_lru_data, test_lru_action> test_lru_pool_t;
    util::mempool::lru_pool_manager::ptr_t mgr = util::mempool::lru_pool_manager::create();
    test_lru_pool_t lru;
    lru.init(mgr);
    mgr->set_proc_item_count(16);
    mgr->set_proc_list_count(16);

    mgr->set_item_max_bound(32);

    memset(&g_stat_lru, 0, sizeof(g_stat_lru));
    test_lru_data *checked_ptr[32];

    for (int i = 0; i < 32; ++i) {
        CASE_EXPECT_TRUE(lru.push(123, checked_ptr[i] = new test_lru_data()));
    }

    for (int i = 0; i < 24; ++i) {
        CASE_EXPECT_EQ(checked_ptr[31 - i], lru.pull(123));
    }

    for (int i = 0; i < 24; ++i) {
        CASE_EXPECT_TRUE(lru.push(123, checked_ptr[i + 8]));
    }

    CASE_EXPECT_EQ(56, g_stat_lru[0]);
    CASE_EXPECT_EQ(24, g_stat_lru[1]);
    CASE_EXPECT_EQ(24, g_stat_lru[2]);
    CASE_EXPECT_EQ(0, g_stat_lru[3]);

    CASE_EXPECT_TRUE(lru.push(123, new test_lru_data()));
    CASE_EXPECT_EQ(1, g_stat_lru[3]);
    mgr->set_item_max_bound(8);
    CASE_EXPECT_TRUE(lru.push(123, new test_lru_data()));
    CASE_EXPECT_EQ(8, g_stat_lru[3]);
    CASE_EXPECT_EQ(41, mgr->list_count().get());
    CASE_EXPECT_EQ(26, mgr->item_count().get());

    CASE_EXPECT_EQ(1, mgr->proc(1));
    CASE_EXPECT_EQ(25, mgr->list_count().get());
    CASE_EXPECT_EQ(25, mgr->item_count().get());

    CASE_EXPECT_EQ(16, mgr->proc(2));
    CASE_EXPECT_EQ(9, mgr->list_count().get());
    CASE_EXPECT_EQ(9, mgr->item_count().get());

    CASE_EXPECT_EQ(1, mgr->proc(3));
    CASE_EXPECT_EQ(8, mgr->list_count().get());
    CASE_EXPECT_EQ(8, mgr->item_count().get());

    CASE_EXPECT_EQ(0, mgr->proc(4));
    CASE_EXPECT_EQ(8, mgr->list_count().get());
    CASE_EXPECT_EQ(8, mgr->item_count().get());
}


CASE_TEST(lru_object_pool_test, timeout) {
    typedef util::mempool::lru_pool<uint32_t, test_lru_data, test_lru_action> test_lru_pool_t;
    util::mempool::lru_pool_manager::ptr_t mgr = util::mempool::lru_pool_manager::create();
    test_lru_pool_t lru;
    lru.init(mgr);
    mgr->set_proc_item_count(16);
    mgr->set_proc_list_count(16);

    mgr->set_item_max_bound(32);
    mgr->set_list_tick_timeout(60);

    memset(&g_stat_lru, 0, sizeof(g_stat_lru));
    test_lru_data *checked_ptr[32];

    mgr->proc(1);
    for (int i = 0; i < 24; ++i) {
        CASE_EXPECT_TRUE(lru.push(123, checked_ptr[i] = new test_lru_data()));
    }
    mgr->proc(5);

    for (int i = 0; i < 8; ++i) {
        CASE_EXPECT_TRUE(lru.push(123, checked_ptr[i + 16] = new test_lru_data()));
    }

    mgr->proc(10);
    CASE_EXPECT_EQ(32, g_stat_lru[0]);
    CASE_EXPECT_EQ(0, g_stat_lru[1]);
    CASE_EXPECT_EQ(0, g_stat_lru[2]);
    CASE_EXPECT_EQ(0, g_stat_lru[3]);

    mgr->proc(61);
    CASE_EXPECT_EQ(32, g_stat_lru[0]);
    CASE_EXPECT_EQ(0, g_stat_lru[1]);
    CASE_EXPECT_EQ(0, g_stat_lru[2]);
    CASE_EXPECT_EQ(0, g_stat_lru[3]);
    CASE_EXPECT_EQ(32, mgr->list_count().get());
    CASE_EXPECT_EQ(32, mgr->item_count().get());

    mgr->proc(62);
    CASE_EXPECT_EQ(32, g_stat_lru[0]);
    CASE_EXPECT_EQ(0, g_stat_lru[1]);
    CASE_EXPECT_EQ(0, g_stat_lru[2]);
    CASE_EXPECT_EQ(16, g_stat_lru[3]);
    CASE_EXPECT_EQ(16, mgr->list_count().get());
    CASE_EXPECT_EQ(16, mgr->item_count().get());

    mgr->proc(62);
    CASE_EXPECT_EQ(32, g_stat_lru[0]);
    CASE_EXPECT_EQ(0, g_stat_lru[1]);
    CASE_EXPECT_EQ(0, g_stat_lru[2]);
    CASE_EXPECT_EQ(24, g_stat_lru[3]);
    CASE_EXPECT_EQ(8, mgr->list_count().get());
    CASE_EXPECT_EQ(8, mgr->item_count().get());
}

struct test_lru_action_donothing : public util::mempool::lru_default_action<test_lru_action_donothing> {
    typedef util::mempool::lru_default_action<test_lru_action_donothing> base_type;
    void push(void *obj) {}
    void pull(void *obj) {}
    void reset(void *obj) {}
    void gc(void *obj) {}
};

CASE_TEST(lru_object_pool_test, adjust_bound) {
    // manual gc and make cache smaller
    {
        util::mempool::lru_pool_manager::ptr_t mgr = util::mempool::lru_pool_manager::create();
        mgr->set_proc_item_count(16);
        mgr->set_proc_list_count(16);

        mgr->set_item_max_bound(32);
        mgr->set_list_tick_timeout(60);
        mgr->set_item_adjust_min(mgr->get_item_max_bound() / 4);
        mgr->set_list_adjust_min(mgr->get_list_bound() / 4);
        size_t imb = mgr->get_item_max_bound();
        size_t lmb = mgr->get_list_bound();

        CASE_EXPECT_NE(imb, mgr->get_item_adjust_min());
        CASE_EXPECT_NE(lmb, mgr->get_list_adjust_min());

        for (int i = 0; i < 1024; ++i) {
            mgr->gc();
        }

        CASE_EXPECT_NE(imb, mgr->get_item_max_bound());
        CASE_EXPECT_NE(lmb, mgr->get_list_bound());

        CASE_EXPECT_EQ(mgr->get_item_max_bound(), mgr->get_item_adjust_min() + 1);
        CASE_EXPECT_EQ(mgr->get_list_bound(), mgr->get_list_adjust_min());

        CASE_EXPECT_NE(0, mgr->get_item_max_bound());
        CASE_EXPECT_NE(0, mgr->get_list_bound());
    }

    // push and make cache larger
    {
        typedef util::mempool::lru_pool<uint32_t, void, test_lru_action_donothing> test_lru_pool_t;
        util::mempool::lru_pool_manager::ptr_t mgr = util::mempool::lru_pool_manager::create();
        test_lru_pool_t lru;
        lru.init(mgr);

        mgr->set_item_max_bound(32);
        mgr->set_item_adjust_max(64);
        mgr->set_list_bound(64);
        mgr->set_list_adjust_max(128);

        for (int i = 0; i < 128; ++i) {
            lru.push(0, &mgr);
        }

        CASE_EXPECT_EQ(64, mgr->get_item_max_bound());

        mgr->set_item_max_bound(256);
        mgr->set_item_adjust_max(256);
        for (int i = 0; i < 128; ++i) {
            lru.push(0, &mgr);
        }
        CASE_EXPECT_EQ(128, mgr->get_list_bound());
        CASE_EXPECT_EQ(128, mgr->item_count().get());
        CASE_EXPECT_EQ(128, mgr->list_count().get());
    }
}
