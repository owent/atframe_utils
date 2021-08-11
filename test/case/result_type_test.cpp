// Copyright 2021 atframework
// Created by owent on 2021-08-10

#include <stdint.h>
#include <cstdlib>
#include <cstring>
#include <string>

#include "frame/test_macros.h"

#include "design_pattern/result_type.h"

CASE_TEST(result_type, all_triviall) {
  using test_type = util::design_pattern::result_type<int, uint64_t>;
  auto success_obj = test_type::make_success(123);
  auto error_obj = test_type::make_error(456U);

  CASE_EXPECT_TRUE(success_obj.is_success());
  CASE_EXPECT_FALSE(success_obj.is_error());
  CASE_EXPECT_FALSE(success_obj.is_none());

  CASE_EXPECT_FALSE(error_obj.is_success());
  CASE_EXPECT_TRUE(error_obj.is_error());
  CASE_EXPECT_FALSE(error_obj.is_none());

  CASE_EXPECT_NE(success_obj.get_success().get(), nullptr);
  CASE_EXPECT_EQ(success_obj.get_error().get(), nullptr);

  CASE_EXPECT_EQ(error_obj.get_success().get(), nullptr);
  CASE_EXPECT_NE(error_obj.get_error().get(), nullptr);

  CASE_EXPECT_EQ(*success_obj.get_success(), 123);
  CASE_EXPECT_EQ(*error_obj.get_error(), 456);

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

  CASE_EXPECT_NE(error_obj.get_success().get(), nullptr);
  CASE_EXPECT_EQ(error_obj.get_error().get(), nullptr);

  CASE_EXPECT_EQ(success_obj.get_success().get(), nullptr);
  CASE_EXPECT_NE(success_obj.get_error().get(), nullptr);

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

  CASE_EXPECT_NE(success_obj.get_success().get(), nullptr);
  CASE_EXPECT_EQ(success_obj.get_error().get(), nullptr);

  CASE_EXPECT_EQ(error_obj.get_success().get(), nullptr);
  CASE_EXPECT_NE(error_obj.get_error().get(), nullptr);

  CASE_EXPECT_EQ(*success_obj.get_success(), 123);
  CASE_EXPECT_EQ(*error_obj.get_error(), 456);
}

CASE_TEST(result_type, one_triviall) {
  using test_type = util::design_pattern::result_type<int, std::string>;
  auto success_obj = test_type::make_success(123);
  auto error_obj = test_type::make_error("456");

  CASE_EXPECT_TRUE(success_obj.is_success());
  CASE_EXPECT_FALSE(success_obj.is_error());
  CASE_EXPECT_FALSE(success_obj.is_none());

  CASE_EXPECT_FALSE(error_obj.is_success());
  CASE_EXPECT_TRUE(error_obj.is_error());
  CASE_EXPECT_FALSE(error_obj.is_none());

  CASE_EXPECT_NE(success_obj.get_success().get(), nullptr);
  CASE_EXPECT_EQ(success_obj.get_error().get(), nullptr);

  CASE_EXPECT_EQ(error_obj.get_success().get(), nullptr);
  CASE_EXPECT_NE(error_obj.get_error().get(), nullptr);

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

  CASE_EXPECT_NE(error_obj.get_success().get(), nullptr);
  CASE_EXPECT_EQ(error_obj.get_error().get(), nullptr);

  CASE_EXPECT_EQ(success_obj.get_success().get(), nullptr);
  CASE_EXPECT_NE(success_obj.get_error().get(), nullptr);

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

  CASE_EXPECT_NE(success_obj.get_success().get(), nullptr);
  CASE_EXPECT_EQ(success_obj.get_error().get(), nullptr);

  CASE_EXPECT_EQ(error_obj.get_success().get(), nullptr);
  CASE_EXPECT_NE(error_obj.get_error().get(), nullptr);

  CASE_EXPECT_EQ(*success_obj.get_success(), 123);
  CASE_EXPECT_EQ(*error_obj.get_error(), "456");
}
