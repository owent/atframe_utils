// Copyright 2021 atframework

#include <cstdlib>
#include <cstring>
#include <string>

#include "frame/test_macros.h"

#include "config/atframe_utils_build_feature.h"

#if (defined(LIBATFRAME_UTILS_ENABLE_GSL_WITH_MS_GSL) && LIBATFRAME_UTILS_ENABLE_GSL_WITH_MS_GSL) || \
    (defined(LIBATFRAME_UTILS_ENABLE_GSL_WITH_GSL_LITE) && LIBATFRAME_UTILS_ENABLE_GSL_WITH_GSL_LITE)
#  define TEST_THIRD_PARTY_GSL 1
#endif

#if (defined(TEST_THIRD_PARTY_GSL) && TEST_THIRD_PARTY_GSL) || \
    (defined(LIBATFRAME_UTILS_ENABLE_GSL_WITH_FALLBACK_STL) && LIBATFRAME_UTILS_ENABLE_GSL_WITH_FALLBACK_STL)
#  include "gsl/select-gsl.h"

static void gsl_test_print_string_view(gsl::string_view s) { CASE_MSG_INFO() << "string_view => " << s << std::endl; }

CASE_TEST(gsl, string_view) {
  gsl_test_print_string_view("hello");
  char world[] = "world";
  gsl_test_print_string_view(world);
  gsl_test_print_string_view(std::string("hello world"));
}

CASE_TEST(gsl, span) {
  int arr[] = {1, 2, 3, 4, 5};
  auto span = gsl::make_span(arr);
  CASE_EXPECT_EQ(5, span.size());
  CASE_EXPECT_EQ(3, span.subspan(2).size());
}

CASE_TEST(gsl, shared_ptr) {
  gsl::shared_ptr<int> p = gsl::make_shared<int>(123);
  CASE_EXPECT_EQ(123, *p);
}

CASE_TEST(gsl, unique_ptr) {
  gsl::unique_ptr<int> p = gsl::make_unique<int>(456);
  CASE_EXPECT_EQ(456, *p);
}

#endif
