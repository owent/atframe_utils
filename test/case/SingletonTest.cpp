#include <time.h>

#include "frame/test_macros.h"
#include "DesignPattern/Singleton.h"

class SingletonUnitTest : public Singleton<SingletonUnitTest>
{
public:
    bool b; 
    int i; 
};

CASE_TEST(SingletonTest, Instance) 
{
	SingletonUnitTest* pl = SingletonUnitTest::Instance();
    SingletonUnitTest& pr = SingletonUnitTest::GetInstance();

    pl->b = true;
    pl->i = 1024;

    CASE_EXPECT_EQ(pl, &pr);
    CASE_EXPECT_EQ(true, pr.b);
    CASE_EXPECT_EQ(1024, pr.i);
}

