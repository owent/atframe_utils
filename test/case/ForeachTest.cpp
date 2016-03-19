#include <vector>
#include <map>

#include "frame/test_macros.h"
#include "std/foreach.h"

CASE_TEST(owent_foreach, Array)
{
	//数组
	const int arr[] = {1, 7, 3, 9, 5, 6, 2, 8, 4};
	int sum1 = 0, sum2 = 0;
	owent_foreach(const int& v, arr) {
		sum1 += v;
	}

	for (int i = 0; i < 9; ++ i) {
		sum2 += arr[i];
	}

    CASE_EXPECT_EQ(sum1, sum2);
}

CASE_TEST(owent_foreach, STL)
{
	//数组
	std::vector<int> vec;
	for (unsigned int i = 0; i < 128; ++ i)
	vec.push_back((i * i + i / 33));

	int sum1 = 0, sum2 = 0, index = 0;
	owent_foreach(int& v, vec) {
		v = index * 33 + index;
		sum1 += v;
		++ index;
	}

	for (unsigned int i = 0; i < vec.size(); ++ i) {
		sum2 += vec[i];
	}

    CASE_EXPECT_EQ(sum1, sum2);
	CASE_EXPECT_NE(sum1, 0);
}

CASE_TEST(owent_foreach, STL_Pair)
{
	//数组
	std::map<int, int> mp;
	for (unsigned int i = 0; i < 128; ++ i)
		mp[i] = i * i + i / 33;

	unsigned int count = 0;
	typedef std::map<int, int>::value_type map_pair; // 由于foreach是宏定义，所以类型里带逗号的话必须这么处理，否则编译器会认为这个逗号是参数分隔符
    owent_foreach(map_pair& pr, mp) {
		++ count;
        pr.second = 0;
    }

    CASE_EXPECT_EQ(mp.size(), count);
	CASE_EXPECT_EQ(mp[15], 0);
}

class SeedForeachRefCopyLimit
{
private:
    SeedForeachRefCopyLimit(const SeedForeachRefCopyLimit&);
    SeedForeachRefCopyLimit& operator=(const SeedForeachRefCopyLimit&);

public:
    int count;
    SeedForeachRefCopyLimit(): count(0){}
};

CASE_TEST(owent_foreach, RefCopyLimit)
{
    SeedForeachRefCopyLimit arr[10];
    int idx = 1, sum = 0;
    owent_foreach(SeedForeachRefCopyLimit& stNode, arr)
    {
        stNode.count = idx*= 2;
        sum += stNode.count;
    }

    CASE_EXPECT_EQ(sum, 2046);
}

CASE_TEST(owent_foreach, Continue)
{
    //数组
    const int arr[] = {1, 7, 3, 9, 5, 6, 2, 8, 4};
    int sum = 0;
    owent_foreach(const int& v, arr) {
        if (v == 3 || v == 8)
            continue;

        sum += v;
    }

    CASE_EXPECT_EQ(34, sum);
}


CASE_TEST(owent_foreach, Break)
{
    //数组
    const int arr[] = {1, 7, 3, 9, 5, 6, 2, 8, 4};
    int sum = 0;
    owent_foreach(const int& v, arr) {
        if (v == 5)
            break;

        sum += v;
    }

    CASE_EXPECT_EQ(20, sum);
}
