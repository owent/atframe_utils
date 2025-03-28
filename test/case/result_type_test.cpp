// Copyright 2021 atframework
// Created by owent on 2021-08-10

#include <nostd/type_traits.h>

#include <stdint.h>
#include <cstdlib>
#include <cstring>
#include <string>

#include "frame/test_macros.h"

#include "design_pattern/result_type.h"

CASE_TEST(result_type, all_triviall) {
  using test_type = atfw::util::design_pattern::result_type<int, uint64_t>;
  auto success_obj = test_type::make_success(123);
  auto error_obj = test_type::make_error(456U);
  static_assert(sizeof(atfw::util::nostd::aligned_storage<sizeof(test_type::error_storage_type::storage_type)>::type) +
                        sizeof(int64_t) >=
                    sizeof(test_type),
                "size invalid for trivial result_type");

  CASE_MSG_INFO() << "sizeof(test_type::error_storage_type): " << sizeof(test_type::error_storage_type::storage_type)
                  << std::endl;
  CASE_MSG_INFO() << "sizeof(atfw::util::nostd::aligned_storage<sizeof(test_type::error_storage_type)>::type): "
                  << sizeof(
                         atfw::util::nostd::aligned_storage<sizeof(test_type::error_storage_type::storage_type)>::type)
                  << std::endl;
  CASE_MSG_INFO() << "sizeof(result_type<int, uint64_t>): " << sizeof(test_type) << std::endl;
  CASE_EXPECT_TRUE(success_obj.is_success());
  CASE_EXPECT_TRUE(success_obj.has_value());
  CASE_EXPECT_TRUE(static_cast<bool>(success_obj));
  CASE_EXPECT_FALSE(success_obj.is_error());
  CASE_EXPECT_FALSE(success_obj.is_none());

  CASE_EXPECT_FALSE(error_obj.is_success());
  CASE_EXPECT_FALSE(error_obj.has_value());
  CASE_EXPECT_FALSE(static_cast<bool>(error_obj));
  CASE_EXPECT_TRUE(error_obj.is_error());
  CASE_EXPECT_FALSE(error_obj.is_none());

  CASE_EXPECT_NE(success_obj.get_success(), nullptr);
  CASE_EXPECT_EQ(success_obj.get_error(), nullptr);

  CASE_EXPECT_EQ(error_obj.get_success(), nullptr);
  CASE_EXPECT_NE(error_obj.get_error(), nullptr);

  CASE_EXPECT_EQ(*success_obj.get_success(), 123);
  CASE_EXPECT_EQ(*error_obj.get_error(), 456);

  {
    test_type empty_obj;
    CASE_EXPECT_FALSE(empty_obj.is_success());
    CASE_EXPECT_FALSE(empty_obj.has_value());
    CASE_EXPECT_FALSE(static_cast<bool>(empty_obj));
    CASE_EXPECT_FALSE(empty_obj.is_error());
    CASE_EXPECT_TRUE(empty_obj.is_none());
  }

  // Swap success
  {
    auto swap_obj = test_type::make_success(321);
    success_obj.swap(swap_obj);

    CASE_EXPECT_EQ(*swap_obj.get_success(), 123);
    CASE_EXPECT_EQ(*success_obj.get_success(), 321);
    swap_obj.swap(success_obj);
  }
  // Swap error
  {
    auto swap_obj = test_type::make_error(654U);
    error_obj.swap(swap_obj);

    CASE_EXPECT_EQ(*swap_obj.get_error(), 456);
    CASE_EXPECT_EQ(*error_obj.get_error(), 654);
    swap_obj.swap(error_obj);
  }

  // Swap success and empty
  {
    test_type empty_obj;
    success_obj.swap(empty_obj);

    CASE_EXPECT_TRUE(empty_obj.is_success());
    CASE_EXPECT_TRUE(success_obj.is_none());
    CASE_EXPECT_EQ(*empty_obj.get_success(), 123);
    success_obj.swap(empty_obj);

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
  }

  // Swap empty and success
  {
    test_type empty_obj;
    empty_obj.swap(success_obj);

    CASE_EXPECT_TRUE(empty_obj.is_success());
    CASE_EXPECT_TRUE(success_obj.is_none());
    CASE_EXPECT_EQ(*empty_obj.get_success(), 123);
    empty_obj.swap(success_obj);

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
  }

  // Swap error and empty
  {
    test_type empty_obj;
    error_obj.swap(empty_obj);

    CASE_EXPECT_TRUE(empty_obj.is_error());
    CASE_EXPECT_TRUE(error_obj.is_none());
    CASE_EXPECT_EQ(*empty_obj.get_error(), 456);
    error_obj.swap(empty_obj);

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
  }

  // Swap empty and error
  {
    test_type empty_obj;
    empty_obj.swap(error_obj);

    CASE_EXPECT_TRUE(empty_obj.is_error());
    CASE_EXPECT_TRUE(error_obj.is_none());
    CASE_EXPECT_EQ(*empty_obj.get_error(), 456);
    empty_obj.swap(error_obj);

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
  }

  // Swap success and error
  success_obj.swap(error_obj);

  CASE_EXPECT_TRUE(error_obj.is_success());
  CASE_EXPECT_FALSE(error_obj.is_error());
  CASE_EXPECT_FALSE(error_obj.is_none());

  CASE_EXPECT_FALSE(success_obj.is_success());
  CASE_EXPECT_TRUE(success_obj.is_error());
  CASE_EXPECT_FALSE(success_obj.is_none());

  CASE_EXPECT_NE(error_obj.get_success(), nullptr);
  CASE_EXPECT_EQ(error_obj.get_error(), nullptr);

  CASE_EXPECT_EQ(success_obj.get_success(), nullptr);
  CASE_EXPECT_NE(success_obj.get_error(), nullptr);

  CASE_EXPECT_EQ(*error_obj.get_success(), 123);
  CASE_EXPECT_EQ(*success_obj.get_error(), 456);

  // Swap error and success
  success_obj.swap(error_obj);

  CASE_EXPECT_TRUE(success_obj.is_success());
  CASE_EXPECT_FALSE(success_obj.is_error());
  CASE_EXPECT_FALSE(success_obj.is_none());

  CASE_EXPECT_FALSE(error_obj.is_success());
  CASE_EXPECT_TRUE(error_obj.is_error());
  CASE_EXPECT_FALSE(error_obj.is_none());

  CASE_EXPECT_NE(success_obj.get_success(), nullptr);
  CASE_EXPECT_EQ(success_obj.get_error(), nullptr);

  CASE_EXPECT_EQ(error_obj.get_success(), nullptr);
  CASE_EXPECT_NE(error_obj.get_error(), nullptr);

  CASE_EXPECT_EQ(*success_obj.get_success(), 123);
  CASE_EXPECT_EQ(*error_obj.get_error(), 456);
}

