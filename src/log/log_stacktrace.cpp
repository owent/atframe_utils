// Copyright 2021 atframework
// Licensed under the MIT licenses.
// Created by owent on 2015-06-29

#include "log/log_stacktrace.h"

#include <mutex>
#include <string>
#include <vector>

#include "design_pattern/singleton.h"
#include "memory/lru_map.h"

#include "common/demangle.h"
#include "common/string_oprs.h"
#include "log/log_wrapper.h"

#ifndef ATFRAMEWORK_UTILS_LOG_STACKTRACE_MAX_STACKS
#  define ATFRAMEWORK_UTILS_LOG_STACKTRACE_MAX_STACKS 256
#endif

#define LOG_STACKTRACE_MAX_STACKS_ARRAY_SIZE (ATFRAMEWORK_UTILS_LOG_STACKTRACE_MAX_STACKS + 1)

// disable some warnings in msvc's headers
#if defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable : 4091)
#endif

// select method to stacktrace
#if defined(ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_LIBUNWIND) && ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_LIBUNWIND
#  include <libunwind.h>

#elif defined(ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_EXECINFO) && ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_EXECINFO
#  include <execinfo.h>

#elif defined(ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_UNWIND) && ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_UNWIND
#  include <unwind.h>

#elif defined(ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_DBGHELP) && ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_DBGHELP
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

#elif defined(ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_DBGENG) && ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_DBGENG
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

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace log {

class ATFW_UTIL_SYMBOL_LOCAL stacktrace_symbol_impl : public stacktrace_symbol {
 public:
  stacktrace_symbol_impl(uintptr_t address, std::string demangle_name, std::string raw_name, std::string offset_hint,
                         std::chrono::system_clock::time_point timeout) noexcept
      : address_(address),
        demangle_name_(std::move(demangle_name)),
        raw_name_(std::move(raw_name)),
        offset_hint_(std::move(offset_hint)),
        timeout_(timeout) {}

  ~stacktrace_symbol_impl() override {}

  nostd::string_view get_demangle_name() const noexcept override { return demangle_name_; }

  nostd::string_view get_raw_name() const noexcept override { return raw_name_; }

  nostd::string_view get_offset_hint() const noexcept override { return offset_hint_; }

  uintptr_t get_address() const noexcept override { return address_; }

  std::chrono::system_clock::time_point get_timeout() const noexcept override { return timeout_; }

 private:
  uintptr_t address_;
  std::string demangle_name_;
  std::string raw_name_;
  std::string offset_hint_;
  std::chrono::system_clock::time_point timeout_;
};

namespace {

struct ATFW_UTIL_SYMBOL_LOCAL stacktrace_global_settings {
  bool need_clear = false;
  size_t lru_cache_size = 300000;
  std::chrono::microseconds lru_cache_timeout =
      std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::seconds{14400});
  inline stacktrace_global_settings() noexcept {}
};

struct ATFW_UTIL_SYMBOL_LOCAL stacktrace_global_manager
    : public ATFRAMEWORK_UTILS_NAMESPACE_ID::design_pattern::local_singleton<stacktrace_global_manager> {
  std::mutex lock;
  memory::lru_map<stacktrace_handle, stacktrace_symbol> stack_caches;
};

static const stacktrace_options &default_stacktrace_options() {
  static stacktrace_options opts = {0, 1, 0};
  return opts;
}

static stacktrace_global_settings &get_stacktrace_settings() {
  static stacktrace_global_settings instance;
  return instance;
}

static ATFW_UTIL_SANITIZER_NO_THREAD void internal_set_stacktrace_lru_cache_size(size_t sz) noexcept {
  get_stacktrace_settings().lru_cache_size = sz;
}

static ATFW_UTIL_SANITIZER_NO_THREAD size_t internal_get_stacktrace_lru_cache_size() noexcept {
  return get_stacktrace_settings().lru_cache_size;
}

static ATFW_UTIL_SANITIZER_NO_THREAD void internal_set_stacktrace_lru_cache_timeout(
    std::chrono::microseconds timeout) noexcept {
  get_stacktrace_settings().lru_cache_timeout = timeout;
}

static ATFW_UTIL_SANITIZER_NO_THREAD std::chrono::microseconds internal_get_stacktrace_lru_cache_timeout() noexcept {
  return get_stacktrace_settings().lru_cache_timeout;
}

static ATFW_UTIL_SANITIZER_NO_THREAD void internal_set_clear_stacktrace_lru_cache(bool v) noexcept {
  get_stacktrace_settings().need_clear = v;
}

static ATFW_UTIL_SANITIZER_NO_THREAD bool internal_get_clear_stacktrace_lru_cache() noexcept {
  return get_stacktrace_settings().need_clear;
}

static std::shared_ptr<stacktrace_symbol> internal_find_stacktrace_symbol(const stacktrace_handle &handle) {
  if (stacktrace_global_manager::is_instance_destroyed()) {
    return nullptr;
  }

  std::lock_guard<std::mutex> lock_guard{stacktrace_global_manager::me()->lock};
  auto &stack_caches = stacktrace_global_manager::me()->stack_caches;

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
      if (now > stack_caches.front().second->get_timeout()) {
        stack_caches.pop_front();
      } else {
        break;
      }
    }
  }

  auto iter = stack_caches.find(handle, false);
  if (iter != stack_caches.end()) {
    return iter->second;
  }

  return nullptr;
}

