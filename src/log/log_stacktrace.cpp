// Copyright 2021 atframework
// Licensed under the MIT licenses.
// Created by owent on 2015-06-29

#include "log/log_stacktrace.h"

#include <mutex>
#include <string>
#include <vector>

#include "memory/lru_map.h"

#include "common/string_oprs.h"
#include "log/log_wrapper.h"

#ifndef LOG_STACKTRACE_MAX_STACKS
#  define LOG_STACKTRACE_MAX_STACKS 256
#endif

#define LOG_STACKTRACE_MAX_STACKS_ARRAY_SIZE (LOG_STACKTRACE_MAX_STACKS + 1)

// disable some warnings in msvc's headers
#if defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable : 4091)
#endif

// select method to stacktrace
#if defined(LOG_STACKTRACE_USING_LIBUNWIND) && LOG_STACKTRACE_USING_LIBUNWIND
#  include <libunwind.h>

#elif defined(LOG_STACKTRACE_USING_EXECINFO) && LOG_STACKTRACE_USING_EXECINFO
#  include <execinfo.h>

#elif defined(LOG_STACKTRACE_USING_UNWIND) && LOG_STACKTRACE_USING_UNWIND
#  include <unwind.h>

#elif defined(LOG_STACKTRACE_USING_DBGHELP) && LOG_STACKTRACE_USING_DBGHELP
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif

#  include <Windows.h>

#  include <DbgHelp.h>

#  ifdef _MSC_VER
#    pragma comment(lib, "dbghelp.lib")

#    ifdef UNICODE
#      include <atlconv.h>
#      define LOG_STACKTRACE_VC_A2W(x) A2W(x)
#      define LOG_STACKTRACE_VC_W2A(x) W2A(x)
#    else
#      define LOG_STACKTRACE_VC_A2W(x) x
#      define LOG_STACKTRACE_VC_W2A(x) x
#    endif

struct SymInitializeHelper {
  SymInitializeHelper() {
    process = GetCurrentProcess();

    // SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
    SymInitialize(process, nullptr, TRUE);
  }

  ~SymInitializeHelper() { SymCleanup(process); }

  static const SymInitializeHelper &Inst() {
    static SymInitializeHelper ret;
    return ret;
  }

  HANDLE process;
};

#  endif

#elif defined(LOG_STACKTRACE_USING_DBGENG) && LOG_STACKTRACE_USING_DBGENG
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif

#  include <Windows.h>

#  include <DbgEng.h>

#  ifdef _MSC_VER
#    pragma comment(lib, "ole32.lib")
#    pragma comment(lib, "dbgeng.lib")

#    ifdef UNICODE
#      include <atlconv.h>
#      define LOG_STACKTRACE_VC_A2W(x) A2W(x)
#      define LOG_STACKTRACE_VC_W2A(x) W2A(x)
#    else
#      define LOG_STACKTRACE_VC_A2W(x) x
#      define LOG_STACKTRACE_VC_W2A(x) x
#    endif

#  endif

#  ifdef __CRT_UUID_DECL  // for __MINGW__
__CRT_UUID_DECL(IDebugClient, 0x27fe5639, 0x8407, 0x4f47, 0x83, 0x64, 0xee, 0x11, 0x8f, 0xb0, 0x8a, 0xc8)
__CRT_UUID_DECL(IDebugControl, 0x5182e668, 0x105e, 0x416e, 0xad, 0x92, 0x24, 0xef, 0x80, 0x04, 0x24, 0xba)
__CRT_UUID_DECL(IDebugSymbols, 0x8c31e98c, 0x983a, 0x48a5, 0x90, 0x16, 0x6f, 0xe5, 0xd6, 0x67, 0xa9, 0x50)
#  elif defined(DEFINE_GUID) && !defined(BOOST_MSVC)
DEFINE_GUID(IID_IDebugClient, 0x27fe5639, 0x8407, 0x4f47, 0x83, 0x64, 0xee, 0x11, 0x8f, 0xb0, 0x8a, 0xc8);
DEFINE_GUID(IID_IDebugControl, 0x5182e668, 0x105e, 0x416e, 0xad, 0x92, 0x24, 0xef, 0x80, 0x04, 0x24, 0xba);
DEFINE_GUID(IID_IDebugSymbols, 0x8c31e98c, 0x983a, 0x48a5, 0x90, 0x16, 0x6f, 0xe5, 0xd6, 0x67, 0xa9, 0x50);
#  endif

template <class T>
class log_stacktrace_com_holder {
 private:
  T *holder_;

 private:
  log_stacktrace_com_holder(const log_stacktrace_com_holder &) = delete;
  log_stacktrace_com_holder &operator=(const log_stacktrace_com_holder &) = delete;

