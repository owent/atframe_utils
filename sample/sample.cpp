
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <type_traits>
#include <vector>

#include "log/log_sink_file_backend.h"
#include "log/log_stacktrace.h"
#include "log/log_wrapper.h"
#include "std/thread.h"

#include "common/file_system.h"
#include "time/time_utility.h"

#include "algorithm/hash.h"
#include "random/random_generator.h"
#include "string/tquerystring.h"

#include "config/ini_loader.h"

#if defined(LIBATFRAME_UTILS_ENABLE_EXCEPTION) && LIBATFRAME_UTILS_ENABLE_EXCEPTION
#  include <stdexcept>
#endif

#define CALL_SAMPLE(x)                                                                        \
  std::cout << std::endl << "=============== begin " << #x << " ==============" << std::endl; \
  x();                                                                                        \
  std::cout << "===============  end " << #x << "  ==============" << std::endl

std::string g_exec_dir;

//=======================================================================================================

#if defined(LOG_WRAPPER_ENABLE_FWAPI) && LOG_WRAPPER_ENABLE_FWAPI

enum test_auto_enum_conversation_for_log_formatter {
  EN_TAECFLF_NONE = 0,
};

struct test_exception_for_log_formatter {
  bool runtime_error;
  bool throw_exception;
  explicit inline test_exception_for_log_formatter(bool a, bool b) : runtime_error(a), throw_exception(b){};
};

namespace LOG_WRAPPER_FWAPI_NAMESPACE_ID {
template <class CharT>
struct formatter<test_auto_enum_conversation_for_log_formatter, CharT> : formatter<CharT *, CharT> {
  template <class FormatContext>
  auto format(const test_auto_enum_conversation_for_log_formatter &obj, FormatContext &ctx) {
    auto ret = ctx.out();
    switch (obj) {
      case EN_TAECFLF_NONE:
        *(ret++) = 'Y';
        break;
      default:
        *(ret++) = 'N';
        break;
    }
    return ret;
  }
};

template <class CharT>
struct formatter<test_exception_for_log_formatter, CharT> : formatter<CharT *, CharT> {
  template <class FormatContext>
  auto format(const test_exception_for_log_formatter &obj, FormatContext &ctx) {
#  if defined(LIBATFRAME_UTILS_ENABLE_EXCEPTION) && LIBATFRAME_UTILS_ENABLE_EXCEPTION
    if (obj.throw_exception) {
      if (obj.runtime_error) {
        throw std::runtime_error("runtime_error");
      } else {
        throw std::logic_error("runtime_error");
      }
    }
#  endif

    auto ret = ctx.out();
    if (obj.runtime_error && obj.throw_exception) {
      *(ret++) = '4';
    } else if (obj.runtime_error) {
      *(ret++) = '3';
    } else if (obj.throw_exception) {
      *(ret++) = '2';
    } else {
      *(ret++) = '1';
    }
    return ret;
  }
};
}  // namespace LOG_WRAPPER_FWAPI_NAMESPACE_ID

struct test_custom_object_for_log_formatter {
  int32_t x;
  std::string y;
};

namespace LOG_WRAPPER_FWAPI_NAMESPACE_ID {
template <class CharT>
struct formatter<test_custom_object_for_log_formatter, CharT> : formatter<CharT *, CharT> {
  template <class FormatContext>
  auto format(const test_custom_object_for_log_formatter &obj, FormatContext &ctx) {
    test_exception_for_log_formatter x(false, false);
    std::cout << util::log::vformat("{}, {}", LOG_WRAPPER_FWAPI_MAKE_FORMAT_ARGS(obj.x, obj.y)) << std::endl;
    return util::log::vformat_to(ctx.out(), "({}, {}, {})", util::log::make_format_args(obj.x, obj.y, x));
  }
};

}  // namespace LOG_WRAPPER_FWAPI_NAMESPACE_ID

#endif

