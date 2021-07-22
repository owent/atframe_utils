#ifndef TEST_DELAYBIND_H
#define TEST_DELAYBIND_H

#pragma once

#include <cstdio>
#include <iostream>
#include "cli/cmd_option.h"

// 延迟绑定初始化
bool delay_print(util::cli::callback_param par) {
  if (par.get_params_number() > 0) puts(par[0]->to_string());
  return false;
}

void delay_init(util::cli::callback_param par, util::cli::cmd_option *stChild) {
  printf("Delay Init Params Num: %d\n", static_cast<int>(par.get_params_number()));
  stChild->bind_cmd("-p, --print", delay_print);
}

void delay_bind() {
  puts("延迟初始化子绑定");
  util::cli::cmd_option::ptr_type f = util::cli::cmd_option::create();
  std::shared_ptr<util::cli::cmd_option> pc = util::cli::cmd_option::create();
  pc->bind_cmd("@OnCallFunc", delay_init, pc.get());  // 设置初始化函数
  // 注意这里不能传入*pc
  // 因为如果传入引用，子CmdOption结构会被复制，然后作为f的子命令，而传入并延迟绑定的是pc的指针
  f->bind_child_cmd("-c, --child", pc);

  f->start("-c a b c -p \"Delay Init Child Option With \\\"delay_print\\\".\"");

  puts("延迟初始化子绑定 测试完成\n");
}

#endif
