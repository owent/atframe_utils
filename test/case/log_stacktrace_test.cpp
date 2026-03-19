// Copyright 2026 atframework

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "log/log_stacktrace.h"

#include "frame/test_macros.h"

CASE_TEST(log_stacktrace, is_enabled) {
  // Just call the function, result depends on build config
  bool enabled = atfw::util::log::is_stacktrace_enabled();
  CASE_MSG_INFO() << "stacktrace enabled: " << (enabled ? "true" : "false") << std::endl;
}

CASE_TEST(log_stacktrace, lru_cache_size) {
  size_t original = atfw::util::log::get_stacktrace_lru_cache_size();

  atfw::util::log::set_stacktrace_lru_cache_size(128);
  CASE_EXPECT_EQ(128, static_cast<int>(atfw::util::log::get_stacktrace_lru_cache_size()));

  atfw::util::log::set_stacktrace_lru_cache_size(0);
  CASE_EXPECT_EQ(0, static_cast<int>(atfw::util::log::get_stacktrace_lru_cache_size()));

  // Restore
  atfw::util::log::set_stacktrace_lru_cache_size(original);
}

CASE_TEST(log_stacktrace, lru_cache_timeout) {
  std::chrono::microseconds original = atfw::util::log::get_stacktrace_lru_cache_timeout();

  atfw::util::log::set_stacktrace_lru_cache_timeout(std::chrono::microseconds(5000000));
  CASE_EXPECT_EQ(5000000, atfw::util::log::get_stacktrace_lru_cache_timeout().count());

  atfw::util::log::set_stacktrace_lru_cache_timeout(std::chrono::microseconds(0));
  CASE_EXPECT_EQ(0, atfw::util::log::get_stacktrace_lru_cache_timeout().count());

  // Restore
  atfw::util::log::set_stacktrace_lru_cache_timeout(original);
}

CASE_TEST(log_stacktrace, clear_cache) {
  // Should not crash
  atfw::util::log::clear_stacktrace_lru_cache();
}

CASE_TEST(log_stacktrace, stacktrace_write) {
  char buf[4096] = {0};
  size_t written = atfw::util::log::stacktrace_write(buf, sizeof(buf));
  // May or may not produce output depending on platform, but shouldn't crash
  CASE_MSG_INFO() << "stacktrace_write returned " << written << " bytes" << std::endl;
  if (written > 0) {
    CASE_MSG_INFO() << "stacktrace:\n" << buf << std::endl;
  }
}

CASE_TEST(log_stacktrace, stacktrace_write_with_options) {
  atfw::util::log::stacktrace_options opts;
  opts.skip_start_frames = 1;
  opts.skip_end_frames = 0;
  opts.max_frames = 10;

  char buf[4096] = {0};
  size_t written = atfw::util::log::stacktrace_write(buf, sizeof(buf), &opts);
  CASE_MSG_INFO() << "stacktrace_write with options returned " << written << " bytes" << std::endl;
}

CASE_TEST(log_stacktrace, stacktrace_write_small_buffer) {
  char buf[16] = {0};
  size_t written = atfw::util::log::stacktrace_write(buf, sizeof(buf));
  // With a very small buffer, output should be truncated but no crash
  CASE_MSG_INFO() << "small buffer stacktrace_write returned " << written << " bytes" << std::endl;
}

CASE_TEST(log_stacktrace, stacktrace_write_zero_buffer) {
  size_t written = atfw::util::log::stacktrace_write(nullptr, 0);
  CASE_EXPECT_EQ(0, static_cast<int>(written));
}

CASE_TEST(log_stacktrace, get_context) {
  if (!atfw::util::log::is_stacktrace_enabled()) {
    return;
  }

  std::vector<atfw::util::log::stacktrace_handle> handles;
  atfw::util::log::stacktrace_get_context(handles);

  CASE_MSG_INFO() << "got " << handles.size() << " stack frames" << std::endl;
  // We should have at least a few frames (this function, test framework, etc.)
  CASE_EXPECT_GT(static_cast<int>(handles.size()), 0);
}

CASE_TEST(log_stacktrace, get_context_with_options) {
  if (!atfw::util::log::is_stacktrace_enabled()) {
    return;
  }

  atfw::util::log::stacktrace_options opts;
  opts.skip_start_frames = 2;
  opts.skip_end_frames = 1;
  opts.max_frames = 5;

  std::vector<atfw::util::log::stacktrace_handle> handles;
  atfw::util::log::stacktrace_get_context(handles, &opts);

  CASE_MSG_INFO() << "got " << handles.size() << " frames with options" << std::endl;
  CASE_EXPECT_LE(static_cast<int>(handles.size()), 5);
}