void log_sample_func1(int times) {
  if (times > 0) {
    log_sample_func1(times - 1);
    return;
  }

  if (util::log::is_stacktrace_enabled()) {
    std::cout << "----------------test stacktrace begin--------------" << std::endl;
    char buffer[2048] = {0};
    util::log::stacktrace_write(buffer, sizeof(buffer));
    std::cout << buffer << std::endl;
    std::cout << "----------------test stacktrace end--------------" << std::endl;
  }

  WLOG_INIT(util::log::log_wrapper::categorize_t::DEFAULT, util::log::log_wrapper::level_t::LOG_LW_DEBUG);
  WLOG_GETCAT(util::log::log_wrapper::categorize_t::DEFAULT)
      ->set_stacktrace_level(util::log::log_wrapper::level_t::LOG_LW_INFO);

  WLOG_GETCAT(util::log::log_wrapper::categorize_t::DEFAULT)->clear_sinks();

  std::cout << "----------------setup log_wrapper done--------------" << std::endl;

  PSTDERROR("try to print error log.\n");
  PSTDOK("try to print ok log.\n");

  std::cout << "----------------sample for PSTDOK/PSTDERROR done--------------" << std::endl;

  WLOGNOTICE("notice log %d", 0);

  util::log::log_sink_file_backend filed_backend;
  filed_backend.set_max_file_size(256);
  filed_backend.set_rotate_size(3);
  filed_backend.set_file_pattern("%Y-%m-%d/%S/%N.log");
  filed_backend.set_writing_alias_pattern("%Y-%m-%d/%S/current.log");

  WLOG_GETCAT(util::log::log_wrapper::categorize_t::DEFAULT)->add_sink(filed_backend);
  std::cout << "----------------setup file system log sink done--------------" << std::endl;

  for (int i = 0; i < 16; ++i) {
    WLOGDEBUG("first dir test log: %d", i);
  }

#if defined(LOG_WRAPPER_ENABLE_FWAPI) && LOG_WRAPPER_ENABLE_FWAPI
  test_custom_object_for_log_formatter custom_obj;
  for (int32_t i = 0; i < 16; ++i) {
    custom_obj.x = i * i;
    custom_obj.y += "H";
    std::string origin_fmt_str = LOG_WRAPPER_FWAPI_NAMESPACE format("{}", custom_obj);

    std::string fmt_str = LOG_WRAPPER_FWAPI_FORMAT("{}", custom_obj);
    char buffer1[256] = {0};
    LOG_WRAPPER_FWAPI_FORMAT_TO(buffer1, "{}", custom_obj);
    std::string fmt_to_str = buffer1;
    char buffer2[16] = {0};
    LOG_WRAPPER_FWAPI_FORMAT_TO_N(buffer2, 10, "{}", custom_obj);
    std::string fmt_to_n_str = buffer2;
    FWLOGDEBUG(
        "test log formatter(std::format/fmt::format) - {}\n\torigin format: {}\n\tformat: {}\n\tformat_to: "
        "{}\n\tformat_to_n: {}",
        i, origin_fmt_str, fmt_str, fmt_to_str, fmt_to_n_str);
  }
#endif

  THREAD_SLEEP_MS(1000);
  util::time::time_utility::update();

  for (int i = 0; i < 16; ++i) {
    WLOGDEBUG("second dir log: %d", i);
  }

  unsigned long long ull_test_in_mingw = 64;
  WLOGINFO("%llu", ull_test_in_mingw);

  WLOG_GETCAT(util::log::log_wrapper::categorize_t::DEFAULT)
      ->set_sink(WLOG_GETCAT(util::log::log_wrapper::categorize_t::DEFAULT)->sink_size() - 1,
                 util::log::log_wrapper::level_t::LOG_LW_DEBUG, util::log::log_wrapper::level_t::LOG_LW_DEBUG);
  WLOGDEBUG("Debug still available %llu", ull_test_in_mingw);
  WLOGINFO("Info not available now %llu", ull_test_in_mingw);

  std::cout << "log are located at " << util::file_system::get_cwd().c_str() << std::endl;

  WLOG_GETCAT(util::log::log_wrapper::categorize_t::DEFAULT)->pop_sink();
  WLOGERROR("No log sink now");
}

class log_sample_functor2 {
 public:
  void func2(int times) {
    if (times & 0x01) {
      func2(times - 1);
    } else {
      log_sample_func1(times - 1);
    }
  }
};

class log_sample_functor3 {
 public:
  static void func3(int times) {
    if (times & 0x01) {
      func3(times - 1);
    } else {
      log_sample_functor2 f;
      f.func2(times - 1);
    }
  }
};

