// Copyright 2021 atframework

#include <algorithm>
#include <cstring>
#include <ctime>
#include <memory>
#include <sstream>

#include <std/intrusive_ptr.h>

#include "frame/test_macros.h"

struct intrusive_ptr_test_clazz {
  int ref_count;
  int *del_count;

  intrusive_ptr_test_clazz(int *dc) : ref_count(0), del_count(dc) {}
  ~intrusive_ptr_test_clazz() { ++(*del_count); }

  friend void intrusive_ptr_add_ref(intrusive_ptr_test_clazz *p) {
    CASE_EXPECT_NE(p, nullptr);
    if (nullptr != p) {
      ++p->ref_count;
    }
  }

  friend void intrusive_ptr_release(intrusive_ptr_test_clazz *p) {
    CASE_EXPECT_NE(p, nullptr);
    if (nullptr != p) {
      --p->ref_count;
      if (0 == p->ref_count) {
        delete p;
      }
    }
  }
};

CASE_TEST(smart_ptr, intrusive_ptr_int) {
  using ptr_t = std::intrusive_ptr<intrusive_ptr_test_clazz>;
  int delete_count = 0;
  {
    ptr_t p = ptr_t(new intrusive_ptr_test_clazz(&delete_count));

    CASE_EXPECT_EQ(p->ref_count, 1);
    CASE_EXPECT_LE(p, p.get());
    CASE_EXPECT_LE(p, p);
    CASE_EXPECT_LE(p.get(), p);
    CASE_EXPECT_GE(p, p.get());
    CASE_EXPECT_GE(p, p);
    CASE_EXPECT_GE(p.get(), p);

    {
      ptr_t p2(p);
      CASE_EXPECT_EQ(p->ref_count, 2);
    }

    CASE_EXPECT_EQ(p->ref_count, 1);
    {
      ptr_t p2(p);
      CASE_EXPECT_EQ(p->ref_count, 2);

      ptr_t prv(std::move(p2));
      CASE_EXPECT_EQ(p->ref_count, 2);
      CASE_EXPECT_EQ(p2.get(), nullptr);
      CASE_EXPECT_NE(prv.get(), nullptr);
    }

    {
      ptr_t p2(p);
      CASE_EXPECT_EQ(p->ref_count, 2);

      ptr_t prv;
      prv = std::move(p2);
      CASE_EXPECT_EQ(p->ref_count, 2);
      CASE_EXPECT_EQ(p2.get(), nullptr);
      CASE_EXPECT_NE(prv.get(), nullptr);
    }

    {
      ptr_t p2(p);
      CASE_EXPECT_EQ(p->ref_count, 2);
      p2.reset(nullptr);
      CASE_EXPECT_EQ(p->ref_count, 1);
      CASE_EXPECT_EQ(p2.get(), nullptr);

      CASE_EXPECT_TRUE(p);
      CASE_EXPECT_FALSE(p2);
    }

    CASE_EXPECT_EQ(delete_count, 0);
  }

  CASE_EXPECT_EQ(delete_count, 1);
}