 public:
  log_stacktrace_com_holder() noexcept : holder_(nullptr) {}
  ~log_stacktrace_com_holder() noexcept {
    if (holder_) {
      holder_->Release();
    }
  }

  T *operator->() const noexcept { return holder_; }

  PVOID *to_pvoid_ptr() noexcept { return reinterpret_cast<PVOID *>(&holder_); }

  bool is_inited() const noexcept { return !!holder_; }
};

#endif

// restore some warnings in msvc's headers
#if defined(_MSC_VER)
#  pragma warning(pop)
#endif

// for demangle
#ifdef __GLIBCXX__
#  include <cxxabi.h>
#  define USING_LIBSTDCXX_ABI 1
#elif defined(_LIBCPP_ABI_VERSION)
#  include <cxxabi.h>
#  define USING_LIBCXX_ABI 1
#endif

LIBATFRAME_UTILS_NAMESPACE_BEGIN
namespace log {
namespace {

struct UTIL_SYMBOL_LOCAL stacktrace_global_settings {
  bool need_clear = false;
  size_t lru_cache_size = 300000;
  std::chrono::microseconds lru_cache_timeout =
      std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::seconds{14400});
  inline stacktrace_global_settings() noexcept {}
};

static const stacktrace_options &default_stacktrace_options() {
  static stacktrace_options opts = {0, 1, 0};
  return opts;
}

static stacktrace_global_settings &get_stacktrace_settings() {
  static stacktrace_global_settings instance;
  return instance;
}

static UTIL_SANITIZER_NO_THREAD void internal_set_stacktrace_lru_cache_size(size_t sz) noexcept {
  get_stacktrace_settings().lru_cache_size = sz;
}

static UTIL_SANITIZER_NO_THREAD size_t internal_get_stacktrace_lru_cache_size() noexcept {
  return get_stacktrace_settings().lru_cache_size;
}

static UTIL_SANITIZER_NO_THREAD void internal_set_stacktrace_lru_cache_timeout(
    std::chrono::microseconds timeout) noexcept {
  get_stacktrace_settings().lru_cache_timeout = timeout;
}

static UTIL_SANITIZER_NO_THREAD std::chrono::microseconds internal_get_stacktrace_lru_cache_timeout() noexcept {
  return get_stacktrace_settings().lru_cache_timeout;
}

static UTIL_SANITIZER_NO_THREAD void internal_set_clear_stacktrace_lru_cache(bool v) noexcept {
  get_stacktrace_settings().need_clear = v;
}

static UTIL_SANITIZER_NO_THREAD bool internal_get_clear_stacktrace_lru_cache() noexcept {
  return get_stacktrace_settings().need_clear;
}

#if defined(LOG_STACKTRACE_USING_LIBUNWIND) && LOG_STACKTRACE_USING_LIBUNWIND
struct UTIL_SYMBOL_LOCAL stacktrace_symbol_group_t {
  std::string demangle_name;
  std::string func_name;
  std::string func_offset;

  std::chrono::system_clock::time_point timeout;
};

static stacktrace_symbol_group_t unw_mutable_symbol_from_cache(unw_word_t key, unw_cursor_t &unw_cur) {
  static std::mutex lock;
  static memory::lru_map<unw_word_t, stacktrace_symbol_group_t> stack_caches;

  // First step: load from cache
  {
    std::lock_guard<std::mutex> lock_guard{lock};

    if (internal_get_clear_stacktrace_lru_cache()) {
      stack_caches.clear();
      internal_set_clear_stacktrace_lru_cache(false);
    } else {
      std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
      while (!stack_caches.empty()) {
        if (!stack_caches.front().second) {
          stack_caches.pop_front();
          continue;
        }
        if (now > stack_caches.front().second->timeout) {
          stack_caches.pop_front();
        } else {
          break;
        }
      }
    }

    auto iter = stack_caches.find(key, false);
    if (iter != stack_caches.end() && iter->second) {
      return *iter->second;
    }
  }

  std::vector<char> func_name_cache;
  func_name_cache.resize(16384, 0);
  unw_word_t unw_offset;

  unw_get_proc_name(&unw_cur, &func_name_cache[0], func_name_cache.size() - 1, &unw_offset);

  stacktrace_symbol_group_t result;
  result.func_name = func_name_cache.data();
  result.func_offset = util::log::format("+{:#x}", unw_offset);
  result.timeout = std::chrono::system_clock::now() + internal_get_stacktrace_lru_cache_timeout();

#  if defined(USING_LIBSTDCXX_ABI) || defined(USING_LIBCXX_ABI)
  int cxx_abi_status;
  char *realfunc_name = abi::__cxa_demangle(&func_name_cache[0], 0, 0, &cxx_abi_status);
  if (nullptr != realfunc_name) {
    result.demangle_name = realfunc_name;
    free(realfunc_name);
  }
#  endif

  {
    // Append into LRU cache
    std::lock_guard<std::mutex> lock_guard{lock};
    stack_caches.insert_key_value(key, result);

    size_t lru_cache_size_limit = get_stacktrace_lru_cache_size();
    while (stack_caches.size() > lru_cache_size_limit) {
      stack_caches.pop_front();
    }
  }

  return result;
}

#elif defined(LOG_STACKTRACE_USING_EXECINFO) && LOG_STACKTRACE_USING_EXECINFO
struct UTIL_SYMBOL_LOCAL stacktrace_symbol_group_t {
  std::string module_name;
  std::string demangle_name;
  std::string func_name;
  std::string func_offset;
  std::string func_address;