struct log_sample_functor4 {
  void operator()(int times) {
    if (times & 0x01) {
      (*this)(times - 1);
    } else {
      log_sample_functor3::func3(times - 1);
    }
  }
};

static void log_sample_func5(int times) {
  if (times & 0x01) {
    log_sample_func5(times - 1);
  } else {
    log_sample_functor4 f;
    f(times - 1);
  }
}

#if defined(LOG_WRAPPER_ENABLE_FWAPI) && LOG_WRAPPER_ENABLE_FWAPI
void log_sample_func6_stdout(const util::log::log_wrapper::caller_info_t &, const char *content, size_t content_size) {
  std::cout.write(content, content_size);
  std::cout << std::endl;
}

void log_sample_func6() {
  WLOG_INIT(util::log::log_wrapper::categorize_t::DEFAULT, util::log::log_wrapper::level_t::LOG_LW_DEBUG);
  WLOG_GETCAT(util::log::log_wrapper::categorize_t::DEFAULT)->clear_sinks();
  WLOG_GETCAT(util::log::log_wrapper::categorize_t::DEFAULT)->add_sink(log_sample_func6_stdout);
  std::cout << "---------------- setup log sink for std::format done--------------" << std::endl;
#  if defined(LOG_WRAPPER_ENABLE_FWAPI) && LOG_WRAPPER_ENABLE_FWAPI
  FWLOGINFO("{} {}: {} {}", "Hello", std::string("World"), 42,
            test_auto_enum_conversation_for_log_formatter::EN_TAECFLF_NONE);
#    if defined(_MSC_VER)
  FWLOGINFO("{:d}", "foo");  // This will cause compile error when using fmtlib and gcc/clang
#    endif
#  endif
  WLOG_GETCAT(util::log::log_wrapper::categorize_t::DEFAULT)->pop_sink();
  FWLOGERROR("No log sink now");
}

