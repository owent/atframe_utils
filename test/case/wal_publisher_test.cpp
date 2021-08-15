#include <algorithm>
#include <cstring>
#include <ctime>
#include <memory>
#include <sstream>
#include <vector>

#include <distributed_system/wal_publisher.h>

#include "frame/test_macros.h"

enum class test_wal_publisher_log_action {
  kDoNothing = 0,
  kRecursivePushBack,
  kFallbackDefault,
};

struct test_wal_publisher_log_type {
  util::distributed_system::wal_time_point timepoint;
  int64_t log_key;
  test_wal_publisher_log_action action;
  int data;
};

struct test_wal_publisher_storage_type {
  std::vector<test_wal_publisher_log_type> logs;
  int64_t global_ignore;
};

struct test_wal_publisher_log_action_getter {
  test_wal_publisher_log_action operator()(const test_wal_publisher_log_type& log) { return log.action; }
};

struct test_wal_publisher_context {};

struct test_wal_publisher_private_type {
  test_wal_publisher_storage_type* storage;
};

using test_wal_publisher_log_operator =
    util::distributed_system::wal_log_operator<int64_t, test_wal_publisher_log_type,
                                               test_wal_publisher_log_action_getter>;

using test_wal_publisher_subscriber_type =
    util::distributed_system::wal_subscriber<test_wal_publisher_private_type, uint64_t>;

using test_wal_publisher_type =
    util::distributed_system::wal_publisher<test_wal_publisher_storage_type, test_wal_publisher_log_operator,
                                            test_wal_publisher_context, test_wal_publisher_private_type,
                                            test_wal_publisher_subscriber_type>;

struct test_wal_publisher_stats {
  int64_t key_alloc;
  size_t merge_count;
  size_t delegate_action_count;
  size_t default_action_count;
  size_t event_on_log_added;
  size_t event_on_log_removed;

  size_t send_subscribe_response;
  size_t event_on_subscribe_added;
  size_t event_on_subscribe_removed;

  size_t last_event_subscriber_count;
  size_t last_event_log_count;

  test_wal_publisher_type::object_type::log_type last_log;
  test_wal_publisher_type::subscriber_pointer last_subscriber;
};