CASE_TEST(result_type, one_triviall) {
  using test_type = atfw::util::design_pattern::result_type<int, std::string>;
  auto success_obj = test_type::make_success(123);
  auto error_obj = test_type::make_error("456");

  CASE_EXPECT_TRUE(success_obj.is_success());
  CASE_EXPECT_FALSE(success_obj.is_error());
  CASE_EXPECT_FALSE(success_obj.is_none());

  CASE_EXPECT_FALSE(error_obj.is_success());
  CASE_EXPECT_TRUE(error_obj.is_error());
  CASE_EXPECT_FALSE(error_obj.is_none());

  CASE_EXPECT_NE(success_obj.get_success(), nullptr);
  CASE_EXPECT_EQ(success_obj.get_error(), nullptr);

  CASE_EXPECT_EQ(error_obj.get_success(), nullptr);
  CASE_EXPECT_NE(error_obj.get_error(), nullptr);

  CASE_EXPECT_EQ(*success_obj.get_success(), 123);
  CASE_EXPECT_EQ(*error_obj.get_error(), "456");

  {
    test_type empty_obj;
    CASE_EXPECT_FALSE(empty_obj.is_success());
    CASE_EXPECT_FALSE(empty_obj.is_error());
    CASE_EXPECT_TRUE(empty_obj.is_none());
  }

  // Swap success
  {
    auto swap_obj = test_type::make_success(321);
    success_obj.swap(swap_obj);

    CASE_EXPECT_EQ(*swap_obj.get_success(), 123);
    CASE_EXPECT_EQ(*success_obj.get_success(), 321);
    swap_obj.swap(success_obj);
  }
  // Swap error
  {
    std::unique_ptr<std::string> move_ptr = std::unique_ptr<std::string>(new std::string("654"));
    auto swap_obj = test_type::make_error(std::move(move_ptr));
    error_obj.swap(swap_obj);

    CASE_EXPECT_EQ(*swap_obj.get_error(), "456");
    CASE_EXPECT_EQ(*error_obj.get_error(), "654");
    swap_obj.swap(error_obj);
  }

  // Swap success and empty
  {
    test_type empty_obj;
    success_obj.swap(empty_obj);

    CASE_EXPECT_TRUE(empty_obj.is_success());
    CASE_EXPECT_TRUE(success_obj.is_none());
    CASE_EXPECT_EQ(*empty_obj.get_success(), 123);
    success_obj.swap(empty_obj);

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
  }

  // Swap empty and success
  {
    test_type empty_obj;
    empty_obj.swap(success_obj);

    CASE_EXPECT_TRUE(empty_obj.is_success());
    CASE_EXPECT_TRUE(success_obj.is_none());
    CASE_EXPECT_EQ(*empty_obj.get_success(), 123);
    empty_obj.swap(success_obj);

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
  }

  // Swap error and empty
  {
    test_type empty_obj;
    error_obj.swap(empty_obj);

    CASE_EXPECT_TRUE(empty_obj.is_error());
    CASE_EXPECT_TRUE(error_obj.is_none());
    CASE_EXPECT_EQ(*empty_obj.get_error(), "456");
    error_obj.swap(empty_obj);

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
  }

  // Swap empty and error
  {
    test_type empty_obj;
    empty_obj.swap(error_obj);

    CASE_EXPECT_TRUE(empty_obj.is_error());
    CASE_EXPECT_TRUE(error_obj.is_none());
    CASE_EXPECT_EQ(*empty_obj.get_error(), "456");
    empty_obj.swap(error_obj);

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
  }

  // Swap success and error
  success_obj.swap(error_obj);

  CASE_EXPECT_TRUE(error_obj.is_success());
  CASE_EXPECT_FALSE(error_obj.is_error());
  CASE_EXPECT_FALSE(error_obj.is_none());

  CASE_EXPECT_FALSE(success_obj.is_success());
  CASE_EXPECT_TRUE(success_obj.is_error());
  CASE_EXPECT_FALSE(success_obj.is_none());

  CASE_EXPECT_NE(error_obj.get_success(), nullptr);
  CASE_EXPECT_EQ(error_obj.get_error(), nullptr);

  CASE_EXPECT_EQ(success_obj.get_success(), nullptr);
  CASE_EXPECT_NE(success_obj.get_error(), nullptr);

  CASE_EXPECT_EQ(*error_obj.get_success(), 123);
  CASE_EXPECT_EQ(*success_obj.get_error(), "456");

  // Swap error and success
  success_obj.swap(error_obj);

  CASE_EXPECT_TRUE(success_obj.is_success());
  CASE_EXPECT_FALSE(success_obj.is_error());
  CASE_EXPECT_FALSE(success_obj.is_none());

  CASE_EXPECT_FALSE(error_obj.is_success());
  CASE_EXPECT_TRUE(error_obj.is_error());
  CASE_EXPECT_FALSE(error_obj.is_none());

  CASE_EXPECT_NE(success_obj.get_success(), nullptr);
  CASE_EXPECT_EQ(success_obj.get_error(), nullptr);

  CASE_EXPECT_EQ(error_obj.get_success(), nullptr);
  CASE_EXPECT_NE(error_obj.get_error(), nullptr);

  CASE_EXPECT_EQ(*success_obj.get_success(), 123);
  CASE_EXPECT_EQ(*error_obj.get_error(), "456");
}