#  if defined(LIBATFRAME_UTILS_ENABLE_EXCEPTION) && LIBATFRAME_UTILS_ENABLE_EXCEPTION
void log_sample_func7() {
  // Do not throw exception
  test_custom_object_for_log_formatter custom_obj;
  custom_obj.x = 123;
  custom_obj.y = __FUNCTION__;
  std::cout << "---------------- catch exception of log format APIs --------------" << std::endl;
  std::cout << "format: " << util::log::format("{},{},{},{}", 1, 2, 3) << std::endl;
  std::cout << "format: " << util::log::format("{},{}", 1, test_exception_for_log_formatter(true, true)) << std::endl;
  std::cout << "format: " << util::log::format("{},{}", 1, test_exception_for_log_formatter(false, true)) << std::endl;

  char string_buffer[256] = {0};
  util::log::format_to(string_buffer, "{},{},{},{}", 1, 2, 3);
  std::cout << "format_to: " << string_buffer << std::endl;
  memset(string_buffer, 0, sizeof(string_buffer));
  util::log::format_to(string_buffer, "{},{}", 1, test_exception_for_log_formatter(true, true));
  std::cout << "format_to: " << string_buffer << std::endl;
  memset(string_buffer, 0, sizeof(string_buffer));
  util::log::format_to(string_buffer, "{},{}", 1, test_exception_for_log_formatter(false, true));
  std::cout << "format_to: " << string_buffer << std::endl;
  memset(string_buffer, 0, sizeof(string_buffer));

  util::log::format_to_n(string_buffer, 100, "{},{},{},{}", 1, 2, 3);
  std::cout << "format_to_n: " << string_buffer << std::endl;
  memset(string_buffer, 0, sizeof(string_buffer));
  util::log::format_to_n(string_buffer, 100, "{},{}", 1, test_exception_for_log_formatter(true, true));
  std::cout << "format_to_n: " << string_buffer << std::endl;
  memset(string_buffer, 0, sizeof(string_buffer));
  util::log::format_to_n(string_buffer, 100, "{},{}", 1, test_exception_for_log_formatter(false, true));
  std::cout << "format_to_n: " << string_buffer << std::endl;
  memset(string_buffer, 0, sizeof(string_buffer));

  std::cout << "vformat: " << util::log::vformat("{},{},{},{}", util::log::make_format_args(1, 2, 3)) << std::endl;
  std::cout << "vformat: "
            << util::log::vformat("{},{}", util::log::make_format_args(1, test_exception_for_log_formatter(true, true)))
            << std::endl;
  std::cout << "vformat: "
            << util::log::vformat("{},{}",
                                  util::log::make_format_args(1, test_exception_for_log_formatter(false, true)))
            << std::endl;

#    if defined(LIBATFRAME_UTILS_ENABLE_STD_FORMAT) && LIBATFRAME_UTILS_ENABLE_STD_FORMAT
  using iterator_type =
      decltype(util::log::format_to(string_buffer, "{},{}", 1, test_exception_for_log_formatter(true, true)));
  // MSVC 1929(VS 16.10) has some problem on type detection and converting, so we speciffy OutputIt here
  util::log::vformat_to<iterator_type>(
      string_buffer, "{},{},{},{}",
      util::log::make_format_args<LOG_WRAPPER_FWAPI_NAMESPACE basic_format_context<iterator_type, char> >(1, 2, 3));
  std::cout << "vformat_to: " << string_buffer << std::endl;
  memset(string_buffer, 0, sizeof(string_buffer));

  util::log::vformat_to<iterator_type>(
      string_buffer, "{},{}",
      util::log::make_format_args<LOG_WRAPPER_FWAPI_NAMESPACE basic_format_context<iterator_type, char> >(
          1, test_exception_for_log_formatter(true, true)));
  std::cout << "vformat_to: " << string_buffer << std::endl;
  memset(string_buffer, 0, sizeof(string_buffer));

  util::log::vformat_to<iterator_type>(
      string_buffer, "{},{}",
      util::log::make_format_args<LOG_WRAPPER_FWAPI_NAMESPACE basic_format_context<iterator_type, char> >(
          1, test_exception_for_log_formatter(false, true)));
  std::cout << "vformat_to: " << string_buffer << std::endl;
#    else
  util::log::vformat_to(string_buffer, "{},{},{},{}", util::log::make_format_args(1, 2, 3));
  std::cout << "vformat_to: " << string_buffer << std::endl;
  memset(string_buffer, 0, sizeof(string_buffer));

  util::log::vformat_to(string_buffer, "{},{}",
                        util::log::make_format_args(1, test_exception_for_log_formatter(true, true)));
  std::cout << "vformat_to: " << string_buffer << std::endl;
  memset(string_buffer, 0, sizeof(string_buffer));

  util::log::vformat_to(string_buffer, "{},{}",
                        util::log::make_format_args(1, test_exception_for_log_formatter(false, true)));
  std::cout << "vformat_to: " << string_buffer << std::endl;
#    endif
}
#  endif

#endif
void log_sample() {
  log_sample_func5(9);
#if defined(LOG_WRAPPER_ENABLE_FWAPI) && LOG_WRAPPER_ENABLE_FWAPI
  log_sample_func6();
#  if defined(LIBATFRAME_UTILS_ENABLE_EXCEPTION) && LIBATFRAME_UTILS_ENABLE_EXCEPTION
  log_sample_func7();
#  endif
#endif
}

//=======================================================================================================

//=======================================================================================================
void random_sample() {
  util::random::mt19937 gen1;
  gen1.init_seed(123);

  std::cout << "Random - mt19937: " << gen1.random() << std::endl;
  std::cout << "Random - mt19937: " << gen1() << std::endl;
  std::cout << "Random - mt19937 - between [100, 10000): " << gen1.random_between(100, 10000) << std::endl;
}

//=======================================================================================================

//=======================================================================================================
void tquerystring_sample() {
  util::tquerystring encode, decode;

  encode.set("a", "wulala");
  encode.set("page", "ok!");

  std::string output;
  encode.encode(output);

  util::types::item_array::ptr_type arr = encode.create_array();

  arr->append("blablabla...");
  arr->append("a and b is ab");
  util::types::item_object::ptr_type obj = encode.create_object();
  obj->set("so", "");
  arr->append(obj);

  encode.set("c", arr);

  std::cout << "encode => " << encode.to_string() << std::endl;
  std::cout << "encode (old) => " << output << std::endl;
  std::cout << "Array => " << arr->to_string() << std::endl;

  decode.decode(encode.to_string().c_str());
  std::cout << "decode => " << decode.to_string() << std::endl;
}

