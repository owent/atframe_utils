#include <algorithm>
#include <cstring>
#include <ctime>
#include <sstream>

#include "frame/test_macros.h"
#include "string/ac_automation.h"

CASE_TEST(ac_automation, basic) {
    util::string::ac_automation<> actree;

    actree.insert_keyword("acd");
    actree.insert_keyword("aceb");
    actree.insert_keyword("bef");
    actree.insert_keyword("cef");
    actree.insert_keyword("ef");

    util::string::ac_automation<>::value_type res = actree.match("acefcabefefefcevfefbc");

    CASE_EXPECT_EQ(5, res.size());
    if (!res.empty()) {
        CASE_EXPECT_EQ(1, res[0].start);
        CASE_EXPECT_EQ(3, res[0].length);

        CASE_EXPECT_EQ(6, res[1].start);
        CASE_EXPECT_EQ(3, res[1].length);
        CASE_EXPECT_EQ('b', res[1].keyword->at(0));
    }
}

CASE_TEST(ac_automation, failed) {
    util::string::ac_automation<> actree;

    actree.insert_keyword("acd");
    actree.insert_keyword("aceb");
    actree.insert_keyword("bef");
    actree.insert_keyword("cef");
    actree.insert_keyword("ef");

    util::string::ac_automation<>::value_type res = actree.match("lolololnmmnmuiyt");

    CASE_EXPECT_EQ(0, res.size());
}

CASE_TEST(ac_automation, skip_space) {
    util::string::ac_automation<> actree;

    actree.insert_keyword("acd");
    actree.insert_keyword("aceb");
    actree.insert_keyword("bef");
    actree.insert_keyword("cef");
    actree.insert_keyword("ef");
    actree.set_skip(' ');

    util::string::ac_automation<>::value_type res = actree.match("ac  efca   b   e f efefcevfefbc");

    CASE_EXPECT_EQ(5, res.size());
    if (!res.empty()) {
        CASE_EXPECT_EQ(1, res[0].start);
        CASE_EXPECT_EQ(5, res[0].length);

        CASE_EXPECT_EQ(11, res[1].start);
        CASE_EXPECT_EQ(7, res[1].length);
        CASE_EXPECT_EQ('b', res[1].keyword->at(0));
    }
}

CASE_TEST(ac_automation, match_char) {
    util::string::ac_automation<> actree;

    actree.insert_keyword("a");

    util::string::ac_automation<>::value_type res = actree.match(" aaa");

    CASE_EXPECT_EQ(3, res.size());

    res = actree.match("a");
    CASE_EXPECT_EQ(1, res.size());

    res = actree.match("b");
    CASE_EXPECT_EQ(0, res.size());

    res = actree.match("");
    CASE_EXPECT_EQ(0, res.size());
}

CASE_TEST(ac_automation, prefix) {
    util::string::ac_automation<> actree;

    actree.insert_keyword("cb");
    actree.insert_keyword("abc");
    actree.insert_keyword("bc");
    actree.set_skip(' ');
    actree.set_skip('\n');

    util::string::ac_automation<>::value_type res = actree.match("ca b\ncd");

    CASE_EXPECT_EQ(1, res.size());
    if (!res.empty()) {
        CASE_EXPECT_EQ(1, res[0].start);
        CASE_EXPECT_EQ(5, res[0].length);
    }
}

CASE_TEST(ac_automation, skip) {
    util::string::ac_automation<> actree;

    actree.insert_keyword("艹");
    actree.insert_keyword("操你妈逼");
    actree.insert_keyword("你妈逼");
    actree.insert_keyword("艹你妈");
    actree.set_skip(' ');
    actree.set_skip('\t');
    actree.set_skip('\r');
    actree.set_skip('\n');

    std::string input = "小册老艹，我干死你操  你妈操  你妈\r\n逼艹 你妈";
    util::string::ac_automation<>::value_type res = actree.match(input);

// CI may not support this encoding
#ifndef _MSC_VER
    CASE_EXPECT_EQ(3, res.size());
#endif

    std::stringstream ss;
    size_t in_idx = 0;
    for (size_t i = 0; i < res.size(); ++i) {
        ss.write(&input[in_idx], res[i].start - in_idx);
        ss << "**";
        in_idx = res[i].start + res[i].length;
    }

    if (in_idx < input.size()) {
        ss.write(&input[in_idx], input.size() - in_idx);
    }

    CASE_MSG_INFO() << "filter resault: " << ss.str() << std::endl;
}
