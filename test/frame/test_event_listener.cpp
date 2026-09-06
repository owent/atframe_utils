// Copyright 2026 atframework

#include "test_event_listener.h"  // NOLINT(build/include_subdir)

#ifdef ATFW_UTILS_TEST_MACRO_TEST_ENABLE_GTEST
#  include <gtest/gtest.h>
#elif defined(ATFW_UTILS_TEST_MACRO_TEST_ENABLE_BOOST_TEST)
#  include <boost/test/framework.hpp>
#  include <boost/test/results_collector.hpp>
#  include <boost/test/tree/observer.hpp>
#  include <boost/test/tree/test_unit.hpp>

#  include <memory>
#  include <vector>
#else
#  include "test_manager.h"  // NOLINT(build/include_subdir)
#endif

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace testing {

test_event_listener::test_event_listener() {}

test_event_listener::~test_event_listener() {}

void test_event_listener::on_test_program_start() {}

void test_event_listener::on_test_program_end(int) {}

void test_event_listener::on_test_suite_start(const test_event_suite_info &) {}

void test_event_listener::on_test_suite_end(const test_event_suite_info &) {}

void test_event_listener::on_test_case_start(const test_event_case_info &) {}

void test_event_listener::on_test_case_end(const test_event_case_info &) {}

#ifdef ATFW_UTILS_TEST_MACRO_TEST_ENABLE_GTEST

namespace {

class test_event_listener_gtest_adapter : public ::testing::EmptyTestEventListener {
 public:
  explicit test_event_listener_gtest_adapter(test_event_listener *listener) : listener_(listener) {}

  ~test_event_listener_gtest_adapter() override { delete listener_; }

  void OnTestProgramStart(const ::testing::UnitTest &) override {
    if (nullptr != listener_) {
      listener_->on_test_program_start();
    }
  }

  void OnTestProgramEnd(const ::testing::UnitTest &unit_test) override {
    if (nullptr != listener_) {
      listener_->on_test_program_end(unit_test.failed_test_count());
    }
  }

  void OnTestSuiteStart(const ::testing::TestSuite &test_suite) override {
    if (nullptr == listener_) {
      return;
    }

    test_event_suite_info info;
    info.name_ = test_suite.name();
    info.total_case_count_ = static_cast<size_t>(test_suite.total_test_count());
    listener_->on_test_suite_start(info);
  }

  void OnTestSuiteEnd(const ::testing::TestSuite &test_suite) override {
    if (nullptr == listener_) {
      return;
    }

    test_event_suite_info info;
    info.name_ = test_suite.name();
    info.total_case_count_ = static_cast<size_t>(test_suite.total_test_count());
    info.run_case_count_ = static_cast<size_t>(test_suite.test_to_run_count());
    info.success_count_ = test_suite.successful_test_count();
    info.failed_count_ = test_suite.failed_test_count();
    listener_->on_test_suite_end(info);
  }

  void OnTestStart(const ::testing::TestInfo &test_info) override {
    if (nullptr == listener_) {
      return;
    }

    test_event_case_info info;
    info.suite_name_ = test_info.test_suite_name();
    info.case_name_ = test_info.name();
    listener_->on_test_case_start(info);
  }

  void OnTestEnd(const ::testing::TestInfo &test_info) override {
    if (nullptr == listener_) {
      return;
    }

    test_event_case_info info;
    info.suite_name_ = test_info.test_suite_name();
    info.case_name_ = test_info.name();
    info.passed_ = (nullptr != test_info.result() && test_info.result()->Passed());
    listener_->on_test_case_end(info);
  }

 private:
  test_event_listener *listener_;
};

}  // namespace

void append_test_event_listener(test_event_listener *listener) {
  if (nullptr == listener) {
    return;
  }

  ::testing::UnitTest::GetInstance()->listeners().Append(new test_event_listener_gtest_adapter(listener));
}

#elif defined(ATFW_UTILS_TEST_MACRO_TEST_ENABLE_BOOST_TEST)

namespace {

class test_event_listener_boost_adapter : public ::boost::unit_test::test_observer {
 public:
  explicit test_event_listener_boost_adapter(test_event_listener *listener) : listener_(listener) {}

  ~test_event_listener_boost_adapter() override { delete listener_; }

  void test_start(::boost::unit_test::counter_t, ::boost::unit_test::test_unit_id) override {
    if (nullptr != listener_) {
      listener_->on_test_program_start();
    }
  }

  void test_finish() override {
    if (nullptr == listener_) {
      return;
    }

    namespace but = ::boost::unit_test;
    listener_->on_test_program_end(
        but::results_collector.results(but::framework::master_test_suite().p_id).result_code());
  }

  void test_unit_start(const ::boost::unit_test::test_unit &tu) override {
    if (nullptr == listener_) {
      return;
    }

    namespace but = ::boost::unit_test;
    const std::string &unit_name = tu.p_name.get();
    if (but::TUT_SUITE == tu.p_type) {
      if (tu.p_id == but::framework::master_test_suite().p_id) {
        return;
      }

      test_event_suite_info info;
      info.name_ = gsl::string_view(unit_name.data(), unit_name.size());
      listener_->on_test_suite_start(info);
    } else if (but::TUT_CASE == tu.p_type) {
      test_event_case_info info;
      if (tu.p_parent_id != but::INV_TEST_UNIT_ID) {
        const but::test_unit &parent = but::framework::get<but::test_unit>(tu.p_parent_id);
        const std::string &parent_name = parent.p_name.get();
        info.suite_name_ = gsl::string_view(parent_name.data(), parent_name.size());
      }
      info.case_name_ = gsl::string_view(unit_name.data(), unit_name.size());
      listener_->on_test_case_start(info);
    }
  }

  void test_unit_finish(const ::boost::unit_test::test_unit &tu, unsigned long) override {
    if (nullptr == listener_) {
      return;
    }

    namespace but = ::boost::unit_test;
    const std::string &unit_name = tu.p_name.get();
    if (but::TUT_SUITE == tu.p_type) {
      if (tu.p_id == but::framework::master_test_suite().p_id) {
        return;
      }

      const but::test_results &res = but::results_collector.results(tu.p_id);
      test_event_suite_info info;
      info.name_ = gsl::string_view(unit_name.data(), unit_name.size());
      info.run_case_count_ = static_cast<size_t>(res.p_test_cases_passed.get() + res.p_test_cases_failed.get() +
                                                 res.p_test_cases_aborted.get());
      info.success_count_ = static_cast<int>(res.p_test_cases_passed.get());
      info.failed_count_ = static_cast<int>(res.p_test_cases_failed.get() + res.p_test_cases_aborted.get());
      listener_->on_test_suite_end(info);
    } else if (but::TUT_CASE == tu.p_type) {
      const but::test_results &res = but::results_collector.results(tu.p_id);
      test_event_case_info info;
      if (tu.p_parent_id != but::INV_TEST_UNIT_ID) {
        const but::test_unit &parent = but::framework::get<but::test_unit>(tu.p_parent_id);
        const std::string &parent_name = parent.p_name.get();
        info.suite_name_ = gsl::string_view(parent_name.data(), parent_name.size());
      }
      info.case_name_ = gsl::string_view(unit_name.data(), unit_name.size());
      info.success_count_ = static_cast<int>(res.p_assertions_passed.get());
      info.failed_count_ = static_cast<int>(res.p_assertions_failed.get());
      info.passed_ = res.passed();
      listener_->on_test_case_end(info);
    }
  }

 private:
  test_event_listener *listener_;
};

static std::vector<std::unique_ptr<test_event_listener_boost_adapter> > &get_boost_adapters() {
  static std::vector<std::unique_ptr<test_event_listener_boost_adapter> > ret;
  return ret;
}

}  // namespace

void append_test_event_listener(test_event_listener *listener) {
  if (nullptr == listener) {
    return;
  }

  std::unique_ptr<test_event_listener_boost_adapter> adapter(new test_event_listener_boost_adapter(listener));
  ::boost::unit_test::framework::register_observer(*adapter);
  get_boost_adapters().push_back(std::move(adapter));
}

#else

void append_test_event_listener(test_event_listener *listener) { test_manager::me().append_event_listener(listener); }

#endif

}  // namespace testing
ATFRAMEWORK_UTILS_NAMESPACE_END
