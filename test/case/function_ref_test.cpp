// Copyright 2021 atframework
// Created by owent on 2021-08-10

#include <nostd/type_traits.h>

#include <stdint.h>
#include <cstdlib>
#include <cstring>
#include <string>

#include "frame/test_macros.h"

#include "gsl/select-gsl.h"
#include "nostd/function_ref.h"

namespace {

static void run_fun(atfw::util::nostd::function_ref<void()> f) { f(); }

CASE_TEST(function_ref_test, Lambda) {
  bool ran = false;
  run_fun([&] { ran = true; });
  CASE_EXPECT_TRUE(ran);
}

static int normal_function() { return 1337; }

CASE_TEST(function_ref_test, Function1) {
  atfw::util::nostd::function_ref<int()> ref(&normal_function);
  CASE_EXPECT_EQ(1337, ref());
}

CASE_TEST(function_ref_test, Function2) {
  atfw::util::nostd::function_ref<int()> ref(normal_function);
  CASE_EXPECT_EQ(1337, ref());
}

CASE_TEST(function_ref_test, ConstFunction) {
  atfw::util::nostd::function_ref<int() const> ref(normal_function);
  CASE_EXPECT_EQ(1337, ref());
}

static int no_except_function() noexcept { return 1337; }

// TODO(jdennett): Add a test for noexcept member functions.
CASE_TEST(function_ref_test, no_except_function) {
  atfw::util::nostd::function_ref<int()> ref(no_except_function);
  CASE_EXPECT_EQ(1337, ref());
}

CASE_TEST(function_ref_test, ForwardsArgs) {
  auto l = [](std::unique_ptr<int> i) { return *i; };
  atfw::util::nostd::function_ref<int(std::unique_ptr<int>)> ref(l);
  CASE_EXPECT_EQ(42, ref(gsl::make_unique<int>(42)));
}

CASE_TEST(function_ref_test, ReturnMoveOnly) {
  auto l = [] { return gsl::make_unique<int>(29); };
  atfw::util::nostd::function_ref<std::unique_ptr<int>()> ref(l);
  CASE_EXPECT_EQ(29, *ref());
}

CASE_TEST(function_ref_test, ManyArgs) {
  auto l = [](int a, int b, int c) { return a + b + c; };
  atfw::util::nostd::function_ref<int(int, int, int)> ref(l);
  CASE_EXPECT_EQ(6, ref(1, 2, 3));
}

CASE_TEST(function_ref_test, VoidResultFromNonVoidFunctor) {
  bool ran = false;
  auto l = [&]() -> int {
    ran = true;
    return 2;
  };
  atfw::util::nostd::function_ref<void()> ref(l);
  ref();
  CASE_EXPECT_TRUE(ran);
}

CASE_TEST(function_ref_test, CastFromDerived) {
  struct Base {};
  struct Derived : public Base {};

  Derived d;
  auto l1 = [&](Base* b) { CASE_EXPECT_EQ(&d, b); };
  atfw::util::nostd::function_ref<void(Derived*)> ref1(l1);
  ref1(&d);

  auto l2 = [&]() -> Derived* { return &d; };
  atfw::util::nostd::function_ref<Base*()> ref2(l2);
  CASE_EXPECT_EQ(&d, ref2());
}

CASE_TEST(function_ref_test, VoidResultFromNonVoidFuncton) {
  atfw::util::nostd::function_ref<void()> ref(normal_function);
  ref();
}

CASE_TEST(function_ref_test, MemberPtr) {
  struct S {
    int i;
  };

  S s{1100111};
  auto mem_ptr = &S::i;
  atfw::util::nostd::function_ref<int(const S& s)> ref(mem_ptr);
  CASE_EXPECT_EQ(1100111, ref(s));
}

CASE_TEST(function_ref_test, MemberFun) {
  struct S {
    int i;
    int get_i() const { return i; }
  };

  S s{22};
  auto mem_fun_ptr = &S::get_i;
  atfw::util::nostd::function_ref<int(const S& s)> ref(mem_fun_ptr);
  CASE_EXPECT_EQ(22, ref(s));
}

CASE_TEST(function_ref_test, PassByValueTypes) {
  using atfw::util::nostd::details::functional_ref_invoker;
  using atfw::util::nostd::details::functional_ref_void_ptr;
  struct small_trivial {
    void* p[2];
  };
  struct large_trivial {
    void* p[3];
  };

  static_assert(std::is_same<functional_ref_invoker<void, int>, void (*)(functional_ref_void_ptr, int)>::value,
                "Scalar types should be passed by value");
  static_assert(std::is_same<functional_ref_invoker<void, small_trivial>,
                             void (*)(functional_ref_void_ptr, small_trivial)>::value,
                "Small trivial types should be passed by value");
  static_assert(std::is_same<functional_ref_invoker<void, large_trivial>,
                             void (*)(functional_ref_void_ptr, large_trivial&&)>::value,
                "Large trivial types should be passed by rvalue reference");

  // References are passed as references.
  static_assert(std::is_same<functional_ref_invoker<void, int&>, void (*)(functional_ref_void_ptr, int&)>::value,
                "Reference types should be preserved");

  // Make sure the address of an object received by reference is the same as the
  // address of the object passed by the caller.
  {
    large_trivial obj;
    auto test = [&obj](large_trivial& input) { CASE_EXPECT_EQ(&input, &obj); };
    atfw::util::nostd::function_ref<void(large_trivial&)> ref(test);
    ref(obj);
  }

  {
    small_trivial obj;
    auto test = [&obj](small_trivial& input) { CASE_EXPECT_EQ(&input, &obj); };
    atfw::util::nostd::function_ref<void(small_trivial&)> ref(test);
    ref(obj);
  }
}

CASE_TEST(function_ref_test, ReferenceToIncompleteType) {
  struct incomplete_type;
  auto test = [](incomplete_type&) {};
  atfw::util::nostd::function_ref<void(incomplete_type&)> ref(test);

  struct incomplete_type {};
  incomplete_type obj;
  ref(obj);
}
}  // namespace
