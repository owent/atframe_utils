// Copyright 2021 atframework

#include "frame/test_macros.h"

static int g_unit_test_event_on_start_status = 0;

CASE_TEST_EVENT_ON_START(unit_test_event_on_start_1) {
  if (0 == g_unit_test_event_on_start_status) {
    g_unit_test_event_on_start_status = 1;
  }
}

CASE_TEST_EVENT_ON_START(unit_test_event_on_start_2, "unit_test_event_on_start_1") {
  if (1 == g_unit_test_event_on_start_status) {
    g_unit_test_event_on_start_status = 2;
  }
}

CASE_TEST(unit_test, event_on_start) { CASE_EXPECT_EQ(2, g_unit_test_event_on_start_status); }

CASE_TEST_EVENT_ON_EXIT(unit_test_event_on_exit_1) {
  if (3 == g_unit_test_event_on_start_status) {
    LIBATFRAME_UTILS_NAMESPACE_ID::cli::shell_stream(std::cout)()
        << "[ FINISH   ] unit_test_event_on_exit_1" << std::endl;
  }
}

CASE_TEST_EVENT_ON_EXIT(unit_test_event_on_exit_2, "unit_test_event_on_exit_1") {
  if (2 == g_unit_test_event_on_start_status) {
    g_unit_test_event_on_start_status = 3;
  }
}