static void internal_replace_stacktrace_symbol(
    gsl::span<std::pair<stacktrace_handle, std::shared_ptr<stacktrace_symbol>>> values) {
  if (values.empty()) {
    return;
  }

  if (stacktrace_global_manager::is_instance_destroyed()) {
    return;
  }

  std::lock_guard<std::mutex> lock_guard{stacktrace_global_manager::me()->lock};
  auto &stack_caches = stacktrace_global_manager::me()->stack_caches;

  if (internal_get_clear_stacktrace_lru_cache()) {
    stack_caches.clear();
    internal_set_clear_stacktrace_lru_cache(false);
  }

  for (auto &val : values) {
    if (val.first && val.second) {
      stack_caches.insert_key_value(val.first, val.second);
    }
  }

  // Append into LRU cache
  size_t lru_cache_size_limit = get_stacktrace_lru_cache_size();
  while (stack_caches.size() > lru_cache_size_limit) {
    stack_caches.pop_front();
  }
}
}  // namespace

#if LOG_STACKTRACE_USING_TRIVALLY_HANDLE
ATFRAMEWORK_UTILS_API stacktrace_handle::stacktrace_handle(uintptr_t address) noexcept : address_(address) {}
#else
ATFRAMEWORK_UTILS_API stacktrace_handle::stacktrace_handle(
    const ATFRAMEWORK_UTILS_NAMESPACE_ID::memory::strong_rc_ptr<impl_type> &impl) noexcept
    : impl_(impl) {}

ATFRAMEWORK_UTILS_API stacktrace_handle::stacktrace_handle(
    ATFRAMEWORK_UTILS_NAMESPACE_ID::memory::strong_rc_ptr<impl_type> &&impl) noexcept
    : impl_(std::move(impl)) {}
#endif

ATFRAMEWORK_UTILS_API stacktrace_handle::~stacktrace_handle() {}

ATFRAMEWORK_UTILS_API stacktrace_handle::stacktrace_handle(const stacktrace_handle &other) noexcept
    :
#if LOG_STACKTRACE_USING_TRIVALLY_HANDLE
      address_(other.address_)
#else
      impl_(other.impl_)
#endif
{
}

ATFRAMEWORK_UTILS_API stacktrace_handle::stacktrace_handle(stacktrace_handle &&other) noexcept
    :
#if LOG_STACKTRACE_USING_TRIVALLY_HANDLE
      address_(other.address_)
#else
      impl_(std::move(other.impl_))
#endif
{
}

ATFRAMEWORK_UTILS_API stacktrace_handle &stacktrace_handle::operator=(const stacktrace_handle &other) noexcept {
#if LOG_STACKTRACE_USING_TRIVALLY_HANDLE
  address_ = other.address_;
#else
  impl_ = other.impl_;
#endif

  return *this;
}

ATFRAMEWORK_UTILS_API stacktrace_handle &stacktrace_handle::operator=(stacktrace_handle &&other) noexcept {
#if LOG_STACKTRACE_USING_TRIVALLY_HANDLE
  address_ = other.address_;
  other.address_ = 0;
#else
  impl_ = std::move(other.impl_);
#endif

  return *this;
}

ATFRAMEWORK_UTILS_API stacktrace_handle::operator bool() const noexcept {
#if LOG_STACKTRACE_USING_TRIVALLY_HANDLE
  return 0 != address_;
#else
  return !!impl_;
#endif
}

ATFRAMEWORK_UTILS_API void stacktrace_handle::swap(stacktrace_handle &other) noexcept {
#if LOG_STACKTRACE_USING_TRIVALLY_HANDLE
  std::swap(address_, other.address_);
#else
  impl_.swap(other.impl_);
#endif
}

#if LOG_STACKTRACE_USING_TRIVALLY_HANDLE
ATFRAMEWORK_UTILS_API size_t stacktrace_handle::hash_code() const noexcept { return std::hash<uintptr_t>()(address_); }

ATFRAMEWORK_UTILS_API bool stacktrace_handle::operator==(const stacktrace_handle &other) const noexcept {
  return address_ == other.address_;
}

ATFRAMEWORK_UTILS_API bool stacktrace_handle::operator<(const stacktrace_handle &other) const noexcept {
  return address_ < other.address_;
}
#endif