  std::chrono::system_clock::time_point timeout;
};

inline bool stacktrace_is_space_char(char c) noexcept { return ' ' == c || '\r' == c || '\t' == c || '\n' == c; }

static const char *stacktrace_skip_space(const char *name) noexcept {
  if (nullptr == name) {
    return name;
  }

  while (*name && stacktrace_is_space_char(*name)) {
    ++name;
  }

  return name;
}

inline bool stacktrace_is_number_char(char c) noexcept { return c >= '0' && c <= '9'; }

inline bool stacktrace_is_ident_char(char c) noexcept {
  return '_' == c || '$' == c || stacktrace_is_number_char(c) || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
         (c & 0x80);  // utf-8 or unicode
}

static const char *stacktrace_get_ident_end(const char *name) noexcept {
  if (nullptr == name) {
    return name;
  }

  while (*name && !stacktrace_is_space_char(*name) && '(' != *name && ')' != *name && '+' != *name) {
    ++name;
  }

  return name;
}

static bool stacktrace_pick_ident(const char *name, const char *&start, const char *&end, char &previous_c,
                                  bool &clear_sym) noexcept {
  previous_c = 0;
  start = name;
  end = name;

  bool ret = false;
  if (nullptr == name) {
    return false;
  }

  while (*name) {
    // gcc in linux will get a string like MODULE_NAME(FUNCTION_NAME+OFFSET) [HEX_ADDRESS]
    // if we meet a (, we should clear the function name cache
    if ('(' == *name) {
      clear_sym = true;
    }

    name = stacktrace_skip_space(name);

    if (stacktrace_is_ident_char(*name)) {
      start = name;
      end = stacktrace_get_ident_end(name);
      ret = true;
      break;
    } else {
      previous_c = *name;
      ++name;
    }
  }

  return ret;
}

static void stacktrace_fix_number(std::string &num) noexcept {
  size_t fixed_len = num.size();
  while (fixed_len > 0 && (num[fixed_len - 1] > '9' || num[fixed_len - 1] < '0')) {
    --fixed_len;
  }

  num.resize(fixed_len);
}

static void stacktrace_pick_symbol_info(const char *name, stacktrace_symbol_group_t &out) {
  out.module_name.clear();
  out.func_name.clear();
  out.func_offset.clear();
  out.func_address.clear();

  if (nullptr == name || 0 == *name) {
    return;
  }

  name = stacktrace_skip_space(name);
  while (name) {
    const char *start;
    char previous_c;
    bool clear_sym = false;

    if (stacktrace_pick_ident(name, start, name, previous_c, clear_sym)) {
      if (stacktrace_is_number_char(*start)) {
        if (clear_sym) {
          out.func_name.clear();
        }

        if ('+' == previous_c) {
          out.func_offset = "+";
          out.func_offset.insert(out.func_offset.end(), start, name);
          stacktrace_fix_number(out.func_offset);
        } else {
          out.func_address.assign(start, name);
          stacktrace_fix_number(out.func_address);
        }
      } else {
        if (out.module_name.empty()) {
          out.module_name.assign(start, name);
        } else {
          out.func_name.assign(start, name);
        }
      }
    } else {
      if (clear_sym) {
        out.func_name.clear();
      }
      break;
    }

    name = stacktrace_skip_space(name);
  }
}

void stacktrace_fill_symbol_info(void *const *stacks, size_t stack_size, std::vector<stacktrace_symbol_group_t> &out) {
  if (stack_size <= 0) {
    return;
  }
  out.reserve(stack_size);
  out.resize(stack_size);

  std::vector<size_t> parse_symbols_idx;
  std::vector<void *> parse_symbols_ptr;
  parse_symbols_idx.reserve((stack_size + 1) / 2);
  parse_symbols_ptr.reserve((stack_size + 1) / 2);

  static std::mutex lock;
  static memory::lru_map<void *, stacktrace_symbol_group_t> stack_caches;

  // First step: load from cache
  {
    std::lock_guard<std::mutex> lock_guard{lock};

    if (internal_get_clear_stacktrace_lru_cache()) {
      stack_caches.clear();
      internal_set_clear_stacktrace_lru_cache(false);
    } else {
      std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
      while (!stack_caches.empty()) {
        if (!stack_caches.front().second) {
          stack_caches.pop_front();
          continue;
        }
        if (now > stack_caches.front().second->timeout) {
          stack_caches.pop_front();
        } else {
          break;
        }
      }
    }

    for (size_t i = 0; i < stack_size; ++i) {
      auto iter = stack_caches.find(stacks[i], false);
      if (iter != stack_caches.end() && iter->second) {
        out[i] = *iter->second;
        continue;
      }

      parse_symbols_idx.push_back(i);
      parse_symbols_ptr.push_back(stacks[i]);
    }
  }

  // Second step: load from symbol
  if (!parse_symbols_ptr.empty()) {
    char **func_name_cache = backtrace_symbols(parse_symbols_ptr.data(), static_cast<int>(parse_symbols_ptr.size()));
    for (size_t i = 0; nullptr != func_name_cache && i < parse_symbols_ptr.size() && i < parse_symbols_idx.size();
         ++i) {
      stacktrace_symbol_group_t &out_symbol = out[parse_symbols_idx[i]];

      if (func_name_cache[i] == nullptr) {
        continue;
      }

      stacktrace_pick_symbol_info(func_name_cache[i], out_symbol);

#  if defined(USING_LIBSTDCXX_ABI) || defined(USING_LIBCXX_ABI)
      if (!out_symbol.func_name.empty()) {
        int cxx_abi_status;
        char *realfunc_name = abi::__cxa_demangle(out_symbol.func_name.c_str(), 0, 0, &cxx_abi_status);
        if (nullptr != realfunc_name) {
          out_symbol.demangle_name = realfunc_name;
        }

        if (nullptr != realfunc_name) {
          free(realfunc_name);
        }
      }
#  endif
    }

    if (nullptr != func_name_cache) {
      ::free(func_name_cache);
    }

    // Append into LRU cache
    std::lock_guard<std::mutex> lock_guard{lock};
    for (size_t i = 0; i < parse_symbols_ptr.size() && i < parse_symbols_idx.size(); ++i) {
      void *key = parse_symbols_ptr[i];
      stacktrace_symbol_group_t &out_symbol = out[parse_symbols_idx[i]];
      out_symbol.timeout = std::chrono::system_clock::now() + internal_get_stacktrace_lru_cache_timeout();
      stack_caches.insert_key_value(key, out_symbol);
    }

    size_t lru_cache_size_limit = get_stacktrace_lru_cache_size();
    while (stack_caches.size() > lru_cache_size_limit) {
      stack_caches.pop_front();
    }
  }
}

#elif (defined(LOG_STACKTRACE_USING_DBGHELP) && LOG_STACKTRACE_USING_DBGHELP)
struct UTIL_SYMBOL_LOCAL stacktrace_symbol_group_t {
  std::string address;
  std::string func_name;
  std::string func_offset;