CASE_TEST(result_type, non_triviall) {
  using test_type = atfw::util::design_pattern::result_type<std::string, std::string>;
  auto success_obj = test_type::make_success("123");
  auto error_obj = test_type::make_error("456");

  CASE_EXPECT_TRUE(success_obj.is_success());
  CASE_EXPECT_FALSE(success_obj.is_error());
  CASE_EXPECT_FALSE(success_obj.is_none());

  CASE_EXPECT_FALSE(error_obj.is_success());
  CASE_EXPECT_TRUE(error_obj.is_error());
  CASE_EXPECT_FALSE(error_obj.is_none());

  CASE_EXPECT_NE(success_obj.get_success(), nullptr);
  CASE_EXPECT_EQ(success_obj.get_error(), nullptr);

  CASE_EXPECT_EQ(error_obj.get_success(), nullptr);
  CASE_EXPECT_NE(error_obj.get_error(), nullptr);

  CASE_EXPECT_EQ(*success_obj.get_success(), "123");
  CASE_EXPECT_EQ(*error_obj.get_error(), "456");

  {
    test_type empty_obj;
    CASE_EXPECT_FALSE(empty_obj.is_success());
    CASE_EXPECT_FALSE(empty_obj.is_error());
    CASE_EXPECT_TRUE(empty_obj.is_none());
  }

  // Swap success
  {
    auto swap_obj = test_type::make_success("321");
    success_obj.swap(swap_obj);

    CASE_EXPECT_EQ(*swap_obj.get_success(), "123");
    CASE_EXPECT_EQ(*success_obj.get_success(), "321");
    swap_obj.swap(success_obj);
  }
  // Swap error
  {
    std::unique_ptr<std::string> move_ptr = std::unique_ptr<std::string>(new std::string("654"));
    auto swap_obj = test_type::make_error(std::move(move_ptr));
    error_obj.swap(swap_obj);

    CASE_EXPECT_EQ(*swap_obj.get_error(), "456");
    CASE_EXPECT_EQ(*error_obj.get_error(), "654");
    swap_obj.swap(error_obj);
  }

  // Swap success and empty
  {
    test_type empty_obj;
    success_obj.swap(empty_obj);

    CASE_EXPECT_TRUE(empty_obj.is_success());
    CASE_EXPECT_TRUE(success_obj.is_none());
    CASE_EXPECT_EQ(*empty_obj.get_success(), "123");
    success_obj.swap(empty_obj);

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
  }

  // Swap empty and success
  {
    test_type empty_obj;
    empty_obj.swap(success_obj);

    CASE_EXPECT_TRUE(empty_obj.is_success());
    CASE_EXPECT_TRUE(success_obj.is_none());
    CASE_EXPECT_EQ(*empty_obj.get_success(), "123");
    empty_obj.swap(success_obj);

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
  }

  // Swap error and empty
  {
    test_type empty_obj;
    error_obj.swap(empty_obj);

    CASE_EXPECT_TRUE(empty_obj.is_error());
    CASE_EXPECT_TRUE(error_obj.is_none());
    CASE_EXPECT_EQ(*empty_obj.get_error(), "456");
    error_obj.swap(empty_obj);

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
  }

  // Swap empty and error
  {
    test_type empty_obj;
    empty_obj.swap(error_obj);

    CASE_EXPECT_TRUE(empty_obj.is_error());
    CASE_EXPECT_TRUE(error_obj.is_none());
    CASE_EXPECT_EQ(*empty_obj.get_error(), "456");
    empty_obj.swap(error_obj);

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
  }

  // Swap success and error
  success_obj.swap(error_obj);

  CASE_EXPECT_TRUE(error_obj.is_success());
  CASE_EXPECT_FALSE(error_obj.is_error());
  CASE_EXPECT_FALSE(error_obj.is_none());

  CASE_EXPECT_FALSE(success_obj.is_success());
  CASE_EXPECT_TRUE(success_obj.is_error());
  CASE_EXPECT_FALSE(success_obj.is_none());

  CASE_EXPECT_NE(error_obj.get_success(), nullptr);
  CASE_EXPECT_EQ(error_obj.get_error(), nullptr);

  CASE_EXPECT_EQ(success_obj.get_success(), nullptr);
  CASE_EXPECT_NE(success_obj.get_error(), nullptr);

  CASE_EXPECT_EQ(*error_obj.get_success(), "123");
  CASE_EXPECT_EQ(*success_obj.get_error(), "456");

  // Swap error and success
  success_obj.swap(error_obj);

  CASE_EXPECT_TRUE(success_obj.is_success());
  CASE_EXPECT_FALSE(success_obj.is_error());
  CASE_EXPECT_FALSE(success_obj.is_none());

  CASE_EXPECT_FALSE(error_obj.is_success());
  CASE_EXPECT_TRUE(error_obj.is_error());
  CASE_EXPECT_FALSE(error_obj.is_none());

  CASE_EXPECT_NE(success_obj.get_success(), nullptr);
  CASE_EXPECT_EQ(success_obj.get_error(), nullptr);

  CASE_EXPECT_EQ(error_obj.get_success(), nullptr);
  CASE_EXPECT_NE(error_obj.get_error(), nullptr);

  CASE_EXPECT_EQ(*success_obj.get_success(), "123");
  CASE_EXPECT_EQ(*error_obj.get_error(), "456");
}

