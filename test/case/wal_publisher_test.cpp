#include <algorithm>
#include <chrono>
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
  kRemoveOneSubscriber,
};

namespace std {
template <>
struct hash<test_wal_publisher_log_action> {
  std::size_t operator()(test_wal_publisher_log_action const& s) const noexcept {
    return std::hash<int>{}(static_cast<int>(s));
  }
};
}  // namespace std

struct test_wal_publisher_log_type {
  util::distributed_system::wal_time_point timepoint;
  int64_t log_key;
  test_wal_publisher_log_action action;

  union {
    int64_t data;
    void* publisher;
  };

  inline explicit test_wal_publisher_log_type(util::distributed_system::wal_time_point t, int64_t k,
                                              test_wal_publisher_log_action act, int64_t d)
      : timepoint(t), log_key(k), action(act), data(d) {}
  inline test_wal_publisher_log_type()
      : timepoint(std::chrono::system_clock::from_time_t(0)),
        log_key(0),
        action(test_wal_publisher_log_action::kDoNothing),
        data(0) {}
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

  inline test_wal_publisher_private_type() : storage(nullptr) {}
  inline explicit test_wal_publisher_private_type(test_wal_publisher_storage_type* input) : storage(input) {}
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

  size_t send_logs_count;
  size_t send_snapshot_count;
  size_t send_subscribe_response;
  size_t event_on_subscribe_heartbeat;
  size_t event_on_subscribe_added;
  size_t event_on_subscribe_removed;

  size_t last_event_subscriber_count;
  size_t last_event_log_count;

  test_wal_publisher_type::object_type::log_type last_log;
  test_wal_publisher_type::subscriber_pointer last_subscriber;
};

namespace details {
test_wal_publisher_stats g_test_wal_publisher_stats{
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, test_wal_publisher_log_type(), nullptr};
}