namespace details {
test_wal_publisher_stats g_test_wal_object_stats{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
}

static test_wal_publisher_type::vtable_pointer create_vtable() {
  using wal_object_type = test_wal_publisher_type::object_type;
  using wal_publisher_type = test_wal_publisher_type;
  using wal_result_code = util::distributed_system::wal_result_code;

  wal_publisher_type::vtable_pointer ret = std::make_shared<wal_publisher_type::vtable_type>();

  // callbacks for wal_object
  ret->load = [](wal_object_type& wal, const wal_object_type::storage_type& from,
                 wal_object_type::callback_param_type) -> wal_result_code {
    *wal.get_private_data().storage = from;
    return wal_result_code::kOk;
  };

  ret->dump = [](const wal_object_type& wal, wal_object_type::storage_type& to,
                 wal_object_type::callback_param_type) -> wal_result_code {
    to = *wal.get_private_data().storage;
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

  ret->alloc_log_key = [](wal_object_type&,
                          wal_object_type::callback_param_type) -> wal_object_type::log_key_result_type {
    return wal_object_type::log_key_result_type::make_success(++details::g_test_wal_object_stats.key_alloc);
  };

  ret->on_log_added = [](wal_object_type&, const wal_object_type::log_pointer&) {
    ++details::g_test_wal_object_stats.event_on_log_added;
  };

  ret->on_log_removed = [](wal_object_type&, const wal_object_type::log_pointer&) {
    ++details::g_test_wal_object_stats.event_on_log_removed;
  };

  ret->delegate_action[test_wal_publisher_log_action::kDoNothing] =
      [](wal_object_type&, const wal_object_type::log_type& log,
         wal_object_type::callback_param_type) -> wal_result_code {
    ++details::g_test_wal_object_stats.delegate_action_count;
    details::g_test_wal_object_stats.last_log = log;
    details::g_test_wal_object_stats.last_event_subscriber_count = 0;
    details::g_test_wal_object_stats.last_event_log_count = 1;

    return wal_result_code::kOk;
  };

  ret->delegate_action[test_wal_publisher_log_action::kRecursivePushBack] =
      [](wal_object_type& wal, const wal_object_type::log_type& log,
         wal_object_type::callback_param_type param) -> wal_result_code {
    ++details::g_test_wal_object_stats.delegate_action_count;
    details::g_test_wal_object_stats.last_log = log;
    details::g_test_wal_object_stats.last_event_subscriber_count = 0;
    details::g_test_wal_object_stats.last_event_log_count = 1;

    auto new_log = wal.allocate_log(log.timepoint, test_wal_publisher_log_action::kDoNothing, param);
    new_log->data = log.data + 1;
    wal.emplace_back(std::move(new_log), param);

    return wal_result_code::kOk;
  };

  ret->default_action = [](wal_object_type&, const wal_object_type::log_type& log,
                           wal_object_type::callback_param_type) -> wal_result_code {
    ++details::g_test_wal_object_stats.default_action_count;
    details::g_test_wal_object_stats.last_log = log;
    details::g_test_wal_object_stats.last_event_subscriber_count = 0;
    details::g_test_wal_object_stats.last_event_log_count = 1;

    return wal_result_code::kOk;
  };

  // ============ callbacks for wal_publisher ============
  ret->send_snapshot = [](wal_publisher_type& publisher, wal_publisher_type::subscriber_iterator begin,
                          wal_publisher_type::subscriber_iterator end,
                          wal_publisher_type::callback_param_type) -> wal_result_code {
    ++details::g_test_wal_object_stats.default_action_count;

    details::g_test_wal_object_stats.last_event_subscriber_count = 0;
    while (begin != end) {
      ++begin;
      ++details::g_test_wal_object_stats.last_event_subscriber_count;
    }
    details::g_test_wal_object_stats.last_event_log_count = publisher.get_private_data().storage->logs.size();
    if (!publisher.get_private_data().storage->logs.empty()) {
      details::g_test_wal_object_stats.last_log = *publisher.get_private_data().storage->logs.rbegin();
    }

    return wal_result_code::kOk;
  };

  ret->send_logs = [](wal_publisher_type&, wal_publisher_type::log_const_iterator log_begin,
                      wal_publisher_type::log_const_iterator log_end,
                      wal_publisher_type::subscriber_iterator subscriber_begin,
                      wal_publisher_type::subscriber_iterator subscriber_end,
                      wal_publisher_type::callback_param_type) -> wal_result_code {
    ++details::g_test_wal_object_stats.default_action_count;

    details::g_test_wal_object_stats.last_event_subscriber_count = 0;
    while (subscriber_begin != subscriber_end) {
      details::g_test_wal_object_stats.last_subscriber = subscriber_begin->second;
      ++subscriber_begin;
      ++details::g_test_wal_object_stats.last_event_subscriber_count;
    }

    details::g_test_wal_object_stats.last_event_log_count = 0;
    while (log_begin != log_end) {
      details::g_test_wal_object_stats.last_log = **log_begin;
      ++log_begin;
      ++details::g_test_wal_object_stats.last_event_log_count;
    }

    return wal_result_code::kOk;
  };

  ret->subscribe_response = [](wal_publisher_type&, const wal_publisher_type::subscriber_pointer&, wal_result_code,
                               wal_publisher_type::callback_param_type) -> wal_result_code {
    ++details::g_test_wal_object_stats.send_subscribe_response;
    return wal_result_code::kOk;
  };

  ret->check_subscriber = [](wal_publisher_type&, const wal_publisher_type::subscriber_pointer& subscriber,
                             wal_publisher_type::callback_param_type) -> bool {
    details::g_test_wal_object_stats.last_subscriber = subscriber;
    return true;
  };

  ret->on_subscriber_added = [](wal_publisher_type&, const wal_publisher_type::subscriber_pointer& subscriber,
                                wal_publisher_type::callback_param_type) {
    ++details::g_test_wal_object_stats.event_on_subscribe_added;
    details::g_test_wal_object_stats.last_subscriber = subscriber;

    return wal_result_code::kOk;
  };

  ret->on_subscriber_removed = [](wal_publisher_type&, const wal_publisher_type::subscriber_pointer& subscriber,
                                  util::distributed_system::wal_unsubscribe_reason,
                                  wal_publisher_type::callback_param_type) {
    ++details::g_test_wal_object_stats.event_on_subscribe_removed;
    details::g_test_wal_object_stats.last_subscriber = subscriber;

    return wal_result_code::kOk;
  };

  return ret;
}

static test_wal_publisher_type::congfigure_pointer create_configure() {
  test_wal_publisher_type::congfigure_pointer ret = test_wal_publisher_type::make_configure();

  ret->gc_expire_duration = std::chrono::duration_cast<test_wal_publisher_type::duration>(std::chrono::seconds{8});
  ret->max_log_size = 8;
  ret->gc_log_size = 4;
  ret->subscriber_timeout = std::chrono::duration_cast<test_wal_publisher_type::duration>(std::chrono::seconds{5});

  return ret;
}

CASE_TEST(wal_publisher, create_failed) {
  test_wal_publisher_storage_type storage;
  // test_wal_publisher_context ctx;

  auto conf = create_configure();
  auto vtable = create_vtable();
  CASE_EXPECT_EQ(nullptr, test_wal_publisher_type::create(vtable, nullptr, &storage));
  CASE_EXPECT_EQ(nullptr, test_wal_publisher_type::create(nullptr, conf, &storage));

  auto vtable_1 = vtable;
  vtable_1->get_meta = nullptr;
  CASE_EXPECT_EQ(nullptr, test_wal_publisher_type::create(vtable_1, conf, &storage));

  auto vtable_2 = vtable;
  vtable_2->get_log_key = nullptr;
  CASE_EXPECT_EQ(nullptr, test_wal_publisher_type::create(vtable_2, conf, &storage));

  auto vtable_3 = vtable;
  vtable_3->alloc_log_key = nullptr;
  CASE_EXPECT_EQ(nullptr, test_wal_publisher_type::create(vtable_3, conf, &storage));

  auto vtable_4 = vtable;
  vtable_4->send_snapshot = nullptr;
  CASE_EXPECT_EQ(nullptr, test_wal_publisher_type::create(vtable_4, conf, &storage));

  auto vtable_5 = vtable;
  vtable_5->send_logs = nullptr;
  CASE_EXPECT_EQ(nullptr, test_wal_publisher_type::create(vtable_5, conf, &storage));
}