CASE_TEST(result_type, has_void) {
  using test_type = atfw::util::design_pattern::result_type<void, uint64_t>;
  auto success_obj = test_type::make_success();
  auto error_obj = test_type::make_error(456U);

  CASE_EXPECT_TRUE(success_obj.is_success());
  CASE_EXPECT_FALSE(success_obj.is_error());
  CASE_EXPECT_FALSE(success_obj.is_none());

  CASE_EXPECT_FALSE(error_obj.is_success());
  CASE_EXPECT_TRUE(error_obj.is_error());
  CASE_EXPECT_FALSE(error_obj.is_none());

  CASE_EXPECT_NE(success_obj.get_success(), nullptr);
  CASE_EXPECT_EQ(success_obj.get_error(), nullptr);

  CASE_EXPECT_EQ(error_obj.get_success(), nullptr);
  CASE_EXPECT_NE(error_obj.get_error(), nullptr);

  CASE_EXPECT_NE(success_obj.get_success(), nullptr);
  CASE_EXPECT_EQ(*error_obj.get_error(), 456);

  {
    test_type empty_obj;
    CASE_EXPECT_FALSE(empty_obj.is_success());
    CASE_EXPECT_FALSE(empty_obj.is_error());
    CASE_EXPECT_TRUE(empty_obj.is_none());
  }

  // Swap success and empty
  {
    test_type empty_obj;
    success_obj.swap(empty_obj);

    CASE_EXPECT_TRUE(empty_obj.is_success());
    CASE_EXPECT_TRUE(success_obj.is_none());
    CASE_EXPECT_NE(empty_obj.get_success(), nullptr);
    success_obj.swap(empty_obj);

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
  }

  // Swap empty and success
  {
    test_type empty_obj;
    empty_obj.swap(success_obj);

    CASE_EXPECT_TRUE(empty_obj.is_success());
    CASE_EXPECT_TRUE(success_obj.is_none());
    CASE_EXPECT_NE(empty_obj.get_success(), nullptr);
    empty_obj.swap(success_obj);

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
  }

  // Swap success and error
  success_obj.swap(error_obj);

  CASE_EXPECT_TRUE(error_obj.is_success());
  CASE_EXPECT_FALSE(error_obj.is_error());
  CASE_EXPECT_FALSE(error_obj.is_none());

  CASE_EXPECT_FALSE(success_obj.is_success());
  CASE_EXPECT_TRUE(success_obj.is_error());
  CASE_EXPECT_FALSE(success_obj.is_none());

  CASE_EXPECT_NE(error_obj.get_success(), nullptr);
  CASE_EXPECT_EQ(error_obj.get_error(), nullptr);

  CASE_EXPECT_EQ(success_obj.get_success(), nullptr);
  CASE_EXPECT_NE(success_obj.get_error(), nullptr);

  CASE_EXPECT_NE(error_obj.get_success(), nullptr);
  CASE_EXPECT_EQ(*success_obj.get_error(), 456);

  // Swap error and success
  success_obj.swap(error_obj);

  CASE_EXPECT_TRUE(success_obj.is_success());
  CASE_EXPECT_FALSE(success_obj.is_error());
  CASE_EXPECT_FALSE(success_obj.is_none());

  CASE_EXPECT_FALSE(error_obj.is_success());
  CASE_EXPECT_TRUE(error_obj.is_error());
  CASE_EXPECT_FALSE(error_obj.is_none());

  CASE_EXPECT_NE(success_obj.get_success(), nullptr);
  CASE_EXPECT_EQ(success_obj.get_error(), nullptr);

  CASE_EXPECT_EQ(error_obj.get_success(), nullptr);
  CASE_EXPECT_NE(error_obj.get_error(), nullptr);

  CASE_EXPECT_NE(success_obj.get_success(), nullptr);
  CASE_EXPECT_EQ(*error_obj.get_error(), 456);
}

CASE_TEST(result_type, move_transform) {
  // void success and then void success
  {
    using test_type = atfw::util::design_pattern::result_type<void, uint32_t>;
    int call_count = 0;
    auto success_obj = test_type::make_success().transform([&call_count]() { call_count = 1; });

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);

    auto error_obj = test_type::make_error(456U).transform([&call_count]() { call_count = 2; });

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(456, *error_obj.get_error());
  }

  // void success and then non-void success
  {
    using test_type = atfw::util::design_pattern::result_type<void, uint32_t>;
    int call_count = 0;
    auto success_obj = test_type::make_success().transform([&call_count]() {
      call_count = 1;
      return 123;
    });

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(123, *success_obj.get_success());

    auto error_obj = test_type::make_error(456U).transform([&call_count]() { call_count = 2; });

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(456, *error_obj.get_error());
  }

  // no-void success and then void success
  {
    using test_type = atfw::util::design_pattern::result_type<int64_t, uint32_t>;
    int call_count = 0;
    int64_t result = 0;
    auto success_obj = test_type::make_success(137).transform([&call_count, &result](int64_t v) {
      call_count = 1;
      result = v;
    });

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(result, 137);

    result = -1;
    auto error_obj = test_type::make_error(456U).transform([&call_count, &result](int64_t v) {
      call_count = 2;
      result = v;
    });

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(456, *error_obj.get_error());
    CASE_EXPECT_EQ(result, -1);
  }

  // no-void success and then non-void success
  {
    using test_type = atfw::util::design_pattern::result_type<int64_t, uint32_t>;
    int call_count = 0;
    auto success_obj = test_type::make_success(137).transform([&call_count](int64_t v) {
      call_count = 1;
      return v + 11;
    });

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(148, *success_obj.get_success());

    auto error_obj = test_type::make_error(456U).transform([&call_count](int64_t v) {
      call_count = 2;
      return v + 11;
    });

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(456, *error_obj.get_error());
  }

  // parameter type checking
  {
    using test_type = atfw::util::design_pattern::result_type<std::string, void>;
    auto success_obj =
        test_type::make_success("hello").transform([](const std::string& v) { return v + " world: cr"; });
    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_EQ(std::string("hello world: cr"), *success_obj.get_success());

    success_obj = test_type::make_success("hello").transform([](std::string v) { return v + " world: copy"; });
    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_EQ(std::string("hello world: copy"), *success_obj.get_success());

    success_obj = test_type::make_success("hello").transform([](std::string&& v) { return v + " world: rvalue"; });
    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_EQ(std::string("hello world: rvalue"), *success_obj.get_success());
  }
}