static test_wal_publisher_type::vtable_pointer create_vtable() {
  using wal_object_type = test_wal_publisher_type::object_type;
  using wal_publisher_type = test_wal_publisher_type;
  using wal_result_code = util::distributed_system::wal_result_code;
  using wal_unsubscribe_reason = util::distributed_system::wal_unsubscribe_reason;

  wal_publisher_type::vtable_pointer ret = std::make_shared<wal_publisher_type::vtable_type>();

  // callbacks for wal_object
  ret->load = [](wal_object_type& wal, const wal_object_type::storage_type& from,
                 wal_object_type::callback_param_type) -> wal_result_code {
    *wal.get_private_data().storage = from;

    wal_publisher_type::log_container_type container;
    for (auto& log : from.logs) {
      container.emplace_back(std::make_shared<wal_object_type::log_type>(log));
    }

    wal.assign_logs(container);
    if (!from.logs.empty()) {
      wal.set_last_removed_key((*from.logs.rbegin()).log_key - 1);
    }
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
    ++details::g_test_wal_publisher_stats.merge_count;
    to.data = from.data;
  };

  ret->get_log_key = [](const wal_object_type&, const wal_object_type::log_type& log) -> wal_object_type::log_key_type {
    return log.log_key;
  };

  ret->allocate_log_key = [](wal_object_type&, const wal_object_type::log_type&,
                             wal_object_type::callback_param_type) -> wal_object_type::log_key_result_type {
    return wal_object_type::log_key_result_type::make_success(++details::g_test_wal_publisher_stats.key_alloc);
  };

  ret->on_log_added = [](wal_object_type&, const wal_object_type::log_pointer&) {
    ++details::g_test_wal_publisher_stats.event_on_log_added;
  };

  ret->on_log_removed = [](wal_object_type&, const wal_object_type::log_pointer&) {
    ++details::g_test_wal_publisher_stats.event_on_log_removed;
  };

  ret->log_action_delegate[test_wal_publisher_log_action::kDoNothing].action =
      [](wal_object_type&, const wal_object_type::log_type& log,
         wal_object_type::callback_param_type) -> wal_result_code {
    ++details::g_test_wal_publisher_stats.delegate_action_count;
    details::g_test_wal_publisher_stats.last_log = log;
    details::g_test_wal_publisher_stats.last_event_subscriber_count = 0;
    details::g_test_wal_publisher_stats.last_event_log_count = 1;

    return wal_result_code::kOk;
  };

  ret->log_action_delegate[test_wal_publisher_log_action::kRecursivePushBack].action =
      [](wal_object_type& wal, const wal_object_type::log_type& log,
         wal_object_type::callback_param_type param) -> wal_result_code {
    ++details::g_test_wal_publisher_stats.delegate_action_count;
    details::g_test_wal_publisher_stats.last_log = log;
    details::g_test_wal_publisher_stats.last_event_subscriber_count = 0;
    details::g_test_wal_publisher_stats.last_event_log_count = 1;

    auto new_log = wal.allocate_log(log.timepoint, test_wal_publisher_log_action::kDoNothing, param);
    new_log->data = log.data + 1;
    wal.emplace_back(std::move(new_log), param);

    return wal_result_code::kOk;
  };

  ret->log_action_delegate[test_wal_publisher_log_action::kRemoveOneSubscriber].action =
      [](wal_object_type&, const wal_object_type::log_type& log,
         wal_object_type::callback_param_type param) -> wal_result_code {
    ++details::g_test_wal_publisher_stats.delegate_action_count;
    details::g_test_wal_publisher_stats.last_log = log;
    details::g_test_wal_publisher_stats.last_event_subscriber_count = 1;
    details::g_test_wal_publisher_stats.last_event_log_count = 1;

    test_wal_publisher_type* publisher = reinterpret_cast<test_wal_publisher_type*>(log.publisher);
    publisher->remove_subscriber(publisher->subscriber_all_range().first->first, wal_unsubscribe_reason::kClientRequest,
                                 param);

    return wal_result_code::kOk;
  };

  ret->default_delegate.action = [](wal_object_type&, const wal_object_type::log_type& log,
                                    wal_object_type::callback_param_type) -> wal_result_code {
    ++details::g_test_wal_publisher_stats.default_action_count;
    details::g_test_wal_publisher_stats.last_log = log;
    details::g_test_wal_publisher_stats.last_event_subscriber_count = 0;
    details::g_test_wal_publisher_stats.last_event_log_count = 1;

    return wal_result_code::kOk;
  };

  // ============ callbacks for wal_publisher ============
  ret->send_snapshot = [](wal_publisher_type& publisher, wal_publisher_type::subscriber_iterator begin,
                          wal_publisher_type::subscriber_iterator end,
                          wal_publisher_type::callback_param_type) -> wal_result_code {
    ++details::g_test_wal_publisher_stats.default_action_count;

    details::g_test_wal_publisher_stats.last_event_subscriber_count = 0;
    while (begin != end) {
      ++begin;
      ++details::g_test_wal_publisher_stats.last_event_subscriber_count;
    }
    details::g_test_wal_publisher_stats.last_event_log_count = publisher.get_private_data().storage->logs.size();
    if (!publisher.get_private_data().storage->logs.empty()) {
      details::g_test_wal_publisher_stats.last_log = *publisher.get_private_data().storage->logs.rbegin();
    }

    ++details::g_test_wal_publisher_stats.send_snapshot_count;
    return wal_result_code::kOk;
  };

  ret->send_logs = [](wal_publisher_type&, wal_publisher_type::log_const_iterator log_begin,
                      wal_publisher_type::log_const_iterator log_end,
                      wal_publisher_type::subscriber_iterator subscriber_begin,
                      wal_publisher_type::subscriber_iterator subscriber_end,
                      wal_publisher_type::callback_param_type) -> wal_result_code {
    ++details::g_test_wal_publisher_stats.default_action_count;

    details::g_test_wal_publisher_stats.last_event_subscriber_count = 0;
    while (subscriber_begin != subscriber_end) {
      details::g_test_wal_publisher_stats.last_subscriber = subscriber_begin->second;
      ++subscriber_begin;
      ++details::g_test_wal_publisher_stats.last_event_subscriber_count;
    }

    details::g_test_wal_publisher_stats.last_event_log_count = 0;
    while (log_begin != log_end) {
      details::g_test_wal_publisher_stats.last_log = **log_begin;
      ++log_begin;
      ++details::g_test_wal_publisher_stats.last_event_log_count;
    }

    ++details::g_test_wal_publisher_stats.send_logs_count;
    return wal_result_code::kOk;
  };

  ret->subscribe_response = [](wal_publisher_type&, const wal_publisher_type::subscriber_pointer&, wal_result_code,
                               wal_publisher_type::callback_param_type) -> wal_result_code {
    ++details::g_test_wal_publisher_stats.send_subscribe_response;
    return wal_result_code::kOk;
  };

  ret->check_subscriber = [](wal_publisher_type&, const wal_publisher_type::subscriber_pointer& subscriber,
                             wal_publisher_type::callback_param_type) -> bool {
    details::g_test_wal_publisher_stats.last_subscriber = subscriber;
    return true;
  };

  ret->on_subscriber_request = [](wal_publisher_type&, const wal_publisher_type::subscriber_pointer& subscriber,
                                  wal_publisher_type::callback_param_type) {
    ++details::g_test_wal_publisher_stats.event_on_subscribe_heartbeat;
    details::g_test_wal_publisher_stats.last_subscriber = subscriber;

    return wal_result_code::kOk;
  };

  ret->on_subscriber_added = [](wal_publisher_type&, const wal_publisher_type::subscriber_pointer& subscriber,
                                wal_publisher_type::callback_param_type) {
    ++details::g_test_wal_publisher_stats.event_on_subscribe_added;
    details::g_test_wal_publisher_stats.last_subscriber = subscriber;

    return wal_result_code::kOk;
  };

  ret->on_subscriber_removed = [](wal_publisher_type&, const wal_publisher_type::subscriber_pointer& subscriber,
                                  util::distributed_system::wal_unsubscribe_reason,
                                  wal_publisher_type::callback_param_type) {
    ++details::g_test_wal_publisher_stats.event_on_subscribe_removed;
    details::g_test_wal_publisher_stats.last_subscriber = subscriber;

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
  vtable_3->allocate_log_key = nullptr;
  CASE_EXPECT_EQ(nullptr, test_wal_publisher_type::create(vtable_3, conf, &storage));

  auto vtable_4 = vtable;
  vtable_4->send_snapshot = nullptr;
  CASE_EXPECT_EQ(nullptr, test_wal_publisher_type::create(vtable_4, conf, &storage));

  auto vtable_5 = vtable;
  vtable_5->send_logs = nullptr;
  CASE_EXPECT_EQ(nullptr, test_wal_publisher_type::create(vtable_5, conf, &storage));
}

CASE_TEST(wal_publisher, load_and_dump) {
  auto old_action_count = details::g_test_wal_publisher_stats.default_action_count +
                          details::g_test_wal_publisher_stats.delegate_action_count;

  test_wal_publisher_storage_type load_storege;
  util::distributed_system::wal_time_point now = std::chrono::system_clock::now();
  load_storege.global_ignore = 123;
  load_storege.logs.push_back(test_wal_publisher_log_type{now, 124, test_wal_publisher_log_action::kDoNothing, 124});
  load_storege.logs.push_back(
      test_wal_publisher_log_type{now, 125, test_wal_publisher_log_action::kFallbackDefault, 125});
  load_storege.logs.push_back(
      test_wal_publisher_log_type{now, 126, test_wal_publisher_log_action::kRecursivePushBack, 126});

  test_wal_publisher_storage_type storage;
  test_wal_publisher_context ctx;

  auto conf = create_configure();
  auto vtable = create_vtable();
  auto publisher = test_wal_publisher_type::create(vtable, conf, &storage);
  CASE_EXPECT_TRUE(!!publisher);
  if (!publisher) {
    return;
  }
  CASE_EXPECT_TRUE(util::distributed_system::wal_result_code::kOk == publisher->load(load_storege, ctx));
  CASE_EXPECT_EQ(old_action_count, details::g_test_wal_publisher_stats.default_action_count +
                                       details::g_test_wal_publisher_stats.delegate_action_count);
  CASE_EXPECT_EQ(4, publisher->get_configure().gc_log_size);
  CASE_EXPECT_EQ(&publisher->get_log_manager().get_configure(), &publisher->get_configure());

  CASE_EXPECT_EQ(123, storage.global_ignore);
  CASE_EXPECT_EQ(3, storage.logs.size());
  CASE_EXPECT_EQ(124, storage.logs[0].data);
  CASE_EXPECT_TRUE(test_wal_publisher_log_action::kRecursivePushBack == storage.logs[2].action);

  CASE_EXPECT_EQ(3, publisher->get_log_manager().get_all_logs().size());
  CASE_EXPECT_EQ(124, (*publisher->get_log_manager().get_all_logs().begin())->data);
  CASE_EXPECT_TRUE(test_wal_publisher_log_action::kRecursivePushBack ==
                   (*publisher->get_log_manager().get_all_logs().rbegin())->action);

  // dump
  test_wal_publisher_storage_type dump_storege;
  CASE_EXPECT_TRUE(util::distributed_system::wal_result_code::kOk == publisher->dump(dump_storege, ctx));

  CASE_EXPECT_EQ(123, dump_storege.global_ignore);
  CASE_EXPECT_EQ(3, dump_storege.logs.size());
  CASE_EXPECT_EQ(124, dump_storege.logs[0].data);
  CASE_EXPECT_TRUE(test_wal_publisher_log_action::kRecursivePushBack == dump_storege.logs[2].action);

  // check broadcast key
  CASE_EXPECT_TRUE(!!publisher->get_broadcast_key_bound());
  if (publisher->get_broadcast_key_bound()) {
    CASE_EXPECT_EQ(126, *publisher->get_broadcast_key_bound());
  }
}

CASE_TEST(wal_publisher, subscriber_basic_operation) {
  util::distributed_system::wal_time_point now = std::chrono::system_clock::now();
  test_wal_publisher_storage_type storage;
  test_wal_publisher_context ctx;

  auto conf = create_configure();
  auto vtable = create_vtable();
  auto publisher = test_wal_publisher_type::create(vtable, conf, &storage);
  CASE_EXPECT_TRUE(!!publisher);
  if (!publisher) {
    return;
  }

  uint64_t subscriber_key_1 = 1;
  uint64_t subscriber_key_2 = 2;
  uint64_t subscriber_key_3 = 3;

  CASE_EXPECT_EQ(nullptr, publisher->find_subscriber(subscriber_key_1, ctx).get());
  auto subscriber = publisher->create_subscriber(subscriber_key_1, now, 0, ctx, &storage);
  CASE_EXPECT_NE(nullptr, subscriber.get());
  CASE_EXPECT_TRUE(now == subscriber->get_last_heartbeat_time_point());
  now += std::chrono::seconds(1);
  auto subscriber_copy = publisher->create_subscriber(subscriber_key_1, now, 0, ctx, &storage);
  CASE_EXPECT_EQ(subscriber.get(), subscriber_copy.get());
  CASE_EXPECT_TRUE(now == subscriber->get_last_heartbeat_time_point());

  publisher->create_subscriber(subscriber_key_2, now, 0, ctx, &storage);
  publisher->create_subscriber(subscriber_key_3, now, 0, ctx, &storage);

  auto find_result = publisher->find_subscriber(subscriber_key_1, ctx);
  CASE_EXPECT_EQ(subscriber.get(), find_result.get());
}

static void test_wal_publisher_add_logs(test_wal_publisher_type::object_type& wal_obj, test_wal_publisher_context ctx,
                                        util::distributed_system::wal_time_point t1,
                                        util::distributed_system::wal_time_point t2,
                                        util::distributed_system::wal_time_point t3) {
  do {
    auto old_default_action_count = details::g_test_wal_publisher_stats.default_action_count;
    auto old_delegate_action_count = details::g_test_wal_publisher_stats.delegate_action_count;

    auto previous_key = details::g_test_wal_publisher_stats.key_alloc;
    auto log = wal_obj.allocate_log(t1, test_wal_publisher_log_action::kDoNothing, ctx);
    CASE_EXPECT_TRUE(!!log);
    if (!log) {
      break;
    }
    log->data = log->log_key + 100;
    CASE_EXPECT_EQ(previous_key + 1, log->log_key);
    CASE_EXPECT_TRUE(test_wal_publisher_log_action::kDoNothing == log->action);
    CASE_EXPECT_TRUE(t1 == log->timepoint);

    wal_obj.push_back(log, ctx);
    CASE_EXPECT_EQ(old_default_action_count, details::g_test_wal_publisher_stats.default_action_count);
    CASE_EXPECT_EQ(old_delegate_action_count + 1, details::g_test_wal_publisher_stats.delegate_action_count);
  } while (false);

  do {
    auto old_default_action_count = details::g_test_wal_publisher_stats.default_action_count;
    auto old_delegate_action_count = details::g_test_wal_publisher_stats.delegate_action_count;

    auto previous_key = details::g_test_wal_publisher_stats.key_alloc;
    auto log = wal_obj.allocate_log(t2, test_wal_publisher_log_action::kRecursivePushBack, ctx);
    CASE_EXPECT_TRUE(!!log);
    if (!log) {
      break;
    }
    log->data = log->log_key + 100;
    CASE_EXPECT_EQ(previous_key + 1, log->log_key);
    CASE_EXPECT_TRUE(test_wal_publisher_log_action::kRecursivePushBack == log->action);
    CASE_EXPECT_TRUE(t2 == log->timepoint);

    wal_obj.push_back(log, ctx);
    CASE_EXPECT_EQ(old_default_action_count, details::g_test_wal_publisher_stats.default_action_count);
    CASE_EXPECT_EQ(old_delegate_action_count + 2, details::g_test_wal_publisher_stats.delegate_action_count);
  } while (false);

  do {
    auto old_default_action_count = details::g_test_wal_publisher_stats.default_action_count;
    auto old_delegate_action_count = details::g_test_wal_publisher_stats.delegate_action_count;

    auto previous_key = details::g_test_wal_publisher_stats.key_alloc;
    auto log = wal_obj.allocate_log(t3, test_wal_publisher_log_action::kFallbackDefault, ctx);
    CASE_EXPECT_TRUE(!!log);
    if (!log) {
      break;
    }
    log->data = log->log_key + 100;
    CASE_EXPECT_EQ(previous_key + 1, log->log_key);
    CASE_EXPECT_TRUE(test_wal_publisher_log_action::kFallbackDefault == log->action);
    CASE_EXPECT_TRUE(t3 == log->timepoint);

    wal_obj.push_back(log, ctx);
    CASE_EXPECT_EQ(old_default_action_count + 1, details::g_test_wal_publisher_stats.default_action_count);
    CASE_EXPECT_EQ(old_delegate_action_count, details::g_test_wal_publisher_stats.delegate_action_count);
  } while (false);
}

CASE_TEST(wal_publisher, subscriber_heartbeat) {
  util::distributed_system::wal_time_point t1 = std::chrono::system_clock::now();
  util::distributed_system::wal_time_point t2 =
      t1 + std::chrono::duration_cast<test_wal_publisher_type::duration>(std::chrono::seconds{3});
  util::distributed_system::wal_time_point t3 =
      t1 + std::chrono::duration_cast<test_wal_publisher_type::duration>(std::chrono::seconds{6});
  test_wal_publisher_storage_type storage;
  test_wal_publisher_context ctx;

  auto conf = create_configure();
  if (conf) {
    conf->subscriber_timeout = std::chrono::duration_cast<test_wal_publisher_type::duration>(std::chrono::seconds{5});
  }
  auto vtable = create_vtable();
  auto publisher = test_wal_publisher_type::create(vtable, conf, &storage);
  CASE_EXPECT_TRUE(!!publisher);
  if (!publisher) {
    return;
  }

  uint64_t subscriber_key_1 = 1;
  uint64_t subscriber_key_2 = 2;
  uint64_t subscriber_key_3 = 3;

  publisher->get_log_manager().set_last_removed_key(details::g_test_wal_publisher_stats.key_alloc);
  test_wal_publisher_add_logs(publisher->get_log_manager(), ctx, t1, t2, t3);

  auto event_on_subscribe_heartbeat = details::g_test_wal_publisher_stats.event_on_subscribe_heartbeat;
  auto event_subscribe_added = details::g_test_wal_publisher_stats.event_on_subscribe_added;
  auto event_subscribe_removed = details::g_test_wal_publisher_stats.event_on_subscribe_removed;
  auto send_logs_count = details::g_test_wal_publisher_stats.send_logs_count;
  auto send_snapshot_count = details::g_test_wal_publisher_stats.send_snapshot_count;

  auto subscriber_1 = publisher->create_subscriber(subscriber_key_1, t1, 0, ctx, &storage);
  auto subscriber_2 = publisher->create_subscriber(subscriber_key_2, t2, 0, ctx, &storage);
  auto subscriber_3 = publisher->create_subscriber(subscriber_key_3, t3, 0, ctx, &storage);

  CASE_EXPECT_EQ(event_on_subscribe_heartbeat + 3, details::g_test_wal_publisher_stats.event_on_subscribe_heartbeat);
  CASE_EXPECT_EQ(event_subscribe_added + 3, details::g_test_wal_publisher_stats.event_on_subscribe_added);
  CASE_EXPECT_EQ(event_subscribe_removed, details::g_test_wal_publisher_stats.event_on_subscribe_removed);
  CASE_EXPECT_EQ(send_logs_count, details::g_test_wal_publisher_stats.send_logs_count);
  CASE_EXPECT_EQ(send_snapshot_count + 3, details::g_test_wal_publisher_stats.send_snapshot_count);

  CASE_EXPECT_GT(publisher->tick(t3, ctx), 0);

  CASE_EXPECT_EQ(event_subscribe_removed + 1, details::g_test_wal_publisher_stats.event_on_subscribe_removed);

  auto all_iter = publisher->get_subscribe_manager().all_range();
  size_t count = 0;
  while (all_iter.first != all_iter.second) {
    CASE_EXPECT_TRUE(all_iter.first != all_iter.second || count == 2);
    ++all_iter.first;
    ++count;
  }

  publisher->remove_subscriber(subscriber_key_1, util::distributed_system::wal_unsubscribe_reason::kClientRequest, ctx);
  all_iter = publisher->get_subscribe_manager().all_range();
  count = 0;
  while (all_iter.first != all_iter.second) {
    CASE_EXPECT_TRUE(all_iter.first != all_iter.second || count == 2);
    ++all_iter.first;
    ++count;
  }
  CASE_EXPECT_EQ(event_subscribe_removed + 1, details::g_test_wal_publisher_stats.event_on_subscribe_removed);

  publisher->remove_subscriber(subscriber_key_3, util::distributed_system::wal_unsubscribe_reason::kClientRequest, ctx);
  all_iter = publisher->get_subscribe_manager().all_range();
  count = 0;
  while (all_iter.first != all_iter.second) {
    CASE_EXPECT_TRUE(all_iter.first != all_iter.second || count == 2);
    ++all_iter.first;
    ++count;
  }
  CASE_EXPECT_EQ(event_subscribe_removed + 2, details::g_test_wal_publisher_stats.event_on_subscribe_removed);
}

CASE_TEST(wal_publisher, subscriber_send_snapshot) {
  util::distributed_system::wal_time_point t1 = std::chrono::system_clock::now();
  util::distributed_system::wal_time_point t2 =
      t1 + std::chrono::duration_cast<test_wal_publisher_type::duration>(std::chrono::seconds{3});
  util::distributed_system::wal_time_point t3 =
      t1 + std::chrono::duration_cast<test_wal_publisher_type::duration>(std::chrono::seconds{6});
  test_wal_publisher_storage_type storage;
  test_wal_publisher_context ctx;

  auto conf = create_configure();
  if (conf) {
    conf->subscriber_timeout = std::chrono::duration_cast<test_wal_publisher_type::duration>(std::chrono::seconds{5});
  }
  auto vtable = create_vtable();
  auto publisher = test_wal_publisher_type::create(vtable, conf, &storage);
  CASE_EXPECT_TRUE(!!publisher);
  if (!publisher) {
    return;
  }

  uint64_t subscriber_key_1 = 1;

  publisher->get_log_manager().set_last_removed_key(details::g_test_wal_publisher_stats.key_alloc);
  test_wal_publisher_add_logs(publisher->get_log_manager(), ctx, t1, t2, t3);

  auto event_on_subscribe_heartbeat = details::g_test_wal_publisher_stats.event_on_subscribe_heartbeat;
  auto event_subscribe_added = details::g_test_wal_publisher_stats.event_on_subscribe_added;
  auto event_subscribe_removed = details::g_test_wal_publisher_stats.event_on_subscribe_removed;
  auto send_logs_count = details::g_test_wal_publisher_stats.send_logs_count;
  auto send_snapshot_count = details::g_test_wal_publisher_stats.send_snapshot_count;

  auto subscriber_1 = publisher->create_subscriber(subscriber_key_1, t1, 0, ctx, &storage);

  CASE_EXPECT_EQ(event_on_subscribe_heartbeat + 1, details::g_test_wal_publisher_stats.event_on_subscribe_heartbeat);
  CASE_EXPECT_EQ(event_subscribe_added + 1, details::g_test_wal_publisher_stats.event_on_subscribe_added);
  CASE_EXPECT_EQ(event_subscribe_removed, details::g_test_wal_publisher_stats.event_on_subscribe_removed);
  CASE_EXPECT_EQ(send_logs_count, details::g_test_wal_publisher_stats.send_logs_count);
  CASE_EXPECT_EQ(send_snapshot_count + 1, details::g_test_wal_publisher_stats.send_snapshot_count);

  auto last_removed_key = (*publisher->get_log_manager().log_cbegin())->log_key - 1;
  publisher->get_log_manager().set_last_removed_key(last_removed_key);

  publisher->receive_subscribe_request(subscriber_key_1, last_removed_key, t3, ctx);
  {
    auto subscriber = publisher->find_subscriber(subscriber_key_1, ctx);
    CASE_EXPECT_TRUE(!!subscriber);
    if (subscriber) {
      CASE_EXPECT_FALSE(subscriber->is_offline(t3 + std::chrono::seconds{4}));
      CASE_EXPECT_TRUE(subscriber->is_offline(t3 + std::chrono::seconds{5}));
      CASE_EXPECT_EQ(t3.time_since_epoch().count(),
                     subscriber->get_last_heartbeat_time_point().time_since_epoch().count());
      CASE_EXPECT_EQ(5, std::chrono::duration_cast<std::chrono::seconds>(subscriber->get_heartbeat_timeout()).count());
    }
  }

  CASE_EXPECT_EQ(event_on_subscribe_heartbeat + 2, details::g_test_wal_publisher_stats.event_on_subscribe_heartbeat);

  CASE_EXPECT_EQ(4, details::g_test_wal_publisher_stats.last_event_log_count);
  CASE_EXPECT_EQ(send_logs_count + 1, details::g_test_wal_publisher_stats.send_logs_count);
  CASE_EXPECT_EQ(send_snapshot_count + 1, details::g_test_wal_publisher_stats.send_snapshot_count);

  publisher->receive_subscribe_request(subscriber_key_1, last_removed_key - 1, t3, ctx);

  CASE_EXPECT_EQ(event_on_subscribe_heartbeat + 3, details::g_test_wal_publisher_stats.event_on_subscribe_heartbeat);

  CASE_EXPECT_EQ(0, details::g_test_wal_publisher_stats.last_event_log_count);
  CASE_EXPECT_EQ(send_logs_count + 1, details::g_test_wal_publisher_stats.send_logs_count);
  CASE_EXPECT_EQ(send_snapshot_count + 2, details::g_test_wal_publisher_stats.send_snapshot_count);
}

CASE_TEST(wal_publisher, subscriber_send_logs) {
  util::distributed_system::wal_time_point t1 = std::chrono::system_clock::now();
  util::distributed_system::wal_time_point t2 =
      t1 + std::chrono::duration_cast<test_wal_publisher_type::duration>(std::chrono::seconds{3});
  util::distributed_system::wal_time_point t3 =
      t1 + std::chrono::duration_cast<test_wal_publisher_type::duration>(std::chrono::seconds{6});
  test_wal_publisher_storage_type storage;
  test_wal_publisher_context ctx;

  auto conf = create_configure();
  if (conf) {
    conf->subscriber_timeout = std::chrono::duration_cast<test_wal_publisher_type::duration>(std::chrono::seconds{5});
  }
  auto vtable = create_vtable();
  auto publisher = test_wal_publisher_type::create(vtable, conf, &storage);
  CASE_EXPECT_TRUE(!!publisher);
  if (!publisher) {
    return;
  }

  uint64_t subscriber_key_1 = 1;

  publisher->get_log_manager().set_last_removed_key(details::g_test_wal_publisher_stats.key_alloc);
  test_wal_publisher_add_logs(publisher->get_log_manager(), ctx, t1, t2, t3);

  auto event_subscribe_added = details::g_test_wal_publisher_stats.event_on_subscribe_added;
  auto event_subscribe_removed = details::g_test_wal_publisher_stats.event_on_subscribe_removed;
  auto send_logs_count = details::g_test_wal_publisher_stats.send_logs_count;
  auto send_snapshot_count = details::g_test_wal_publisher_stats.send_snapshot_count;

  auto subscriber_1 = publisher->create_subscriber(subscriber_key_1, t3, 0, ctx, &storage);

  CASE_EXPECT_EQ(event_subscribe_added + 1, details::g_test_wal_publisher_stats.event_on_subscribe_added);
  CASE_EXPECT_EQ(event_subscribe_removed, details::g_test_wal_publisher_stats.event_on_subscribe_removed);
  CASE_EXPECT_EQ(send_logs_count, details::g_test_wal_publisher_stats.send_logs_count);
  CASE_EXPECT_EQ(send_snapshot_count + 1, details::g_test_wal_publisher_stats.send_snapshot_count);

  auto from_key = (*publisher->get_log_manager().log_cbegin())->log_key + 1;

  publisher->receive_subscribe_request(subscriber_key_1, from_key, t3, ctx);

  CASE_EXPECT_EQ(2, details::g_test_wal_publisher_stats.last_event_log_count);
  CASE_EXPECT_EQ(send_logs_count + 1, details::g_test_wal_publisher_stats.send_logs_count);
  CASE_EXPECT_EQ(send_snapshot_count + 1, details::g_test_wal_publisher_stats.send_snapshot_count);
}

CASE_TEST(wal_publisher, remove_subscriber_by_check_callback) {
  util::distributed_system::wal_time_point t1 = std::chrono::system_clock::now();
  util::distributed_system::wal_time_point t2 =
      t1 + std::chrono::duration_cast<test_wal_publisher_type::duration>(std::chrono::seconds{3});
  util::distributed_system::wal_time_point t3 =
      t1 + std::chrono::duration_cast<test_wal_publisher_type::duration>(std::chrono::seconds{6});
  test_wal_publisher_storage_type storage;
  test_wal_publisher_context ctx;

  auto conf = create_configure();
  if (conf) {
    conf->subscriber_timeout = std::chrono::duration_cast<test_wal_publisher_type::duration>(std::chrono::seconds{5});
  }

  std::shared_ptr<bool> check_result = std::make_shared<bool>(true);
  auto vtable = create_vtable();
  vtable->check_subscriber = [check_result](
                                 test_wal_publisher_type&, const test_wal_publisher_type::subscriber_pointer&,
                                 test_wal_publisher_type::callback_param_type) -> bool { return *check_result; };

  auto publisher = test_wal_publisher_type::create(vtable, conf, &storage);
  CASE_EXPECT_TRUE(!!publisher);
  if (!publisher) {
    return;
  }

  uint64_t subscriber_key_1 = 1;
  uint64_t subscriber_key_2 = 2;

  publisher->get_log_manager().set_last_removed_key(details::g_test_wal_publisher_stats.key_alloc);
  test_wal_publisher_add_logs(publisher->get_log_manager(), ctx, t1, t2, t3);

  auto event_subscribe_added = details::g_test_wal_publisher_stats.event_on_subscribe_added;
  auto event_subscribe_removed = details::g_test_wal_publisher_stats.event_on_subscribe_removed;

  auto subscriber_1 = publisher->create_subscriber(subscriber_key_1, t2, 0, ctx, &storage);
  auto subscriber_2 = publisher->create_subscriber(subscriber_key_2, t3, 0, ctx, &storage);

  CASE_EXPECT_EQ(event_subscribe_added + 2, details::g_test_wal_publisher_stats.event_on_subscribe_added);
  CASE_EXPECT_EQ(event_subscribe_removed, details::g_test_wal_publisher_stats.event_on_subscribe_removed);

  CASE_EXPECT_NE(nullptr, publisher->find_subscriber(subscriber_key_1, ctx).get());
  CASE_EXPECT_EQ(event_subscribe_added + 2, details::g_test_wal_publisher_stats.event_on_subscribe_added);
  CASE_EXPECT_EQ(event_subscribe_removed, details::g_test_wal_publisher_stats.event_on_subscribe_removed);

  *check_result = false;
  CASE_EXPECT_EQ(nullptr, publisher->find_subscriber(subscriber_key_1, ctx).get());
  CASE_EXPECT_EQ(event_subscribe_added + 2, details::g_test_wal_publisher_stats.event_on_subscribe_added);
  CASE_EXPECT_EQ(event_subscribe_removed + 1, details::g_test_wal_publisher_stats.event_on_subscribe_removed);

  CASE_EXPECT_EQ(subscriber_key_2, publisher->get_subscribe_manager().all_range().first->second->get_key());
}

CASE_TEST(wal_publisher, broadcast) {
  util::distributed_system::wal_time_point t1 = std::chrono::system_clock::now();
  util::distributed_system::wal_time_point t2 =
      t1 + std::chrono::duration_cast<test_wal_publisher_type::duration>(std::chrono::seconds{3});
  util::distributed_system::wal_time_point t3 =
      t1 + std::chrono::duration_cast<test_wal_publisher_type::duration>(std::chrono::seconds{6});
  test_wal_publisher_storage_type storage;
  test_wal_publisher_context ctx;

  auto conf = create_configure();
  auto vtable = create_vtable();
  auto publisher = test_wal_publisher_type::create(vtable, conf, &storage);
  CASE_EXPECT_TRUE(!!publisher);
  if (!publisher) {
    return;
  }

  uint64_t subscriber_key_1 = 1;
  uint64_t subscriber_key_2 = 2;
  uint64_t subscriber_key_3 = 3;

  publisher->get_log_manager().set_last_removed_key(details::g_test_wal_publisher_stats.key_alloc);
  test_wal_publisher_add_logs(publisher->get_log_manager(), ctx, t1, t2, t3);

  auto event_subscribe_added = details::g_test_wal_publisher_stats.event_on_subscribe_added;
  auto event_subscribe_removed = details::g_test_wal_publisher_stats.event_on_subscribe_removed;
  auto send_logs_count = details::g_test_wal_publisher_stats.send_logs_count;

  auto subscriber_1 = publisher->create_subscriber(subscriber_key_1, t1, 0, ctx, &storage);
  auto subscriber_2 = publisher->create_subscriber(subscriber_key_2, t2, 0, ctx, &storage);
  auto subscriber_3 = publisher->create_subscriber(subscriber_key_3, t3, 0, ctx, &storage);

  CASE_EXPECT_EQ(event_subscribe_added + 3, details::g_test_wal_publisher_stats.event_on_subscribe_added);
  CASE_EXPECT_EQ(event_subscribe_removed, details::g_test_wal_publisher_stats.event_on_subscribe_removed);
  CASE_EXPECT_EQ(send_logs_count, details::g_test_wal_publisher_stats.send_logs_count);

  CASE_EXPECT_EQ(publisher->broadcast(ctx), 4);

  CASE_EXPECT_EQ(event_subscribe_removed, details::g_test_wal_publisher_stats.event_on_subscribe_removed);
  CASE_EXPECT_EQ(send_logs_count + 1, details::g_test_wal_publisher_stats.send_logs_count);
  CASE_EXPECT_EQ(3, details::g_test_wal_publisher_stats.last_event_subscriber_count);
  CASE_EXPECT_EQ(4, details::g_test_wal_publisher_stats.last_event_log_count);

  do {
    auto log = publisher->allocate_log(t1, test_wal_publisher_log_action::kRemoveOneSubscriber, ctx);
    CASE_EXPECT_TRUE(!!log);
    if (!log) {
      break;
    }
    log->publisher = reinterpret_cast<void*>(publisher.get());
    publisher->emplace_back_log(std::move(log), ctx);

    CASE_EXPECT_EQ(event_subscribe_removed + 1, details::g_test_wal_publisher_stats.event_on_subscribe_removed);
  } while (false);

  CASE_EXPECT_EQ(publisher->broadcast(ctx), 1);

  CASE_EXPECT_EQ(send_logs_count + 2, details::g_test_wal_publisher_stats.send_logs_count);
  CASE_EXPECT_EQ(2, details::g_test_wal_publisher_stats.last_event_subscriber_count);
  CASE_EXPECT_EQ(1, details::g_test_wal_publisher_stats.last_event_log_count);
}

CASE_TEST(wal_publisher, enable_last_broadcast_for_removed_subscriber) {
  util::distributed_system::wal_time_point t1 = std::chrono::system_clock::now();
  util::distributed_system::wal_time_point t2 =
      t1 + std::chrono::duration_cast<test_wal_publisher_type::duration>(std::chrono::seconds{3});
  util::distributed_system::wal_time_point t3 =
      t1 + std::chrono::duration_cast<test_wal_publisher_type::duration>(std::chrono::seconds{6});
  test_wal_publisher_storage_type storage;
  test_wal_publisher_context ctx;

  auto conf = create_configure();
  conf->enable_last_broadcast_for_removed_subscriber = true;

  auto vtable = create_vtable();
  auto publisher = test_wal_publisher_type::create(vtable, conf, &storage);
  CASE_EXPECT_TRUE(!!publisher);
  if (!publisher) {
    return;
  }

  uint64_t subscriber_key_1 = 1;
  uint64_t subscriber_key_2 = 2;
  uint64_t subscriber_key_3 = 3;

  publisher->get_log_manager().set_last_removed_key(details::g_test_wal_publisher_stats.key_alloc);
  test_wal_publisher_add_logs(publisher->get_log_manager(), ctx, t1, t2, t3);

  auto event_subscribe_added = details::g_test_wal_publisher_stats.event_on_subscribe_added;
  auto event_subscribe_removed = details::g_test_wal_publisher_stats.event_on_subscribe_removed;
  auto send_logs_count = details::g_test_wal_publisher_stats.send_logs_count;

  auto subscriber_1 = publisher->create_subscriber(subscriber_key_1, t1, 0, ctx, &storage);
  auto subscriber_2 = publisher->create_subscriber(subscriber_key_2, t2, 0, ctx, &storage);
  auto subscriber_3 = publisher->create_subscriber(subscriber_key_3, t3, 0, ctx, &storage);

  CASE_EXPECT_EQ(event_subscribe_added + 3, details::g_test_wal_publisher_stats.event_on_subscribe_added);
  CASE_EXPECT_EQ(event_subscribe_removed, details::g_test_wal_publisher_stats.event_on_subscribe_removed);
  CASE_EXPECT_EQ(send_logs_count, details::g_test_wal_publisher_stats.send_logs_count);

  CASE_EXPECT_EQ(publisher->broadcast(ctx), 4);

  CASE_EXPECT_EQ(event_subscribe_removed, details::g_test_wal_publisher_stats.event_on_subscribe_removed);
  CASE_EXPECT_EQ(send_logs_count + 1, details::g_test_wal_publisher_stats.send_logs_count);
  CASE_EXPECT_EQ(3, details::g_test_wal_publisher_stats.last_event_subscriber_count);
  CASE_EXPECT_EQ(4, details::g_test_wal_publisher_stats.last_event_log_count);

  do {
    auto log = publisher->allocate_log(t1, test_wal_publisher_log_action::kRemoveOneSubscriber, ctx);
    CASE_EXPECT_TRUE(!!log);
    if (!log) {
      break;
    }
    log->publisher = reinterpret_cast<void*>(publisher.get());
    publisher->emplace_back_log(std::move(log), ctx);

    CASE_EXPECT_EQ(event_subscribe_removed + 1, details::g_test_wal_publisher_stats.event_on_subscribe_removed);
    CASE_EXPECT_EQ(1, publisher->get_subscribe_gc_pool().size());
  } while (false);

  CASE_EXPECT_EQ(1, publisher->get_subscribe_gc_pool().size());
  CASE_EXPECT_EQ(publisher->broadcast(ctx), 1);
  CASE_EXPECT_EQ(0, publisher->get_subscribe_gc_pool().size());

  CASE_EXPECT_EQ(send_logs_count + 3, details::g_test_wal_publisher_stats.send_logs_count);
  CASE_EXPECT_EQ(1, details::g_test_wal_publisher_stats.last_event_subscriber_count);
  CASE_EXPECT_EQ(1, details::g_test_wal_publisher_stats.last_event_log_count);
}