  std::chrono::system_clock::time_point timeout;
};

struct UTIL_SYMBOL_LOCAL dbghelper_cache_key_hash {
  size_t operator()(const std::pair<HANDLE, PVOID> &key) const noexcept {
    size_t result = std::hash<HANDLE>()(key.first);
    result ^= std::hash<PVOID>()(key.second) + 0x9e3779b9 + (result << 6) + (result >> 2);
    return result;
  }
};

static stacktrace_symbol_group_t dbghelper_mutable_symbol_from_cache(HANDLE process, PVOID stack) {
  static std::mutex lock;
  static memory::lru_map<std::pair<HANDLE, PVOID>, stacktrace_symbol_group_t, dbghelper_cache_key_hash> stack_caches;

  std::pair<HANDLE, PVOID> key{process, stack};

  // First step: load from cache
  {
    std::lock_guard<std::mutex> lock_guard{lock};

    if (internal_get_clear_stacktrace_lru_cache()) {
      stack_caches.clear();
      internal_set_clear_stacktrace_lru_cache(false);
    } else {
      std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
      while (!stack_caches.empty()) {
        if (!stack_caches.front().second) {
          stack_caches.pop_front();
          continue;
        }
        if (now > stack_caches.front().second->timeout) {
          stack_caches.pop_front();
        } else {
          break;
        }
      }
    }

    auto iter = stack_caches.find(key, false);
    if (iter != stack_caches.end() && iter->second) {
      return *iter->second;
    }
  }

#  ifdef UNICODE
  USES_CONVERSION;
#  endif

  SYMBOL_INFO *symbol;
  DWORD64 displacement = 0;
  stacktrace_symbol_group_t result;
  result.timeout = std::chrono::system_clock::now() + internal_get_stacktrace_lru_cache_timeout();

  symbol = reinterpret_cast<SYMBOL_INFO *>(malloc(sizeof(SYMBOL_INFO) + (MAX_SYM_NAME + 1) * sizeof(TCHAR)));
  symbol->MaxNameLen = MAX_SYM_NAME;
  symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

  if (SymFromAddr(SymInitializeHelper::Inst().process, reinterpret_cast<ULONG64>(stack), &displacement, symbol)) {
    result.func_name = LOG_STACKTRACE_VC_W2A(symbol->Name);
    result.func_offset = util::log::format("+{:#x}", displacement);
    result.address = util::log::format("{:#x}", symbol->Address);
  } else {
    result.address = util::log::format("{}", reinterpret_cast<const void *>(stack));
  }
  free(symbol);

  {
    // Append into LRU cache
    std::lock_guard<std::mutex> lock_guard{lock};
    stack_caches.insert_key_value(key, result);

    size_t lru_cache_size_limit = get_stacktrace_lru_cache_size();
    while (stack_caches.size() > lru_cache_size_limit) {
      stack_caches.pop_front();
    }
  }

  return result;
}
#endif

}  // namespace