CASE_TEST(result_type, copy_transform) {
  // void success and then void success
  {
    using test_type = atfw::util::design_pattern::result_type<void, uint32_t>;
    int call_count = 0;
    auto success_obj1 = test_type::make_success();
    auto success_obj = success_obj1.transform([&call_count]() { call_count = 1; });

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);

    auto error_obj1 = test_type::make_error(456U);
    auto error_obj = error_obj1.transform([&call_count]() { call_count = 2; });

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(456, *error_obj.get_error());
  }

  // void success and then non-void success
  {
    using test_type = atfw::util::design_pattern::result_type<void, uint32_t>;
    int call_count = 0;
    auto success_obj1 = test_type::make_success();
    auto success_obj = success_obj1.transform([&call_count]() {
      call_count = 1;
      return 123;
    });

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(123, *success_obj.get_success());

    auto error_obj1 = test_type::make_error(456U);
    auto error_obj = error_obj1.transform([&call_count]() { call_count = 2; });

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(456, *error_obj.get_error());
  }

  // no-void success and then void success
  {
    using test_type = atfw::util::design_pattern::result_type<int64_t, uint32_t>;
    int call_count = 0;
    int64_t result = 0;
    auto success_obj1 = test_type::make_success(137);
    auto success_obj = success_obj1.transform([&call_count, &result](int64_t v) {
      call_count = 1;
      result = v;
    });

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(result, 137);

    result = -1;
    auto error_obj1 = test_type::make_error(456U);
    auto error_obj = error_obj1.transform([&call_count, &result](int64_t v) {
      call_count = 2;
      result = v;
    });

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(456, *error_obj.get_error());
    CASE_EXPECT_EQ(result, -1);
  }

  // no-void success and then non-void success
  {
    using test_type = atfw::util::design_pattern::result_type<int64_t, uint32_t>;
    int call_count = 0;
    auto success_obj1 = test_type::make_success(137);
    auto success_obj = success_obj1.transform([&call_count](int64_t v) {
      call_count = 1;
      return v + 11;
    });

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(148, *success_obj.get_success());

    auto error_obj1 = test_type::make_error(456U);
    auto error_obj = error_obj1.transform([&call_count](int64_t v) {
      call_count = 2;
      return v + 11;
    });

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(456, *error_obj.get_error());
  }
}

CASE_TEST(result_type, move_transform_error) {
  // void success and then void success
  {
    using test_type = atfw::util::design_pattern::result_type<uint32_t, void>;
    int call_count = 0;
    auto error_obj = test_type::make_error().transform_error([&call_count]() { call_count = 1; });

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);

    auto success_obj = test_type::make_success(456U).transform_error([&call_count]() { call_count = 2; });

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(456, *success_obj.get_success());
  }

  // void success and then non-void success
  {
    using test_type = atfw::util::design_pattern::result_type<uint32_t, void>;
    int call_count = 0;
    auto error_obj = test_type::make_error().transform_error([&call_count]() {
      call_count = 1;
      return 123;
    });

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(123, *error_obj.get_error());

    auto success_obj = test_type::make_success(456U).transform_error([&call_count]() { call_count = 2; });

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(456, *success_obj.get_success());
  }

  // no-void success and then void success
  {
    using test_type = atfw::util::design_pattern::result_type<uint32_t, int64_t>;
    int call_count = 0;
    int64_t result = 0;
    auto error_obj = test_type::make_error(137).transform_error([&call_count, &result](int64_t v) {
      call_count = 1;
      result = v;
    });

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(result, 137);

    result = -1;
    auto success_obj = test_type::make_success(456U).transform_error([&call_count, &result](int64_t v) {
      call_count = 2;
      result = v;
    });

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(456, *success_obj.get_success());
    CASE_EXPECT_EQ(result, -1);
  }

  // no-void success and then non-void success
  {
    using test_type = atfw::util::design_pattern::result_type<uint32_t, int64_t>;
    int call_count = 0;
    auto error_obj = test_type::make_error(137).transform_error([&call_count](int64_t v) {
      call_count = 1;
      return v + 11;
    });

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(148, *error_obj.get_error());

    auto success_obj = test_type::make_success(456U).transform_error([&call_count](int64_t v) {
      call_count = 2;
      return v + 11;
    });

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(456, *success_obj.get_success());
  }

  // parameter type checking
  {
    using test_type = atfw::util::design_pattern::result_type<void, std::string>;
    auto error_obj =
        test_type::make_error("hello").transform_error([](const std::string& v) { return v + " world: cr"; });
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_EQ(std::string("hello world: cr"), *error_obj.get_error());

    error_obj = test_type::make_error("hello").transform_error([](std::string v) { return v + " world: copy"; });
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_EQ(std::string("hello world: copy"), *error_obj.get_error());

    error_obj = test_type::make_error("hello").transform_error([](std::string&& v) { return v + " world: rvalue"; });
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_EQ(std::string("hello world: rvalue"), *error_obj.get_error());
  }
}