#if defined(ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_LIBUNWIND) && ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_LIBUNWIND
struct stacktrace_handle::impl_type {
  unw_word_t unw_word;
  std::shared_ptr<stacktrace_symbol> symbol;
};

ATFRAMEWORK_UTILS_API size_t stacktrace_handle::hash_code() const noexcept {
  if (!impl_) {
    return 0;
  }

  return std::hash<unw_word_t>()(impl_->unw_word);
}

ATFRAMEWORK_UTILS_API bool stacktrace_handle::operator==(const stacktrace_handle &other) const noexcept {
  if (impl_ == other.impl_) {
    return true;
  }

  if (!impl_ != !other.impl_) {
    return false;
  }

  return impl_->unw_word == other.impl_->unw_word;
}

ATFRAMEWORK_UTILS_API bool stacktrace_handle::operator<(const stacktrace_handle &other) const noexcept {
  if (impl_ == other.impl_) {
    return false;
  }

  if (!impl_ != !other.impl_) {
    return !impl_;
  }

  return impl_->unw_word < other.impl_->unw_word;
}
#endif

namespace {

#if defined(ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_LIBUNWIND) && ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_LIBUNWIND
static stacktrace_handle unw_mutable_symbol_from_cache(unw_word_t key, unw_cursor_t &unw_cur) {
  auto handle_impl = ATFRAMEWORK_UTILS_NAMESPACE_ID::memory::make_strong_rc<stacktrace_handle::impl_type>();
  handle_impl->unw_word = key;
  stacktrace_handle handle{handle_impl};
  handle_impl->symbol = internal_find_stacktrace_symbol(handle);
  if (handle_impl->symbol) {
    return handle;
  }

  std::vector<char> func_name_cache;
  func_name_cache.resize(16384, 0);
  unw_word_t unw_offset;

  unw_get_proc_name(&unw_cur, &func_name_cache[0], func_name_cache.size() - 1, &unw_offset);

  auto symbol = std::make_shared<stacktrace_symbol_impl>(
      static_cast<uintptr_t>(key), demangle(func_name_cache.data()), func_name_cache.data(),
      ATFRAMEWORK_UTILS_NAMESPACE_ID::string::format("+{:#x}", unw_offset),
      std::chrono::system_clock::now() + internal_get_stacktrace_lru_cache_timeout());
  handle_impl->symbol = std::static_pointer_cast<stacktrace_symbol>(symbol);

  std::pair<stacktrace_handle, std::shared_ptr<stacktrace_symbol>> data[1] = {
      std::pair<stacktrace_handle, std::shared_ptr<stacktrace_symbol>>{handle, handle_impl->symbol}};
  internal_replace_stacktrace_symbol(gsl::make_span(data));

  return handle;
}

#elif defined(ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_EXECINFO) && ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_EXECINFO

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

static std::shared_ptr<stacktrace_symbol> stacktrace_create_symbol(const stacktrace_handle &handle, const char *name) {
  if (nullptr == name || 0 == *name) {
    return nullptr;
  }

  std::string module_name;
  std::string raw_name;
  std::string offset_hint;

  name = stacktrace_skip_space(name);
  while (name) {
    const char *start;
    char previous_c;
    bool clear_sym = false;

    if (stacktrace_pick_ident(name, start, name, previous_c, clear_sym)) {
      if (stacktrace_is_number_char(*start)) {
        if (clear_sym) {
          raw_name.clear();
        }

        if ('+' == previous_c) {
          offset_hint = "+";
          offset_hint.insert(offset_hint.end(), start, name);
          stacktrace_fix_number(offset_hint);
        }
      } else {
        if (module_name.empty()) {
          module_name.assign(start, name);
        } else {
          raw_name.assign(start, name);
        }
      }
    } else {
      if (clear_sym) {
        raw_name.clear();
      }
      break;
    }

    name = stacktrace_skip_space(name);
  }
  if (raw_name.empty()) {
    return nullptr;
  }

  auto symbol = std::make_shared<stacktrace_symbol_impl>(
      handle.get_address(), demangle(raw_name.c_str()), raw_name, offset_hint,
      std::chrono::system_clock::now() + internal_get_stacktrace_lru_cache_timeout());
  return std::static_pointer_cast<stacktrace_symbol>(symbol);
}

static void stacktrace_fill_symbol_info(gsl::span<stacktrace_handle> stack_handles,
                                        std::vector<std::shared_ptr<stacktrace_symbol>> &symbols) {
  if (stack_handles.empty()) {
    return;
  }

  std::vector<std::pair<stacktrace_handle, std::shared_ptr<stacktrace_symbol>>> new_handle_pairs;
  std::vector<size_t> parse_symbols_idx;
  std::vector<void *> parse_symbols_ptr;
  parse_symbols_idx.reserve((stack_handles.size() + 1) / 2);
  parse_symbols_ptr.reserve((stack_handles.size() + 1) / 2);

  for (auto &handle : stack_handles) {
    std::shared_ptr<stacktrace_symbol> symbol = internal_find_stacktrace_symbol(handle);
    if (!symbol) {
      parse_symbols_idx.push_back(symbols.size());
      parse_symbols_ptr.push_back(reinterpret_cast<void *>(handle.get_address()));  // NOLINT(performance-no-int-to-ptr)
      new_handle_pairs.push_back(std::pair<stacktrace_handle, std::shared_ptr<stacktrace_symbol>>{handle, nullptr});
    }

    symbols.emplace_back(std::move(symbol));
  }

  // Second step: load from symbol
  if (!parse_symbols_ptr.empty()) {
    char **func_name_cache = backtrace_symbols(parse_symbols_ptr.data(), static_cast<int>(parse_symbols_ptr.size()));
    for (size_t i = 0; nullptr != func_name_cache && i < parse_symbols_ptr.size() && i < parse_symbols_idx.size() &&
                       i < new_handle_pairs.size();
         ++i) {
      symbols[parse_symbols_idx[i]] = stacktrace_create_symbol(new_handle_pairs[i].first, func_name_cache[i]);
      new_handle_pairs[i].second = symbols[parse_symbols_idx[i]];
    }
  }

  internal_replace_stacktrace_symbol(gsl::make_span(new_handle_pairs));
}

#elif defined(ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_UNWIND) && ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_UNWIND

static void stacktrace_fill_symbol_info(gsl::span<stacktrace_handle> stack_handles,
                                        std::vector<std::shared_ptr<stacktrace_symbol>> &symbols) {
  if (stack_handles.empty()) {
    return;
  }

  std::vector<std::pair<stacktrace_handle, std::shared_ptr<stacktrace_symbol>>> new_handle_pairs;
  std::vector<size_t> parse_symbols_idx;
  parse_symbols_idx.reserve((stack_handles.size() + 1) / 2);

  for (auto &handle : stack_handles) {
    std::shared_ptr<stacktrace_symbol> symbol = internal_find_stacktrace_symbol(handle);
    if (!symbol) {
      parse_symbols_idx.push_back(symbols.size());
      new_handle_pairs.push_back(std::pair<stacktrace_handle, std::shared_ptr<stacktrace_symbol>>{handle, nullptr});
    }

    symbols.emplace_back(std::move(symbol));
  }

  // Second step: load from symbol
  if (!new_handle_pairs.empty()) {
    for (size_t i = 0; i < parse_symbols_idx.size() && i < new_handle_pairs.size(); ++i) {
      std::string fake_name =
          ATFRAMEWORK_UTILS_NAMESPACE_ID::string::format("{:#x}", new_handle_pairs[i].first.get_address());
      std::shared_ptr<stacktrace_symbol> symbol =
          std::static_pointer_cast<stacktrace_symbol>(std::make_shared<stacktrace_symbol_impl>(
              new_handle_pairs[i].first.get_address(), fake_name, fake_name, "",
              std::chrono::system_clock::now() + internal_get_stacktrace_lru_cache_timeout()));
      symbols[parse_symbols_idx[i]] = symbol;
      new_handle_pairs[i].second = symbol;
    }
  }

  internal_replace_stacktrace_symbol(gsl::make_span(new_handle_pairs));
}

#elif (defined(ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_DBGHELP) && ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_DBGHELP) || \
    (defined(ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_DBGENG) && ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_DBGENG)
#  if !defined(_MSC_VER)
static void stacktrace_fill_symbol_info(gsl::span<stacktrace_handle> stack_handles,
                                        std::vector<std::shared_ptr<stacktrace_symbol>> &symbols) {
  if (stack_handles.empty()) {
    return;
  }

  std::vector<std::pair<stacktrace_handle, std::shared_ptr<stacktrace_symbol>>> new_handle_pairs;
  std::vector<size_t> parse_symbols_idx;
  parse_symbols_idx.reserve((stack_handles.size() + 1) / 2);

  for (auto &handle : stack_handles) {
    std::shared_ptr<stacktrace_symbol> symbol = internal_find_stacktrace_symbol(handle);
    if (!symbol) {
      parse_symbols_idx.push_back(symbols.size());
      new_handle_pairs.push_back(std::pair<stacktrace_handle, std::shared_ptr<stacktrace_symbol>>{handle, nullptr});
    }

    symbols.emplace_back(std::move(symbol));
  }

  // Second step: load from symbol
  if (!new_handle_pairs.empty()) {
    for (size_t i = 0; i < parse_symbols_idx.size() && i < new_handle_pairs.size(); ++i) {
      std::string fake_name =
          ATFRAMEWORK_UTILS_NAMESPACE_ID::string::format("{:#x}", new_handle_pairs[i].first.get_address());
      std::shared_ptr<stacktrace_symbol> symbol =
          std::static_pointer_cast<stacktrace_symbol>(std::make_shared<stacktrace_symbol_impl>(
              new_handle_pairs[i].first.get_address(), fake_name, fake_name, "",
              std::chrono::system_clock::now() + internal_get_stacktrace_lru_cache_timeout()));
      symbols[parse_symbols_idx[i]] = symbol;
      new_handle_pairs[i].second = symbol;
    }
  }

  internal_replace_stacktrace_symbol(gsl::make_span(new_handle_pairs));
}
#  elif (defined(ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_DBGHELP) && ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_DBGHELP)
struct ATFW_UTIL_SYMBOL_LOCAL stacktrace_symbol_group_t {
  std::string demangle_name;
  std::string raw_name;
  std::string offset_hint;