//=======================================================================================================

//=======================================================================================================
void hash_sample() {
  char str_buff[] = "Hello World!\nI'm OWenT\n";
  std::cout << "Hashed String: " << std::endl << str_buff << std::endl;
  std::cout << "FNV-1:   " << util::hash::hash_fnv1<uint32_t>(str_buff, strlen(str_buff)) << std::endl;
  std::cout << "FNV-1A:  " << util::hash::hash_fnv1a<uint32_t>(str_buff, strlen(str_buff)) << std::endl;
  std::cout << "SDBM:    " << util::hash::hash_sdbm<uint32_t>(str_buff, strlen(str_buff)) << std::endl;
  std::cout << "RS:      " << util::hash::hash_rs<uint32_t>(str_buff, strlen(str_buff)) << std::endl;
  std::cout << "JS:      " << util::hash::hash_js<uint32_t>(str_buff, strlen(str_buff)) << std::endl;
  std::cout << "PJW:     " << util::hash::hash_pjw<uint32_t>(str_buff, strlen(str_buff)) << std::endl;
  std::cout << "ELF:     " << util::hash::hash_elf<uint32_t>(str_buff, strlen(str_buff)) << std::endl;
  std::cout << "BKDR:    " << util::hash::hash_bkdr<uint32_t>(str_buff, strlen(str_buff)) << std::endl;
  std::cout << "DJB:     " << util::hash::hash_djb<uint32_t>(str_buff, strlen(str_buff)) << std::endl;
  std::cout << "AP:      " << util::hash::hash_ap<uint32_t>(str_buff, strlen(str_buff)) << std::endl;
}
//=======================================================================================================

//=======================================================================================================
extern int cmd_option_sample_main();

void cmd_option_sample() { cmd_option_sample_main(); }
//=======================================================================================================

//=======================================================================================================

struct GameConf {
  GameConf();
  struct ChannelConfig {
    size_t membus_buffer_length;
    size_t network_buffer_length;
    size_t vserver_buffer_length;
  };
  ChannelConfig channel;

  struct LogConfig {
    int level;
    bool auto_update_time;
    bool print_file;
    bool print_function;
    bool print_type;
    std::string print_time;

    int std_level_min;
    int std_level_max;

    int fs_level_min;
    int fs_level_max;
    std::string fs_path;
    std::string fs_suffix;
    size_t fs_file_number;
    size_t fs_file_size;
    bool fs_enable_buffer;
  };
  LogConfig log;

  struct VServerConfig {
    std::string main;
    uint32_t logic_frame_duration;
    int32_t logic_x;
    int32_t logic_x_offset;
    size_t logic_block_init_number;
  };
  VServerConfig vserver;

  bool init();
};

GameConf::GameConf() {}

