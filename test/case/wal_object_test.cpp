// Copyright 2021 atframework

#include <algorithm>
#include <chrono>
#include <cstring>
#include <ctime>
#include <memory>
#include <sstream>
#include <vector>

#include <distributed_system/wal_object.h>

#include "frame/test_macros.h"

enum class test_wal_object_log_action {
  kDoNothing = 0,
  kRecursivePushBack,
  kIgnore,
  kFallbackDefault,
  kBreakOnDelegatePatcher,
  kBreakOnDefaultPatcher,
};

namespace std {
template <>
struct hash<test_wal_object_log_action> {
  std::size_t operator()(test_wal_object_log_action const& s) const noexcept {
    return std::hash<int>{}(static_cast<int>(s));
  }
};
}  // namespace std

struct test_wal_object_log_type {
  LIBATFRAME_UTILS_NAMESPACE_ID::distributed_system::wal_time_point timepoint;
  int64_t log_key;
  test_wal_object_log_action action;
  int64_t data;
};

struct test_wal_object_log_storage_type {
  std::vector<test_wal_object_log_type> logs;
  int64_t global_ignore;
};

struct test_wal_object_log_action_getter {
  test_wal_object_log_action operator()(const test_wal_object_log_type& log) { return log.action; }
};

struct test_wal_object_context {};

struct test_wal_object_private_type {
  test_wal_object_log_storage_type* storage;

  inline test_wal_object_private_type() : storage(nullptr) {}
  inline explicit test_wal_object_private_type(test_wal_object_log_storage_type* input) : storage(input) {}
};

namespace mt {
using test_wal_object_log_operator =
    LIBATFRAME_UTILS_NAMESPACE_ID::distributed_system::wal_log_operator<int64_t, test_wal_object_log_type,
                                                                        test_wal_object_log_action_getter>;
using test_wal_object_type =
    LIBATFRAME_UTILS_NAMESPACE_ID::distributed_system::wal_object<test_wal_object_log_storage_type,
                                                                  test_wal_object_log_operator, test_wal_object_context,
                                                                  test_wal_object_private_type>;

struct test_wal_object_stats {
  int64_t key_alloc;
  size_t merge_count;
  size_t delegate_action_count;
  size_t default_action_count;
  size_t delegate_patcher_count;
  size_t default_patcher_count;
  size_t event_on_log_added;
  size_t event_on_log_removed;