  std::chrono::system_clock::time_point timeout;
};

struct ATFW_UTIL_SYMBOL_LOCAL dbghelper_cache_key_hash {
  size_t operator()(const std::pair<HANDLE, PVOID> &key) const noexcept {
    size_t result = std::hash<HANDLE>()(key.first);
    result ^= std::hash<PVOID>()(key.second) + 0x9e3779b9 + (result << 6) + (result >> 2);
    return result;
  }
};

static stacktrace_symbol_group_t dbghelper_mutable_symbol_from_cache(HANDLE process, PVOID stack) {
  std::pair<HANDLE, PVOID> key{process, stack};

#    ifdef UNICODE
  USES_CONVERSION;
#    endif

  SYMBOL_INFO *symbol;
  DWORD64 displacement = 0;
  stacktrace_symbol_group_t result;
  result.timeout = std::chrono::system_clock::now() + internal_get_stacktrace_lru_cache_timeout();

  symbol = reinterpret_cast<SYMBOL_INFO *>(malloc(sizeof(SYMBOL_INFO) + (MAX_SYM_NAME + 1) * sizeof(TCHAR)));
  symbol->MaxNameLen = MAX_SYM_NAME;
  symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

  if (SymFromAddr(SymInitializeHelper::Inst().process, reinterpret_cast<ULONG64>(stack), &displacement, symbol)) {
    result.raw_name = LOG_STACKTRACE_VC_W2A(symbol->Name);
    result.offset_hint = ATFRAMEWORK_UTILS_NAMESPACE_ID::string::format("+{:#x}", displacement);
    result.demangle_name = demangle(result.raw_name.c_str());
  } else {
    result.raw_name = ATFRAMEWORK_UTILS_NAMESPACE_ID::string::format("{}", reinterpret_cast<const void *>(stack));
    result.demangle_name = demangle(result.raw_name.c_str());
  }
  free(symbol);

  return result;
}

static void stacktrace_fill_symbol_info(gsl::span<stacktrace_handle> stack_handles,
                                        std::vector<std::shared_ptr<stacktrace_symbol>> &symbols) {
  if (stack_handles.empty()) {
    return;
  }

  std::vector<std::pair<stacktrace_handle, std::shared_ptr<stacktrace_symbol>>> new_handle_pairs;
  std::vector<size_t> parse_symbols_idx;
  parse_symbols_idx.reserve((stack_handles.size() + 1) / 2);

  for (auto &handle : stack_handles) {
    std::shared_ptr<stacktrace_symbol> symbol = internal_find_stacktrace_symbol(handle);
    if (!symbol) {
      parse_symbols_idx.push_back(symbols.size());
      new_handle_pairs.push_back(std::pair<stacktrace_handle, std::shared_ptr<stacktrace_symbol>>{handle, nullptr});
    }

    symbols.emplace_back(std::move(symbol));
  }

  // Second step: load from symbol
  if (!new_handle_pairs.empty()) {
    for (size_t i = 0; i < parse_symbols_idx.size() && i < new_handle_pairs.size(); ++i) {
      stacktrace_symbol_group_t symbol_group = dbghelper_mutable_symbol_from_cache(
          SymInitializeHelper::Inst().process, reinterpret_cast<PVOID>(new_handle_pairs[i].first.get_address()));
      std::shared_ptr<stacktrace_symbol> symbol =
          std::static_pointer_cast<stacktrace_symbol>(std::make_shared<stacktrace_symbol_impl>(
              new_handle_pairs[i].first.get_address(), symbol_group.raw_name, symbol_group.raw_name,
              symbol_group.offset_hint,
              std::chrono::system_clock::now() + internal_get_stacktrace_lru_cache_timeout()));
      symbols[parse_symbols_idx[i]] = symbol;
      new_handle_pairs[i].second = symbol;
    }
  }

  internal_replace_stacktrace_symbol(gsl::make_span(new_handle_pairs));
}

#  else

static void stacktrace_fill_symbol_info(gsl::span<stacktrace_handle> stack_handles,
                                        std::vector<std::shared_ptr<stacktrace_symbol>> &symbols) {
  if (stack_handles.empty()) {
    return;
  }

  std::vector<std::pair<stacktrace_handle, std::shared_ptr<stacktrace_symbol>>> new_handle_pairs;
  std::vector<size_t> parse_symbols_idx;
  parse_symbols_idx.reserve((stack_handles.size() + 1) / 2);

  for (auto &handle : stack_handles) {
    std::shared_ptr<stacktrace_symbol> symbol = internal_find_stacktrace_symbol(handle);
    if (!symbol) {
      parse_symbols_idx.push_back(symbols.size());
      new_handle_pairs.push_back(std::pair<stacktrace_handle, std::shared_ptr<stacktrace_symbol>>{handle, nullptr});
    }

    symbols.emplace_back(std::move(symbol));
  }

  if (new_handle_pairs.empty()) {
    return;
  }

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

    if (!dbg_sym.is_inited()) {
      error_msg = "IDebugClient.QueryInterface(IDebugSymbols) failed";
    }
  } while (false);