bool GameConf::init() {
  util::config::ini_loader conf_loader;

  conf_loader.load_file((g_exec_dir + "/config.ini").c_str());

  // 通道配置
  {
    conf_loader.dump_to("channel.mem_bus.buffer.max_length", channel.membus_buffer_length);
    conf_loader.dump_to("channel.network.buffer.max_length", channel.network_buffer_length);
    conf_loader.dump_to("channel.vserver.buffer.max_length", channel.vserver_buffer_length);
  }

  // 默认资源和脚本目录配置
  {
    std::list<std::string> paths;
    conf_loader.dump_to("resource.res.dir", paths);

    paths.clear();
    conf_loader.dump_to("resource.script.client.dir", paths);

    paths.clear();
    conf_loader.dump_to("resource.script.client.cdir", paths);

    paths.clear();
    conf_loader.dump_to("resource.script.vserver.dir", paths);

    paths.clear();
    conf_loader.dump_to("resource.script.vserver.cdir", paths);

    conf_loader.dump_to("resource.script.vserver.main", vserver.main);
    conf_loader.dump_to("resource.script.vserver.logic_frame_duration", vserver.logic_frame_duration);
    conf_loader.dump_to("resource.script.vserver.logic_x", vserver.logic_x);
    conf_loader.dump_to("resource.script.vserver.logic_x_offset", vserver.logic_x_offset);
    conf_loader.dump_to("resource.script.vserver.logic_block_init_number", vserver.logic_block_init_number);
  }

  // 日志配置
  {
    conf_loader.dump_to("system.log.level", log.level);
    conf_loader.dump_to("system.log.auto_update_time", log.auto_update_time);
    conf_loader.dump_to("system.log.print_file", log.print_file);
    conf_loader.dump_to("system.log.print_function", log.print_function);
    conf_loader.dump_to("system.log.print_type", log.print_type);
    conf_loader.dump_to("system.log.print_time", log.print_time);

    conf_loader.dump_to("system.log.std.level.min", log.std_level_min);
    conf_loader.dump_to("system.log.std.level.max", log.std_level_max);

    conf_loader.dump_to("system.log.fs.level.min", log.fs_level_min);
    conf_loader.dump_to("system.log.fs.level.max", log.fs_level_max);
    conf_loader.dump_to("system.log.fs.path", log.fs_path);
    conf_loader.dump_to("system.log.fs.suffix", log.fs_suffix);
    conf_loader.dump_to("system.log.fs.file_number", log.fs_file_number);
    conf_loader.dump_to("system.log.fs.file_size", log.fs_file_size);
    conf_loader.dump_to("system.log.fs.enable_buffer", log.fs_enable_buffer);
  }

  // 转储时间周期
  {
    util::config::duration_value dur;
    conf_loader.dump_to("system.interval_ns", dur, true);
    printf("system.interval_ns: sec %lld, nsec: %lld\n", static_cast<long long>(dur.sec),
           static_cast<long long>(dur.nsec));
    conf_loader.dump_to("system.interval_us", dur, true);
    printf("system.interval_us: sec %lld, nsec: %lld\n", static_cast<long long>(dur.sec),
           static_cast<long long>(dur.nsec));
    conf_loader.dump_to("system.interval_ms", dur, true);
    printf("system.interval_ms: sec %lld, nsec: %lld\n", static_cast<long long>(dur.sec),
           static_cast<long long>(dur.nsec));
    conf_loader.dump_to("system.interval_s", dur, true);
    printf("system.interval_s: sec %lld, nsec: %lld\n", static_cast<long long>(dur.sec),
           static_cast<long long>(dur.nsec));
    conf_loader.dump_to("system.interval_m", dur, true);
    printf("system.interval_m: sec %lld, nsec: %lld\n", static_cast<long long>(dur.sec),
           static_cast<long long>(dur.nsec));
    conf_loader.dump_to("system.interval_h", dur, true);
    printf("system.interval_h: sec %lld, nsec: %lld\n", static_cast<long long>(dur.sec),
           static_cast<long long>(dur.nsec));
    conf_loader.dump_to("system.interval_d", dur, true);
    printf("system.interval_d: sec %lld, nsec: %lld\n", static_cast<long long>(dur.sec),
           static_cast<long long>(dur.nsec));
    conf_loader.dump_to("system.interval_w", dur, true);
    printf("system.interval_w: sec %lld, nsec: %lld\n", static_cast<long long>(dur.sec),
           static_cast<long long>(dur.nsec));
  }

  return true;
}

void iniloader_sample1() {
  GameConf gconf;

  puts("===== before load ini =====");
  printf("system.log.print_time: %s(%p)\n", gconf.log.print_time.c_str(), gconf.log.print_time.c_str());
  printf("resource.script.vserver.main: %s\n", gconf.vserver.main.c_str());

  gconf.init();

  puts("===== after load ini =====");
  printf("system.log.print_time: %s(%p)\n", gconf.log.print_time.c_str(), gconf.log.print_time.c_str());
  printf("resource.script.vserver.main: %s\n", gconf.vserver.main.c_str());
}

