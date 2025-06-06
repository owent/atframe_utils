
#include <algorithm>
#include <cstdio>
#include <functional>
#include <iostream>

#include "cli/cmd_option.h"

// 高级用法示例源码
// 延迟子绑定
#include "cmd_option_delay_bind.h"
// 绑定仿函数
#include "cmd_option_bind_obj.h"
// 命令忽略大小写
#include "cmd_option_caseignore_bind.h"

class foo;
foo *g_test = nullptr;

class foo {
 public:
  void print_t(ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::callback_param, std::string &str) {
    printf("sizeof(float) => %d\n", static_cast<int>(sizeof(float)));
    printf("sizeof(double) => %d\n", static_cast<int>(sizeof(double)));
    printf("sizeof(long double) => %d\n", static_cast<int>(sizeof(long double)));

    str = "Hello World!";
  }

  void print(ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::callback_param par) {
    puts("bt func:");
    int len = static_cast<int>(par.get_params_number());
    for (int i = 0; i < len; ++i) {
      puts(par[i]->to_string());
    }

    g_test = this;
  }

  void print_t2(ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::callback_param par, int i) {
    printf("Mem Fun B2 Params Num: %d, i => %d\n", static_cast<int>(par.get_params_number()), i);
    std::cout << "Mem Fun B2 Params[0] as bool:" << par[0]->to_bool() << "\n";
    std::cout << "Mem Fun B2 Params[0] as char:" << par[0]->to_char() << "\n";
    std::cout << "Mem Fun B2 Params[0] as short:" << par[0]->to_short() << "\n";
    std::cout << "Mem Fun B2 Params[0] as int:" << par[0]->to_int() << "\n";
    std::cout << "Mem Fun B2 Params[0] as long:" << par[0]->to_long() << "\n";
    std::cout << "Mem Fun B2 Params[0] as longlong:" << par[0]->to_longlong() << "\n";
    std::cout << "Mem Fun B2 Params[0] as double:" << par[0]->to_double() << "\n";
    std::cout << "Mem Fun B2 Params[0] as float:" << par[0]->to_float() << "\n";
    std::cout << "Mem Fun B2 Params[0] as logic_bool:" << par[0]->to_logic_bool() << "\n";
    std::cout << "Mem Fun B2 Params[0] as uchar:" << par[0]->to_uchar() << "\n";
    std::cout << "Mem Fun B2 Params[0] as ushort:" << par[0]->to_ushort() << "\n";
    std::cout << "Mem Fun B2 Params[0] as uint:" << par[0]->to_uint() << "\n";
    std::cout << "Mem Fun B2 Params[0] as ulong:" << par[0]->to_ulong() << "\n";
    std::cout << "Mem Fun B2 Params[0] as ulonglong:" << par[0]->to_ulonglong() << "\n";
    std::cout << "Mem Fun B2 Params[0] as int8:" << par[0]->to_int8() << "\n";
    std::cout << "Mem Fun B2 Params[0] as int16:" << par[0]->to_int16() << "\n";
    std::cout << "Mem Fun B2 Params[0] as int32:" << par[0]->to_int32() << "\n";
    std::cout << "Mem Fun B2 Params[0] as int64:" << par[0]->to_int64() << "\n";
    std::cout << "Mem Fun B2 Params[0] as uint8:" << par[0]->to_uint8() << "\n";
    std::cout << "Mem Fun B2 Params[0] as uint16:" << par[0]->to_uint16() << "\n";
    std::cout << "Mem Fun B2 Params[0] as uint32:" << par[0]->to_uint32() << "\n";
    std::cout << "Mem Fun B2 Params[0] as uint64:" << par[0]->to_uint64() << "\n";
  }

  void print_t3(ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::callback_param par, int i, double d) {
    printf("Mem Fun B3 Params Num: %d, i => %d, d => %lf\n", static_cast<int>(par.get_params_number()), i, d);
    for (size_t idx = 0; idx < par.get_params_number(); ++idx) {
      std::cout << "Mem Fun B3 Params[" << idx << "] as logic_bool:" << par[idx]->to_logic_bool()
                << ", origin value:" << par[idx]->to_cpp_string() << "\n";
    }
  }

  virtual ~foo() {}
};

class foo2 : public foo {
  void print(ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::callback_param, std::string &str) { str = "Hello World! - child"; }
};

void print(ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::callback_param par, std::string *str) {
  printf("sizeof(short) => %d\n", static_cast<int>(sizeof(short)));
  printf("sizeof(int) => %d\n", static_cast<int>(sizeof(int)));
  printf("sizeof(long) => %d\n", static_cast<int>(sizeof(long)));

  (*str) = par.get("par1")->to_string();
}

void print(ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::callback_param) { puts("do nothing! - free func without parameter\n"); }

void print2(ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::callback_param par, double d) {
  printf("Free Fun B2 Params Num: %d, d => %lf\n", static_cast<int>(par.get_params_number()), d);
}

