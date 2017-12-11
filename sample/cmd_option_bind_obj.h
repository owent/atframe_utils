#ifndef TEST_BINDOBJ_H
#define TEST_BINDOBJ_H


#pragma once

#include "cli/cmd_option.h"
#include <cstdio>
#include <iostream>


// 绑定函数对象例程
struct obj_bind0 {
    int m;
    obj_bind0(int _m) : m(_m) {}

    void operator()(util::cli::callback_param par) {
        printf("On Bind Obj Params:");
        int len = static_cast<int>(par.get_params_number());
        for (int i = 0; i < len; ++i) {
            printf(" %s", par[i]->to_string());
        }

        printf(" m => %d\n", m);
    }
};

struct obj_bind1 {
    int m;
    obj_bind1(int _m) : m(_m) {}

    void operator()(util::cli::callback_param par, int a) {
        printf("On Bind Obj1 Params:");
        int len = static_cast<int>(par.get_params_number());
        for (int i = 0; i < len; ++i) {
            printf(" %s", par[i]->to_string());
        }

        printf(" m => %d, a => %d\n", m, a);
    }
};

struct obj_bind2 {
    int m;
    obj_bind2(int _m) : m(_m) {}

    void operator()(util::cli::callback_param par, int a, double b) {
        printf("On Bind Obj2 Params:");
        int len = static_cast<int>(par.get_params_number());
        for (int i = 0; i < len; ++i) {
            printf(" %s", par[i]->to_string());
        }

        printf(" m => %d, a => %d, b => %lf\n", m, a, b);
    }
};

struct obj_bind3 {
    int m;
    obj_bind3(int _m) : m(_m) {}

    void operator()(util::cli::callback_param par, int a, double b, bool c) {
        printf("On Bind Obj3 Params:");
        int len = static_cast<int>(par.get_params_number());
        for (int i = 0; i < len; ++i) {
            printf(" %s", par[i]->to_string());
        }

        printf(" m => %d, a => %d, b => %lf, c => %d\n", m, a, b, c);
    }
};

void bind_obj_init() {
    puts("绑定函数对象");

    util::cli::cmd_option::ptr_type co = util::cli::cmd_option::create();
    co->bind_cmd("-obj, --object, --bind_object", obj_bind0(105));
    co->bind_cmd("-obj1, --object1", obj_bind1(205), 206);
    co->bind_cmd("-obj2, --object2", obj_bind2(305), 306, 30.6);
    co->bind_cmd("-obj3, --object3", obj_bind3(405), 406, 40.6, true);
    co->start("-obj p1 p2 p3 -obj1 test -obj2 -obj3");

    puts("绑定函数对象 测试完成\n");
}

#endif
