#include <algorithm>
#include <cstring>
#include <cstring>
#include <ctime>
#include <memory>
#include <sstream>


#include "frame/test_macros.h"
#include "std/smart_ptr.h"

struct intrusive_ptr_test_clazz {
    int ref_count;
    int *del_count;

    intrusive_ptr_test_clazz(int *dc) : ref_count(0), del_count(dc) {}
    ~intrusive_ptr_test_clazz() { ++(*del_count); }

    friend void intrusive_ptr_add_ref(intrusive_ptr_test_clazz *p) {
        CASE_EXPECT_NE(p, NULL);
        if (NULL != p) {
            ++p->ref_count;
        }
    }

    friend void intrusive_ptr_release(intrusive_ptr_test_clazz *p) {
        CASE_EXPECT_NE(p, NULL);
        if (NULL != p) {
            --p->ref_count;
            if (0 == p->ref_count) {
                delete p;
            }
        }
    }
};

CASE_TEST(smart_ptr, intrusive_ptr_int) {
    typedef std::intrusive_ptr<intrusive_ptr_test_clazz> ptr_t;
    int delete_count = 0;
    {
        ptr_t p = ptr_t(new intrusive_ptr_test_clazz(&delete_count));

        CASE_EXPECT_EQ(p->ref_count, 1);

        {
            ptr_t p2(p);
            CASE_EXPECT_EQ(p->ref_count, 2);
        }

        CASE_EXPECT_EQ(p->ref_count, 1);
#if defined(UTIL_CONFIG_COMPILER_CXX_RVALUE_REFERENCES) && UTIL_CONFIG_COMPILER_CXX_RVALUE_REFERENCES
        {
            ptr_t p2(p);
            CASE_EXPECT_EQ(p->ref_count, 2);

            ptr_t prv(std::move(p2));
            CASE_EXPECT_EQ(p->ref_count, 2);
            CASE_EXPECT_EQ(p2.get(), NULL);
            CASE_EXPECT_NE(prv.get(), NULL);
        }

        {
            ptr_t p2(p);
            CASE_EXPECT_EQ(p->ref_count, 2);

            ptr_t prv;
            prv = std::move(p2);
            CASE_EXPECT_EQ(p->ref_count, 2);
            CASE_EXPECT_EQ(p2.get(), NULL);
            CASE_EXPECT_NE(prv.get(), NULL);
        }
#endif

        {
            ptr_t p2(p);
            CASE_EXPECT_EQ(p->ref_count, 2);
            p2.reset(NULL);
            CASE_EXPECT_EQ(p->ref_count, 1);
            CASE_EXPECT_EQ(p2.get(), NULL);

            CASE_EXPECT_TRUE(p);
            CASE_EXPECT_FALSE(p2);
        }

        CASE_EXPECT_EQ(delete_count, 0);
    }

    CASE_EXPECT_EQ(delete_count, 1);
}