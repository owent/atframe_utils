#include <time.h>

#include "design_pattern/singleton.h"
#include "frame/test_macros.h"

class singleton_unit_test1 : public util::design_pattern::singleton<singleton_unit_test1> {
public:
    bool b;
    int i;

protected:
    singleton_unit_test1(): b(false), i(-1) {}
    ~singleton_unit_test1() {}
};

CASE_TEST(singleton_test, instance) {
    singleton_unit_test1 *pl = singleton_unit_test1::instance();
    singleton_unit_test1 &pr = singleton_unit_test1::get_instance();

    pl->b = true;
    pl->i = 1024;

    CASE_EXPECT_EQ(pl, &pr);
    CASE_EXPECT_EQ(true, pr.b);
    CASE_EXPECT_EQ(1024, pr.i);
}

class singleton_unit_test2 {
    UTIL_DESIGN_PATTERN_SINGLETON_VISIBLE_DECL(singleton_unit_test2)
public:
    bool b;
    int i;

    singleton_unit_test2(): b(false), i(0) {}
};
UTIL_SYMBOL_VISIBLE singleton_unit_test2::singleton_data_t singleton_unit_test2::singleton_wrapper_t::data;

CASE_TEST(singleton_unit_test2, instance) {
    singleton_unit_test2 *pl = singleton_unit_test2::instance();
    singleton_unit_test2 &pr = *singleton_unit_test2::me();

    pl->b = true;
    pl->i = 1024;

    CASE_EXPECT_EQ(pl, &pr);
    CASE_EXPECT_EQ(true, pr.b);
    CASE_EXPECT_EQ(1024, pr.i);
}

class singleton_unit_test3 : public util::design_pattern::local_singleton<singleton_unit_test3> {
public:
    bool b;
    int i;

protected:
    singleton_unit_test3(): b(false), i(-1) {}
    ~singleton_unit_test3() {}
};

CASE_TEST(singleton_test, instance) {
    singleton_unit_test3 *pl = singleton_unit_test3::instance();
    singleton_unit_test3 &pr = singleton_unit_test3::get_instance();

    pl->b = true;
    pl->i = 1024;

    CASE_EXPECT_EQ(pl, &pr);
    CASE_EXPECT_EQ(true, pr.b);
    CASE_EXPECT_EQ(1024, pr.i);
}