LIBATFRAME_UTILS_API bool is_stacktrace_enabled() noexcept {
#if defined(LOG_STACKTRACE_USING_LIBUNWIND) && LOG_STACKTRACE_USING_LIBUNWIND
  return true;
#elif defined(LOG_STACKTRACE_USING_EXECINFO) && LOG_STACKTRACE_USING_EXECINFO
  return true;
#elif defined(LOG_STACKTRACE_USING_UNWIND) && LOG_STACKTRACE_USING_UNWIND
  return true;
#elif defined(LOG_STACKTRACE_USING_DBGHELP) && LOG_STACKTRACE_USING_DBGHELP
  return true;
#elif defined(LOG_STACKTRACE_USING_DBGENG) && LOG_STACKTRACE_USING_DBGENG
  return true;
#else
  return false;
#endif
}

LIBATFRAME_UTILS_API void set_stacktrace_lru_cache_size(size_t sz) noexcept {
  internal_set_stacktrace_lru_cache_size(sz);
}

LIBATFRAME_UTILS_API size_t get_stacktrace_lru_cache_size() noexcept {
  return internal_get_stacktrace_lru_cache_size();
}

LIBATFRAME_UTILS_API void set_stacktrace_lru_cache_timeout(std::chrono::microseconds timeout) noexcept {
  internal_set_stacktrace_lru_cache_timeout(timeout);
}

LIBATFRAME_UTILS_API std::chrono::microseconds get_stacktrace_lru_cache_timeout() noexcept {
  return internal_get_stacktrace_lru_cache_timeout();
}

LIBATFRAME_UTILS_API void clear_stacktrace_lru_cache() noexcept { internal_set_clear_stacktrace_lru_cache(true); }

#if defined(LOG_STACKTRACE_USING_LIBUNWIND) && LOG_STACKTRACE_USING_LIBUNWIND
LIBATFRAME_UTILS_API size_t stacktrace_write(char *buf, size_t bufsz, const stacktrace_options *options) {
  if (nullptr == buf || bufsz <= 0) {
    return 0;
  }

  if (nullptr == options) {
    options = &default_stacktrace_options();
  }

  unw_context_t unw_ctx;
  unw_cursor_t unw_cur;
  unw_proc_info_t unw_proc;
  unw_getcontext(&unw_ctx);
  unw_init_local(&unw_cur, &unw_ctx);

  int frame_id = 0;
  int skip_frames = 1 + static_cast<int>(options->skip_start_frames);
  int frames_count = LOG_STACKTRACE_MAX_STACKS;

  if (options->skip_end_frames > 0) {
    frames_count = 1;
    while (unw_step(&unw_cur) > 0) {
      ++frames_count;
    }

    // restore cursor
    unw_init_local(&unw_cur, &unw_ctx);

    if (frames_count <= skip_frames + static_cast<int>(options->skip_end_frames)) {
      frames_count = 0;
    } else {
      frames_count -= static_cast<int>(options->skip_end_frames);
    }
  }

  size_t ret = 0;
  do {
    if (frames_count <= 0) {
      break;
    }
    --frames_count;

    if (0 != options->max_frames && frame_id >= static_cast<int>(options->max_frames)) {
      break;
    }

    if (skip_frames <= 0) {
      unw_get_proc_info(&unw_cur, &unw_proc);
      if (0 == unw_proc.start_ip) {
        break;
      }
      stacktrace_symbol_group_t stack_symbol = unw_mutable_symbol_from_cache(unw_proc.start_ip, unw_cur);

      auto res = util::log::format_to_n(
          buf, bufsz, "Frame #{:#02}: ({}{}) [{:#x}]\r\n", frame_id,
          stack_symbol.demangle_name.empty() ? stack_symbol.func_name : stack_symbol.demangle_name,
          stack_symbol.func_offset, unw_proc.start_ip);

      if (res.size <= 0) {
        break;
      }

      ret += res.size;
      buf += res.size;
      bufsz -= res.size;
    }

    if (unw_step(&unw_cur) <= 0) {
      break;
    }

    if (skip_frames > 0) {
      --skip_frames;
    } else {
      ++frame_id;
    }
  } while (true);

  return ret;
}

