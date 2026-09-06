// Copyright 2026 atframework

#include "frame/test_macros.h"   // IWYU pragma: keep
#include "frame/test_manager.h"  // IWYU pragma: keep

#if defined(ATFW_UTILS_TEST_MACRO_TEST_ENABLE_BOOST_TEST)

#  ifdef BOOST_TEST_ALTERNATIVE_INIT_API
bool init_unit_test() {
#  else
boost::unit_test::test_suite *init_unit_test_suite(int argc, char *argv[]) {
#  endif

  ::atfw::util::testing::run_tests(argc, argv);

#  ifdef BOOST_TEST_ALTERNATIVE_INIT_API
  return true;
#  else
  return 0;
#  endif
}

#endif

#if !defined(ATFW_UTILS_TEST_MACRO_TEST_ENABLE_BOOST_TEST) || defined(BOOST_TEST_DYN_LINK) || \
    defined(BOOST_TEST_NO_MAIN)

int main(int argc, char *argv[]) {
#  ifdef ATFW_UTILS_TEST_MACRO_TEST_ENABLE_GTEST
  ::testing::InitGoogleTest(&argc, argv);
  ::atfw::util::testing::run_event_on_start();
  int ret = RUN_ALL_TESTS();
  ::atfw::util::testing::run_event_on_exit();
  return ret;
#  elif defined(ATFW_UTILS_TEST_MACRO_TEST_ENABLE_BOOST_TEST)
// prototype for user's unit test init function
#    ifdef BOOST_TEST_ALTERNATIVE_INIT_API
  boost::unit_test::init_unit_test_func init_func = &init_unit_test;
#    else
  boost::unit_test::init_unit_test_func init_func = &init_unit_test_suite;
#    endif

  ::atfw::util::testing::run_event_on_start();
  int ret = ::boost::unit_test::unit_test_main(init_func, argc, argv);
  ::atfw::util::testing::run_event_on_exit();
  return ret;
#  else

  return ::atfw::util::testing::run_tests(argc, argv);
#  endif
}

#endif