  test_wal_object_type::log_type last_log;
};

namespace details {
test_wal_object_stats g_test_wal_object_stats{1, 0, 0, 0, 0, 0, 0, 0, test_wal_object_log_type()};
}

static test_wal_object_type::vtable_pointer create_vtable() {
  using wal_object_type = test_wal_object_type;
  using wal_result_code = LIBATFRAME_UTILS_NAMESPACE_ID::distributed_system::wal_result_code;

  wal_object_type::vtable_pointer ret = test_wal_object_log_operator::make_strong<wal_object_type::vtable_type>();

  ret->load = [](wal_object_type& wal, const wal_object_type::storage_type& from,
                 wal_object_type::callback_param_type) -> wal_result_code {
    *wal.get_private_data().storage = from;
    wal.set_global_ingore_key(from.global_ignore);

    std::vector<wal_object_type::log_pointer> container;
    for (auto& log : from.logs) {
      container.emplace_back(test_wal_object_log_operator::make_strong<wal_object_type::log_type>(log));
    }

    wal.assign_logs(container.begin(), container.end());
    if (!from.logs.empty()) {
      wal.set_last_removed_key((*from.logs.begin()).log_key - 1);
    }
    return wal_result_code::kOk;
  };

  ret->dump = [](const wal_object_type& wal, wal_object_type::storage_type& to,
                 wal_object_type::callback_param_type) -> wal_result_code {
    to = *wal.get_private_data().storage;
    if (nullptr != wal.get_global_ingore_key()) {
      to.global_ignore = *wal.get_global_ingore_key();
    }
    return wal_result_code::kOk;
  };

  ret->get_meta = [](const wal_object_type&,
                     const wal_object_type::log_type& log) -> wal_object_type::meta_result_type {
    return wal_object_type::meta_result_type::make_success(log.timepoint, log.log_key, log.action);
  };

  ret->set_meta = [](const wal_object_type&, wal_object_type::log_type& log, const wal_object_type::meta_type& meta) {
    log.action = meta.action_case;
    log.timepoint = meta.timepoint;
    log.log_key = meta.log_key;
  };

  ret->merge_log = [](const wal_object_type&, wal_object_type::callback_param_type, wal_object_type::log_type& to,
                      const wal_object_type::log_type& from) {
    ++details::g_test_wal_object_stats.merge_count;
    to.data = from.data;
  };

  ret->get_log_key = [](const wal_object_type&, const wal_object_type::log_type& log) -> wal_object_type::log_key_type {
    return log.log_key;
  };

  ret->allocate_log_key = [](wal_object_type&, const wal_object_type::log_type&,
                             wal_object_type::callback_param_type) -> wal_object_type::log_key_result_type {
    return wal_object_type::log_key_result_type::make_success(++details::g_test_wal_object_stats.key_alloc);
  };

  ret->on_log_added = [](wal_object_type&, const wal_object_type::log_pointer&) {
    ++details::g_test_wal_object_stats.event_on_log_added;
  };

  ret->on_log_removed = [](wal_object_type&, const wal_object_type::log_pointer&) {
    ++details::g_test_wal_object_stats.event_on_log_removed;
  };

  ret->log_action_delegate[test_wal_object_log_action::kDoNothing].action =
      [](wal_object_type&, const wal_object_type::log_type& log,
         wal_object_type::callback_param_type) -> wal_result_code {
    ++details::g_test_wal_object_stats.delegate_action_count;
    details::g_test_wal_object_stats.last_log = log;

    return wal_result_code::kOk;
  };

  ret->log_action_delegate[test_wal_object_log_action::kRecursivePushBack].action =
      [](wal_object_type& wal, const wal_object_type::log_type& log,
         wal_object_type::callback_param_type param) -> wal_result_code {
    ++details::g_test_wal_object_stats.delegate_action_count;
    details::g_test_wal_object_stats.last_log = log;

    auto new_log = wal.allocate_log(log.timepoint, test_wal_object_log_action::kDoNothing, param);
    new_log->data = log.data + 1;
    CASE_EXPECT_TRUE(wal_result_code::kPending == wal.emplace_back(std::move(new_log), param));

    return wal_result_code::kOk;
  };

  ret->log_action_delegate[test_wal_object_log_action::kIgnore].action =
      [](wal_object_type&, const wal_object_type::log_type& log,
         wal_object_type::callback_param_type) -> wal_result_code {
    ++details::g_test_wal_object_stats.delegate_action_count;
    details::g_test_wal_object_stats.last_log = log;

    return wal_result_code::kIgnore;
  };

  ret->log_action_delegate[test_wal_object_log_action::kBreakOnDelegatePatcher].patch =
      [](wal_object_type&, wal_object_type::log_type&, wal_object_type::callback_param_type) -> wal_result_code {
    ++details::g_test_wal_object_stats.delegate_patcher_count;
    return wal_result_code::kIgnore;
  };

  ret->default_delegate.action = [](wal_object_type&, const wal_object_type::log_type& log,
                                    wal_object_type::callback_param_type) -> wal_result_code {
    ++details::g_test_wal_object_stats.default_action_count;
    details::g_test_wal_object_stats.last_log = log;

    return wal_result_code::kOk;
  };

  ret->default_delegate.patch = [](wal_object_type&, wal_object_type::log_type& log,
                                   wal_object_type::callback_param_type) -> wal_result_code {
    ++details::g_test_wal_object_stats.default_patcher_count;
    if (log.action == test_wal_object_log_action::kBreakOnDefaultPatcher) {
      return wal_result_code::kActionNotSet;
    }

    return wal_result_code::kOk;
  };

  return ret;
}

static test_wal_object_type::configure_pointer create_configure() {
  test_wal_object_type::configure_pointer ret =
      test_wal_object_log_operator::make_strong<test_wal_object_type::configure_type>();
  test_wal_object_type::default_configure(*ret);

  ret->gc_expire_duration = std::chrono::duration_cast<test_wal_object_type::duration>(std::chrono::seconds{8});
  ret->max_log_size = 8;
  ret->gc_log_size = 4;

  return ret;
}

CASE_TEST(wal_object, create_failed_mt) {
  test_wal_object_log_storage_type storage;
  // test_wal_object_context ctx;

  auto conf = create_configure();
  auto vtable = create_vtable();
  CASE_EXPECT_EQ(nullptr, test_wal_object_type::create(vtable, nullptr, &storage));
  CASE_EXPECT_EQ(nullptr, test_wal_object_type::create(nullptr, conf, &storage));

  auto vtable_1 = vtable;
  vtable_1->get_meta = nullptr;
  CASE_EXPECT_EQ(nullptr, test_wal_object_type::create(vtable_1, conf, &storage));

  auto vtable_2 = vtable;
  vtable_2->get_log_key = nullptr;
  CASE_EXPECT_EQ(nullptr, test_wal_object_type::create(vtable_2, conf, &storage));

  auto vtable_3 = vtable;
  vtable_3->allocate_log_key = nullptr;
  CASE_EXPECT_EQ(nullptr, test_wal_object_type::create(vtable_3, conf, &storage));
}

CASE_TEST(wal_object, load_and_dump_mt) {
  auto old_action_count =
      details::g_test_wal_object_stats.default_action_count + details::g_test_wal_object_stats.delegate_action_count;

  test_wal_object_log_storage_type load_storege;
  LIBATFRAME_UTILS_NAMESPACE_ID::distributed_system::wal_time_point now = std::chrono::system_clock::now();
  load_storege.global_ignore = 123;
  load_storege.logs.push_back(test_wal_object_log_type{now, 124, test_wal_object_log_action::kDoNothing, 124});
  load_storege.logs.push_back(test_wal_object_log_type{now, 125, test_wal_object_log_action::kFallbackDefault, 125});
  load_storege.logs.push_back(test_wal_object_log_type{now, 126, test_wal_object_log_action::kRecursivePushBack, 126});

  test_wal_object_log_storage_type storage;
  test_wal_object_context ctx;

  auto conf = create_configure();
  auto vtable = create_vtable();
  auto wal_obj = test_wal_object_type::create(vtable, conf, &storage);
  CASE_EXPECT_TRUE(!!wal_obj);
  if (!wal_obj) {
    return;
  }
  CASE_EXPECT_TRUE(LIBATFRAME_UTILS_NAMESPACE_ID::distributed_system::wal_result_code::kOk ==
                   wal_obj->load(load_storege, ctx));
  CASE_EXPECT_EQ(old_action_count, details::g_test_wal_object_stats.default_action_count +
                                       details::g_test_wal_object_stats.delegate_action_count);

  CASE_EXPECT_EQ(123, storage.global_ignore);
  CASE_EXPECT_EQ(3, storage.logs.size());
  CASE_EXPECT_EQ(124, storage.logs[0].data);
  CASE_EXPECT_TRUE(test_wal_object_log_action::kRecursivePushBack == storage.logs[2].action);

  CASE_EXPECT_EQ(3, wal_obj->get_all_logs().size());
  CASE_EXPECT_EQ(124, (*wal_obj->get_all_logs().begin())->data);
  CASE_EXPECT_TRUE(test_wal_object_log_action::kRecursivePushBack == (*wal_obj->get_all_logs().rbegin())->action);

  // dump
  test_wal_object_log_storage_type dump_storege;
  CASE_EXPECT_TRUE(LIBATFRAME_UTILS_NAMESPACE_ID::distributed_system::wal_result_code::kOk ==
                   wal_obj->dump(dump_storege, ctx));

  CASE_EXPECT_EQ(123, dump_storege.global_ignore);
  CASE_EXPECT_EQ(3, dump_storege.logs.size());
  CASE_EXPECT_EQ(124, dump_storege.logs[0].data);
  CASE_EXPECT_TRUE(test_wal_object_log_action::kRecursivePushBack == dump_storege.logs[2].action);
}

CASE_TEST(wal_object, add_action_mt) {
  test_wal_object_log_storage_type storage;
  test_wal_object_context ctx;
  LIBATFRAME_UTILS_NAMESPACE_ID::distributed_system::wal_time_point now = std::chrono::system_clock::now();

  auto conf = create_configure();
  auto vtable = create_vtable();
  auto wal_obj = test_wal_object_type::create(vtable, conf, &storage);
  CASE_EXPECT_TRUE(!!wal_obj);
  if (!wal_obj) {
    return;
  }

  do {
    auto old_default_action_count = details::g_test_wal_object_stats.default_action_count;
    auto old_delegate_action_count = details::g_test_wal_object_stats.delegate_action_count;
    auto old_default_patcher_count = details::g_test_wal_object_stats.default_patcher_count;
    auto old_delegate_patcher_count = details::g_test_wal_object_stats.delegate_patcher_count;

    auto previous_key = details::g_test_wal_object_stats.key_alloc;
    auto log = wal_obj->allocate_log(now, test_wal_object_log_action::kDoNothing, ctx);
    CASE_EXPECT_TRUE(!!log);
    if (!log) {
      break;
    }
    log->data = log->log_key + 100;
    CASE_EXPECT_EQ(previous_key + 1, log->log_key);
    CASE_EXPECT_TRUE(test_wal_object_log_action::kDoNothing == log->action);
    CASE_EXPECT_TRUE(now == log->timepoint);

    wal_obj->push_back(log, ctx);
    CASE_EXPECT_EQ(1, wal_obj->get_all_logs().size());
    CASE_EXPECT_EQ(old_default_action_count, details::g_test_wal_object_stats.default_action_count);
    CASE_EXPECT_EQ(old_delegate_action_count + 1, details::g_test_wal_object_stats.delegate_action_count);
    CASE_EXPECT_EQ(old_default_patcher_count, details::g_test_wal_object_stats.default_patcher_count);
    CASE_EXPECT_EQ(old_delegate_patcher_count, details::g_test_wal_object_stats.delegate_patcher_count);
  } while (false);

  int64_t find_key = 0;
  do {
    auto old_default_action_count = details::g_test_wal_object_stats.default_action_count;
    auto old_delegate_action_count = details::g_test_wal_object_stats.delegate_action_count;
    auto old_default_patcher_count = details::g_test_wal_object_stats.default_patcher_count;
    auto old_delegate_patcher_count = details::g_test_wal_object_stats.delegate_patcher_count;

    auto previous_key = details::g_test_wal_object_stats.key_alloc;
    auto log = wal_obj->allocate_log(now, test_wal_object_log_action::kRecursivePushBack, ctx);
    CASE_EXPECT_TRUE(!!log);
    if (!log) {
      break;
    }
    log->data = log->log_key + 100;
    find_key = log->log_key;
    CASE_EXPECT_EQ(previous_key + 1, log->log_key);
    CASE_EXPECT_TRUE(test_wal_object_log_action::kRecursivePushBack == log->action);
    CASE_EXPECT_TRUE(now == log->timepoint);

    wal_obj->push_back(log, ctx);
    CASE_EXPECT_EQ(3, wal_obj->get_all_logs().size());
    CASE_EXPECT_EQ(old_default_action_count, details::g_test_wal_object_stats.default_action_count);
    CASE_EXPECT_EQ(old_delegate_action_count + 2, details::g_test_wal_object_stats.delegate_action_count);
    CASE_EXPECT_EQ(old_default_patcher_count, details::g_test_wal_object_stats.default_patcher_count);
    CASE_EXPECT_EQ(old_delegate_patcher_count, details::g_test_wal_object_stats.delegate_patcher_count);
  } while (false);

  do {
    auto old_default_action_count = details::g_test_wal_object_stats.default_action_count;
    auto old_delegate_action_count = details::g_test_wal_object_stats.delegate_action_count;
    auto old_default_patcher_count = details::g_test_wal_object_stats.default_patcher_count;
    auto old_delegate_patcher_count = details::g_test_wal_object_stats.delegate_patcher_count;

    auto previous_key = details::g_test_wal_object_stats.key_alloc;
    auto log = wal_obj->allocate_log(now, test_wal_object_log_action::kFallbackDefault, ctx);
    CASE_EXPECT_TRUE(!!log);
    if (!log) {
      break;
    }
    log->data = log->log_key + 100;
    CASE_EXPECT_EQ(previous_key + 1, log->log_key);
    CASE_EXPECT_TRUE(test_wal_object_log_action::kFallbackDefault == log->action);
    CASE_EXPECT_TRUE(now == log->timepoint);

    wal_obj->push_back(log, ctx);
    CASE_EXPECT_EQ(4, wal_obj->get_all_logs().size());
    CASE_EXPECT_EQ(old_default_action_count + 1, details::g_test_wal_object_stats.default_action_count);
    CASE_EXPECT_EQ(old_delegate_action_count, details::g_test_wal_object_stats.delegate_action_count);
    CASE_EXPECT_EQ(old_default_patcher_count + 1, details::g_test_wal_object_stats.default_patcher_count);
    CASE_EXPECT_EQ(old_delegate_patcher_count, details::g_test_wal_object_stats.delegate_patcher_count);
  } while (false);

  do {
    auto old_default_action_count = details::g_test_wal_object_stats.default_action_count;
    auto old_delegate_action_count = details::g_test_wal_object_stats.delegate_action_count;
    auto old_default_patcher_count = details::g_test_wal_object_stats.default_patcher_count;
    auto old_delegate_patcher_count = details::g_test_wal_object_stats.delegate_patcher_count;

    auto previous_key = details::g_test_wal_object_stats.key_alloc;
    auto log = wal_obj->allocate_log(now, test_wal_object_log_action::kBreakOnDelegatePatcher, ctx);
    CASE_EXPECT_TRUE(!!log);
    if (!log) {
      break;
    }
    log->data = log->log_key + 100;
    CASE_EXPECT_EQ(previous_key + 1, log->log_key);
    CASE_EXPECT_TRUE(test_wal_object_log_action::kBreakOnDelegatePatcher == log->action);
    CASE_EXPECT_TRUE(now == log->timepoint);

    wal_obj->push_back(log, ctx);
    CASE_EXPECT_EQ(4, wal_obj->get_all_logs().size());
    CASE_EXPECT_EQ(old_default_action_count, details::g_test_wal_object_stats.default_action_count);
    CASE_EXPECT_EQ(old_delegate_action_count, details::g_test_wal_object_stats.delegate_action_count);
    CASE_EXPECT_EQ(old_default_patcher_count, details::g_test_wal_object_stats.default_patcher_count);
    CASE_EXPECT_EQ(old_delegate_patcher_count + 1, details::g_test_wal_object_stats.delegate_patcher_count);
  } while (false);

  do {
    auto old_default_action_count = details::g_test_wal_object_stats.default_action_count;
    auto old_delegate_action_count = details::g_test_wal_object_stats.delegate_action_count;
    auto old_default_patcher_count = details::g_test_wal_object_stats.default_patcher_count;
    auto old_delegate_patcher_count = details::g_test_wal_object_stats.delegate_patcher_count;

    auto previous_key = details::g_test_wal_object_stats.key_alloc;
    auto log = wal_obj->allocate_log(now, test_wal_object_log_action::kBreakOnDefaultPatcher, ctx);
    CASE_EXPECT_TRUE(!!log);
    if (!log) {
      break;
    }
    log->data = log->log_key + 100;
    CASE_EXPECT_EQ(previous_key + 1, log->log_key);
    CASE_EXPECT_TRUE(test_wal_object_log_action::kBreakOnDefaultPatcher == log->action);
    CASE_EXPECT_TRUE(now == log->timepoint);

    wal_obj->push_back(log, ctx);
    CASE_EXPECT_EQ(4, wal_obj->get_all_logs().size());
    CASE_EXPECT_EQ(old_default_action_count, details::g_test_wal_object_stats.default_action_count);
    CASE_EXPECT_EQ(old_delegate_action_count, details::g_test_wal_object_stats.delegate_action_count);
    CASE_EXPECT_EQ(old_default_patcher_count + 1, details::g_test_wal_object_stats.default_patcher_count);
    CASE_EXPECT_EQ(old_delegate_patcher_count, details::g_test_wal_object_stats.delegate_patcher_count);
  } while (false);

  // ============ iterators ============
  {
    auto find_ptr = wal_obj->find_log(find_key);
    CASE_EXPECT_TRUE(!!find_ptr);
    CASE_EXPECT_EQ(find_key + 100, find_ptr->data);
    CASE_EXPECT_TRUE(test_wal_object_log_action::kRecursivePushBack == find_ptr->action);
  }

  {
    const auto& wal_cobj = *wal_obj;
    auto find_ptr = wal_cobj.find_log(find_key);
    CASE_EXPECT_TRUE(!!find_ptr);
    CASE_EXPECT_EQ(find_key + 100, find_ptr->data);
    CASE_EXPECT_TRUE(test_wal_object_log_action::kRecursivePushBack == find_ptr->action);
  }

  do {
    auto range = wal_obj->log_all_range();
    CASE_EXPECT_TRUE(range.first != wal_obj->log_end());
    if (range.first == wal_obj->log_end()) {
      break;
    }

    CASE_EXPECT_TRUE(range.first == wal_obj->log_begin());
    CASE_EXPECT_TRUE(range.second == wal_obj->log_end());

    CASE_EXPECT_EQ(find_key + 99, (*range.first)->data);
    CASE_EXPECT_TRUE(test_wal_object_log_action::kDoNothing == (*range.first)->action);
  } while (false);

  do {
    const test_wal_object_type& obj = *wal_obj;
    auto range = obj.log_all_range();
    CASE_EXPECT_TRUE(range.first != wal_obj->log_end());
    if (range.first == wal_obj->log_end()) {
      break;
    }

    CASE_EXPECT_TRUE(range.first == obj.log_cbegin());
    CASE_EXPECT_TRUE(range.second == obj.log_cend());

    CASE_EXPECT_EQ(find_key + 99, (*range.first)->data);
    CASE_EXPECT_TRUE(test_wal_object_log_action::kDoNothing == (*range.first)->action);
  } while (false);

  do {
    auto iter = wal_obj->log_lower_bound(find_key);
    CASE_EXPECT_TRUE(iter != wal_obj->log_end());
    if (iter == wal_obj->log_end()) {
      break;
    }

    CASE_EXPECT_EQ(find_key + 100, (*iter)->data);
    CASE_EXPECT_TRUE(test_wal_object_log_action::kRecursivePushBack == (*iter)->action);
  } while (false);

  do {
    const test_wal_object_type& obj = *wal_obj;
    auto iter = obj.log_lower_bound(find_key);
    CASE_EXPECT_TRUE(iter != obj.log_cend());
    if (iter == obj.log_cend()) {
      break;
    }

    CASE_EXPECT_EQ(find_key + 100, (*iter)->data);
    CASE_EXPECT_TRUE(test_wal_object_log_action::kRecursivePushBack == (*iter)->action);
  } while (false);

  do {
    auto iter = wal_obj->log_upper_bound(find_key + 1);
    CASE_EXPECT_TRUE(iter != wal_obj->log_end());
    if (iter == wal_obj->log_end()) {
      break;
    }

    CASE_EXPECT_EQ(find_key + 102, (*iter)->data);
    CASE_EXPECT_TRUE(test_wal_object_log_action::kFallbackDefault == (*iter)->action);
  } while (false);

  do {
    const test_wal_object_type& obj = *wal_obj;
    auto iter = obj.log_upper_bound(find_key + 1);
    CASE_EXPECT_TRUE(iter != obj.log_cend());
    if (iter == obj.log_cend()) {
      break;
    }

    CASE_EXPECT_EQ(find_key + 102, (*iter)->data);
    CASE_EXPECT_TRUE(test_wal_object_log_action::kFallbackDefault == (*iter)->action);
  } while (false);
}

CASE_TEST(wal_object, gc_mt) {
  test_wal_object_log_storage_type storage;
  test_wal_object_context ctx;
  LIBATFRAME_UTILS_NAMESPACE_ID::distributed_system::wal_time_point now = std::chrono::system_clock::now();

  auto conf = create_configure();
  auto vtable = create_vtable();

  conf->gc_expire_duration = std::chrono::duration_cast<test_wal_object_type::duration>(std::chrono::seconds{8});
  conf->max_log_size = 8;
  conf->gc_log_size = 4;

  auto wal_obj = test_wal_object_type::create(vtable, conf, &storage);
  CASE_EXPECT_TRUE(!!wal_obj);
  if (!wal_obj) {
    return;
  }

  CASE_EXPECT_EQ(4, wal_obj->get_configure().gc_log_size);
  CASE_EXPECT_EQ(8, wal_obj->get_configure().max_log_size);
  CASE_EXPECT_TRUE(std::chrono::duration_cast<test_wal_object_type::duration>(std::chrono::seconds{8}) ==
                   wal_obj->get_configure().gc_expire_duration);
  auto old_remove_count = details::g_test_wal_object_stats.event_on_log_removed;
  auto old_add_count = details::g_test_wal_object_stats.event_on_log_added;

  auto begin_key = details::g_test_wal_object_stats.key_alloc;

  for (int i = 0; i < 3; ++i) {
    do {
      now += std::chrono::duration_cast<test_wal_object_type::duration>(std::chrono::seconds{1});
      auto old_default_action_count = details::g_test_wal_object_stats.default_action_count;
      auto old_delegate_action_count = details::g_test_wal_object_stats.delegate_action_count;

      auto previous_key = details::g_test_wal_object_stats.key_alloc;
      auto log = wal_obj->allocate_log(now, test_wal_object_log_action::kDoNothing, ctx);
      CASE_EXPECT_TRUE(!!log);
      if (!log) {
        break;
      }
      log->data = log->log_key + 100;
      CASE_EXPECT_EQ(previous_key + 1, log->log_key);
      CASE_EXPECT_TRUE(test_wal_object_log_action::kDoNothing == log->action);
      CASE_EXPECT_TRUE(now == log->timepoint);

      wal_obj->push_back(log, ctx);
      CASE_EXPECT_EQ(old_default_action_count, details::g_test_wal_object_stats.default_action_count);
      CASE_EXPECT_EQ(old_delegate_action_count + 1, details::g_test_wal_object_stats.delegate_action_count);
    } while (false);

    do {
      now += std::chrono::duration_cast<test_wal_object_type::duration>(std::chrono::seconds{1});
      auto old_default_action_count = details::g_test_wal_object_stats.default_action_count;
      auto old_delegate_action_count = details::g_test_wal_object_stats.delegate_action_count;

      auto previous_key = details::g_test_wal_object_stats.key_alloc;
      auto log = wal_obj->allocate_log(now, test_wal_object_log_action::kRecursivePushBack, ctx);
      CASE_EXPECT_TRUE(!!log);
      if (!log) {
        break;
      }
      log->data = log->log_key + 100;
      CASE_EXPECT_EQ(previous_key + 1, log->log_key);
      CASE_EXPECT_TRUE(test_wal_object_log_action::kRecursivePushBack == log->action);
      CASE_EXPECT_TRUE(now == log->timepoint);

      wal_obj->push_back(log, ctx);
      CASE_EXPECT_EQ(old_default_action_count, details::g_test_wal_object_stats.default_action_count);
      CASE_EXPECT_EQ(old_delegate_action_count + 2, details::g_test_wal_object_stats.delegate_action_count);
    } while (false);

    do {
      now += std::chrono::duration_cast<test_wal_object_type::duration>(std::chrono::seconds{1});
      auto old_default_action_count = details::g_test_wal_object_stats.default_action_count;
      auto old_delegate_action_count = details::g_test_wal_object_stats.delegate_action_count;

      auto previous_key = details::g_test_wal_object_stats.key_alloc;
      auto log = wal_obj->allocate_log(now, test_wal_object_log_action::kFallbackDefault, ctx);
      CASE_EXPECT_TRUE(!!log);
      if (!log) {
        break;
      }
      log->data = log->log_key + 100;
      CASE_EXPECT_EQ(previous_key + 1, log->log_key);
      CASE_EXPECT_TRUE(test_wal_object_log_action::kFallbackDefault == log->action);
      CASE_EXPECT_TRUE(now == log->timepoint);

      wal_obj->push_back(log, ctx);
      CASE_EXPECT_EQ(old_default_action_count + 1, details::g_test_wal_object_stats.default_action_count);
      CASE_EXPECT_EQ(old_delegate_action_count, details::g_test_wal_object_stats.delegate_action_count);
    } while (false);
  }

  CASE_EXPECT_EQ(0, wal_obj->gc(now));
  CASE_EXPECT_EQ(8, wal_obj->get_all_logs().size());
  CASE_EXPECT_EQ(old_add_count + 12, details::g_test_wal_object_stats.event_on_log_added);
  CASE_EXPECT_EQ(old_remove_count + 4, details::g_test_wal_object_stats.event_on_log_removed);

  now += std::chrono::duration_cast<test_wal_object_type::duration>(std::chrono::seconds{4});
  CASE_EXPECT_EQ(0, wal_obj->gc(now, &begin_key, 1));
  CASE_EXPECT_EQ(8, wal_obj->get_all_logs().size());
  CASE_EXPECT_EQ(old_remove_count + 4, details::g_test_wal_object_stats.event_on_log_removed);

  CASE_EXPECT_EQ(1, wal_obj->gc(now, nullptr, 1));
  CASE_EXPECT_EQ(7, wal_obj->get_all_logs().size());
  CASE_EXPECT_EQ(old_remove_count + 5, details::g_test_wal_object_stats.event_on_log_removed);

  CASE_EXPECT_EQ(2, wal_obj->gc(now, nullptr));
  CASE_EXPECT_EQ(5, wal_obj->get_all_logs().size());
  CASE_EXPECT_EQ(old_remove_count + 7, details::g_test_wal_object_stats.event_on_log_removed);

  now += std::chrono::duration_cast<test_wal_object_type::duration>(std::chrono::seconds{2});
  CASE_EXPECT_EQ(1, wal_obj->gc(now, nullptr));
  CASE_EXPECT_EQ(4, wal_obj->get_all_logs().size());
  CASE_EXPECT_EQ(old_remove_count + 8, details::g_test_wal_object_stats.event_on_log_removed);

  auto last_removed_key = (*wal_obj->log_cbegin())->log_key - 1;
  CASE_EXPECT_TRUE(wal_obj->get_last_removed_key() && *wal_obj->get_last_removed_key() == last_removed_key);
}

CASE_TEST(wal_object, ignore_mt) {
  test_wal_object_log_storage_type storage;
  test_wal_object_context ctx;
  LIBATFRAME_UTILS_NAMESPACE_ID::distributed_system::wal_time_point now = std::chrono::system_clock::now();

  auto conf = create_configure();
  auto vtable = create_vtable();
  auto wal_obj = test_wal_object_type::create(vtable, conf, &storage);
  CASE_EXPECT_TRUE(!!wal_obj);
  if (!wal_obj) {
    return;
  }

  do {
    auto old_default_action_count = details::g_test_wal_object_stats.default_action_count;
    auto old_delegate_action_count = details::g_test_wal_object_stats.delegate_action_count;

    auto log = wal_obj->allocate_log(now, test_wal_object_log_action::kDoNothing, ctx);
    CASE_EXPECT_TRUE(!!log);
    if (!log) {
      break;
    }
    log->data = log->log_key + 100;
    wal_obj->set_global_ingore_key(log->log_key);
    auto push_back_result = wal_obj->push_back(log, ctx);
    CASE_EXPECT_TRUE(LIBATFRAME_UTILS_NAMESPACE_ID::distributed_system::wal_result_code::kIgnore == push_back_result);

    CASE_EXPECT_EQ(0, wal_obj->get_all_logs().size());
    CASE_EXPECT_EQ(old_default_action_count, details::g_test_wal_object_stats.default_action_count);
    CASE_EXPECT_EQ(old_delegate_action_count, details::g_test_wal_object_stats.delegate_action_count);
  } while (false);

  do {
    auto old_default_action_count = details::g_test_wal_object_stats.default_action_count;
    auto old_delegate_action_count = details::g_test_wal_object_stats.delegate_action_count;

    auto log = wal_obj->allocate_log(now, test_wal_object_log_action::kIgnore, ctx);
    CASE_EXPECT_TRUE(!!log);
    if (!log) {
      break;
    }
    log->data = log->log_key + 100;
    auto push_back_result = wal_obj->push_back(log, ctx);
    CASE_EXPECT_TRUE(LIBATFRAME_UTILS_NAMESPACE_ID::distributed_system::wal_result_code::kIgnore == push_back_result);

    CASE_EXPECT_EQ(0, wal_obj->get_all_logs().size());
    CASE_EXPECT_EQ(old_default_action_count, details::g_test_wal_object_stats.default_action_count);
    CASE_EXPECT_EQ(old_delegate_action_count + 1, details::g_test_wal_object_stats.delegate_action_count);
  } while (false);
}

CASE_TEST(wal_object, reorder_mt) {
  test_wal_object_log_storage_type storage;
  test_wal_object_context ctx;
  LIBATFRAME_UTILS_NAMESPACE_ID::distributed_system::wal_time_point now = std::chrono::system_clock::now();

  auto conf = create_configure();
  auto vtable = create_vtable();
  auto wal_obj = test_wal_object_type::create(vtable, conf, &storage);
  CASE_EXPECT_TRUE(!!wal_obj);
  if (!wal_obj) {
    return;
  }

  test_wal_object_type::log_pointer log1;
  test_wal_object_type::log_pointer log2;
  do {
    log1 = wal_obj->allocate_log(now, test_wal_object_log_action::kDoNothing, ctx);
    CASE_EXPECT_TRUE(!!log1);
    if (!log1) {
      break;
    }
    log1->data = log1->log_key + 100;
  } while (false);

  do {
    log2 = wal_obj->allocate_log(now, test_wal_object_log_action::kDoNothing, ctx);
    CASE_EXPECT_TRUE(!!log2);
    if (!log2) {
      break;
    }
    log2->data = log2->log_key + 100;
  } while (false);

  if (!log1 || !log2) {
    return;
  }

  wal_obj->push_back(log2, ctx);
  wal_obj->push_back(log1, ctx);

  auto iter = wal_obj->log_begin();
  CASE_EXPECT_EQ(log1.get(), (*iter).get());

  ++iter;
  CASE_EXPECT_EQ(log2.get(), (*iter).get());
}
}  // namespace mt

