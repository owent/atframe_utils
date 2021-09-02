#include <algorithm>
#include <chrono>
#include <cstring>
#include <ctime>
#include <memory>
#include <sstream>
#include <vector>

#include <distributed_system/wal_client.h>

#include "frame/test_macros.h"

enum class test_wal_client_log_action {
  kDoNothing = 0,
  kFallbackDefault,
};

namespace std {
template <>
struct hash<test_wal_client_log_action> {
  std::size_t operator()(test_wal_client_log_action const& s) const noexcept {
    return std::hash<int>{}(static_cast<int>(s));
  }
};
}  // namespace std

struct test_wal_client_log_type {
  util::distributed_system::wal_time_point timepoint;
  int64_t log_key;
  test_wal_client_log_action action;

  union {
    int64_t data;
  };
};

struct test_wal_client_storage_type {
  std::vector<test_wal_client_log_type> logs;
  int64_t global_ignore;
};

struct test_wal_client_log_action_getter {
  test_wal_client_log_action operator()(const test_wal_client_log_type& log) { return log.action; }
};

struct test_wal_client_context {};

struct test_wal_client_private_type {
  test_wal_client_storage_type* storage;

  inline test_wal_client_private_type() : storage(nullptr) {}
  inline explicit test_wal_client_private_type(test_wal_client_storage_type* input) : storage(input) {}
};

using test_wal_client_log_operator =
    util::distributed_system::wal_log_operator<int64_t, test_wal_client_log_type, test_wal_client_log_action_getter>;

using test_wal_client_type =
    util::distributed_system::wal_client<test_wal_client_storage_type, test_wal_client_log_operator,
                                         test_wal_client_context, test_wal_client_private_type,
                                         test_wal_client_storage_type>;

struct test_wal_client_stats {
  int64_t key_alloc;
  size_t merge_count;
  size_t delegate_action_count;
  size_t default_action_count;
  size_t event_on_log_added;
  size_t event_on_log_removed;

  size_t receive_subscribe_response_count;
  size_t receive_snapshot_count;
  size_t subscribe_request_count;

  test_wal_client_type::object_type::log_type last_log;
};

namespace details {
test_wal_client_stats g_test_wal_client_stats{1, 0, 0, 0, 0, 0, 0, 0, 0, test_wal_client_log_type()};
}

static test_wal_client_type::vtable_pointer create_vtable() {
  using wal_object_type = test_wal_client_type::object_type;
  using wal_client_type = test_wal_client_type;
  using wal_result_code = util::distributed_system::wal_result_code;

  wal_client_type::vtable_pointer ret = std::make_shared<wal_client_type::vtable_type>();

  // callbacks for wal_object
  ret->load = [](wal_object_type& wal, const wal_object_type::storage_type& from,
                 wal_object_type::callback_param_type) -> wal_result_code {
    *wal.get_private_data().storage = from;

    wal_client_type::log_container_type container;
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
    ++details::g_test_wal_client_stats.merge_count;
    to.data = from.data;
  };

  ret->get_log_key = [](const wal_object_type&, const wal_object_type::log_type& log) -> wal_object_type::log_key_type {
    return log.log_key;
  };

  ret->allocate_log_key = [](wal_object_type&, const wal_object_type::log_type&,
                             wal_object_type::callback_param_type) -> wal_object_type::log_key_result_type {
    return wal_object_type::log_key_result_type::make_success(++details::g_test_wal_client_stats.key_alloc);
  };

  ret->on_log_added = [](wal_object_type&, const wal_object_type::log_pointer&) {
    ++details::g_test_wal_client_stats.event_on_log_added;
  };

  ret->on_log_removed = [](wal_object_type&, const wal_object_type::log_pointer&) {
    ++details::g_test_wal_client_stats.event_on_log_removed;
  };

  ret->log_action_delegate[test_wal_client_log_action::kDoNothing].action =
      [](wal_object_type&, const wal_object_type::log_type& log,
         wal_object_type::callback_param_type) -> wal_result_code {
    ++details::g_test_wal_client_stats.delegate_action_count;
    details::g_test_wal_client_stats.last_log = log;

    return wal_result_code::kOk;
  };

  ret->default_delegate.action = [](wal_object_type&, const wal_object_type::log_type& log,
                                    wal_object_type::callback_param_type) -> wal_result_code {
    ++details::g_test_wal_client_stats.default_action_count;
    details::g_test_wal_client_stats.last_log = log;

    return wal_result_code::kOk;
  };

  // ============ callbacks for wal_client ============
  ret->on_receive_snapshot = [](wal_client_type& client, const wal_client_type::snapshot_type& from,
                                wal_object_type::callback_param_type) -> wal_result_code {
    ++details::g_test_wal_client_stats.receive_snapshot_count;
    if (nullptr != client.get_log_manager().get_private_data().storage) {
      *client.get_log_manager().get_private_data().storage = from;
    }

    wal_client_type::log_container_type container;
    for (auto& log : from.logs) {
      container.emplace_back(std::make_shared<wal_object_type::log_type>(log));
    }

    client.get_log_manager().assign_logs(container);
    if (!from.logs.empty()) {
      client.get_log_manager().set_last_removed_key((*from.logs.rbegin()).log_key - 1);
    }
    return wal_result_code::kOk;
  };

  ret->on_receive_subscribe_response = [](wal_client_type&, wal_object_type::callback_param_type) -> wal_result_code {
    ++details::g_test_wal_client_stats.receive_subscribe_response_count;
    return wal_result_code::kOk;
  };

  ret->subscribe_request = [](wal_client_type&, wal_object_type::callback_param_type) -> wal_result_code {
    ++details::g_test_wal_client_stats.subscribe_request_count;
    return wal_result_code::kOk;
  };

  return ret;
}