CASE_TEST(log_stacktrace, parse_symbols) {
  if (!atfw::util::log::is_stacktrace_enabled()) {
    return;
  }

  std::vector<atfw::util::log::stacktrace_handle> handles;
  atfw::util::log::stacktrace_get_context(handles);

  if (handles.empty()) {
    return;
  }

  std::vector<std::shared_ptr<atfw::util::log::stacktrace_symbol>> symbols;
  atfw::util::log::stacktrace_parse_symbols(handles, symbols);

  CASE_MSG_INFO() << "parsed " << symbols.size() << " symbols from " << handles.size() << " handles" << std::endl;

  for (size_t i = 0; i < symbols.size() && i < 5; ++i) {
    if (symbols[i]) {
      CASE_MSG_INFO() << "  [" << i << "] demangle: " << symbols[i]->get_demangle_name()
                      << " raw: " << symbols[i]->get_raw_name() << " offset: " << symbols[i]->get_offset_hint()
                      << " addr: " << symbols[i]->get_address() << std::endl;
    }
  }
}

CASE_TEST(log_stacktrace, find_symbol) {
  if (!atfw::util::log::is_stacktrace_enabled()) {
    return;
  }

  std::vector<atfw::util::log::stacktrace_handle> handles;
  atfw::util::log::stacktrace_get_context(handles);

  if (handles.empty()) {
    return;
  }

  auto sym = atfw::util::log::stacktrace_find_symbol(handles[0]);
  if (sym) {
    CASE_MSG_INFO() << "found symbol: " << sym->get_demangle_name() << std::endl;
    // Symbol object should have an address
    CASE_EXPECT_NE(0, static_cast<int>(sym->get_address()));
  }
}

CASE_TEST(log_stacktrace, handle_trivial) {
#if LOG_STACKTRACE_USING_TRIVALLY_HANDLE
  atfw::util::log::stacktrace_handle h1(static_cast<uintptr_t>(0x12345));
  CASE_EXPECT_TRUE(static_cast<bool>(h1));
  CASE_EXPECT_EQ(static_cast<uintptr_t>(0x12345), h1.get_address());

  atfw::util::log::stacktrace_handle h2(static_cast<uintptr_t>(0));
  CASE_EXPECT_FALSE(static_cast<bool>(h2));

  // Copy
  atfw::util::log::stacktrace_handle h3(h1);
  CASE_EXPECT_TRUE(h1 == h3);

  // Move
  atfw::util::log::stacktrace_handle h4(std::move(h3));
  CASE_EXPECT_TRUE(h1 == h4);

  // Hash
  size_t hash1 = h1.hash_code();
  size_t hash4 = h4.hash_code();
  CASE_EXPECT_EQ(hash1, hash4);

  // Ordering
  atfw::util::log::stacktrace_handle h5(static_cast<uintptr_t>(0x12346));
  CASE_EXPECT_TRUE(h1 < h5);

  // Swap
  h1.swap(h2);
  CASE_EXPECT_FALSE(static_cast<bool>(h1));
  CASE_EXPECT_EQ(static_cast<uintptr_t>(0x12345), h2.get_address());
#else
  CASE_MSG_INFO() << "Skipped: non-trivial handle mode" << std::endl;
#endif
}

CASE_TEST(log_stacktrace, handle_in_unordered_set) {
#if LOG_STACKTRACE_USING_TRIVALLY_HANDLE
  std::unordered_set<atfw::util::log::stacktrace_handle> handle_set;
  handle_set.insert(atfw::util::log::stacktrace_handle(static_cast<uintptr_t>(0x100)));
  handle_set.insert(atfw::util::log::stacktrace_handle(static_cast<uintptr_t>(0x200)));
  handle_set.insert(atfw::util::log::stacktrace_handle(static_cast<uintptr_t>(0x100)));  // duplicate

  CASE_EXPECT_EQ(2, static_cast<int>(handle_set.size()));
#else
  CASE_MSG_INFO() << "Skipped: non-trivial handle mode" << std::endl;
#endif
}

CASE_TEST(log_stacktrace, handle_assignment) {
#if LOG_STACKTRACE_USING_TRIVALLY_HANDLE
  atfw::util::log::stacktrace_handle h1(static_cast<uintptr_t>(0xABC));
  atfw::util::log::stacktrace_handle h2(static_cast<uintptr_t>(0));

  // Copy assignment
  h2 = h1;
  CASE_EXPECT_TRUE(h1 == h2);

  // Move assignment
  atfw::util::log::stacktrace_handle h3(static_cast<uintptr_t>(0));
  h3 = std::move(h2);
  CASE_EXPECT_TRUE(h1 == h3);
#else
  CASE_MSG_INFO() << "Skipped: non-trivial handle mode" << std::endl;
#endif
}