#elif defined(LOG_STACKTRACE_USING_EXECINFO) && LOG_STACKTRACE_USING_EXECINFO
LIBATFRAME_UTILS_API size_t stacktrace_write(char *buf, size_t bufsz, const stacktrace_options *options) {
  if (nullptr == buf || bufsz <= 0) {
    return 0;
  }

  if (nullptr == options) {
    options = &default_stacktrace_options();
  }

  size_t ret = 0;
  void *stacks[LOG_STACKTRACE_MAX_STACKS_ARRAY_SIZE] = {nullptr};
  size_t frames_count = static_cast<size_t>(backtrace(stacks, LOG_STACKTRACE_MAX_STACKS_ARRAY_SIZE));
  size_t skip_frames = 1 + static_cast<size_t>(options->skip_start_frames);

  if (frames_count <= skip_frames + options->skip_end_frames) {
    frames_count = 0;
  } else if (options->skip_end_frames > 0) {
    frames_count -= options->skip_end_frames;
  }

  std::vector<stacktrace_symbol_group_t> frame_symbols;
  stacktrace_fill_symbol_info(stacks + skip_frames, frames_count - skip_frames, frame_symbols);

  for (size_t i = skip_frames; i < frames_count; i++) {
    int frame_id = static_cast<int>(i - skip_frames);
    if (0 != options->max_frames && frame_id >= static_cast<int>(options->max_frames)) {
      break;
    }

    if (nullptr == stacks[i] || 0x01 == reinterpret_cast<intptr_t>(stacks[i])) {
      break;
    }

    util::log::format_to_n_result<char *> res;
    if (i - skip_frames < frame_symbols.size()) {
      stacktrace_symbol_group_t &symbol = frame_symbols[i - skip_frames];
      res = util::log::format_to_n(buf, bufsz, "Frame #{:#02}: ({}{}) [{}]\r\n", frame_id,
                                   symbol.demangle_name.empty() ? symbol.func_name : symbol.demangle_name,
                                   symbol.func_offset, symbol.func_address);
    } else {
      res = util::log::format_to_n(buf, bufsz, "Frame #{:#02}: [{}]\r\n", frame_id, stacks[i]);
    }

    if (res.size <= 0) {
      break;
    }

    ret += res.size;
    buf += res.size;
    bufsz -= res.size;
  }

  return ret;
}

#elif defined(LOG_STACKTRACE_USING_UNWIND) && LOG_STACKTRACE_USING_UNWIND
struct UTIL_SYMBOL_LOCAL stacktrace_unwind_state_t {
  size_t frames_to_skip;
  _Unwind_Word *current;
  _Unwind_Word *end;
};

static _Unwind_Reason_Code stacktrace_unwind_callback(::_Unwind_Context *context, void *arg) noexcept {
  // Note: do not write `::_Unwind_GetIP` because it is a macro on some platforms.
  // Use `_Unwind_GetIP` instead!
  stacktrace_unwind_state_t *const state = reinterpret_cast<stacktrace_unwind_state_t *>(arg);
  if (state->frames_to_skip) {
    --state->frames_to_skip;
    return _Unwind_GetIP(context) ? ::_URC_NO_REASON : ::_URC_END_OF_STACK;
  }

  *state->current = _Unwind_GetIP(context);

  ++state->current;
  if (!*(state->current - 1) || state->current == state->end) {
    return ::_URC_END_OF_STACK;
  }

  return ::_URC_NO_REASON;
}