void print3(ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::callback_param par, double d, int i) {
  printf("Free Fun B3 Params Num: %d, i => %d, d => %lf\n", static_cast<int>(par.get_params_number()), i, d);
}

static int complex_bind_func(ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::callback_param, int i) {
  printf("%d\n", i);
  return 0;
}

static void on_error(ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::callback_param params) {
  puts("Error:");
  std::stringstream args;
  for (ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::cmd_option_list::cmd_array_type::const_iterator iter =
           params.get_cmd_array().begin();
       iter != params.get_cmd_array().end(); ++iter) {
    args << " \"" << iter->first << '"';
  }
  for (size_t i = 0; i < params.get_params_number(); ++i) {
    if (params[i]) {
      args << " \"" << params[i]->to_cpp_string() << '"';
    }
  }

  ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::cmd_option_list::value_type err_msg = params.get("@ErrorMsg");
  printf("Error Command: %s, Error Message: %s\n", args.str().c_str(), err_msg ? err_msg->to_string() : "");
}

int cmd_option_sample_main() {
  ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::cmd_option::ptr_type co =
      ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::cmd_option::create();
  ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::cmd_option::ptr_type cco =
      ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::cmd_option::create();

  foo f, *pf = new foo2();
  std::string str;

  // 绑定错误处理函数
  co->bind_cmd("@OnError", on_error);

  // 带参数类函数绑定（参数为引用）
  co->bind_cmd("-bt1, --bind_class_func_param", &foo::print_t, pf, std::ref(str))
      ->set_help_msg("-bt1, --bind_class_func_param    带参数类绑定");
  // 带参数类函数绑定（多个参数）
  co->bind_cmd("-bt2, --bind_class_func_param2", &foo::print_t2, pf, 1011);
  co->bind_cmd("-bt3, --bind_class_func_param3", &foo::print_t3, pf, 1013, 10.13);
  // 无参数类函数绑定
  co->bind_cmd("-bt, --bind_class_func", &foo::print, std::ref(f))
      ->set_help_msg("-bt, --bind_class_func    无参数类绑定");
  // 带参数普通函数绑定(自动类型推断)
  cco->bind_cmd("-bf1, --bind_func_param1",
                (void (*)(ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::callback_param, std::string *))print, &str)
      ->set_help_msg("-bf1, --bind_func_param    带参数函数绑定");

  cco->bind_cmd("-bf2, --bind_func_param2", print2, 20.11);
  cco->bind_cmd("-bf3, --bind_func_param3", print3, 20.11, 2013);

  // 无参数普通函数绑定
  cco->bind_cmd("-bf, --bind_func", (void (*)(ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::callback_param))print)
      ->set_help_msg("-bf, --bind_func    无参数函数绑定");
  // 复杂多指令绑定
  co->bind_cmd("wo  ;le  , ge, cha,de", complex_bind_func, 100);
  // 绑定子绑定对象
  co->bind_child_cmd("-c, --child", cco)->set_help_msg("-c, --child    ");
  // 绑定多指令的默认执行函数
  co->bind_cmd("@OnDefault", (void (*)(ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::callback_param))print);

  // 单指令启动测试
  co->start("-c lala def -bf1 par1=par2 par3 -bf2 -bf3 fp1");
  printf("-bf1 has set str=%s with parameter par1=par2\n", str.c_str());

  co->start(
      "-bt btpar1 \"test end of line\\r\\n\tanother line\"-bt1 with one param --bind_class_func_param2 33 p1 p2 p3 "
      "-bt3 yes no true false 1 0"
      "p4");
  printf("成员函数绑定传入对象引用包装测试: %s\n", (g_test == &f) ? "通过" : "失败！！！！！！！！！！！");

  // 多指令启动测试
  const char *strCmds[] = {"path", "par1=par2", "par2", "wo", "le", "ge", "cha"};
  co->start(7, strCmds);
  puts(str.c_str());

  // 错误处理测试
  co->start("do_nothing");
  puts("绑定测试完毕\n");

  // 添加帮助信息测试
  co->get_binded_cmd("wo")->add_help_msg("wo, le, ge, cha, de")->add_help_msg("   复杂指令绑定");
  std::shared_ptr<std::vector<const char *> > cmds = co->get_cmd_names();
  printf("CMDS: ");
  for (unsigned int i = 0; i < cmds->size(); ++i) {
    printf(" %s;", (*cmds)[i]);
  }
  puts("");
  co->bind_help_cmd("h, help")->set_help_msg("h, help      帮助信息");
  co->start("help");
  puts("默认帮助函数测试完成\n");

  // 高级用法 例程函数

  // 延迟绑定初始化
  delay_bind();

  // 绑定函数对象
  bind_obj_init();

  // 命令忽略大小写
  bind_ci_cmd_init();

  // 回收资源
  delete pf;

  return 0;
}