  if (nullptr != error_msg) {
    std::shared_ptr<stacktrace_symbol> symbol =
        std::static_pointer_cast<stacktrace_symbol>(std::make_shared<stacktrace_symbol_impl>(
            0, error_msg, error_msg, "",
            std::chrono::system_clock::now() + internal_get_stacktrace_lru_cache_timeout()));
    symbols.emplace_back(std::move(symbol));
    return;
  }

  // Second step: load from symbol
  if (!new_handle_pairs.empty()) {
    bool try_read_sym = true;
    for (size_t i = 0; i < parse_symbols_idx.size() && i < new_handle_pairs.size(); ++i) {
      const ULONG64 offset = static_cast<ULONG64>(new_handle_pairs[i].first.get_address());

      std::string raw_symbol_name;
      if (try_read_sym) {
        // 先尝试用栈上的缓冲区
        TCHAR raw_name_buffer[4000] = {0};
        ULONG size = 0;
        bool res_get_name =
            (S_OK == dbg_sym->GetNameByOffset(offset, raw_name_buffer,
                                              (ULONG)(sizeof(raw_name_buffer) / sizeof(raw_name_buffer[0])), &size, 0));
        if (!res_get_name && size != 0) {  // 栈上的缓冲区不够再用堆内存
          std::string result;
          result.resize((size + 1) * sizeof(raw_name_buffer[0]));
          res_get_name =
              (S_OK == dbg_sym->GetNameByOffset(offset, (PSTR)&result[0],
                                                (ULONG)(result.size() / sizeof(raw_name_buffer[0])), &size, 0));

          if (res_get_name) {
            raw_symbol_name = LOG_STACKTRACE_VC_W2A(result.c_str());
          }
        } else if (res_get_name) {
          raw_symbol_name = LOG_STACKTRACE_VC_W2A(raw_name_buffer);
        }

        if (!res_get_name) {
          try_read_sym = false;
        }
      }

      // 读不到符号，就只写出地址
      if (!try_read_sym) {
        raw_symbol_name = ATFRAMEWORK_UTILS_NAMESPACE_ID::string::format("{:#x}", offset);
      }

      std::shared_ptr<stacktrace_symbol> symbol =
          std::static_pointer_cast<stacktrace_symbol>(std::make_shared<stacktrace_symbol_impl>(
              new_handle_pairs[i].first.get_address(), demangle(raw_symbol_name.c_str()), raw_symbol_name, "",
              std::chrono::system_clock::now() + internal_get_stacktrace_lru_cache_timeout()));
      symbols[parse_symbols_idx[i]] = symbol;
      new_handle_pairs[i].second = symbol;
    }
  }