CASE_TEST(result_type, copy_transform_error) {
  // void success and then void success
  {
    using test_type = atfw::util::design_pattern::result_type<uint32_t, void>;
    int call_count = 0;
    auto error_obj1 = test_type::make_error();
    auto error_obj = error_obj1.transform_error([&call_count]() { call_count = 1; });

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);

    auto success_obj1 = test_type::make_success(456U);
    auto success_obj = success_obj1.transform_error([&call_count]() { call_count = 2; });

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(456, *success_obj.get_success());
  }

  // void success and then non-void success
  {
    using test_type = atfw::util::design_pattern::result_type<uint32_t, void>;
    int call_count = 0;
    auto error_obj1 = test_type::make_error();
    auto error_obj = error_obj1.transform_error([&call_count]() {
      call_count = 1;
      return 123;
    });

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(123, *error_obj.get_error());

    auto success_obj1 = test_type::make_success(456U);
    auto success_obj = success_obj1.transform_error([&call_count]() { call_count = 2; });

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(456, *success_obj.get_success());
  }

  // no-void success and then void success
  {
    using test_type = atfw::util::design_pattern::result_type<uint32_t, int64_t>;
    int call_count = 0;
    int64_t result = 0;
    auto error_obj1 = test_type::make_error(137);
    auto error_obj = error_obj1.transform_error([&call_count, &result](int64_t v) {
      call_count = 1;
      result = v;
    });

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(result, 137);

    result = -1;
    auto success_obj1 = test_type::make_success(456U);
    auto success_obj = success_obj1.transform_error([&call_count, &result](int64_t v) {
      call_count = 2;
      result = v;
    });

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(456, *success_obj.get_success());
    CASE_EXPECT_EQ(result, -1);
  }

  // no-void success and then non-void success
  {
    using test_type = atfw::util::design_pattern::result_type<uint32_t, int64_t>;
    int call_count = 0;
    auto error_obj1 = test_type::make_error(137);
    auto error_obj = error_obj1.transform_error([&call_count](int64_t v) {
      call_count = 1;
      return v + 11;
    });

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(148, *error_obj.get_error());

    auto success_obj1 = test_type::make_success(456U);
    auto success_obj = success_obj1.transform_error([&call_count](int64_t v) {
      call_count = 2;
      return v + 11;
    });

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(456, *success_obj.get_success());
  }
}

CASE_TEST(result_type, move_and_then) {
  // void success and then void success
  {
    using test_type = atfw::util::design_pattern::result_type<void, uint32_t>;
    using next_type = atfw::util::design_pattern::result_type<void, uint32_t>;
    int call_count = 0;
    auto success_obj = test_type::make_success().and_then([&call_count]() {
      call_count = 1;
      return next_type::make_success();
    });

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);

    auto error_obj = test_type::make_error(456U).and_then([&call_count]() {
      call_count = 2;
      return next_type::make_success();
    });

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(456, *error_obj.get_error());
  }

  // void success and then non-void success
  {
    using test_type = atfw::util::design_pattern::result_type<void, uint32_t>;
    using next_type = atfw::util::design_pattern::result_type<int, uint32_t>;
    int call_count = 0;
    auto success_obj = test_type::make_success().and_then([&call_count]() {
      call_count = 1;
      return next_type::make_success(123);
    });

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(123, *success_obj.get_success());

    auto error_obj = test_type::make_error(456U).and_then([&call_count]() {
      call_count = 2;
      return next_type::make_success(123);
    });

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(456, *error_obj.get_error());
  }

  // no-void success and then void success
  {
    using test_type = atfw::util::design_pattern::result_type<int64_t, uint32_t>;
    using next_type = atfw::util::design_pattern::result_type<void, uint32_t>;
    int call_count = 0;
    int64_t result = 0;
    auto success_obj = test_type::make_success(137).and_then([&call_count, &result](int64_t v) {
      call_count = 1;
      result = v;
      return next_type::make_success();
    });

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(result, 137);

    result = -1;
    auto error_obj = test_type::make_error(456U).and_then([&call_count, &result](int64_t v) {
      call_count = 2;
      result = v;
      return next_type::make_success();
    });

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(456, *error_obj.get_error());
    CASE_EXPECT_EQ(result, -1);
  }

  // no-void success and then non-void success
  {
    using test_type = atfw::util::design_pattern::result_type<int64_t, uint32_t>;
    using next_type = atfw::util::design_pattern::result_type<int64_t, uint32_t>;
    int call_count = 0;
    auto success_obj = test_type::make_success(137).and_then([&call_count](int64_t v) {
      call_count = 1;
      return next_type::make_success(v + 11);
    });

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(148, *success_obj.get_success());

    auto error_obj = test_type::make_error(456U).and_then([&call_count](int64_t v) {
      call_count = 2;
      return next_type::make_success(v + 11);
    });

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(456, *error_obj.get_error());
  }

  // parameter type checking
  {
    using test_type = atfw::util::design_pattern::result_type<std::string, void>;
    auto success_obj = test_type::make_success("hello").and_then(
        [](const std::string& v) { return test_type::make_success(v + " world: cr"); });
    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_EQ(std::string("hello world: cr"), *success_obj.get_success());

    success_obj = test_type::make_success("hello").and_then(
        [](std::string v) { return test_type::make_success(v + " world: copy"); });
    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_EQ(std::string("hello world: copy"), *success_obj.get_success());

    success_obj = test_type::make_success("hello").and_then(
        [](std::string&& v) { return test_type::make_success(v + " world: rvalue"); });
    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_EQ(std::string("hello world: rvalue"), *success_obj.get_success());
  }
}