LIBATFRAME_UTILS_API size_t stacktrace_write(char *buf, size_t bufsz, const stacktrace_options *options) {
  if (nullptr == buf || bufsz <= 0) {
    return 0;
  }

  if (nullptr == options) {
    options = &default_stacktrace_options();
  }

  size_t ret = 0;

  _Unwind_Word stacks[LOG_STACKTRACE_MAX_STACKS_ARRAY_SIZE];
  stacktrace_unwind_state_t state;
  state.frames_to_skip = 0;
  state.current = stacks;
  state.end = stacks + LOG_STACKTRACE_MAX_STACKS_ARRAY_SIZE;

  ::_Unwind_Backtrace(&stacktrace_unwind_callback, &state);
  size_t frames_count = state.current - &stacks[0];
  size_t skip_frames = 1 + static_cast<size_t>(options->skip_start_frames);

  if (frames_count <= skip_frames + options->skip_end_frames) {
    frames_count = 0;
  } else if (options->skip_end_frames > 0) {
    frames_count -= options->skip_end_frames;
  }

  for (size_t i = skip_frames; i < frames_count; ++i) {
    int frame_id = static_cast<int>(i - skip_frames);

    if (0 != options->max_frames && frame_id >= static_cast<int>(options->max_frames)) {
      break;
    }

    if (0 == stacks[i]) {
      break;
    }

    int res = UTIL_STRFUNC_SNPRINTF(buf, bufsz, "Frame #%02d: () [0x%llx]\r\n", frame_id,
                                    static_cast<unsigned long long>(stacks[i]));

    if (res <= 0) {
      break;
    }

    ret += static_cast<size_t>(res);
    buf += res;
    bufsz -= static_cast<size_t>(res);
  }

  return ret;
}

#elif (defined(LOG_STACKTRACE_USING_DBGHELP) && LOG_STACKTRACE_USING_DBGHELP) || \
    (defined(LOG_STACKTRACE_USING_DBGENG) && LOG_STACKTRACE_USING_DBGENG)