void iniloader_sample2() {
  util::config::ini_loader cfg_loader;
  cfg_loader.load_file((g_exec_dir + "/test.ini").c_str());

  // 转储整数
  {
    int t1 = 9999, t2 = 9999;
    cfg_loader.dump_to("a.b.c1", t1);
    cfg_loader.dump_to("a.b.c4", t2, true);
    printf("a.b.c1 = %d\na.b.c4 = %d\n", t1, t2);
  }

  // 转储浮点数
  {
    float t1 = 0.0;
    cfg_loader.dump_to("a.b.c2", t1);
    printf("a.b.c2 = %f\n", t1);
  }

  // 转储字符串
  {
    char t1[32] = {0};
    std::string t2, t3 = "0000000000000000";
    std::string t4, t5;

    cfg_loader.dump_to("d.c.b.a.e.f1", t2);                    // 字符串
    cfg_loader.dump_to("d.c.b.a.e.f1", t3.begin(), t3.end());  // 字符串迭代器
    cfg_loader.dump_to("d.c.b.a.e.f2", t1);                    // 字符串
    cfg_loader.dump_to("d.c.b.a.e.f2", t1 + 16, t1 + 32);      // 字符串指针迭代器
    cfg_loader.dump_to("a.b.c3", t4);                          // 带不可打印字符的字符串
    cfg_loader.dump_to("d.c.b.a.e.f3", t5);                    // 字符串 - Section使用.分割层级

    printf("len(t2) = %d\nlen(t3) = %d\n", (int)t2.size(), (int)t3.size());
    printf("d.c.b.a.e.f2 = %s\n", t1);
    printf("d.c.b.a.e.f2 = %s(+16)\n", t1 + 16);
    printf("d.c.b.a.e.f3 = %s\n", t5.c_str());
    printf("a.b.c3 = %s\n", t4.c_str());
  }

  // 转储到 vector
  {
    std::vector<std::string> t1;
    std::list<bool> t2;
    cfg_loader.dump_to("a.arr", t1);
    cfg_loader.dump_to("a.bool", t2);

    for (size_t i = 0; i < t1.size(); ++i) {
      printf("t1[%d] = %s\n", (int)i, t1[i].c_str());
    }

    size_t index = 0;
    for (std::list<bool>::iterator iter = t2.begin(); iter != t2.end(); ++iter) {
      printf("t2[%d] = %s\n", (int)index++, (*iter) ? "true" : "false");
    }
  }

  // 转储时间周期
  {
    util::config::duration_value dur;
    cfg_loader.dump_to("system.interval_ns", dur, true);
    printf("system.interval_ns: sec %lld, nsec: %lld\n", static_cast<long long>(dur.sec),
           static_cast<long long>(dur.nsec));
    cfg_loader.dump_to("system.interval_us", dur, true);
    printf("system.interval_us: sec %lld, nsec: %lld\n", static_cast<long long>(dur.sec),
           static_cast<long long>(dur.nsec));
    cfg_loader.dump_to("system.interval_ms", dur, true);
    printf("system.interval_ms: sec %lld, nsec: %lld\n", static_cast<long long>(dur.sec),
           static_cast<long long>(dur.nsec));
    cfg_loader.dump_to("system.interval_s", dur, true);
    printf("system.interval_s: sec %lld, nsec: %lld\n", static_cast<long long>(dur.sec),
           static_cast<long long>(dur.nsec));
    cfg_loader.dump_to("system.interval_m", dur, true);
    printf("system.interval_m: sec %lld, nsec: %lld\n", static_cast<long long>(dur.sec),
           static_cast<long long>(dur.nsec));
    cfg_loader.dump_to("system.interval_h", dur, true);
    printf("system.interval_h: sec %lld, nsec: %lld\n", static_cast<long long>(dur.sec),
           static_cast<long long>(dur.nsec));
    cfg_loader.dump_to("system.interval_d", dur, true);
    printf("system.interval_d: sec %lld, nsec: %lld\n", static_cast<long long>(dur.sec),
           static_cast<long long>(dur.nsec));
    cfg_loader.dump_to("system.interval_w", dur, true);
    printf("system.interval_w: sec %lld, nsec: %lld\n", static_cast<long long>(dur.sec),
           static_cast<long long>(dur.nsec));
  }
}

void iniloader_sample() {
  iniloader_sample1();
  iniloader_sample2();
}
//=======================================================================================================

int main(int, char *argv[]) {
  util::time::time_utility::update();

  util::file_system::dirname(argv[0], 0, g_exec_dir);

  CALL_SAMPLE(tquerystring_sample);
  CALL_SAMPLE(hash_sample);
  CALL_SAMPLE(random_sample);
  CALL_SAMPLE(log_sample);
  CALL_SAMPLE(cmd_option_sample);
  CALL_SAMPLE(iniloader_sample);

  return 0;
}