CASE_TEST(result_type, copy_and_then) {
  // void success and then void success
  {
    using test_type = atfw::util::design_pattern::result_type<void, uint32_t>;
    using next_type = atfw::util::design_pattern::result_type<void, uint32_t>;
    int call_count = 0;
    auto success_obj1 = test_type::make_success();
    auto success_obj = success_obj1.and_then([&call_count]() {
      call_count = 1;
      return next_type::make_success();
    });

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);

    auto error_obj1 = test_type::make_error(456U);
    auto error_obj = error_obj1.and_then([&call_count]() {
      call_count = 2;
      return next_type::make_success();
    });

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(456, *error_obj.get_error());
  }

  // void success and then non-void success
  {
    using test_type = atfw::util::design_pattern::result_type<void, uint32_t>;
    using next_type = atfw::util::design_pattern::result_type<int, uint32_t>;
    int call_count = 0;
    auto success_obj1 = test_type::make_success();
    auto success_obj = success_obj1.and_then([&call_count]() {
      call_count = 1;
      return next_type::make_success(123);
    });

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(123, *success_obj.get_success());

    auto error_obj1 = test_type::make_error(456U);
    auto error_obj = error_obj1.and_then([&call_count]() {
      call_count = 2;
      return next_type::make_success(123);
    });

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(456, *error_obj.get_error());
  }

  // no-void success and then void success
  {
    using test_type = atfw::util::design_pattern::result_type<int64_t, uint32_t>;
    using next_type = atfw::util::design_pattern::result_type<void, uint32_t>;
    int call_count = 0;
    int64_t result = 0;
    auto success_obj1 = test_type::make_success(137);
    auto success_obj = success_obj1.and_then([&call_count, &result](int64_t v) {
      call_count = 1;
      result = v;
      return next_type::make_success();
    });

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(result, 137);

    result = -1;
    auto error_obj1 = test_type::make_error(456U);
    auto error_obj = error_obj1.and_then([&call_count, &result](int64_t v) {
      call_count = 2;
      result = v;
      return next_type::make_success();
    });

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(456, *error_obj.get_error());
    CASE_EXPECT_EQ(result, -1);
  }

  // no-void success and then non-void success
  {
    using test_type = atfw::util::design_pattern::result_type<int64_t, uint32_t>;
    using next_type = atfw::util::design_pattern::result_type<int64_t, uint32_t>;
    int call_count = 0;
    auto success_obj1 = test_type::make_success(137);
    auto success_obj = success_obj1.and_then([&call_count](int64_t v) {
      call_count = 1;
      return next_type::make_success(v + 11);
    });

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(148, *success_obj.get_success());

    auto error_obj1 = test_type::make_error(456U);
    auto error_obj = error_obj1.and_then([&call_count](int64_t v) {
      call_count = 2;
      return next_type::make_success(v + 11);
    });

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(456, *error_obj.get_error());
  }
}

CASE_TEST(result_type, move_or_else) {
  // void success and then void success
  {
    using test_type = atfw::util::design_pattern::result_type<uint32_t, void>;
    using next_type = atfw::util::design_pattern::result_type<uint32_t, void>;
    int call_count = 0;
    auto error_obj = test_type::make_error().or_else([&call_count]() {
      call_count = 1;
      return next_type::make_error();
    });

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);

    auto success_obj = test_type::make_success(456U).or_else([&call_count]() {
      call_count = 2;
      return next_type::make_error();
    });

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(456, *success_obj.get_success());
  }

  // void success and then non-void success
  {
    using test_type = atfw::util::design_pattern::result_type<uint32_t, void>;
    using next_type = atfw::util::design_pattern::result_type<uint32_t, int>;
    int call_count = 0;
    auto error_obj = test_type::make_error().or_else([&call_count]() {
      call_count = 1;
      return next_type::make_error(123);
    });

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(123, *error_obj.get_error());

    auto success_obj = test_type::make_success(456U).or_else([&call_count]() {
      call_count = 2;
      return next_type::make_error(123);
    });

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(456, *success_obj.get_success());
  }

  // no-void success and then void success
  {
    using test_type = atfw::util::design_pattern::result_type<uint32_t, int64_t>;
    using next_type = atfw::util::design_pattern::result_type<uint32_t, void>;
    int call_count = 0;
    int64_t result = 0;
    auto error_obj = test_type::make_error(137).or_else([&call_count, &result](int64_t v) {
      call_count = 1;
      result = v;
      return next_type::make_error();
    });

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(result, 137);

    result = -1;
    auto success_obj = test_type::make_success(456U).or_else([&call_count, &result](int64_t v) {
      call_count = 2;
      result = v;
      return next_type::make_error();
    });

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(456, *success_obj.get_success());
    CASE_EXPECT_EQ(result, -1);
  }

  // no-void success and then non-void success
  {
    using test_type = atfw::util::design_pattern::result_type<uint32_t, int64_t>;
    using next_type = atfw::util::design_pattern::result_type<uint32_t, int64_t>;
    int call_count = 0;
    auto error_obj = test_type::make_error(137).or_else([&call_count](int64_t v) {
      call_count = 1;
      return next_type::make_error(v + 11);
    });

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(148, *error_obj.get_error());

    auto success_obj = test_type::make_success(456U).or_else([&call_count](int64_t v) {
      call_count = 2;
      return next_type::make_error(v + 11);
    });

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(456, *success_obj.get_success());
  }

  // parameter type checking
  {
    using test_type = atfw::util::design_pattern::result_type<void, std::string>;
    auto error_obj = test_type::make_error("hello").or_else(
        [](const std::string& v) { return test_type::make_error(v + " world: cr"); });
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_EQ(std::string("hello world: cr"), *error_obj.get_error());

    error_obj =
        test_type::make_error("hello").or_else([](std::string v) { return test_type::make_error(v + " world: copy"); });
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_EQ(std::string("hello world: copy"), *error_obj.get_error());

    error_obj = test_type::make_error("hello").or_else(
        [](std::string&& v) { return test_type::make_error(v + " world: rvalue"); });
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_EQ(std::string("hello world: rvalue"), *error_obj.get_error());
  }
}