static test_wal_client_type::congfigure_pointer create_configure() {
  test_wal_client_type::congfigure_pointer ret = test_wal_client_type::make_configure();

  ret->max_log_size = 8;
  ret->gc_log_size = 4;

  ret->subscriber_heartbeat_interval =
      std::chrono::duration_cast<test_wal_client_type::duration>(std::chrono::seconds{10});
  ret->subscriber_heartbeat_retry_interval =
      std::chrono::duration_cast<test_wal_client_type::duration>(std::chrono::seconds{5});

  return ret;
}

CASE_TEST(wal_client, create_failed) {
  util::distributed_system::wal_time_point now = std::chrono::system_clock::now();
  test_wal_client_storage_type storage;
  // test_wal_client_context ctx;

  auto conf = create_configure();
  auto vtable = create_vtable();
  CASE_EXPECT_EQ(nullptr, test_wal_client_type::create(now, vtable, nullptr, &storage));
  CASE_EXPECT_EQ(nullptr, test_wal_client_type::create(now, nullptr, conf, &storage));

  auto vtable_1 = vtable;
  vtable_1->get_meta = nullptr;
  CASE_EXPECT_EQ(nullptr, test_wal_client_type::create(now, vtable_1, conf, &storage));

  auto vtable_2 = vtable;
  vtable_2->get_log_key = nullptr;
  CASE_EXPECT_EQ(nullptr, test_wal_client_type::create(now, vtable_2, conf, &storage));

  auto vtable_3 = vtable;
  vtable_3->allocate_log_key = nullptr;
  CASE_EXPECT_EQ(nullptr, test_wal_client_type::create(now, vtable_3, conf, &storage));

  auto vtable_4 = vtable;
  vtable_4->on_receive_snapshot = nullptr;
  CASE_EXPECT_EQ(nullptr, test_wal_client_type::create(now, vtable_4, conf, &storage));
}

CASE_TEST(wal_client, load_and_dump) {
  auto old_action_count =
      details::g_test_wal_client_stats.default_action_count + details::g_test_wal_client_stats.delegate_action_count;

  test_wal_client_storage_type load_storege;
  util::distributed_system::wal_time_point now = std::chrono::system_clock::now();
  load_storege.global_ignore = 123;
  load_storege.logs.push_back(test_wal_client_log_type{now, 124, test_wal_client_log_action::kDoNothing, 124});
  load_storege.logs.push_back(test_wal_client_log_type{now, 125, test_wal_client_log_action::kFallbackDefault, 125});

  test_wal_client_storage_type storage;
  test_wal_client_context ctx;

  auto conf = create_configure();
  auto vtable = create_vtable();
  auto client = test_wal_client_type::create(now, vtable, conf, &storage);
  CASE_EXPECT_TRUE(!!client);
  if (!client) {
    return;
  }
  CASE_EXPECT_TRUE(util::distributed_system::wal_result_code::kOk == client->load(load_storege, ctx));
  CASE_EXPECT_EQ(old_action_count, details::g_test_wal_client_stats.default_action_count +
                                       details::g_test_wal_client_stats.delegate_action_count);
  CASE_EXPECT_EQ(4, client->get_configure().gc_log_size);
  CASE_EXPECT_EQ(&client->get_log_manager().get_configure(), &client->get_configure());

  CASE_EXPECT_EQ(123, storage.global_ignore);
  CASE_EXPECT_EQ(2, storage.logs.size());
  CASE_EXPECT_EQ(124, storage.logs[0].data);
  CASE_EXPECT_TRUE(test_wal_client_log_action::kFallbackDefault == storage.logs[1].action);

  CASE_EXPECT_EQ(2, client->get_log_manager().get_all_logs().size());
  CASE_EXPECT_EQ(124, (*client->get_log_manager().get_all_logs().begin())->data);
  CASE_EXPECT_TRUE(test_wal_client_log_action::kFallbackDefault ==
                   (*client->get_log_manager().get_all_logs().rbegin())->action);

  // dump
  test_wal_client_storage_type dump_storege;
  CASE_EXPECT_TRUE(util::distributed_system::wal_result_code::kOk == client->dump(dump_storege, ctx));

  CASE_EXPECT_EQ(123, dump_storege.global_ignore);
  CASE_EXPECT_EQ(2, dump_storege.logs.size());
  CASE_EXPECT_EQ(124, dump_storege.logs[0].data);
  CASE_EXPECT_TRUE(test_wal_client_log_action::kFallbackDefault == dump_storege.logs[1].action);

  // check last finished key
  CASE_EXPECT_TRUE(!!client->get_last_finished_log_key());
  if (client->get_last_finished_log_key()) {
    CASE_EXPECT_EQ(125, *client->get_last_finished_log_key());
  }
}

// TODO: heartbeat
// TODO: retry heartbeat
// TODO: receive_snapshot
// TODO: receive_logs : get_last_finished_log_key()
// TODO: ignore logs that already received
// TODO: subscribe_request