  internal_replace_stacktrace_symbol(gsl::make_span(new_handle_pairs));
}

#  endif
#endif

}  // namespace

ATFRAMEWORK_UTILS_API stacktrace_symbol::stacktrace_symbol() {}

ATFRAMEWORK_UTILS_API stacktrace_symbol::~stacktrace_symbol() {}

ATFRAMEWORK_UTILS_API bool is_stacktrace_enabled() noexcept {
#if defined(ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_LIBUNWIND) && ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_LIBUNWIND
  return true;
#elif defined(ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_EXECINFO) && ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_EXECINFO
  return true;
#elif defined(ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_UNWIND) && ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_UNWIND
  return true;
#elif defined(ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_DBGHELP) && ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_DBGHELP
  return true;
#elif defined(ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_DBGENG) && ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_DBGENG
  return true;
#else
  return false;
#endif
}

ATFRAMEWORK_UTILS_API void set_stacktrace_lru_cache_size(size_t sz) noexcept {
  internal_set_stacktrace_lru_cache_size(sz);
}

ATFRAMEWORK_UTILS_API size_t get_stacktrace_lru_cache_size() noexcept {
  return internal_get_stacktrace_lru_cache_size();
}

ATFRAMEWORK_UTILS_API void set_stacktrace_lru_cache_timeout(std::chrono::microseconds timeout) noexcept {
  internal_set_stacktrace_lru_cache_timeout(timeout);
}