CASE_TEST(result_type, copy_or_else) {
  // void success and then void success
  {
    using test_type = atfw::util::design_pattern::result_type<uint32_t, void>;
    using next_type = atfw::util::design_pattern::result_type<uint32_t, void>;
    int call_count = 0;
    auto error_obj1 = test_type::make_error();
    auto error_obj = error_obj1.or_else([&call_count]() {
      call_count = 1;
      return next_type::make_error();
    });

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);

    auto success_obj1 = test_type::make_success(456U);
    auto success_obj = success_obj1.or_else([&call_count]() {
      call_count = 2;
      return next_type::make_error();
    });

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(456, *success_obj.get_success());
  }

  // void success and then non-void success
  {
    using test_type = atfw::util::design_pattern::result_type<uint32_t, void>;
    using next_type = atfw::util::design_pattern::result_type<uint32_t, int>;
    int call_count = 0;
    auto error_obj1 = test_type::make_error();
    auto error_obj = error_obj1.or_else([&call_count]() {
      call_count = 1;
      return next_type::make_error(123);
    });

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(123, *error_obj.get_error());

    auto success_obj1 = test_type::make_success(456U);
    auto success_obj = success_obj1.or_else([&call_count]() {
      call_count = 2;
      return next_type::make_error(123);
    });

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(456, *success_obj.get_success());
  }

  // no-void success and then void success
  {
    using test_type = atfw::util::design_pattern::result_type<uint32_t, int64_t>;
    using next_type = atfw::util::design_pattern::result_type<uint32_t, void>;
    int call_count = 0;
    int64_t result = 0;
    auto error_obj1 = test_type::make_error(137);
    auto error_obj = error_obj1.or_else([&call_count, &result](int64_t v) {
      call_count = 1;
      result = v;
      return next_type::make_error();
    });

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(result, 137);

    result = -1;
    auto success_obj1 = test_type::make_success(456U);
    auto success_obj = success_obj1.or_else([&call_count, &result](int64_t v) {
      call_count = 2;
      result = v;
      return next_type::make_error();
    });

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(456, *success_obj.get_success());
    CASE_EXPECT_EQ(result, -1);
  }

  // no-void success and then non-void success
  {
    using test_type = atfw::util::design_pattern::result_type<uint32_t, int64_t>;
    using next_type = atfw::util::design_pattern::result_type<uint32_t, int64_t>;
    int call_count = 0;
    auto error_obj1 = test_type::make_error(137);
    auto error_obj = error_obj1.or_else([&call_count](int64_t v) {
      call_count = 1;
      return next_type::make_error(v + 11);
    });

    CASE_EXPECT_FALSE(error_obj.is_success());
    CASE_EXPECT_TRUE(error_obj.is_error());
    CASE_EXPECT_FALSE(error_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(148, *error_obj.get_error());

    auto success_obj1 = test_type::make_success(456U);
    auto success_obj = success_obj1.or_else([&call_count](int64_t v) {
      call_count = 2;
      return next_type::make_error(v + 11);
    });

    CASE_EXPECT_TRUE(success_obj.is_success());
    CASE_EXPECT_FALSE(success_obj.is_error());
    CASE_EXPECT_FALSE(success_obj.is_none());
    CASE_EXPECT_EQ(call_count, 1);
    CASE_EXPECT_EQ(456, *success_obj.get_success());
  }
}

CASE_TEST(result_type, success_to_error) {
  using test_type = atfw::util::design_pattern::result_type<int, int>;
  using next_type = atfw::util::design_pattern::result_type<bool, int64_t>;
  auto result_obj = test_type::make_success(123).and_then([](int v) { return next_type::make_error(v + 300); });
  CASE_EXPECT_FALSE(result_obj.is_success());
  CASE_EXPECT_EQ(423, *result_obj.get_error());
}

CASE_TEST(result_type, error_to_success) {
  using test_type = atfw::util::design_pattern::result_type<int, int>;
  using next_type = atfw::util::design_pattern::result_type<int64_t, bool>;
  auto result_obj = test_type::make_error(123).or_else([](int v) { return next_type::make_success(v + 300); });
  CASE_EXPECT_TRUE(result_obj.is_success());
  CASE_EXPECT_EQ(423, *result_obj.get_success());
}

struct result_type_test_inplace_storage_type {
  char data[256];
};

struct result_type_test_unique_ptr_storage_type {
  char data[256];
};

ATFW_UTIL_MACRO_DECLARE_INPLACE_RESULT_STORAGE(result_type_test_inplace_storage_type);

CASE_TEST(result_type, custom_inplace_storage) {
  using test_inplace_type = atfw::util::design_pattern::result_type<result_type_test_inplace_storage_type, void>;
  using test_unique_type = atfw::util::design_pattern::result_type<result_type_test_unique_ptr_storage_type, void>;
  CASE_EXPECT_GE(sizeof(test_inplace_type), 256);
  CASE_EXPECT_LT(sizeof(test_unique_type), 64);
}

CASE_TEST(result_type, continue_transform_transform_error) {
  // transform
  {
    using test_type = atfw::util::design_pattern::result_type<uint32_t, int64_t>;
    int call_count = 0;
    auto result = test_type::make_success(123U)
                      .transform([](uint32_t v) { return v + 1000; })
                      .transform([](uint32_t v) { return v + 20000; })
                      .transform_error([&call_count](int64_t v) {
                        ++call_count;
                        return v + 300000;
                      })
                      .transform([](uint32_t v) { return v + 300000; });

    CASE_EXPECT_EQ(321123, *result.get_success());
    CASE_EXPECT_EQ(0, call_count);
  }

  // transform_error
  {
    using test_type = atfw::util::design_pattern::result_type<uint32_t, int64_t>;
    int call_count = 0;
    auto result = test_type::make_error(123U)
                      .transform_error([](int64_t v) { return v + 2000; })
                      .transform_error([](int64_t v) { return v + 40000; })
                      .transform([&call_count](uint32_t v) {
                        ++call_count;
                        return v + 300000;
                      })
                      .transform_error([](int64_t v) { return v + 600000; });

    CASE_EXPECT_EQ(642123, *result.get_error());
    CASE_EXPECT_EQ(0, call_count);
  }
}