LIBATFRAME_UTILS_API size_t stacktrace_write(char *buf, size_t bufsz, const stacktrace_options *options) {
  if (nullptr == buf || bufsz <= 0) {
    return 0;
  }

  if (nullptr == options) {
    options = &default_stacktrace_options();
  }

  PVOID stacks[LOG_STACKTRACE_MAX_STACKS_ARRAY_SIZE];
  USHORT frames_count = CaptureStackBackTrace(0, LOG_STACKTRACE_MAX_STACKS_ARRAY_SIZE, stacks, nullptr);

  size_t ret = 0;
  USHORT skip_frames = 1 + static_cast<USHORT>(options->skip_start_frames);

  if (frames_count <= skip_frames + static_cast<USHORT>(options->skip_end_frames)) {
    frames_count = 0;
  } else if (options->skip_end_frames > 0) {
    frames_count -= static_cast<USHORT>(options->skip_end_frames);
  }

#  if !defined(_MSC_VER)
  for (USHORT i = skip_frames; i < frames_count; ++i) {
    int frame_id = static_cast<int>(i - skip_frames);

    if (0 != options->max_frames && frame_id >= static_cast<int>(options->max_frames)) {
      break;
    }

    if (nullptr == stacks[i]) {
      break;
    }

    int res = UTIL_STRFUNC_SNPRINTF(buf, bufsz, "Frame #%02d: () [0x%llx]\r\n", frame_id,
                                    reinterpret_cast<unsigned long long>(stacks[i]));

    if (res <= 0) {
      break;
    }

    ret += static_cast<size_t>(res);
    buf += res;
    bufsz -= static_cast<size_t>(res);
  }

#  elif (defined(LOG_STACKTRACE_USING_DBGHELP) && LOG_STACKTRACE_USING_DBGHELP)
  for (USHORT i = skip_frames; i < frames_count; ++i) {
    int frame_id = static_cast<int>(i - skip_frames);

    if (0 != options->max_frames && frame_id >= static_cast<int>(options->max_frames)) {
      break;
    }

    if (nullptr == stacks[i]) {
      break;
    }

    util::log::format_to_n_result<char *> res;
    stacktrace_symbol_group_t symbol_info =
        dbghelper_mutable_symbol_from_cache(SymInitializeHelper::Inst().process, stacks[i]);

    if (!symbol_info.func_name.empty()) {
      res = util::log::format_to_n(buf, bufsz, "Frame #{:#02}: ({}{}) [{}]\r\n", frame_id, symbol_info.func_name,
                                   symbol_info.func_offset, symbol_info.address);
    } else {
      res = util::log::format_to_n(buf, bufsz, "Frame #{:#02}: () [{}]\r\n", frame_id, symbol_info.address);
    }

    if (res.size <= 0) {
      break;
    }

    ret += res.size;
    buf += res.size;
    bufsz -= res.size;
  }
#  else
#    ifdef UNICODE
  USES_CONVERSION;
#    endif

  log_stacktrace_com_holder<IDebugClient> dbg_cli;
  log_stacktrace_com_holder<IDebugControl> dbg_ctrl;
  log_stacktrace_com_holder<IDebugSymbols> dbg_sym;
  const char *error_msg = nullptr;
  do {
    if (S_OK != ::DebugCreate(__uuidof(IDebugClient), dbg_cli.to_pvoid_ptr())) {
      error_msg = "DebugCreate(IDebugClient) failed";
      break;
    }

    if (S_OK != dbg_cli->QueryInterface(__uuidof(IDebugControl), dbg_ctrl.to_pvoid_ptr())) {
      error_msg = "IDebugClient.QueryInterface(IDebugControl) failed";
      break;
    }

    if (S_OK != dbg_cli->AttachProcess(0, ::GetCurrentProcessId(),
                                       DEBUG_ATTACH_NONINVASIVE | DEBUG_ATTACH_NONINVASIVE_NO_SUSPEND)) {
      error_msg = "IDebugClient.AttachProcess(GetCurrentProcessId()) failed";
      break;
    }

    if (S_OK != dbg_ctrl->WaitForEvent(DEBUG_WAIT_DEFAULT, INFINITE)) {
      error_msg = "IDebugClient.WaitForEvent(DEBUG_WAIT_DEFAULT) failed";
      break;
    }

    // No cheking: QueryInterface sets the output parameter to nullptr in case of error.
    dbg_cli->QueryInterface(__uuidof(IDebugSymbols), dbg_sym.to_pvoid_ptr());

    bool try_read_sym = true;
    for (USHORT i = skip_frames; i < frames_count; ++i) {
      int frame_id = static_cast<int>(i - skip_frames);
      if (0 != options->max_frames && frame_id >= static_cast<int>(options->max_frames)) {
        break;
      }

      if (nullptr == stacks[i]) {
        break;
      }
      const ULONG64 offset = reinterpret_cast<ULONG64>(stacks[i]);

      int res = 0;
      if (try_read_sym) {
        if (!dbg_sym.is_inited()) {
          error_msg = "IDebugClient.QueryInterface(IDebugSymbols) failed";
          break;
        }

        // 先尝试用栈上的缓冲区
        TCHAR func_name[256] = {0};
        ULONG size = 0;
        bool res_get_name =
            (S_OK ==
             dbg_sym->GetNameByOffset(offset, func_name, (ULONG)(sizeof(func_name) / sizeof(func_name[0])), &size, 0));
        if (!res_get_name && size != 0) {  // 栈上的缓冲区不够再用堆内存
          std::string result;
          result.resize((size + 1) * sizeof(func_name[0]));
          res_get_name = (S_OK == dbg_sym->GetNameByOffset(offset, (PSTR)&result[0],
                                                           (ULONG)(result.size() / sizeof(func_name[0])), &size, 0));

          if (res_get_name) {
            res = UTIL_STRFUNC_SNPRINTF(buf, bufsz, "Frame #%02d: (%s) [0x%llx]\r\n", frame_id,
                                        LOG_STACKTRACE_VC_W2A(result.c_str()), static_cast<unsigned long long>(offset));
          }
        } else if (res_get_name) {
          res = UTIL_STRFUNC_SNPRINTF(buf, bufsz, "Frame #%02d: (%s) [0x%llx]\r\n", frame_id,
                                      LOG_STACKTRACE_VC_W2A(func_name), static_cast<unsigned long long>(offset));
        }

        if (!res_get_name) {
          try_read_sym = false;
        }
      }

      // 读不到符号，就只写出地址
      if (!try_read_sym) {
        res = UTIL_STRFUNC_SNPRINTF(buf, bufsz, "Frame #%02d: () [0x%llx]\r\n", frame_id,
                                    static_cast<unsigned long long>(offset));
      }

      if (res <= 0) {
        break;
      }

      ret += static_cast<size_t>(res);
      buf += res;
      bufsz -= static_cast<size_t>(res);
    }
  } while (false);

  // append error msg
  if (error_msg != nullptr) {
    size_t error_msg_len = strlen(error_msg);
    if (nullptr != buf && bufsz > error_msg_len) {
      memcpy(buf, error_msg, error_msg_len + 1);
      ret += error_msg_len;
    }
  }
#  endif
  return ret;
}
#else
LIBATFRAME_UTILS_API size_t stacktrace_write(char *buf, size_t bufsz, const stacktrace_options *) {
  const char *msg = "stacktrace disabled.";
  if (nullptr == buf || bufsz <= strlen(msg)) {
    return 0;
  }

  size_t ret = strlen(msg);
  memcpy(buf, msg, ret + 1);
  return ret;
}
#endif
}  // namespace log
LIBATFRAME_UTILS_NAMESPACE_END