ATFRAMEWORK_UTILS_API std::chrono::microseconds get_stacktrace_lru_cache_timeout() noexcept {
  return internal_get_stacktrace_lru_cache_timeout();
}

ATFRAMEWORK_UTILS_API void clear_stacktrace_lru_cache() noexcept { internal_set_clear_stacktrace_lru_cache(true); }

#if defined(ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_LIBUNWIND) && ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_LIBUNWIND

ATFRAMEWORK_UTILS_API void stacktrace_get_context(std::vector<stacktrace_handle> &stack_handles,
                                                  const stacktrace_options *options) noexcept {
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
  int frames_count = ATFRAMEWORK_UTILS_LOG_STACKTRACE_MAX_STACKS;

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

  while (true) {
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
      stacktrace_handle handle = unw_mutable_symbol_from_cache(unw_proc.start_ip, unw_cur);
      if (handle) {
        stack_handles.emplace_back(std::move(handle));
      }
    }

    if (unw_step(&unw_cur) <= 0) {
      break;
    }

    if (skip_frames > 0) {
      --skip_frames;
    } else {
      ++frame_id;
    }
  }
}

ATFRAMEWORK_UTILS_API void stacktrace_parse_symbols(gsl::span<stacktrace_handle> stack_handles,
                                                    std::vector<std::shared_ptr<stacktrace_symbol>> &symbols) noexcept {
  symbols.reserve(symbols.size() + stack_handles.size());
  for (auto &handle : stack_handles) {
    if (handle.get_internal_impl() && handle.get_internal_impl()->symbol) {
      symbols.push_back(handle.get_internal_impl()->symbol);
    }
  }
}

#elif defined(ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_EXECINFO) && ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_EXECINFO

ATFRAMEWORK_UTILS_API void stacktrace_get_context(std::vector<stacktrace_handle> &stack_handles,
                                                  const stacktrace_options *options) noexcept {
  if (nullptr == options) {
    options = &default_stacktrace_options();
  }

  void *stacks[LOG_STACKTRACE_MAX_STACKS_ARRAY_SIZE] = {nullptr};
  size_t frames_count = static_cast<size_t>(backtrace(stacks, LOG_STACKTRACE_MAX_STACKS_ARRAY_SIZE));
  size_t skip_frames = 1 + static_cast<size_t>(options->skip_start_frames);

  if (frames_count <= skip_frames + options->skip_end_frames) {
    frames_count = 0;
  } else if (options->skip_end_frames > 0) {
    frames_count -= options->skip_end_frames;
  }

  for (size_t i = skip_frames; i < frames_count; i++) {
    size_t frame_id = i - skip_frames;
    if (0 != options->max_frames && frame_id >= options->max_frames) {
      break;
    }

    if (nullptr == stacks[i] || 0x01 == reinterpret_cast<intptr_t>(stacks[i])) {
      break;
    }

    stack_handles.emplace_back(stacktrace_handle{reinterpret_cast<uintptr_t>(stacks[i])});
  }
}

ATFRAMEWORK_UTILS_API void stacktrace_parse_symbols(gsl::span<stacktrace_handle> stack_handles,
                                                    std::vector<std::shared_ptr<stacktrace_symbol>> &symbols) noexcept {
  symbols.reserve(symbols.size() + stack_handles.size());
  stacktrace_fill_symbol_info(stack_handles, symbols);
}

#elif defined(ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_UNWIND) && ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_UNWIND
struct ATFW_UTIL_SYMBOL_LOCAL stacktrace_unwind_state_t {
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

ATFRAMEWORK_UTILS_API void stacktrace_get_context(std::vector<stacktrace_handle> &stack_handles,
                                                  const stacktrace_options *options) noexcept {
  if (nullptr == options) {
    options = &default_stacktrace_options();
  }

  _Unwind_Word stacks[LOG_STACKTRACE_MAX_STACKS_ARRAY_SIZE];
  stacktrace_unwind_state_t state;
  state.frames_to_skip = 0;
  state.current = stacks;
  state.end = stacks + LOG_STACKTRACE_MAX_STACKS_ARRAY_SIZE;

  ::_Unwind_Backtrace(&stacktrace_unwind_callback, &state);
  size_t frames_count = static_cast<size_t>(state.current - &stacks[0]);
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

    stack_handles.emplace_back(stacktrace_handle{stacks[i]});
  }
}

ATFRAMEWORK_UTILS_API void stacktrace_parse_symbols(gsl::span<stacktrace_handle> stack_handles,
                                                    std::vector<std::shared_ptr<stacktrace_symbol>> &symbols) noexcept {
  symbols.reserve(symbols.size() + stack_handles.size());
  stacktrace_fill_symbol_info(stack_handles, symbols);
}