namespace st {
using test_wal_object_log_operator = LIBATFRAME_UTILS_NAMESPACE_ID::distributed_system::wal_log_operator_with_mt_mode<
    int64_t, test_wal_object_log_type, test_wal_object_log_action_getter,
    LIBATFRAME_UTILS_NAMESPACE_ID::distributed_system::wal_mt_mode::kSingleThread>;
using test_wal_object_type =
    LIBATFRAME_UTILS_NAMESPACE_ID::distributed_system::wal_object<test_wal_object_log_storage_type,
                                                                  test_wal_object_log_operator, test_wal_object_context,
                                                                  test_wal_object_private_type>;

struct test_wal_object_stats {
  int64_t key_alloc;
  size_t merge_count;
  size_t delegate_action_count;
  size_t default_action_count;
  size_t delegate_patcher_count;
  size_t default_patcher_count;
  size_t event_on_log_added;
  size_t event_on_log_removed;

  test_wal_object_type::log_type last_log;
};

namespace details {
test_wal_object_stats g_test_wal_object_stats{1, 0, 0, 0, 0, 0, 0, 0, test_wal_object_log_type()};
}

static test_wal_object_type::vtable_pointer create_vtable() {
  using wal_object_type = test_wal_object_type;
  using wal_result_code = LIBATFRAME_UTILS_NAMESPACE_ID::distributed_system::wal_result_code;

  wal_object_type::vtable_pointer ret = test_wal_object_log_operator::make_strong<wal_object_type::vtable_type>();

  ret->load = [](wal_object_type& wal, const wal_object_type::storage_type& from,
                 wal_object_type::callback_param_type) -> wal_result_code {
    *wal.get_private_data().storage = from;
    wal.set_global_ingore_key(from.global_ignore);

    std::vector<wal_object_type::log_pointer> container;
    for (auto& log : from.logs) {
      container.emplace_back(test_wal_object_log_operator::make_strong<wal_object_type::log_type>(log));
    }

    wal.assign_logs(container.begin(), container.end());
    if (!from.logs.empty()) {
      wal.set_last_removed_key((*from.logs.begin()).log_key - 1);
    }
    return wal_result_code::kOk;
  };

  ret->dump = [](const wal_object_type& wal, wal_object_type::storage_type& to,
                 wal_object_type::callback_param_type) -> wal_result_code {
    to = *wal.get_private_data().storage;
    if (nullptr != wal.get_global_ingore_key()) {
      to.global_ignore = *wal.get_global_ingore_key();
    }
    return wal_result_code::kOk;
  };

  ret->get_meta = [](const wal_object_type&,
                     const wal_object_type::log_type& log) -> wal_object_type::meta_result_type {
    return wal_object_type::meta_result_type::make_success(log.timepoint, log.log_key, log.action);
  };

  ret->set_meta = [](const wal_object_type&, wal_object_type::log_type& log, const wal_object_type::meta_type& meta) {
    log.action = meta.action_case;
    log.timepoint = meta.timepoint;
    log.log_key = meta.log_key;
  };

  ret->merge_log = [](const wal_object_type&, wal_object_type::callback_param_type, wal_object_type::log_type& to,
                      const wal_object_type::log_type& from) {
    ++details::g_test_wal_object_stats.merge_count;
    to.data = from.data;
  };

  ret->get_log_key = [](const wal_object_type&, const wal_object_type::log_type& log) -> wal_object_type::log_key_type {
    return log.log_key;
  };

  ret->allocate_log_key = [](wal_object_type&, const wal_object_type::log_type&,
                             wal_object_type::callback_param_type) -> wal_object_type::log_key_result_type {
    return wal_object_type::log_key_result_type::make_success(++details::g_test_wal_object_stats.key_alloc);
  };

  ret->on_log_added = [](wal_object_type&, const wal_object_type::log_pointer&) {
    ++details::g_test_wal_object_stats.event_on_log_added;
  };

  ret->on_log_removed = [](wal_object_type&, const wal_object_type::log_pointer&) {
    ++details::g_test_wal_object_stats.event_on_log_removed;
  };

  ret->log_action_delegate[test_wal_object_log_action::kDoNothing].action =
      [](wal_object_type&, const wal_object_type::log_type& log,
         wal_object_type::callback_param_type) -> wal_result_code {
    ++details::g_test_wal_object_stats.delegate_action_count;
    details::g_test_wal_object_stats.last_log = log;

    return wal_result_code::kOk;
  };

  ret->log_action_delegate[test_wal_object_log_action::kRecursivePushBack].action =
      [](wal_object_type& wal, const wal_object_type::log_type& log,
         wal_object_type::callback_param_type param) -> wal_result_code {
    ++details::g_test_wal_object_stats.delegate_action_count;
    details::g_test_wal_object_stats.last_log = log;

    auto new_log = wal.allocate_log(log.timepoint, test_wal_object_log_action::kDoNothing, param);
    new_log->data = log.data + 1;
    CASE_EXPECT_TRUE(wal_result_code::kPending == wal.emplace_back(std::move(new_log), param));

    return wal_result_code::kOk;
  };

  ret->log_action_delegate[test_wal_object_log_action::kIgnore].action =
      [](wal_object_type&, const wal_object_type::log_type& log,
         wal_object_type::callback_param_type) -> wal_result_code {
    ++details::g_test_wal_object_stats.delegate_action_count;
    details::g_test_wal_object_stats.last_log = log;

    return wal_result_code::kIgnore;
  };

  ret->log_action_delegate[test_wal_object_log_action::kBreakOnDelegatePatcher].patch =
      [](wal_object_type&, wal_object_type::log_type&, wal_object_type::callback_param_type) -> wal_result_code {
    ++details::g_test_wal_object_stats.delegate_patcher_count;
    return wal_result_code::kIgnore;
  };

  ret->default_delegate.action = [](wal_object_type&, const wal_object_type::log_type& log,
                                    wal_object_type::callback_param_type) -> wal_result_code {
    ++details::g_test_wal_object_stats.default_action_count;
    details::g_test_wal_object_stats.last_log = log;

    return wal_result_code::kOk;
  };

  ret->default_delegate.patch = [](wal_object_type&, wal_object_type::log_type& log,
                                   wal_object_type::callback_param_type) -> wal_result_code {
    ++details::g_test_wal_object_stats.default_patcher_count;
    if (log.action == test_wal_object_log_action::kBreakOnDefaultPatcher) {
      return wal_result_code::kActionNotSet;
    }

    return wal_result_code::kOk;
  };

  return ret;
}

static test_wal_object_type::configure_pointer create_configure() {
  test_wal_object_type::configure_pointer ret =
      test_wal_object_log_operator::make_strong<test_wal_object_type::configure_type>();
  test_wal_object_type::default_configure(*ret);

  ret->gc_expire_duration = std::chrono::duration_cast<test_wal_object_type::duration>(std::chrono::seconds{8});
  ret->max_log_size = 8;
  ret->gc_log_size = 4;

  return ret;
}

CASE_TEST(wal_object, create_failed_st) {
  test_wal_object_log_storage_type storage;
  // test_wal_object_context ctx;

  auto conf = create_configure();
  auto vtable = create_vtable();
  CASE_EXPECT_EQ(nullptr, test_wal_object_type::create(vtable, nullptr, &storage));
  CASE_EXPECT_EQ(nullptr, test_wal_object_type::create(nullptr, conf, &storage));

  auto vtable_1 = vtable;
  vtable_1->get_meta = nullptr;
  CASE_EXPECT_EQ(nullptr, test_wal_object_type::create(vtable_1, conf, &storage));

  auto vtable_2 = vtable;
  vtable_2->get_log_key = nullptr;
  CASE_EXPECT_EQ(nullptr, test_wal_object_type::create(vtable_2, conf, &storage));

  auto vtable_3 = vtable;
  vtable_3->allocate_log_key = nullptr;
  CASE_EXPECT_EQ(nullptr, test_wal_object_type::create(vtable_3, conf, &storage));
}

CASE_TEST(wal_object, load_and_dump_st) {
  auto old_action_count =
      details::g_test_wal_object_stats.default_action_count + details::g_test_wal_object_stats.delegate_action_count;

  test_wal_object_log_storage_type load_storege;
  LIBATFRAME_UTILS_NAMESPACE_ID::distributed_system::wal_time_point now = std::chrono::system_clock::now();
  load_storege.global_ignore = 123;
  load_storege.logs.push_back(test_wal_object_log_type{now, 124, test_wal_object_log_action::kDoNothing, 124});
  load_storege.logs.push_back(test_wal_object_log_type{now, 125, test_wal_object_log_action::kFallbackDefault, 125});
  load_storege.logs.push_back(test_wal_object_log_type{now, 126, test_wal_object_log_action::kRecursivePushBack, 126});

  test_wal_object_log_storage_type storage;
  test_wal_object_context ctx;

  auto conf = create_configure();
  auto vtable = create_vtable();
  auto wal_obj = test_wal_object_type::create(vtable, conf, &storage);
  CASE_EXPECT_TRUE(!!wal_obj);
  if (!wal_obj) {
    return;
  }
  CASE_EXPECT_TRUE(LIBATFRAME_UTILS_NAMESPACE_ID::distributed_system::wal_result_code::kOk ==
                   wal_obj->load(load_storege, ctx));
  CASE_EXPECT_EQ(old_action_count, details::g_test_wal_object_stats.default_action_count +
                                       details::g_test_wal_object_stats.delegate_action_count);

  CASE_EXPECT_EQ(123, storage.global_ignore);
  CASE_EXPECT_EQ(3, storage.logs.size());
  CASE_EXPECT_EQ(124, storage.logs[0].data);
  CASE_EXPECT_TRUE(test_wal_object_log_action::kRecursivePushBack == storage.logs[2].action);

  CASE_EXPECT_EQ(3, wal_obj->get_all_logs().size());
  CASE_EXPECT_EQ(124, (*wal_obj->get_all_logs().begin())->data);
  CASE_EXPECT_TRUE(test_wal_object_log_action::kRecursivePushBack == (*wal_obj->get_all_logs().rbegin())->action);

  // dump
  test_wal_object_log_storage_type dump_storege;
  CASE_EXPECT_TRUE(LIBATFRAME_UTILS_NAMESPACE_ID::distributed_system::wal_result_code::kOk ==
                   wal_obj->dump(dump_storege, ctx));

  CASE_EXPECT_EQ(123, dump_storege.global_ignore);
  CASE_EXPECT_EQ(3, dump_storege.logs.size());
  CASE_EXPECT_EQ(124, dump_storege.logs[0].data);
  CASE_EXPECT_TRUE(test_wal_object_log_action::kRecursivePushBack == dump_storege.logs[2].action);
}

CASE_TEST(wal_object, add_action_st) {
  test_wal_object_log_storage_type storage;
  test_wal_object_context ctx;
  LIBATFRAME_UTILS_NAMESPACE_ID::distributed_system::wal_time_point now = std::chrono::system_clock::now();

  auto conf = create_configure();
  auto vtable = create_vtable();
  auto wal_obj = test_wal_object_type::create(vtable, conf, &storage);
  CASE_EXPECT_TRUE(!!wal_obj);
  if (!wal_obj) {
    return;
  }

  do {
    auto old_default_action_count = details::g_test_wal_object_stats.default_action_count;
    auto old_delegate_action_count = details::g_test_wal_object_stats.delegate_action_count;
    auto old_default_patcher_count = details::g_test_wal_object_stats.default_patcher_count;
    auto old_delegate_patcher_count = details::g_test_wal_object_stats.delegate_patcher_count;

    auto previous_key = details::g_test_wal_object_stats.key_alloc;
    auto log = wal_obj->allocate_log(now, test_wal_object_log_action::kDoNothing, ctx);
    CASE_EXPECT_TRUE(!!log);
    if (!log) {
      break;
    }
    log->data = log->log_key + 100;
    CASE_EXPECT_EQ(previous_key + 1, log->log_key);
    CASE_EXPECT_TRUE(test_wal_object_log_action::kDoNothing == log->action);
    CASE_EXPECT_TRUE(now == log->timepoint);

    wal_obj->push_back(log, ctx);
    CASE_EXPECT_EQ(1, wal_obj->get_all_logs().size());
    CASE_EXPECT_EQ(old_default_action_count, details::g_test_wal_object_stats.default_action_count);
    CASE_EXPECT_EQ(old_delegate_action_count + 1, details::g_test_wal_object_stats.delegate_action_count);
    CASE_EXPECT_EQ(old_default_patcher_count, details::g_test_wal_object_stats.default_patcher_count);
    CASE_EXPECT_EQ(old_delegate_patcher_count, details::g_test_wal_object_stats.delegate_patcher_count);
  } while (false);

  int64_t find_key = 0;
  do {
    auto old_default_action_count = details::g_test_wal_object_stats.default_action_count;
    auto old_delegate_action_count = details::g_test_wal_object_stats.delegate_action_count;
    auto old_default_patcher_count = details::g_test_wal_object_stats.default_patcher_count;
    auto old_delegate_patcher_count = details::g_test_wal_object_stats.delegate_patcher_count;

    auto previous_key = details::g_test_wal_object_stats.key_alloc;
    auto log = wal_obj->allocate_log(now, test_wal_object_log_action::kRecursivePushBack, ctx);
    CASE_EXPECT_TRUE(!!log);
    if (!log) {
      break;
    }
    log->data = log->log_key + 100;
    find_key = log->log_key;
    CASE_EXPECT_EQ(previous_key + 1, log->log_key);
    CASE_EXPECT_TRUE(test_wal_object_log_action::kRecursivePushBack == log->action);
    CASE_EXPECT_TRUE(now == log->timepoint);

    wal_obj->push_back(log, ctx);
    CASE_EXPECT_EQ(3, wal_obj->get_all_logs().size());
    CASE_EXPECT_EQ(old_default_action_count, details::g_test_wal_object_stats.default_action_count);
    CASE_EXPECT_EQ(old_delegate_action_count + 2, details::g_test_wal_object_stats.delegate_action_count);
    CASE_EXPECT_EQ(old_default_patcher_count, details::g_test_wal_object_stats.default_patcher_count);
    CASE_EXPECT_EQ(old_delegate_patcher_count, details::g_test_wal_object_stats.delegate_patcher_count);
  } while (false);

  do {
    auto old_default_action_count = details::g_test_wal_object_stats.default_action_count;
    auto old_delegate_action_count = details::g_test_wal_object_stats.delegate_action_count;
    auto old_default_patcher_count = details::g_test_wal_object_stats.default_patcher_count;
    auto old_delegate_patcher_count = details::g_test_wal_object_stats.delegate_patcher_count;

    auto previous_key = details::g_test_wal_object_stats.key_alloc;
    auto log = wal_obj->allocate_log(now, test_wal_object_log_action::kFallbackDefault, ctx);
    CASE_EXPECT_TRUE(!!log);
    if (!log) {
      break;
    }
    log->data = log->log_key + 100;
    CASE_EXPECT_EQ(previous_key + 1, log->log_key);
    CASE_EXPECT_TRUE(test_wal_object_log_action::kFallbackDefault == log->action);
    CASE_EXPECT_TRUE(now == log->timepoint);

    wal_obj->push_back(log, ctx);
    CASE_EXPECT_EQ(4, wal_obj->get_all_logs().size());
    CASE_EXPECT_EQ(old_default_action_count + 1, details::g_test_wal_object_stats.default_action_count);
    CASE_EXPECT_EQ(old_delegate_action_count, details::g_test_wal_object_stats.delegate_action_count);
    CASE_EXPECT_EQ(old_default_patcher_count + 1, details::g_test_wal_object_stats.default_patcher_count);
    CASE_EXPECT_EQ(old_delegate_patcher_count, details::g_test_wal_object_stats.delegate_patcher_count);
  } while (false);

  do {
    auto old_default_action_count = details::g_test_wal_object_stats.default_action_count;
    auto old_delegate_action_count = details::g_test_wal_object_stats.delegate_action_count;
    auto old_default_patcher_count = details::g_test_wal_object_stats.default_patcher_count;
    auto old_delegate_patcher_count = details::g_test_wal_object_stats.delegate_patcher_count;

    auto previous_key = details::g_test_wal_object_stats.key_alloc;
    auto log = wal_obj->allocate_log(now, test_wal_object_log_action::kBreakOnDelegatePatcher, ctx);
    CASE_EXPECT_TRUE(!!log);
    if (!log) {
      break;
    }
    log->data = log->log_key + 100;
    CASE_EXPECT_EQ(previous_key + 1, log->log_key);
    CASE_EXPECT_TRUE(test_wal_object_log_action::kBreakOnDelegatePatcher == log->action);
    CASE_EXPECT_TRUE(now == log->timepoint);

    wal_obj->push_back(log, ctx);
    CASE_EXPECT_EQ(4, wal_obj->get_all_logs().size());
    CASE_EXPECT_EQ(old_default_action_count, details::g_test_wal_object_stats.default_action_count);
    CASE_EXPECT_EQ(old_delegate_action_count, details::g_test_wal_object_stats.delegate_action_count);
    CASE_EXPECT_EQ(old_default_patcher_count, details::g_test_wal_object_stats.default_patcher_count);
    CASE_EXPECT_EQ(old_delegate_patcher_count + 1, details::g_test_wal_object_stats.delegate_patcher_count);
  } while (false);

  do {
    auto old_default_action_count = details::g_test_wal_object_stats.default_action_count;
    auto old_delegate_action_count = details::g_test_wal_object_stats.delegate_action_count;
    auto old_default_patcher_count = details::g_test_wal_object_stats.default_patcher_count;
    auto old_delegate_patcher_count = details::g_test_wal_object_stats.delegate_patcher_count;

    auto previous_key = details::g_test_wal_object_stats.key_alloc;
    auto log = wal_obj->allocate_log(now, test_wal_object_log_action::kBreakOnDefaultPatcher, ctx);
    CASE_EXPECT_TRUE(!!log);
    if (!log) {
      break;
    }
    log->data = log->log_key + 100;
    CASE_EXPECT_EQ(previous_key + 1, log->log_key);
    CASE_EXPECT_TRUE(test_wal_object_log_action::kBreakOnDefaultPatcher == log->action);
    CASE_EXPECT_TRUE(now == log->timepoint);

    wal_obj->push_back(log, ctx);
    CASE_EXPECT_EQ(4, wal_obj->get_all_logs().size());
    CASE_EXPECT_EQ(old_default_action_count, details::g_test_wal_object_stats.default_action_count);
    CASE_EXPECT_EQ(old_delegate_action_count, details::g_test_wal_object_stats.delegate_action_count);
    CASE_EXPECT_EQ(old_default_patcher_count + 1, details::g_test_wal_object_stats.default_patcher_count);
    CASE_EXPECT_EQ(old_delegate_patcher_count, details::g_test_wal_object_stats.delegate_patcher_count);
  } while (false);

  // ============ iterators ============
  {
    auto find_ptr = wal_obj->find_log(find_key);
    CASE_EXPECT_TRUE(!!find_ptr);
    CASE_EXPECT_EQ(find_key + 100, find_ptr->data);
    CASE_EXPECT_TRUE(test_wal_object_log_action::kRecursivePushBack == find_ptr->action);
  }

  {
    const auto& wal_cobj = *wal_obj;
    auto find_ptr = wal_cobj.find_log(find_key);
    CASE_EXPECT_TRUE(!!find_ptr);
    CASE_EXPECT_EQ(find_key + 100, find_ptr->data);
    CASE_EXPECT_TRUE(test_wal_object_log_action::kRecursivePushBack == find_ptr->action);
  }

  do {
    auto range = wal_obj->log_all_range();
    CASE_EXPECT_TRUE(range.first != wal_obj->log_end());
    if (range.first == wal_obj->log_end()) {
      break;
    }

    CASE_EXPECT_TRUE(range.first == wal_obj->log_begin());
    CASE_EXPECT_TRUE(range.second == wal_obj->log_end());

    CASE_EXPECT_EQ(find_key + 99, (*range.first)->data);
    CASE_EXPECT_TRUE(test_wal_object_log_action::kDoNothing == (*range.first)->action);
  } while (false);

  do {
    const test_wal_object_type& obj = *wal_obj;
    auto range = obj.log_all_range();
    CASE_EXPECT_TRUE(range.first != wal_obj->log_end());
    if (range.first == wal_obj->log_end()) {
      break;
    }

    CASE_EXPECT_TRUE(range.first == obj.log_cbegin());
    CASE_EXPECT_TRUE(range.second == obj.log_cend());

    CASE_EXPECT_EQ(find_key + 99, (*range.first)->data);
    CASE_EXPECT_TRUE(test_wal_object_log_action::kDoNothing == (*range.first)->action);
  } while (false);

  do {
    auto iter = wal_obj->log_lower_bound(find_key);
    CASE_EXPECT_TRUE(iter != wal_obj->log_end());
    if (iter == wal_obj->log_end()) {
      break;
    }

    CASE_EXPECT_EQ(find_key + 100, (*iter)->data);
    CASE_EXPECT_TRUE(test_wal_object_log_action::kRecursivePushBack == (*iter)->action);
  } while (false);

  do {
    const test_wal_object_type& obj = *wal_obj;
    auto iter = obj.log_lower_bound(find_key);
    CASE_EXPECT_TRUE(iter != obj.log_cend());
    if (iter == obj.log_cend()) {
      break;
    }

    CASE_EXPECT_EQ(find_key + 100, (*iter)->data);
    CASE_EXPECT_TRUE(test_wal_object_log_action::kRecursivePushBack == (*iter)->action);
  } while (false);

  do {
    auto iter = wal_obj->log_upper_bound(find_key + 1);
    CASE_EXPECT_TRUE(iter != wal_obj->log_end());
    if (iter == wal_obj->log_end()) {
      break;
    }

    CASE_EXPECT_EQ(find_key + 102, (*iter)->data);
    CASE_EXPECT_TRUE(test_wal_object_log_action::kFallbackDefault == (*iter)->action);
  } while (false);

  do {
    const test_wal_object_type& obj = *wal_obj;
    auto iter = obj.log_upper_bound(find_key + 1);
    CASE_EXPECT_TRUE(iter != obj.log_cend());
    if (iter == obj.log_cend()) {
      break;
    }

    CASE_EXPECT_EQ(find_key + 102, (*iter)->data);
    CASE_EXPECT_TRUE(test_wal_object_log_action::kFallbackDefault == (*iter)->action);
  } while (false);
}

CASE_TEST(wal_object, gc_st) {
  test_wal_object_log_storage_type storage;
  test_wal_object_context ctx;
  LIBATFRAME_UTILS_NAMESPACE_ID::distributed_system::wal_time_point now = std::chrono::system_clock::now();

  auto conf = create_configure();
  auto vtable = create_vtable();

  conf->gc_expire_duration = std::chrono::duration_cast<test_wal_object_type::duration>(std::chrono::seconds{8});
  conf->max_log_size = 8;
  conf->gc_log_size = 4;

  auto wal_obj = test_wal_object_type::create(vtable, conf, &storage);
  CASE_EXPECT_TRUE(!!wal_obj);
  if (!wal_obj) {
    return;
  }

  CASE_EXPECT_EQ(4, wal_obj->get_configure().gc_log_size);
  CASE_EXPECT_EQ(8, wal_obj->get_configure().max_log_size);
  CASE_EXPECT_TRUE(std::chrono::duration_cast<test_wal_object_type::duration>(std::chrono::seconds{8}) ==
                   wal_obj->get_configure().gc_expire_duration);
  auto old_remove_count = details::g_test_wal_object_stats.event_on_log_removed;
  auto old_add_count = details::g_test_wal_object_stats.event_on_log_added;

  auto begin_key = details::g_test_wal_object_stats.key_alloc;

  for (int i = 0; i < 3; ++i) {
    do {
      now += std::chrono::duration_cast<test_wal_object_type::duration>(std::chrono::seconds{1});
      auto old_default_action_count = details::g_test_wal_object_stats.default_action_count;
      auto old_delegate_action_count = details::g_test_wal_object_stats.delegate_action_count;

      auto previous_key = details::g_test_wal_object_stats.key_alloc;
      auto log = wal_obj->allocate_log(now, test_wal_object_log_action::kDoNothing, ctx);
      CASE_EXPECT_TRUE(!!log);
      if (!log) {
        break;
      }
      log->data = log->log_key + 100;
      CASE_EXPECT_EQ(previous_key + 1, log->log_key);
      CASE_EXPECT_TRUE(test_wal_object_log_action::kDoNothing == log->action);
      CASE_EXPECT_TRUE(now == log->timepoint);

      wal_obj->push_back(log, ctx);
      CASE_EXPECT_EQ(old_default_action_count, details::g_test_wal_object_stats.default_action_count);
      CASE_EXPECT_EQ(old_delegate_action_count + 1, details::g_test_wal_object_stats.delegate_action_count);
    } while (false);

    do {
      now += std::chrono::duration_cast<test_wal_object_type::duration>(std::chrono::seconds{1});
      auto old_default_action_count = details::g_test_wal_object_stats.default_action_count;
      auto old_delegate_action_count = details::g_test_wal_object_stats.delegate_action_count;

      auto previous_key = details::g_test_wal_object_stats.key_alloc;
      auto log = wal_obj->allocate_log(now, test_wal_object_log_action::kRecursivePushBack, ctx);
      CASE_EXPECT_TRUE(!!log);
      if (!log) {
        break;
      }
      log->data = log->log_key + 100;
      CASE_EXPECT_EQ(previous_key + 1, log->log_key);
      CASE_EXPECT_TRUE(test_wal_object_log_action::kRecursivePushBack == log->action);
      CASE_EXPECT_TRUE(now == log->timepoint);

      wal_obj->push_back(log, ctx);
      CASE_EXPECT_EQ(old_default_action_count, details::g_test_wal_object_stats.default_action_count);
      CASE_EXPECT_EQ(old_delegate_action_count + 2, details::g_test_wal_object_stats.delegate_action_count);
    } while (false);

    do {
      now += std::chrono::duration_cast<test_wal_object_type::duration>(std::chrono::seconds{1});
      auto old_default_action_count = details::g_test_wal_object_stats.default_action_count;
      auto old_delegate_action_count = details::g_test_wal_object_stats.delegate_action_count;

      auto previous_key = details::g_test_wal_object_stats.key_alloc;
      auto log = wal_obj->allocate_log(now, test_wal_object_log_action::kFallbackDefault, ctx);
      CASE_EXPECT_TRUE(!!log);
      if (!log) {
        break;
      }
      log->data = log->log_key + 100;
      CASE_EXPECT_EQ(previous_key + 1, log->log_key);
      CASE_EXPECT_TRUE(test_wal_object_log_action::kFallbackDefault == log->action);
      CASE_EXPECT_TRUE(now == log->timepoint);

      wal_obj->push_back(log, ctx);
      CASE_EXPECT_EQ(old_default_action_count + 1, details::g_test_wal_object_stats.default_action_count);
      CASE_EXPECT_EQ(old_delegate_action_count, details::g_test_wal_object_stats.delegate_action_count);
    } while (false);
  }

  CASE_EXPECT_EQ(0, wal_obj->gc(now));
  CASE_EXPECT_EQ(8, wal_obj->get_all_logs().size());
  CASE_EXPECT_EQ(old_add_count + 12, details::g_test_wal_object_stats.event_on_log_added);
  CASE_EXPECT_EQ(old_remove_count + 4, details::g_test_wal_object_stats.event_on_log_removed);

  now += std::chrono::duration_cast<test_wal_object_type::duration>(std::chrono::seconds{4});
  CASE_EXPECT_EQ(0, wal_obj->gc(now, &begin_key, 1));
  CASE_EXPECT_EQ(8, wal_obj->get_all_logs().size());
  CASE_EXPECT_EQ(old_remove_count + 4, details::g_test_wal_object_stats.event_on_log_removed);

  CASE_EXPECT_EQ(1, wal_obj->gc(now, nullptr, 1));
  CASE_EXPECT_EQ(7, wal_obj->get_all_logs().size());
  CASE_EXPECT_EQ(old_remove_count + 5, details::g_test_wal_object_stats.event_on_log_removed);

  CASE_EXPECT_EQ(2, wal_obj->gc(now, nullptr));
  CASE_EXPECT_EQ(5, wal_obj->get_all_logs().size());
  CASE_EXPECT_EQ(old_remove_count + 7, details::g_test_wal_object_stats.event_on_log_removed);

  now += std::chrono::duration_cast<test_wal_object_type::duration>(std::chrono::seconds{2});
  CASE_EXPECT_EQ(1, wal_obj->gc(now, nullptr));
  CASE_EXPECT_EQ(4, wal_obj->get_all_logs().size());
  CASE_EXPECT_EQ(old_remove_count + 8, details::g_test_wal_object_stats.event_on_log_removed);

  auto last_removed_key = (*wal_obj->log_cbegin())->log_key - 1;
  CASE_EXPECT_TRUE(wal_obj->get_last_removed_key() && *wal_obj->get_last_removed_key() == last_removed_key);
}

CASE_TEST(wal_object, ignore_st) {
  test_wal_object_log_storage_type storage;
  test_wal_object_context ctx;
  LIBATFRAME_UTILS_NAMESPACE_ID::distributed_system::wal_time_point now = std::chrono::system_clock::now();

  auto conf = create_configure();
  auto vtable = create_vtable();
  auto wal_obj = test_wal_object_type::create(vtable, conf, &storage);
  CASE_EXPECT_TRUE(!!wal_obj);
  if (!wal_obj) {
    return;
  }

  do {
    auto old_default_action_count = details::g_test_wal_object_stats.default_action_count;
    auto old_delegate_action_count = details::g_test_wal_object_stats.delegate_action_count;

    auto log = wal_obj->allocate_log(now, test_wal_object_log_action::kDoNothing, ctx);
    CASE_EXPECT_TRUE(!!log);
    if (!log) {
      break;
    }
    log->data = log->log_key + 100;
    wal_obj->set_global_ingore_key(log->log_key);
    auto push_back_result = wal_obj->push_back(log, ctx);
    CASE_EXPECT_TRUE(LIBATFRAME_UTILS_NAMESPACE_ID::distributed_system::wal_result_code::kIgnore == push_back_result);

    CASE_EXPECT_EQ(0, wal_obj->get_all_logs().size());
    CASE_EXPECT_EQ(old_default_action_count, details::g_test_wal_object_stats.default_action_count);
    CASE_EXPECT_EQ(old_delegate_action_count, details::g_test_wal_object_stats.delegate_action_count);
  } while (false);

  do {
    auto old_default_action_count = details::g_test_wal_object_stats.default_action_count;
    auto old_delegate_action_count = details::g_test_wal_object_stats.delegate_action_count;

    auto log = wal_obj->allocate_log(now, test_wal_object_log_action::kIgnore, ctx);
    CASE_EXPECT_TRUE(!!log);
    if (!log) {
      break;
    }
    log->data = log->log_key + 100;
    auto push_back_result = wal_obj->push_back(log, ctx);
    CASE_EXPECT_TRUE(LIBATFRAME_UTILS_NAMESPACE_ID::distributed_system::wal_result_code::kIgnore == push_back_result);

    CASE_EXPECT_EQ(0, wal_obj->get_all_logs().size());
    CASE_EXPECT_EQ(old_default_action_count, details::g_test_wal_object_stats.default_action_count);
    CASE_EXPECT_EQ(old_delegate_action_count + 1, details::g_test_wal_object_stats.delegate_action_count);
  } while (false);
}

CASE_TEST(wal_object, reorder_st) {
  test_wal_object_log_storage_type storage;
  test_wal_object_context ctx;
  LIBATFRAME_UTILS_NAMESPACE_ID::distributed_system::wal_time_point now = std::chrono::system_clock::now();

  auto conf = create_configure();
  auto vtable = create_vtable();
  auto wal_obj = test_wal_object_type::create(vtable, conf, &storage);
  CASE_EXPECT_TRUE(!!wal_obj);
  if (!wal_obj) {
    return;
  }

  test_wal_object_type::log_pointer log1;
  test_wal_object_type::log_pointer log2;
  do {
    log1 = wal_obj->allocate_log(now, test_wal_object_log_action::kDoNothing, ctx);
    CASE_EXPECT_TRUE(!!log1);
    if (!log1) {
      break;
    }
    log1->data = log1->log_key + 100;
  } while (false);

  do {
    log2 = wal_obj->allocate_log(now, test_wal_object_log_action::kDoNothing, ctx);
    CASE_EXPECT_TRUE(!!log2);
    if (!log2) {
      break;
    }
    log2->data = log2->log_key + 100;
  } while (false);

  if (!log1 || !log2) {
    return;
  }

  wal_obj->push_back(log2, ctx);
  wal_obj->push_back(log1, ctx);

  auto iter = wal_obj->log_begin();
  CASE_EXPECT_EQ(log1.get(), (*iter).get());

  ++iter;
  CASE_EXPECT_EQ(log2.get(), (*iter).get());
}
}  // namespace st
