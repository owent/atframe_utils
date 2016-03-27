#include <time.h>

#include "design_pattern/singleton.h"
#include "frame/test_macros.h"

class singleton_unit_test : public singleton<singleton_unit_test> {
public:
    bool b;
    int i;
};

CASE_TEST(singleton_test, instance) {
    singleton_unit_test *pl = singleton_unit_test::instance();
    singleton_unit_test &pr = singleton_unit_test::get_instance();

    pl->b = true;
    pl->i = 1024;

    CASE_EXPECT_EQ(pl, &pr);
    CASE_EXPECT_EQ(true, pr.b);
    CASE_EXPECT_EQ(1024, pr.i);
}