#elif (defined(ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_DBGHELP) && ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_DBGHELP) || \
    (defined(ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_DBGENG) && ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_DBGENG)

ATFRAMEWORK_UTILS_API void stacktrace_get_context(std::vector<stacktrace_handle> &stack_handles,
                                                  const stacktrace_options *options) noexcept {
  if (nullptr == options) {
    options = &default_stacktrace_options();
  }

  PVOID stacks[LOG_STACKTRACE_MAX_STACKS_ARRAY_SIZE];
  USHORT frames_count = CaptureStackBackTrace(0, LOG_STACKTRACE_MAX_STACKS_ARRAY_SIZE, stacks, nullptr);

  USHORT skip_frames = 1 + static_cast<USHORT>(options->skip_start_frames);

  if (frames_count <= skip_frames + static_cast<USHORT>(options->skip_end_frames)) {
    frames_count = 0;
  } else if (options->skip_end_frames > 0) {
    frames_count -= static_cast<USHORT>(options->skip_end_frames);
  }

  for (USHORT i = skip_frames; i < frames_count; ++i) {
    USHORT frame_id = i - skip_frames;

    if (0 != options->max_frames && frame_id >= static_cast<USHORT>(options->max_frames)) {
      break;
    }

    if (nullptr == stacks[i]) {
      break;
    }

    stack_handles.emplace_back(stacktrace_handle{reinterpret_cast<uintptr_t>(stacks[i])});
  }
}

ATFRAMEWORK_UTILS_API void stacktrace_parse_symbols(gsl::span<stacktrace_handle> stack_handles,
                                                    std::vector<std::shared_ptr<stacktrace_symbol>> &symbols) noexcept {
  symbols.reserve(symbols.size() + stack_handles.size());
  stacktrace_fill_symbol_info(stack_handles, symbols);
}
#else
ATFRAMEWORK_UTILS_API void stacktrace_get_context(std::vector<stacktrace_handle> &,
                                                  const stacktrace_options *) noexcept {}

ATFRAMEWORK_UTILS_API void stacktrace_parse_symbols(gsl::span<stacktrace_handle> stack_handles,
                                                    std::vector<std::shared_ptr<stacktrace_symbol>> &symbols) noexcept {
  if (stack_handles.empty()) {
    return;
  }

  std::shared_ptr<stacktrace_symbol> symbol = internal_find_stacktrace_symbol(stacktrace_handle{0});
  if (!symbol) {
    symbol = std::static_pointer_cast<stacktrace_symbol>(std::make_shared<stacktrace_symbol_impl>(
        0, "stacktrace disabled.", "stacktrace disabled.", "",
        std::chrono::system_clock::now() + internal_get_stacktrace_lru_cache_timeout()));
    std::pair<stacktrace_handle, std::shared_ptr<stacktrace_symbol>> new_handle_pairs{stacktrace_handle{0}, symbol};
    internal_replace_stacktrace_symbol(gsl::make_span(new_handle_pairs));
  }

  symbols.push_back(symbol);
}
#endif

ATFRAMEWORK_UTILS_API std::shared_ptr<stacktrace_symbol> stacktrace_find_symbol(
    stacktrace_handle stack_handle) noexcept {
  auto ret = internal_find_stacktrace_symbol(stack_handle);
  if (ret) {
    return ret;
  }

  std::vector<std::shared_ptr<stacktrace_symbol>> out;
  stacktrace_parse_symbols(gsl::span<stacktrace_handle>{&stack_handle, 1}, out);
  if (!out.empty()) {
    return *out.begin();
  }

  return nullptr;
}

ATFRAMEWORK_UTILS_API size_t stacktrace_write(char *buf, size_t bufsz, const stacktrace_options *options) {
  if (nullptr == buf || bufsz <= 0) {
    return 0;
  }

  std::vector<stacktrace_handle> stack_handles;
  stack_handles.reserve(64);

  stacktrace_get_context(stack_handles, options);
  std::vector<std::shared_ptr<stacktrace_symbol>> symbols;
  stacktrace_parse_symbols(gsl::make_span(stack_handles), symbols);

  int frame_id = 0;
  size_t ret = 0;
  for (auto &symbol : symbols) {
    if (!symbol) {
      ++frame_id;
      continue;
    }

    auto res = ATFRAMEWORK_UTILS_NAMESPACE_ID::string::format_to_n(buf, bufsz, "Frame #{:#03}: ({}{}) [{:#x}]\r\n",
                                                                   frame_id, symbol->get_demangle_name(),
                                                                   symbol->get_offset_hint(), symbol->get_address());

    if (res.size <= 0) {
      break;
    }

    ret += static_cast<size_t>(res.size);
    buf += res.size;
    bufsz -= static_cast<size_t>(res.size);

    ++frame_id;
  }

  return ret;
}

}  // namespace log
ATFRAMEWORK_UTILS_NAMESPACE_END
