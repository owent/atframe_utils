// Copyright 2024 atframework

#include "frame/test_macros.h"

#include "config/compiler_features.h"

#include "memory/rc_ptr.h"

#include <stdint.h>
#include <functional>
#include <map>
#include <utility>
#include <vector>

#if defined(__GNUC__) && !defined(__clang__) && !defined(__apple_build_version__)
#  if (__GNUC__ * 100 + __GNUC_MINOR__ * 10) >= 460
#    pragma GCC diagnostic push
#  endif
#elif defined(__clang__) || defined(__apple_build_version__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif

namespace {
template <class X>
struct checked_deleter {
  void operator()(X *p) { delete p; }
};

template <class X>
struct checked_array_deleter {
  void operator()(X *p) { delete[] p; }
};

}  // namespace

namespace strong_rc {
namespace n_element_type {

void f(int &) {}

void test() {
  typedef util::memory::strong_rc_ptr<int>::element_type T;
  T t;
  f(t);
}

}  // namespace n_element_type

namespace n_constructors {

class incomplete;

void default_constructor() {
  {
    util::memory::strong_rc_ptr<int> pi;
    CASE_EXPECT_TRUE(pi ? false : true);
    CASE_EXPECT_TRUE(!pi);
    CASE_EXPECT_TRUE(pi.get() == 0);
    CASE_EXPECT_TRUE(pi.use_count() == 0);
  }

  {
    util::memory::strong_rc_ptr<void> pv;
    CASE_EXPECT_TRUE(pv ? false : true);
    CASE_EXPECT_TRUE(!pv);
    CASE_EXPECT_TRUE(pv.get() == 0);
    CASE_EXPECT_TRUE(pv.use_count() == 0);
  }

  {
    util::memory::strong_rc_ptr<incomplete> px;
    CASE_EXPECT_TRUE(px ? false : true);
    CASE_EXPECT_TRUE(!px);
    CASE_EXPECT_TRUE(px.get() == 0);
    CASE_EXPECT_TRUE(px.use_count() == 0);
  }
}

struct A {
  int dummy;
};

struct X {
  static int64_t instances;

  X() { ++instances; }

  ~X() { --instances; }

 private:
  X(X const &);
  X &operator=(X const &);
};

int64_t X::instances = 0;

// virtual inheritance stresses the implementation

struct Y : public A, public virtual X {
  static int64_t instances;

  Y() { ++instances; }

  ~Y() { --instances; }

 private:
  Y(Y const &);
  Y &operator=(Y const &);
};

int64_t Y::instances = 0;

template <class T>
void pc0_test(T *p) {
  CASE_EXPECT_TRUE(p == 0);
  util::memory::strong_rc_ptr<T> pt(p);
  CASE_EXPECT_TRUE(pt ? false : true);
  CASE_EXPECT_TRUE(!pt);
  CASE_EXPECT_TRUE(pt.get() == 0);
  CASE_EXPECT_TRUE(pt.use_count() == 1);
  CASE_EXPECT_TRUE(pt.unique());
}

void pointer_constructor() {
  pc0_test(static_cast<int *>(0));

  pc0_test(static_cast<int const *>(0));
  pc0_test(static_cast<int volatile *>(0));
  pc0_test(static_cast<int const volatile *>(0));

  {
    util::memory::strong_rc_ptr<int const> pi(static_cast<int *>(0));
    CASE_EXPECT_TRUE(pi ? false : true);
    CASE_EXPECT_TRUE(!pi);
    CASE_EXPECT_TRUE(pi.get() == 0);
    CASE_EXPECT_TRUE(pi.use_count() == 1);
    CASE_EXPECT_TRUE(pi.unique());
  }

  {
    util::memory::strong_rc_ptr<int volatile> pi(static_cast<int *>(0));
    CASE_EXPECT_TRUE(pi ? false : true);
    CASE_EXPECT_TRUE(!pi);
    CASE_EXPECT_TRUE(pi.get() == 0);
    CASE_EXPECT_TRUE(pi.use_count() == 1);
    CASE_EXPECT_TRUE(pi.unique());
  }

  {
    util::memory::strong_rc_ptr<void> pv(static_cast<int *>(0));
    CASE_EXPECT_TRUE(pv ? false : true);
    CASE_EXPECT_TRUE(!pv);
    CASE_EXPECT_TRUE(pv.get() == 0);
    CASE_EXPECT_TRUE(pv.use_count() == 1);
    CASE_EXPECT_TRUE(pv.unique());
  }

  {
    util::memory::strong_rc_ptr<void const> pv(static_cast<int *>(0));
    CASE_EXPECT_TRUE(pv ? false : true);
    CASE_EXPECT_TRUE(!pv);
    CASE_EXPECT_TRUE(pv.get() == 0);
    CASE_EXPECT_TRUE(pv.use_count() == 1);
    CASE_EXPECT_TRUE(pv.unique());
  }

  pc0_test(static_cast<X *>(0));
  pc0_test(static_cast<X const *>(0));
  pc0_test(static_cast<X volatile *>(0));
  pc0_test(static_cast<X const volatile *>(0));

  {
    util::memory::strong_rc_ptr<X const> px(static_cast<X *>(0));
    CASE_EXPECT_TRUE(px ? false : true);
    CASE_EXPECT_TRUE(!px);
    CASE_EXPECT_TRUE(px.get() == 0);
    CASE_EXPECT_TRUE(px.use_count() == 1);
    CASE_EXPECT_TRUE(px.unique());
  }

  {
    util::memory::strong_rc_ptr<X> px(static_cast<Y *>(0));
    CASE_EXPECT_TRUE(px ? false : true);
    CASE_EXPECT_TRUE(!px);
    CASE_EXPECT_TRUE(px.get() == 0);
    CASE_EXPECT_TRUE(px.use_count() == 1);
    CASE_EXPECT_TRUE(px.unique());
  }

  {
    util::memory::strong_rc_ptr<X const> px(static_cast<Y *>(0));
    CASE_EXPECT_TRUE(px ? false : true);
    CASE_EXPECT_TRUE(!px);
    CASE_EXPECT_TRUE(px.get() == 0);
    CASE_EXPECT_TRUE(px.use_count() == 1);
    CASE_EXPECT_TRUE(px.unique());
  }

  {
    util::memory::strong_rc_ptr<void> pv(static_cast<X *>(0));
    CASE_EXPECT_TRUE(pv ? false : true);
    CASE_EXPECT_TRUE(!pv);
    CASE_EXPECT_TRUE(pv.get() == 0);
    CASE_EXPECT_TRUE(pv.use_count() == 1);
    CASE_EXPECT_TRUE(pv.unique());
  }

  {
    util::memory::strong_rc_ptr<void const> pv(static_cast<X *>(0));
    CASE_EXPECT_TRUE(pv ? false : true);
    CASE_EXPECT_TRUE(!pv);
    CASE_EXPECT_TRUE(pv.get() == 0);
    CASE_EXPECT_TRUE(pv.use_count() == 1);
    CASE_EXPECT_TRUE(pv.unique());
  }

  {
    int *p = new int(7);
    util::memory::strong_rc_ptr<int> pi(p);
    CASE_EXPECT_TRUE(pi ? true : false);
    CASE_EXPECT_TRUE(!!pi);
    CASE_EXPECT_TRUE(pi.get() == p);
    CASE_EXPECT_TRUE(pi.use_count() == 1);
    CASE_EXPECT_TRUE(pi.unique());
    CASE_EXPECT_TRUE(*pi == 7);
  }

  {
    int *p = new int(7);
    util::memory::strong_rc_ptr<int const> pi(p);
    CASE_EXPECT_TRUE(pi ? true : false);
    CASE_EXPECT_TRUE(!!pi);
    CASE_EXPECT_TRUE(pi.get() == p);
    CASE_EXPECT_TRUE(pi.use_count() == 1);
    CASE_EXPECT_TRUE(pi.unique());
    CASE_EXPECT_TRUE(*pi == 7);
  }

  {
    int *p = new int(7);
    util::memory::strong_rc_ptr<void> pv(p);
    CASE_EXPECT_TRUE(pv ? true : false);
    CASE_EXPECT_TRUE(!!pv);
    CASE_EXPECT_TRUE(pv.get() == p);
    CASE_EXPECT_TRUE(pv.use_count() == 1);
    CASE_EXPECT_TRUE(pv.unique());
  }

  {
    int *p = new int(7);
    util::memory::strong_rc_ptr<void const> pv(p);
    CASE_EXPECT_TRUE(pv ? true : false);
    CASE_EXPECT_TRUE(!!pv);
    CASE_EXPECT_TRUE(pv.get() == p);
    CASE_EXPECT_TRUE(pv.use_count() == 1);
    CASE_EXPECT_TRUE(pv.unique());
  }

  CASE_EXPECT_TRUE(X::instances == 0);

  {
    X *p = new X;
    util::memory::strong_rc_ptr<X> px(p);
    CASE_EXPECT_TRUE(px ? true : false);
    CASE_EXPECT_TRUE(!!px);
    CASE_EXPECT_TRUE(px.get() == p);
    CASE_EXPECT_TRUE(px.use_count() == 1);
    CASE_EXPECT_TRUE(px.unique());
    CASE_EXPECT_TRUE(X::instances == 1);
  }

  CASE_EXPECT_TRUE(X::instances == 0);

  {
    X *p = new X;
    util::memory::strong_rc_ptr<X const> px(p);
    CASE_EXPECT_TRUE(px ? true : false);
    CASE_EXPECT_TRUE(!!px);
    CASE_EXPECT_TRUE(px.get() == p);
    CASE_EXPECT_TRUE(px.use_count() == 1);
    CASE_EXPECT_TRUE(px.unique());
    CASE_EXPECT_TRUE(X::instances == 1);
  }

  CASE_EXPECT_TRUE(X::instances == 0);

  {
    X *p = new X;
    util::memory::strong_rc_ptr<void> pv(p);
    CASE_EXPECT_TRUE(pv ? true : false);
    CASE_EXPECT_TRUE(!!pv);
    CASE_EXPECT_TRUE(pv.get() == p);
    CASE_EXPECT_TRUE(pv.use_count() == 1);
    CASE_EXPECT_TRUE(pv.unique());
    CASE_EXPECT_TRUE(X::instances == 1);
  }

  CASE_EXPECT_TRUE(X::instances == 0);

  {
    X *p = new X;
    util::memory::strong_rc_ptr<void const> pv(p);
    CASE_EXPECT_TRUE(pv ? true : false);
    CASE_EXPECT_TRUE(!!pv);
    CASE_EXPECT_TRUE(pv.get() == p);
    CASE_EXPECT_TRUE(pv.use_count() == 1);
    CASE_EXPECT_TRUE(pv.unique());
    CASE_EXPECT_TRUE(X::instances == 1);
  }

  CASE_EXPECT_TRUE(X::instances == 0);
  CASE_EXPECT_TRUE(Y::instances == 0);

  {
    Y *p = new Y;
    util::memory::strong_rc_ptr<X> px(p);
    CASE_EXPECT_TRUE(px ? true : false);
    CASE_EXPECT_TRUE(!!px);
    CASE_EXPECT_TRUE(px.get() == p);
    CASE_EXPECT_TRUE(px.use_count() == 1);
    CASE_EXPECT_TRUE(px.unique());
    CASE_EXPECT_TRUE(X::instances == 1);
    CASE_EXPECT_TRUE(Y::instances == 1);
  }

  CASE_EXPECT_TRUE(X::instances == 0);
  CASE_EXPECT_TRUE(Y::instances == 0);

  {
    Y *p = new Y;
    util::memory::strong_rc_ptr<X const> px(p);
    CASE_EXPECT_TRUE(px ? true : false);
    CASE_EXPECT_TRUE(!!px);
    CASE_EXPECT_TRUE(px.get() == p);
    CASE_EXPECT_TRUE(px.use_count() == 1);
    CASE_EXPECT_TRUE(px.unique());
    CASE_EXPECT_TRUE(X::instances == 1);
    CASE_EXPECT_TRUE(Y::instances == 1);
  }

  CASE_EXPECT_TRUE(X::instances == 0);
  CASE_EXPECT_TRUE(Y::instances == 0);
}

int m = 0;

void deleter(int *p) { CASE_EXPECT_TRUE(p == 0); }

void deleter2(int *p) {
  CASE_EXPECT_TRUE(p == &m);
  ++*p;
}

struct deleter3 {
  void operator()(incomplete *p) { CASE_EXPECT_TRUE(p == 0); }
};

// Borland C++ 5.5.1 fails on static_cast<incomplete*>(0)

incomplete *p0 = 0;

void deleter_constructor() {
  {
    util::memory::strong_rc_ptr<int> pi(static_cast<int *>(0), deleter);
    CASE_EXPECT_TRUE(pi ? false : true);
    CASE_EXPECT_TRUE(!pi);
    CASE_EXPECT_TRUE(pi.get() == 0);
    CASE_EXPECT_TRUE(pi.use_count() == 1);
    CASE_EXPECT_TRUE(pi.unique());
  }

  {
    util::memory::strong_rc_ptr<void> pv(static_cast<int *>(0), &deleter);
    CASE_EXPECT_TRUE(pv ? false : true);
    CASE_EXPECT_TRUE(!pv);
    CASE_EXPECT_TRUE(pv.get() == 0);
    CASE_EXPECT_TRUE(pv.use_count() == 1);
    CASE_EXPECT_TRUE(pv.unique());
  }

  {
    util::memory::strong_rc_ptr<void const> pv(static_cast<int *>(0), deleter);
    CASE_EXPECT_TRUE(pv ? false : true);
    CASE_EXPECT_TRUE(!pv);
    CASE_EXPECT_TRUE(pv.get() == 0);
    CASE_EXPECT_TRUE(pv.use_count() == 1);
    CASE_EXPECT_TRUE(pv.unique());
  }

  {
    util::memory::strong_rc_ptr<incomplete> px(p0, deleter3());
    CASE_EXPECT_TRUE(px ? false : true);
    CASE_EXPECT_TRUE(!px);
    CASE_EXPECT_TRUE(px.get() == 0);
    CASE_EXPECT_TRUE(px.use_count() == 1);
    CASE_EXPECT_TRUE(px.unique());
  }

  {
    util::memory::strong_rc_ptr<void> pv(p0, deleter3());
    CASE_EXPECT_TRUE(pv ? false : true);
    CASE_EXPECT_TRUE(!pv);
    CASE_EXPECT_TRUE(pv.get() == 0);
    CASE_EXPECT_TRUE(pv.use_count() == 1);
    CASE_EXPECT_TRUE(pv.unique());
  }

  {
    util::memory::strong_rc_ptr<void const> pv(p0, deleter3());
    CASE_EXPECT_TRUE(pv ? false : true);
    CASE_EXPECT_TRUE(!pv);
    CASE_EXPECT_TRUE(pv.get() == 0);
    CASE_EXPECT_TRUE(pv.use_count() == 1);
    CASE_EXPECT_TRUE(pv.unique());
  }

  CASE_EXPECT_TRUE(m == 0);

  {
    util::memory::strong_rc_ptr<int> pi(&m, deleter2);
    CASE_EXPECT_TRUE(pi ? true : false);
    CASE_EXPECT_TRUE(!!pi);
    CASE_EXPECT_TRUE(pi.get() == &m);
    CASE_EXPECT_TRUE(pi.use_count() == 1);
    CASE_EXPECT_TRUE(pi.unique());
  }

  CASE_EXPECT_TRUE(m == 1);

  {
    util::memory::strong_rc_ptr<int const> pi(&m, &deleter2);
    CASE_EXPECT_TRUE(pi ? true : false);
    CASE_EXPECT_TRUE(!!pi);
    CASE_EXPECT_TRUE(pi.get() == &m);
    CASE_EXPECT_TRUE(pi.use_count() == 1);
    CASE_EXPECT_TRUE(pi.unique());
  }

  CASE_EXPECT_TRUE(m == 2);

  {
    util::memory::strong_rc_ptr<void> pv(&m, deleter2);
    CASE_EXPECT_TRUE(pv ? true : false);
    CASE_EXPECT_TRUE(!!pv);
    CASE_EXPECT_TRUE(pv.get() == &m);
    CASE_EXPECT_TRUE(pv.use_count() == 1);
    CASE_EXPECT_TRUE(pv.unique());
  }

  CASE_EXPECT_TRUE(m == 3);

  {
    util::memory::strong_rc_ptr<void const> pv(&m, &deleter2);
    CASE_EXPECT_TRUE(pv ? true : false);
    CASE_EXPECT_TRUE(!!pv);
    CASE_EXPECT_TRUE(pv.get() == &m);
    CASE_EXPECT_TRUE(pv.use_count() == 1);
    CASE_EXPECT_TRUE(pv.unique());
  }

  CASE_EXPECT_TRUE(m == 4);
}

void copy_constructor() {
  {
    util::memory::strong_rc_ptr<int> pi;

    util::memory::strong_rc_ptr<int> pi2(pi);
    CASE_EXPECT_TRUE(pi2 == pi);
    CASE_EXPECT_TRUE(pi2 ? false : true);
    CASE_EXPECT_TRUE(!pi2);
    CASE_EXPECT_TRUE(pi2.get() == 0);
    CASE_EXPECT_TRUE(pi2.use_count() == pi.use_count());

    util::memory::strong_rc_ptr<void> pi3(pi);
    CASE_EXPECT_TRUE(pi3 == pi);
    CASE_EXPECT_TRUE(pi3 ? false : true);
    CASE_EXPECT_TRUE(!pi3);
    CASE_EXPECT_TRUE(pi3.get() == 0);
    CASE_EXPECT_TRUE(pi3.use_count() == pi.use_count());

    util::memory::strong_rc_ptr<void> pi4(pi3);
    CASE_EXPECT_TRUE(pi4 == pi3);
    CASE_EXPECT_TRUE(pi4 ? false : true);
    CASE_EXPECT_TRUE(!pi4);
    CASE_EXPECT_TRUE(pi4.get() == 0);
    CASE_EXPECT_TRUE(pi4.use_count() == pi3.use_count());
  }

  {
    util::memory::strong_rc_ptr<void> pv;

    util::memory::strong_rc_ptr<void> pv2(pv);
    CASE_EXPECT_TRUE(pv2 == pv);
    CASE_EXPECT_TRUE(pv2 ? false : true);
    CASE_EXPECT_TRUE(!pv2);
    CASE_EXPECT_TRUE(pv2.get() == 0);
    CASE_EXPECT_TRUE(pv2.use_count() == pv.use_count());
  }

  {
    util::memory::strong_rc_ptr<incomplete> px;

    util::memory::strong_rc_ptr<incomplete> px2(px);
    CASE_EXPECT_TRUE(px2 == px);
    CASE_EXPECT_TRUE(px2 ? false : true);
    CASE_EXPECT_TRUE(!px2);
    CASE_EXPECT_TRUE(px2.get() == 0);
    CASE_EXPECT_TRUE(px2.use_count() == px.use_count());

    util::memory::strong_rc_ptr<void> px3(px);
    CASE_EXPECT_TRUE(px3 == px);
    CASE_EXPECT_TRUE(px3 ? false : true);
    CASE_EXPECT_TRUE(!px3);
    CASE_EXPECT_TRUE(px3.get() == 0);
    CASE_EXPECT_TRUE(px3.use_count() == px.use_count());
  }

  {
    util::memory::strong_rc_ptr<int> pi(static_cast<int *>(0));

    util::memory::strong_rc_ptr<int> pi2(pi);
    CASE_EXPECT_TRUE(pi2 == pi);
    CASE_EXPECT_TRUE(pi2 ? false : true);
    CASE_EXPECT_TRUE(!pi2);
    CASE_EXPECT_TRUE(pi2.get() == 0);
    CASE_EXPECT_TRUE(pi2.use_count() == 2);
    CASE_EXPECT_TRUE(!pi2.unique());
    CASE_EXPECT_TRUE(pi2.use_count() == pi.use_count());
    CASE_EXPECT_TRUE(!(pi < pi2 || pi2 < pi));  // shared ownership test

    util::memory::strong_rc_ptr<void> pi3(pi);
    CASE_EXPECT_TRUE(pi3 == pi);
    CASE_EXPECT_TRUE(pi3 ? false : true);
    CASE_EXPECT_TRUE(!pi3);
    CASE_EXPECT_TRUE(pi3.get() == 0);
    CASE_EXPECT_TRUE(pi3.use_count() == 3);
    CASE_EXPECT_TRUE(!pi3.unique());
    CASE_EXPECT_TRUE(pi3.use_count() == pi.use_count());
    CASE_EXPECT_TRUE(!(pi < pi3 || pi3 < pi));  // shared ownership test

    util::memory::strong_rc_ptr<void> pi4(pi2);
    CASE_EXPECT_TRUE(pi4 == pi2);
    CASE_EXPECT_TRUE(pi4 ? false : true);
    CASE_EXPECT_TRUE(!pi4);
    CASE_EXPECT_TRUE(pi4.get() == 0);
    CASE_EXPECT_TRUE(pi4.use_count() == 4);
    CASE_EXPECT_TRUE(!pi4.unique());
    CASE_EXPECT_TRUE(pi4.use_count() == pi2.use_count());
    CASE_EXPECT_TRUE(!(pi2 < pi4 || pi4 < pi2));  // shared ownership test

    CASE_EXPECT_TRUE(pi3.use_count() == pi4.use_count());
    CASE_EXPECT_TRUE(!(pi3 < pi4 || pi4 < pi3));  // shared ownership test
  }

  {
    util::memory::strong_rc_ptr<X> px(static_cast<X *>(0));

    util::memory::strong_rc_ptr<X> px2(px);
    CASE_EXPECT_TRUE(px2 == px);
    CASE_EXPECT_TRUE(px2 ? false : true);
    CASE_EXPECT_TRUE(!px2);
    CASE_EXPECT_TRUE(px2.get() == 0);
    CASE_EXPECT_TRUE(px2.use_count() == 2);
    CASE_EXPECT_TRUE(!px2.unique());
    CASE_EXPECT_TRUE(px2.use_count() == px.use_count());
    CASE_EXPECT_TRUE(!(px < px2 || px2 < px));  // shared ownership test

    util::memory::strong_rc_ptr<void> px3(px);
    CASE_EXPECT_TRUE(px3 == px);
    CASE_EXPECT_TRUE(px3 ? false : true);
    CASE_EXPECT_TRUE(!px3);
    CASE_EXPECT_TRUE(px3.get() == 0);
    CASE_EXPECT_TRUE(px3.use_count() == 3);
    CASE_EXPECT_TRUE(!px3.unique());
    CASE_EXPECT_TRUE(px3.use_count() == px.use_count());
    CASE_EXPECT_TRUE(!(px < px3 || px3 < px));  // shared ownership test

    util::memory::strong_rc_ptr<void> px4(px2);
    CASE_EXPECT_TRUE(px4 == px2);
    CASE_EXPECT_TRUE(px4 ? false : true);
    CASE_EXPECT_TRUE(!px4);
    CASE_EXPECT_TRUE(px4.get() == 0);
    CASE_EXPECT_TRUE(px4.use_count() == 4);
    CASE_EXPECT_TRUE(!px4.unique());
    CASE_EXPECT_TRUE(px4.use_count() == px2.use_count());
    CASE_EXPECT_TRUE(!(px2 < px4 || px4 < px2));  // shared ownership test

    CASE_EXPECT_TRUE(px3.use_count() == px4.use_count());
    CASE_EXPECT_TRUE(!(px3 < px4 || px4 < px3));  // shared ownership test
  }

  {
    int *p = new int(7);
    util::memory::strong_rc_ptr<int> pi(p);

    util::memory::strong_rc_ptr<int> pi2(pi);
    CASE_EXPECT_TRUE(pi2 == pi);
    CASE_EXPECT_TRUE(pi2 ? true : false);
    CASE_EXPECT_TRUE(!!pi2);
    CASE_EXPECT_TRUE(pi2.get() == p);
    CASE_EXPECT_TRUE(pi2.use_count() == 2);
    CASE_EXPECT_TRUE(!pi2.unique());
    CASE_EXPECT_TRUE(*pi2 == 7);
    CASE_EXPECT_TRUE(pi2.use_count() == pi.use_count());
    CASE_EXPECT_TRUE(!(pi < pi2 || pi2 < pi));  // shared ownership test
  }

  {
    int *p = new int(7);
    util::memory::strong_rc_ptr<void> pv(p);
    CASE_EXPECT_TRUE(pv.get() == p);

    util::memory::strong_rc_ptr<void> pv2(pv);
    CASE_EXPECT_TRUE(pv2 == pv);
    CASE_EXPECT_TRUE(pv2 ? true : false);
    CASE_EXPECT_TRUE(!!pv2);
    CASE_EXPECT_TRUE(pv2.get() == p);
    CASE_EXPECT_TRUE(pv2.use_count() == 2);
    CASE_EXPECT_TRUE(!pv2.unique());
    CASE_EXPECT_TRUE(pv2.use_count() == pv.use_count());
    CASE_EXPECT_TRUE(!(pv < pv2 || pv2 < pv));  // shared ownership test
  }

  CASE_EXPECT_TRUE(X::instances == 0);

  {
    X *p = new X;
    util::memory::strong_rc_ptr<X> px(p);
    CASE_EXPECT_TRUE(px.get() == p);

    util::memory::strong_rc_ptr<X> px2(px);
    CASE_EXPECT_TRUE(px2 == px);
    CASE_EXPECT_TRUE(px2 ? true : false);
    CASE_EXPECT_TRUE(!!px2);
    CASE_EXPECT_TRUE(px2.get() == p);
    CASE_EXPECT_TRUE(px2.use_count() == 2);
    CASE_EXPECT_TRUE(!px2.unique());

    CASE_EXPECT_TRUE(X::instances == 1);

    CASE_EXPECT_TRUE(px2.use_count() == px.use_count());
    CASE_EXPECT_TRUE(!(px < px2 || px2 < px));  // shared ownership test

    util::memory::strong_rc_ptr<void> px3(px);
    CASE_EXPECT_TRUE(px3 == px);
    CASE_EXPECT_TRUE(px3 ? true : false);
    CASE_EXPECT_TRUE(!!px3);
    CASE_EXPECT_TRUE(px3.get() == p);
    CASE_EXPECT_TRUE(px3.use_count() == 3);
    CASE_EXPECT_TRUE(!px3.unique());
    CASE_EXPECT_TRUE(px3.use_count() == px.use_count());
    CASE_EXPECT_TRUE(!(px < px3 || px3 < px));  // shared ownership test

    util::memory::strong_rc_ptr<void> px4(px2);
    CASE_EXPECT_TRUE(px4 == px2);
    CASE_EXPECT_TRUE(px4 ? true : false);
    CASE_EXPECT_TRUE(!!px4);
    CASE_EXPECT_TRUE(px4.get() == p);
    CASE_EXPECT_TRUE(px4.use_count() == 4);
    CASE_EXPECT_TRUE(!px4.unique());
    CASE_EXPECT_TRUE(px4.use_count() == px2.use_count());
    CASE_EXPECT_TRUE(!(px2 < px4 || px4 < px2));  // shared ownership test

    CASE_EXPECT_TRUE(px3.use_count() == px4.use_count());
    CASE_EXPECT_TRUE(!(px3 < px4 || px4 < px3));  // shared ownership test
  }

  CASE_EXPECT_TRUE(X::instances == 0);
  CASE_EXPECT_TRUE(Y::instances == 0);

  {
    Y *p = new Y;
    util::memory::strong_rc_ptr<Y> py(p);
    CASE_EXPECT_TRUE(py.get() == p);

    util::memory::strong_rc_ptr<X> px(py);
    CASE_EXPECT_TRUE(px == py);
    CASE_EXPECT_TRUE(px ? true : false);
    CASE_EXPECT_TRUE(!!px);
    CASE_EXPECT_TRUE(px.get() == p);
    CASE_EXPECT_TRUE(px.use_count() == 2);
    CASE_EXPECT_TRUE(!px.unique());
    CASE_EXPECT_TRUE(px.use_count() == py.use_count());
    CASE_EXPECT_TRUE(!(px < py || py < px));  // shared ownership test

    CASE_EXPECT_TRUE(X::instances == 1);
    CASE_EXPECT_TRUE(Y::instances == 1);

    util::memory::strong_rc_ptr<void const> pv(px);
    CASE_EXPECT_TRUE(pv == px);
    CASE_EXPECT_TRUE(pv ? true : false);
    CASE_EXPECT_TRUE(!!pv);
    CASE_EXPECT_TRUE(pv.get() == px.get());
    CASE_EXPECT_TRUE(pv.use_count() == 3);
    CASE_EXPECT_TRUE(!pv.unique());
    CASE_EXPECT_TRUE(pv.use_count() == px.use_count());
    CASE_EXPECT_TRUE(!(px < pv || pv < px));  // shared ownership test

    util::memory::strong_rc_ptr<void const> pv2(py);
    CASE_EXPECT_TRUE(pv2 == py);
    CASE_EXPECT_TRUE(pv2 ? true : false);
    CASE_EXPECT_TRUE(!!pv2);
    CASE_EXPECT_TRUE(pv2.get() == py.get());
    CASE_EXPECT_TRUE(pv2.use_count() == 4);
    CASE_EXPECT_TRUE(!pv2.unique());
    CASE_EXPECT_TRUE(pv2.use_count() == py.use_count());
    CASE_EXPECT_TRUE(!(py < pv2 || pv2 < py));  // shared ownership test
  }

  CASE_EXPECT_TRUE(X::instances == 0);
  CASE_EXPECT_TRUE(Y::instances == 0);
}

void weak_ptr_constructor() {
#if defined(LIBATFRAME_UTILS_ENABLE_EXCEPTION) && LIBATFRAME_UTILS_ENABLE_EXCEPTION
  {
    util::memory::weak_rc_ptr<Y> wp;
    CASE_EXPECT_TRUE(wp.use_count() == 0);

    try {
      util::memory::strong_rc_ptr<Y> p2(wp);
      CASE_EXPECT_ERROR("shared_ptr<Y> p2(wp) failed to throw");
    } catch (std::bad_weak_ptr const &) {
    }

    try {
      util::memory::strong_rc_ptr<X> p3(wp);
      CASE_EXPECT_ERROR("shared_ptr<X> p3(wp) failed to throw");
    } catch (std::bad_weak_ptr const &) {
    }
  }
#endif

  {
    util::memory::strong_rc_ptr<Y> p;
    util::memory::weak_rc_ptr<Y> wp(p);

    // 0 allowed but not required
    if (wp.use_count() != 0) {
      util::memory::strong_rc_ptr<Y> p2(wp);
      CASE_EXPECT_TRUE(p2.use_count() == wp.use_count());
      CASE_EXPECT_TRUE(p2.get() == 0);

      util::memory::strong_rc_ptr<X> p3(wp);
      CASE_EXPECT_TRUE(p3.use_count() == wp.use_count());
      CASE_EXPECT_TRUE(p3.get() == 0);
    }
  }

  {
    util::memory::strong_rc_ptr<Y> p(new Y);
    util::memory::weak_rc_ptr<Y> wp(p);

    {
      util::memory::strong_rc_ptr<Y> p2(wp);
      CASE_EXPECT_TRUE(p2 ? true : false);
      CASE_EXPECT_TRUE(!!p2);
      CASE_EXPECT_TRUE(p2.get() == p.get());
      CASE_EXPECT_TRUE(p2.use_count() == 2);
      CASE_EXPECT_TRUE(!p2.unique());
      CASE_EXPECT_TRUE(p2.use_count() == wp.use_count());

      CASE_EXPECT_TRUE(p.use_count() == p2.use_count());
      CASE_EXPECT_TRUE(!(p < p2 || p2 < p));  // shared ownership test

      util::memory::strong_rc_ptr<X> p3(wp);
      CASE_EXPECT_TRUE(p3 ? true : false);
      CASE_EXPECT_TRUE(!!p3);
      CASE_EXPECT_TRUE(p3.get() == p.get());
      CASE_EXPECT_TRUE(p3.use_count() == 3);
      CASE_EXPECT_TRUE(!p3.unique());
      CASE_EXPECT_TRUE(p3.use_count() == wp.use_count());

      CASE_EXPECT_TRUE(p.use_count() == p3.use_count());
    }

    p.reset();
    CASE_EXPECT_TRUE(wp.use_count() == 0);

#if defined(LIBATFRAME_UTILS_ENABLE_EXCEPTION) && LIBATFRAME_UTILS_ENABLE_EXCEPTION
    try {
      util::memory::strong_rc_ptr<Y> p2(wp);
      CASE_EXPECT_ERROR("shared_ptr<Y> p2(wp) failed to throw");
    } catch (std::bad_weak_ptr const &) {
    }

    try {
      util::memory::strong_rc_ptr<X> p3(wp);
      CASE_EXPECT_ERROR("shared_ptr<X> p3(wp) failed to throw");
    } catch (std::bad_weak_ptr const &) {
    }
#endif
  }
}

void test() {
  default_constructor();
  pointer_constructor();
  deleter_constructor();
  copy_constructor();
  weak_ptr_constructor();
}

}  // namespace n_constructors

namespace n_assignment {

class incomplete;

struct A {
  int dummy;
};

struct X {
  static int64_t instances;

  X() { ++instances; }

  ~X() { --instances; }

 private:
  X(X const &);
  X &operator=(X const &);
};

int64_t X::instances = 0;

struct Y : public A, public virtual X {
  static int64_t instances;

  Y() { ++instances; }

  ~Y() { --instances; }

 private:
  Y(Y const &);
  Y &operator=(Y const &);
};

int64_t Y::instances = 0;

void copy_assignment() {
  {
    util::memory::strong_rc_ptr<incomplete> p1;

    p1 = p1;

    CASE_EXPECT_TRUE(p1 == p1);
    CASE_EXPECT_TRUE(p1 ? false : true);
    CASE_EXPECT_TRUE(!p1);
    CASE_EXPECT_TRUE(p1.get() == 0);

    util::memory::strong_rc_ptr<incomplete> p2;

    p1 = p2;

    CASE_EXPECT_TRUE(p1 == p2);
    CASE_EXPECT_TRUE(p1 ? false : true);
    CASE_EXPECT_TRUE(!p1);
    CASE_EXPECT_TRUE(p1.get() == 0);

    util::memory::strong_rc_ptr<incomplete> p3(p1);

    p1 = p3;

    CASE_EXPECT_TRUE(p1 == p3);
    CASE_EXPECT_TRUE(p1 ? false : true);
    CASE_EXPECT_TRUE(!p1);
    CASE_EXPECT_TRUE(p1.get() == 0);
  }

  {
    util::memory::strong_rc_ptr<void> p1;

    p1 = p1;

    CASE_EXPECT_TRUE(p1 == p1);
    CASE_EXPECT_TRUE(p1 ? false : true);
    CASE_EXPECT_TRUE(!p1);
    CASE_EXPECT_TRUE(p1.get() == 0);

    util::memory::strong_rc_ptr<void> p2;

    p1 = p2;

    CASE_EXPECT_TRUE(p1 == p2);
    CASE_EXPECT_TRUE(p1 ? false : true);
    CASE_EXPECT_TRUE(!p1);
    CASE_EXPECT_TRUE(p1.get() == 0);

    util::memory::strong_rc_ptr<void> p3(p1);

    p1 = p3;

    CASE_EXPECT_TRUE(p1 == p3);
    CASE_EXPECT_TRUE(p1 ? false : true);
    CASE_EXPECT_TRUE(!p1);
    CASE_EXPECT_TRUE(p1.get() == 0);

    util::memory::strong_rc_ptr<void> p4(new int);
    CASE_EXPECT_TRUE(p4.use_count() == 1);

    p1 = p4;

    CASE_EXPECT_TRUE(p1 == p4);
    CASE_EXPECT_TRUE(!(p1 < p4 || p4 < p1));
    CASE_EXPECT_TRUE(p1.use_count() == 2);
    CASE_EXPECT_TRUE(p4.use_count() == 2);

    p1 = p3;

    CASE_EXPECT_TRUE(p1 == p3);
    CASE_EXPECT_TRUE(p4.use_count() == 1);
  }

  {
    util::memory::strong_rc_ptr<X> p1;

    p1 = p1;

    CASE_EXPECT_TRUE(p1 == p1);
    CASE_EXPECT_TRUE(p1 ? false : true);
    CASE_EXPECT_TRUE(!p1);
    CASE_EXPECT_TRUE(p1.get() == 0);

    util::memory::strong_rc_ptr<X> p2;

    p1 = p2;

    CASE_EXPECT_TRUE(p1 == p2);
    CASE_EXPECT_TRUE(p1 ? false : true);
    CASE_EXPECT_TRUE(!p1);
    CASE_EXPECT_TRUE(p1.get() == 0);

    util::memory::strong_rc_ptr<X> p3(p1);

    p1 = p3;

    CASE_EXPECT_TRUE(p1 == p3);
    CASE_EXPECT_TRUE(p1 ? false : true);
    CASE_EXPECT_TRUE(!p1);
    CASE_EXPECT_TRUE(p1.get() == 0);

    CASE_EXPECT_TRUE(X::instances == 0);

    util::memory::strong_rc_ptr<X> p4(new X);

    CASE_EXPECT_TRUE(X::instances == 1);

    p1 = p4;

    CASE_EXPECT_TRUE(X::instances == 1);

    CASE_EXPECT_TRUE(p1 == p4);
    CASE_EXPECT_TRUE(!(p1 < p4 || p4 < p1));

    CASE_EXPECT_TRUE(p1.use_count() == 2);

    p1 = p2;

    CASE_EXPECT_TRUE(p1 == p2);
    CASE_EXPECT_TRUE(X::instances == 1);

    p4 = p3;

    CASE_EXPECT_TRUE(p4 == p3);
    CASE_EXPECT_TRUE(X::instances == 0);
  }
}

void conversion_assignment() {
  {
    util::memory::strong_rc_ptr<void> p1;

    util::memory::strong_rc_ptr<incomplete> p2;

    p1 = p2;

    CASE_EXPECT_TRUE(p1 == p2);
    CASE_EXPECT_TRUE(p1 ? false : true);
    CASE_EXPECT_TRUE(!p1);
    CASE_EXPECT_TRUE(p1.get() == 0);

    util::memory::strong_rc_ptr<int> p4(new int);
    CASE_EXPECT_TRUE(p4.use_count() == 1);

    util::memory::strong_rc_ptr<void> p5(p4);
    CASE_EXPECT_TRUE(p4.use_count() == 2);

    p1 = p4;

    CASE_EXPECT_TRUE(p1 == p4);
    CASE_EXPECT_TRUE(!(p1 < p5 || p5 < p1));
    CASE_EXPECT_TRUE(p1.use_count() == 3);
    CASE_EXPECT_TRUE(p4.use_count() == 3);

    p1 = p2;

    CASE_EXPECT_TRUE(p1 == p2);
    CASE_EXPECT_TRUE(p4.use_count() == 2);
  }

  {
    util::memory::strong_rc_ptr<X> p1;

    util::memory::strong_rc_ptr<Y> p2;

    p1 = p2;

    CASE_EXPECT_TRUE(p1 == p2);
    CASE_EXPECT_TRUE(p1 ? false : true);
    CASE_EXPECT_TRUE(!p1);
    CASE_EXPECT_TRUE(p1.get() == 0);

    CASE_EXPECT_TRUE(X::instances == 0);
    CASE_EXPECT_TRUE(Y::instances == 0);

    util::memory::strong_rc_ptr<Y> p4(new Y);

    CASE_EXPECT_TRUE(X::instances == 1);
    CASE_EXPECT_TRUE(Y::instances == 1);
    CASE_EXPECT_TRUE(p4.use_count() == 1);

    util::memory::strong_rc_ptr<X> p5(p4);
    CASE_EXPECT_TRUE(p4.use_count() == 2);

    p1 = p4;

    CASE_EXPECT_TRUE(X::instances == 1);
    CASE_EXPECT_TRUE(Y::instances == 1);

    CASE_EXPECT_TRUE(p1 == p4);
    CASE_EXPECT_TRUE(!(p1 < p5 || p5 < p1));

    CASE_EXPECT_TRUE(p1.use_count() == 3);
    CASE_EXPECT_TRUE(p4.use_count() == 3);

    p1 = p2;

    CASE_EXPECT_TRUE(p1 == p2);
    CASE_EXPECT_TRUE(X::instances == 1);
    CASE_EXPECT_TRUE(Y::instances == 1);
    CASE_EXPECT_TRUE(p4.use_count() == 2);

    p4 = p2;
    p5 = p2;

    CASE_EXPECT_TRUE(p4 == p2);
    CASE_EXPECT_TRUE(X::instances == 0);
    CASE_EXPECT_TRUE(Y::instances == 0);
  }
}

void test() {
  copy_assignment();
  conversion_assignment();
}

}  // namespace n_assignment

namespace n_reset {

class incomplete;

incomplete *p0 = 0;

void deleter(incomplete *) {}

struct X {
  static int64_t instances;

  X() { ++instances; }

  ~X() { --instances; }

 private:
  X(X const &);
  X &operator=(X const &);
};

int64_t X::instances = 0;

void plain_reset() {
  {
    util::memory::strong_rc_ptr<int> pi;
    pi.reset();
    CASE_EXPECT_TRUE(pi ? false : true);
    CASE_EXPECT_TRUE(!pi);
    CASE_EXPECT_TRUE(pi.get() == 0);
    CASE_EXPECT_TRUE(pi.use_count() == 0);
  }

  {
    util::memory::strong_rc_ptr<int> pi(static_cast<int *>(0));
    pi.reset();
    CASE_EXPECT_TRUE(pi ? false : true);
    CASE_EXPECT_TRUE(!pi);
    CASE_EXPECT_TRUE(pi.get() == 0);
    CASE_EXPECT_TRUE(pi.use_count() == 0);
  }

  {
    util::memory::strong_rc_ptr<int> pi(new int);
    pi.reset();
    CASE_EXPECT_TRUE(pi ? false : true);
    CASE_EXPECT_TRUE(!pi);
    CASE_EXPECT_TRUE(pi.get() == 0);
    CASE_EXPECT_TRUE(pi.use_count() == 0);
  }

  {
    util::memory::strong_rc_ptr<incomplete> px;
    px.reset();
    CASE_EXPECT_TRUE(px ? false : true);
    CASE_EXPECT_TRUE(!px);
    CASE_EXPECT_TRUE(px.get() == 0);
    CASE_EXPECT_TRUE(px.use_count() == 0);
  }

  {
    util::memory::strong_rc_ptr<incomplete> px(p0, deleter);
    px.reset();
    CASE_EXPECT_TRUE(px ? false : true);
    CASE_EXPECT_TRUE(!px);
    CASE_EXPECT_TRUE(px.get() == 0);
    CASE_EXPECT_TRUE(px.use_count() == 0);
  }

  {
    util::memory::strong_rc_ptr<X> px;
    px.reset();
    CASE_EXPECT_TRUE(px ? false : true);
    CASE_EXPECT_TRUE(!px);
    CASE_EXPECT_TRUE(px.get() == 0);
    CASE_EXPECT_TRUE(px.use_count() == 0);
  }

  {
    CASE_EXPECT_TRUE(X::instances == 0);
    util::memory::strong_rc_ptr<X> px(new X);
    CASE_EXPECT_TRUE(X::instances == 1);
    px.reset();
    CASE_EXPECT_TRUE(px ? false : true);
    CASE_EXPECT_TRUE(!px);
    CASE_EXPECT_TRUE(px.get() == 0);
    CASE_EXPECT_TRUE(px.use_count() == 0);
    CASE_EXPECT_TRUE(X::instances == 0);
  }

  {
    util::memory::strong_rc_ptr<void> pv;
    pv.reset();
    CASE_EXPECT_TRUE(pv ? false : true);
    CASE_EXPECT_TRUE(!pv);
    CASE_EXPECT_TRUE(pv.get() == 0);
    CASE_EXPECT_TRUE(pv.use_count() == 0);
  }

  {
    CASE_EXPECT_TRUE(X::instances == 0);
    util::memory::strong_rc_ptr<void> pv(new X);
    CASE_EXPECT_TRUE(X::instances == 1);
    pv.reset();
    CASE_EXPECT_TRUE(pv ? false : true);
    CASE_EXPECT_TRUE(!pv);
    CASE_EXPECT_TRUE(pv.get() == 0);
    CASE_EXPECT_TRUE(pv.use_count() == 0);
    CASE_EXPECT_TRUE(X::instances == 0);
  }
}

struct A {
  int dummy;
};

struct Y : public A, public virtual X {
  static int64_t instances;

  Y() { ++instances; }

  ~Y() { --instances; }

 private:
  Y(Y const &);
  Y &operator=(Y const &);
};

int64_t Y::instances = 0;

void pointer_reset() {
  {
    util::memory::strong_rc_ptr<int> pi;
    CASE_EXPECT_TRUE(pi.use_count() == 0);

    pi.reset(static_cast<int *>(nullptr));
    CASE_EXPECT_TRUE(pi ? false : true);
    CASE_EXPECT_TRUE(!pi);
    CASE_EXPECT_TRUE(pi.get() == 0);
    CASE_EXPECT_TRUE(pi.use_count() == 1);
    CASE_EXPECT_TRUE(pi.unique());

    int *p = new int;
    pi.reset(p);
    CASE_EXPECT_TRUE(pi ? true : false);
    CASE_EXPECT_TRUE(!!pi);
    CASE_EXPECT_TRUE(pi.get() == p);
    CASE_EXPECT_TRUE(pi.use_count() == 1);
    CASE_EXPECT_TRUE(pi.unique());

    pi.reset(static_cast<int *>(0));
    CASE_EXPECT_TRUE(pi ? false : true);
    CASE_EXPECT_TRUE(!pi);
    CASE_EXPECT_TRUE(pi.get() == 0);
    CASE_EXPECT_TRUE(pi.use_count() == 1);
    CASE_EXPECT_TRUE(pi.unique());
  }

  {
    util::memory::strong_rc_ptr<X> px;

    px.reset(static_cast<X *>(0));
    CASE_EXPECT_TRUE(px ? false : true);
    CASE_EXPECT_TRUE(!px);
    CASE_EXPECT_TRUE(px.get() == 0);
    CASE_EXPECT_TRUE(px.use_count() == 1);
    CASE_EXPECT_TRUE(px.unique());
    CASE_EXPECT_TRUE(X::instances == 0);

    X *p = new X;
    px.reset(p);
    CASE_EXPECT_TRUE(px ? true : false);
    CASE_EXPECT_TRUE(!!px);
    CASE_EXPECT_TRUE(px.get() == p);
    CASE_EXPECT_TRUE(px.use_count() == 1);
    CASE_EXPECT_TRUE(px.unique());
    CASE_EXPECT_TRUE(X::instances == 1);

    px.reset(static_cast<X *>(0));
    CASE_EXPECT_TRUE(px ? false : true);
    CASE_EXPECT_TRUE(!px);
    CASE_EXPECT_TRUE(px.get() == 0);
    CASE_EXPECT_TRUE(px.use_count() == 1);
    CASE_EXPECT_TRUE(px.unique());
    CASE_EXPECT_TRUE(X::instances == 0);
    CASE_EXPECT_TRUE(Y::instances == 0);

    Y *q = new Y;
    px.reset(q);
    CASE_EXPECT_TRUE(px ? true : false);
    CASE_EXPECT_TRUE(!!px);
    CASE_EXPECT_TRUE(px.get() == q);
    CASE_EXPECT_TRUE(px.use_count() == 1);
    CASE_EXPECT_TRUE(px.unique());
    CASE_EXPECT_TRUE(X::instances == 1);
    CASE_EXPECT_TRUE(Y::instances == 1);

    px.reset(static_cast<Y *>(0));
    CASE_EXPECT_TRUE(px ? false : true);
    CASE_EXPECT_TRUE(!px);
    CASE_EXPECT_TRUE(px.get() == 0);
    CASE_EXPECT_TRUE(px.use_count() == 1);
    CASE_EXPECT_TRUE(px.unique());
    CASE_EXPECT_TRUE(X::instances == 0);
    CASE_EXPECT_TRUE(Y::instances == 0);
  }

  {
    util::memory::strong_rc_ptr<void> pv;

    pv.reset(static_cast<X *>(0));
    CASE_EXPECT_TRUE(pv ? false : true);
    CASE_EXPECT_TRUE(!pv);
    CASE_EXPECT_TRUE(pv.get() == 0);
    CASE_EXPECT_TRUE(pv.use_count() == 1);
    CASE_EXPECT_TRUE(pv.unique());
    CASE_EXPECT_TRUE(X::instances == 0);

    X *p = new X;
    pv.reset(p);
    CASE_EXPECT_TRUE(pv ? true : false);
    CASE_EXPECT_TRUE(!!pv);
    CASE_EXPECT_TRUE(pv.get() == p);
    CASE_EXPECT_TRUE(pv.use_count() == 1);
    CASE_EXPECT_TRUE(pv.unique());
    CASE_EXPECT_TRUE(X::instances == 1);

    pv.reset(static_cast<X *>(0));
    CASE_EXPECT_TRUE(pv ? false : true);
    CASE_EXPECT_TRUE(!pv);
    CASE_EXPECT_TRUE(pv.get() == 0);
    CASE_EXPECT_TRUE(pv.use_count() == 1);
    CASE_EXPECT_TRUE(pv.unique());
    CASE_EXPECT_TRUE(X::instances == 0);
    CASE_EXPECT_TRUE(Y::instances == 0);

    Y *q = new Y;
    pv.reset(q);
    CASE_EXPECT_TRUE(pv ? true : false);
    CASE_EXPECT_TRUE(!!pv);
    CASE_EXPECT_TRUE(pv.get() == q);
    CASE_EXPECT_TRUE(pv.use_count() == 1);
    CASE_EXPECT_TRUE(pv.unique());
    CASE_EXPECT_TRUE(X::instances == 1);
    CASE_EXPECT_TRUE(Y::instances == 1);

    pv.reset(static_cast<Y *>(0));
    CASE_EXPECT_TRUE(pv ? false : true);
    CASE_EXPECT_TRUE(!pv);
    CASE_EXPECT_TRUE(pv.get() == 0);
    CASE_EXPECT_TRUE(pv.use_count() == 1);
    CASE_EXPECT_TRUE(pv.unique());
    CASE_EXPECT_TRUE(X::instances == 0);
    CASE_EXPECT_TRUE(Y::instances == 0);
  }
}

void *deleted = 0;

void deleter2(void *p) { deleted = p; }

void deleter_reset() {
  {
    util::memory::strong_rc_ptr<int> pi;

    pi.reset(static_cast<int *>(0), deleter2);
    CASE_EXPECT_TRUE(pi ? false : true);
    CASE_EXPECT_TRUE(!pi);
    CASE_EXPECT_TRUE(pi.get() == 0);
    CASE_EXPECT_TRUE(pi.use_count() == 1);
    CASE_EXPECT_TRUE(pi.unique());

    deleted = &pi;

    int m = 0;
    pi.reset(&m, deleter2);
    CASE_EXPECT_TRUE(deleted == 0);
    CASE_EXPECT_TRUE(pi ? true : false);
    CASE_EXPECT_TRUE(!!pi);
    CASE_EXPECT_TRUE(pi.get() == &m);
    CASE_EXPECT_TRUE(pi.use_count() == 1);
    CASE_EXPECT_TRUE(pi.unique());

    pi.reset(static_cast<int *>(0), deleter2);
    CASE_EXPECT_TRUE(deleted == &m);
    CASE_EXPECT_TRUE(pi ? false : true);
    CASE_EXPECT_TRUE(!pi);
    CASE_EXPECT_TRUE(pi.get() == 0);
    CASE_EXPECT_TRUE(pi.use_count() == 1);
    CASE_EXPECT_TRUE(pi.unique());

    pi.reset();
    CASE_EXPECT_TRUE(deleted == 0);
  }

  {
    util::memory::strong_rc_ptr<X> px;

    px.reset(static_cast<X *>(0), deleter2);
    CASE_EXPECT_TRUE(px ? false : true);
    CASE_EXPECT_TRUE(!px);
    CASE_EXPECT_TRUE(px.get() == 0);
    CASE_EXPECT_TRUE(px.use_count() == 1);
    CASE_EXPECT_TRUE(px.unique());

    deleted = &px;

    X x;
    px.reset(&x, deleter2);
    CASE_EXPECT_TRUE(deleted == 0);
    CASE_EXPECT_TRUE(px ? true : false);
    CASE_EXPECT_TRUE(!!px);
    CASE_EXPECT_TRUE(px.get() == &x);
    CASE_EXPECT_TRUE(px.use_count() == 1);
    CASE_EXPECT_TRUE(px.unique());

    px.reset(static_cast<X *>(0), deleter2);
    CASE_EXPECT_TRUE(deleted == &x);
    CASE_EXPECT_TRUE(px ? false : true);
    CASE_EXPECT_TRUE(!px);
    CASE_EXPECT_TRUE(px.get() == 0);
    CASE_EXPECT_TRUE(px.use_count() == 1);
    CASE_EXPECT_TRUE(px.unique());

    Y y;
    px.reset(&y, deleter2);
    CASE_EXPECT_TRUE(deleted == 0);
    CASE_EXPECT_TRUE(px ? true : false);
    CASE_EXPECT_TRUE(!!px);
    CASE_EXPECT_TRUE(px.get() == &y);
    CASE_EXPECT_TRUE(px.use_count() == 1);
    CASE_EXPECT_TRUE(px.unique());

    px.reset(static_cast<Y *>(0), deleter2);
    CASE_EXPECT_TRUE(deleted == &y);
    CASE_EXPECT_TRUE(px ? false : true);
    CASE_EXPECT_TRUE(!px);
    CASE_EXPECT_TRUE(px.get() == 0);
    CASE_EXPECT_TRUE(px.use_count() == 1);
    CASE_EXPECT_TRUE(px.unique());

    px.reset();
    CASE_EXPECT_TRUE(deleted == 0);
  }

  {
    util::memory::strong_rc_ptr<void> pv;

    pv.reset(static_cast<X *>(0), deleter2);
    CASE_EXPECT_TRUE(pv ? false : true);
    CASE_EXPECT_TRUE(!pv);
    CASE_EXPECT_TRUE(pv.get() == 0);
    CASE_EXPECT_TRUE(pv.use_count() == 1);
    CASE_EXPECT_TRUE(pv.unique());

    deleted = &pv;

    X x;
    pv.reset(&x, deleter2);
    CASE_EXPECT_TRUE(deleted == 0);
    CASE_EXPECT_TRUE(pv ? true : false);
    CASE_EXPECT_TRUE(!!pv);
    CASE_EXPECT_TRUE(pv.get() == &x);
    CASE_EXPECT_TRUE(pv.use_count() == 1);
    CASE_EXPECT_TRUE(pv.unique());

    pv.reset(static_cast<X *>(0), deleter2);
    CASE_EXPECT_TRUE(deleted == &x);
    CASE_EXPECT_TRUE(pv ? false : true);
    CASE_EXPECT_TRUE(!pv);
    CASE_EXPECT_TRUE(pv.get() == 0);
    CASE_EXPECT_TRUE(pv.use_count() == 1);
    CASE_EXPECT_TRUE(pv.unique());

    Y y;
    pv.reset(&y, deleter2);
    CASE_EXPECT_TRUE(deleted == 0);
    CASE_EXPECT_TRUE(pv ? true : false);
    CASE_EXPECT_TRUE(!!pv);
    CASE_EXPECT_TRUE(pv.get() == &y);
    CASE_EXPECT_TRUE(pv.use_count() == 1);
    CASE_EXPECT_TRUE(pv.unique());

    pv.reset(static_cast<Y *>(0), deleter2);
    CASE_EXPECT_TRUE(deleted == &y);
    CASE_EXPECT_TRUE(pv ? false : true);
    CASE_EXPECT_TRUE(!pv);
    CASE_EXPECT_TRUE(pv.get() == 0);
    CASE_EXPECT_TRUE(pv.use_count() == 1);
    CASE_EXPECT_TRUE(pv.unique());

    pv.reset();
    CASE_EXPECT_TRUE(deleted == 0);
  }

  {
    util::memory::strong_rc_ptr<incomplete> px;

    px.reset(p0, deleter2);
    CASE_EXPECT_TRUE(px ? false : true);
    CASE_EXPECT_TRUE(!px);
    CASE_EXPECT_TRUE(px.get() == 0);
    CASE_EXPECT_TRUE(px.use_count() == 1);
    CASE_EXPECT_TRUE(px.unique());

    deleted = &px;
    px.reset(p0, deleter2);
    CASE_EXPECT_TRUE(deleted == 0);
  }
}

void test() {
  plain_reset();
  pointer_reset();
  deleter_reset();
}

}  // namespace n_reset

namespace n_access {

struct X {};

void test() {
  {
    util::memory::strong_rc_ptr<X> px;
    CASE_EXPECT_TRUE(px.get() == 0);
    CASE_EXPECT_TRUE(px ? false : true);
    CASE_EXPECT_TRUE(!px);
  }

  {
    util::memory::strong_rc_ptr<X> px(static_cast<X *>(0));
    CASE_EXPECT_TRUE(px.get() == 0);
    CASE_EXPECT_TRUE(px ? false : true);
    CASE_EXPECT_TRUE(!px);
  }

  {
    util::memory::strong_rc_ptr<X> px(static_cast<X *>(0), checked_deleter<X>());
    CASE_EXPECT_TRUE(px.get() == 0);
    CASE_EXPECT_TRUE(px ? false : true);
    CASE_EXPECT_TRUE(!px);
  }

  {
    X *p = new X;
    util::memory::strong_rc_ptr<X> px(p);
    CASE_EXPECT_TRUE(px.get() == p);
    CASE_EXPECT_TRUE(px ? true : false);
    CASE_EXPECT_TRUE(!!px);
    CASE_EXPECT_TRUE(&*px == px.get());
    CASE_EXPECT_TRUE(px.operator->() == px.get());
  }

  {
    X *p = new X;
    util::memory::strong_rc_ptr<X> px(p, checked_deleter<X>());
    CASE_EXPECT_TRUE(px.get() == p);
    CASE_EXPECT_TRUE(px ? true : false);
    CASE_EXPECT_TRUE(!!px);
    CASE_EXPECT_TRUE(&*px == px.get());
    CASE_EXPECT_TRUE(px.operator->() == px.get());
  }
}

}  // namespace n_access

namespace n_use_count {

struct X {};

void test() {
  {
    util::memory::strong_rc_ptr<X> px(static_cast<X *>(0));
    CASE_EXPECT_TRUE(px.use_count() == 1);
    CASE_EXPECT_TRUE(px.unique());

    util::memory::strong_rc_ptr<X> px2(px);
    CASE_EXPECT_TRUE(px2.use_count() == 2);
    CASE_EXPECT_TRUE(!px2.unique());
    CASE_EXPECT_TRUE(px.use_count() == 2);
    CASE_EXPECT_TRUE(!px.unique());
  }

  {
    util::memory::strong_rc_ptr<X> px(new X);
    CASE_EXPECT_TRUE(px.use_count() == 1);
    CASE_EXPECT_TRUE(px.unique());

    util::memory::strong_rc_ptr<X> px2(px);
    CASE_EXPECT_TRUE(px2.use_count() == 2);
    CASE_EXPECT_TRUE(!px2.unique());
    CASE_EXPECT_TRUE(px.use_count() == 2);
    CASE_EXPECT_TRUE(!px.unique());
  }

  {
    util::memory::strong_rc_ptr<X> px(new X, checked_deleter<X>());
    CASE_EXPECT_TRUE(px.use_count() == 1);
    CASE_EXPECT_TRUE(px.unique());

    util::memory::strong_rc_ptr<X> px2(px);
    CASE_EXPECT_TRUE(px2.use_count() == 2);
    CASE_EXPECT_TRUE(!px2.unique());
    CASE_EXPECT_TRUE(px.use_count() == 2);
    CASE_EXPECT_TRUE(!px.unique());
  }
}

}  // namespace n_use_count

namespace n_swap {

struct X {};

void test() {
  {
    util::memory::strong_rc_ptr<X> px;
    util::memory::strong_rc_ptr<X> px2;

    px.swap(px2);

    CASE_EXPECT_TRUE(px.get() == 0);
    CASE_EXPECT_TRUE(px2.get() == 0);

    using std::swap;
    swap(px, px2);

    CASE_EXPECT_TRUE(px.get() == 0);
    CASE_EXPECT_TRUE(px2.get() == 0);
  }

  {
    X *p = new X;
    util::memory::strong_rc_ptr<X> px;
    util::memory::strong_rc_ptr<X> px2(p);
    util::memory::strong_rc_ptr<X> px3(px2);

    px.swap(px2);

    CASE_EXPECT_TRUE(px.get() == p);
    CASE_EXPECT_TRUE(px.use_count() == 2);
    CASE_EXPECT_TRUE(px2.get() == 0);
    CASE_EXPECT_TRUE(px3.get() == p);
    CASE_EXPECT_TRUE(px3.use_count() == 2);

    using std::swap;
    swap(px, px2);

    CASE_EXPECT_TRUE(px.get() == 0);
    CASE_EXPECT_TRUE(px2.get() == p);
    CASE_EXPECT_TRUE(px2.use_count() == 2);
    CASE_EXPECT_TRUE(px3.get() == p);
    CASE_EXPECT_TRUE(px3.use_count() == 2);
  }

  {
    X *p1 = new X;
    X *p2 = new X;
    util::memory::strong_rc_ptr<X> px(p1);
    util::memory::strong_rc_ptr<X> px2(p2);
    util::memory::strong_rc_ptr<X> px3(px2);

    px.swap(px2);

    CASE_EXPECT_TRUE(px.get() == p2);
    CASE_EXPECT_TRUE(px.use_count() == 2);
    CASE_EXPECT_TRUE(px2.get() == p1);
    CASE_EXPECT_TRUE(px2.use_count() == 1);
    CASE_EXPECT_TRUE(px3.get() == p2);
    CASE_EXPECT_TRUE(px3.use_count() == 2);

    using std::swap;
    swap(px, px2);

    CASE_EXPECT_TRUE(px.get() == p1);
    CASE_EXPECT_TRUE(px.use_count() == 1);
    CASE_EXPECT_TRUE(px2.get() == p2);
    CASE_EXPECT_TRUE(px2.use_count() == 2);
    CASE_EXPECT_TRUE(px3.get() == p2);
    CASE_EXPECT_TRUE(px3.use_count() == 2);
  }
}

}  // namespace n_swap

namespace n_comparison {

struct X {
  int dummy;
};

struct Y {
  int dummy2;
};

struct Z : public X, public virtual Y {};

void test() {
  {
    util::memory::strong_rc_ptr<X> px;
    CASE_EXPECT_TRUE(px == px);
    CASE_EXPECT_TRUE(!(px != px));
    CASE_EXPECT_TRUE(!(px < px));

    util::memory::strong_rc_ptr<X> px2;

    CASE_EXPECT_TRUE(px.get() == px2.get());
    CASE_EXPECT_TRUE(px == px2);
    CASE_EXPECT_TRUE(!(px != px2));
    CASE_EXPECT_TRUE(!(px < px2 && px2 < px));
  }

  {
    util::memory::strong_rc_ptr<X> px;
    util::memory::strong_rc_ptr<X> px2(px);

    CASE_EXPECT_TRUE(px2 == px2);
    CASE_EXPECT_TRUE(!(px2 != px2));
    CASE_EXPECT_TRUE(!(px2 < px2));

    CASE_EXPECT_TRUE(px.get() == px2.get());
    CASE_EXPECT_TRUE(px == px2);
    CASE_EXPECT_TRUE(!(px != px2));
    CASE_EXPECT_TRUE(!(px < px2 && px2 < px));
  }

  {
    util::memory::strong_rc_ptr<X> px;
    util::memory::strong_rc_ptr<X> px2(new X);

    CASE_EXPECT_TRUE(px2 == px2);
    CASE_EXPECT_TRUE(!(px2 != px2));
    CASE_EXPECT_TRUE(!(px2 < px2));

    CASE_EXPECT_TRUE(px.get() != px2.get());
    CASE_EXPECT_TRUE(px != px2);
    CASE_EXPECT_TRUE(!(px == px2));
    CASE_EXPECT_TRUE(px < px2 || px2 < px);
    CASE_EXPECT_TRUE(!(px < px2 && px2 < px));
  }

  {
    util::memory::strong_rc_ptr<X> px(new X);
    util::memory::strong_rc_ptr<X> px2(new X);

    CASE_EXPECT_TRUE(px.get() != px2.get());
    CASE_EXPECT_TRUE(px != px2);
    CASE_EXPECT_TRUE(!(px == px2));
    CASE_EXPECT_TRUE(px < px2 || px2 < px);
    CASE_EXPECT_TRUE(!(px < px2 && px2 < px));
  }

  {
    util::memory::strong_rc_ptr<X> px(new X);
    util::memory::strong_rc_ptr<X> px2(px);

    CASE_EXPECT_TRUE(px2 == px2);
    CASE_EXPECT_TRUE(!(px2 != px2));
    CASE_EXPECT_TRUE(!(px2 < px2));

    CASE_EXPECT_TRUE(px.get() == px2.get());
    CASE_EXPECT_TRUE(px == px2);
    CASE_EXPECT_TRUE(!(px != px2));
    CASE_EXPECT_TRUE(!(px < px2 || px2 < px));
  }

  {
    util::memory::strong_rc_ptr<X> px(new X);
    util::memory::strong_rc_ptr<Y> py(new Y);
    util::memory::strong_rc_ptr<Z> pz(new Z);

    CASE_EXPECT_TRUE(px.get() != pz.get());
    CASE_EXPECT_TRUE(px != pz);
    CASE_EXPECT_TRUE(!(px == pz));

    CASE_EXPECT_TRUE(py.get() != pz.get());
    CASE_EXPECT_TRUE(py != pz);
    CASE_EXPECT_TRUE(!(py == pz));

    CASE_EXPECT_TRUE(px < py || py < px);
    CASE_EXPECT_TRUE(px < pz || pz < px);
    CASE_EXPECT_TRUE(py < pz || pz < py);

    CASE_EXPECT_TRUE(!(px < py && py < px));
    CASE_EXPECT_TRUE(!(px < pz && pz < px));
    CASE_EXPECT_TRUE(!(py < pz && pz < py));

    util::memory::strong_rc_ptr<void> pvx(px);

    CASE_EXPECT_TRUE(pvx == pvx);
    CASE_EXPECT_TRUE(!(pvx != pvx));
    CASE_EXPECT_TRUE(!(pvx < pvx));

    util::memory::strong_rc_ptr<void> pvy(py);
    util::memory::strong_rc_ptr<void> pvz(pz);

    CASE_EXPECT_TRUE(pvx < pvy || pvy < pvx);
    CASE_EXPECT_TRUE(pvx < pvz || pvz < pvx);
    CASE_EXPECT_TRUE(pvy < pvz || pvz < pvy);

    CASE_EXPECT_TRUE(!(pvx < pvy && pvy < pvx));
    CASE_EXPECT_TRUE(!(pvx < pvz && pvz < pvx));
    CASE_EXPECT_TRUE(!(pvy < pvz && pvz < pvy));
  }

  {
    util::memory::strong_rc_ptr<Z> pz(new Z);
    util::memory::strong_rc_ptr<X> px(pz);

    CASE_EXPECT_TRUE(px == px);
    CASE_EXPECT_TRUE(!(px != px));
    CASE_EXPECT_TRUE(!(px < px));

    util::memory::strong_rc_ptr<Y> py(pz);

    CASE_EXPECT_TRUE(px.get() == pz.get());
    CASE_EXPECT_TRUE(px == pz);
    CASE_EXPECT_TRUE(!(px != pz));

    CASE_EXPECT_TRUE(py.get() == pz.get());
    CASE_EXPECT_TRUE(py == pz);
    CASE_EXPECT_TRUE(!(py != pz));

    CASE_EXPECT_TRUE(px < py || py < px);
    CASE_EXPECT_TRUE(!(px < pz || pz < px));
    CASE_EXPECT_TRUE(!(py < pz || pz < py));

    util::memory::strong_rc_ptr<void> pvx(px);
    util::memory::strong_rc_ptr<void> pvy(py);
    util::memory::strong_rc_ptr<void> pvz(pz);

    // pvx and pvy aren't equal...
    CASE_EXPECT_TRUE(pvx.get() != pvy.get());
    CASE_EXPECT_TRUE(pvx != pvy);
    CASE_EXPECT_TRUE(!(pvx == pvy));

    // ... but they share ownership ...
    CASE_EXPECT_TRUE(pvx < pvy || pvy < pvx);

    // ... with pvz
    // CASE_EXPECT_TRUE(!(pvx < pvz || pvz < pvx));
    CASE_EXPECT_TRUE(pvy < pvz || pvz < pvy);
  }
}

}  // namespace n_comparison

namespace n_static_cast {

struct X {};

struct Y : public X {};

void test() {
  {
    util::memory::strong_rc_ptr<void> pv;

    util::memory::strong_rc_ptr<int> pi = util::memory::static_pointer_cast<int>(pv);
    CASE_EXPECT_TRUE(pi.get() == 0);

    util::memory::strong_rc_ptr<X> px = util::memory::static_pointer_cast<X>(pv);
    CASE_EXPECT_TRUE(px.get() == 0);
  }

  {
    util::memory::strong_rc_ptr<int> pi(new int);
    util::memory::strong_rc_ptr<void> pv(pi);

    util::memory::strong_rc_ptr<int> pi2 = util::memory::static_pointer_cast<int>(pv);
    CASE_EXPECT_TRUE(pi.get() == pi2.get());
    CASE_EXPECT_TRUE(!(pi < pi2 || pi2 < pi));
    CASE_EXPECT_TRUE(pi.use_count() == 3);
    CASE_EXPECT_TRUE(pv.use_count() == 3);
    CASE_EXPECT_TRUE(pi2.use_count() == 3);
  }

  {
    util::memory::strong_rc_ptr<X> px(new X);
    util::memory::strong_rc_ptr<void> pv(px);

    util::memory::strong_rc_ptr<X> px2 = util::memory::static_pointer_cast<X>(pv);
    CASE_EXPECT_TRUE(px.get() == px2.get());
    CASE_EXPECT_TRUE(!(px < px2 || px2 < px));
    CASE_EXPECT_TRUE(px.use_count() == 3);
    CASE_EXPECT_TRUE(pv.use_count() == 3);
    CASE_EXPECT_TRUE(px2.use_count() == 3);
  }

  {
    util::memory::strong_rc_ptr<X> px(new Y);

    util::memory::strong_rc_ptr<Y> py = util::memory::static_pointer_cast<Y>(px);
    CASE_EXPECT_TRUE(px.get() == py.get());
    CASE_EXPECT_TRUE(px.use_count() == 2);
    CASE_EXPECT_TRUE(py.use_count() == 2);

    util::memory::strong_rc_ptr<X> px2(py);
    CASE_EXPECT_TRUE(!(px < px2 || px2 < px));
  }
}

}  // namespace n_static_cast

namespace n_const_cast {

struct X;

void test() {
  {
    util::memory::strong_rc_ptr<void const volatile> px;

    util::memory::strong_rc_ptr<void> px2 = util::memory::const_pointer_cast<void>(px);
    CASE_EXPECT_TRUE(px2.get() == 0);
  }

  {
    util::memory::strong_rc_ptr<int const volatile> px;

    util::memory::strong_rc_ptr<int> px2 = util::memory::const_pointer_cast<int>(px);
    CASE_EXPECT_TRUE(px2.get() == 0);
  }

  {
    util::memory::strong_rc_ptr<X const volatile> px;

    util::memory::strong_rc_ptr<X> px2 = util::memory::const_pointer_cast<X>(px);
    CASE_EXPECT_TRUE(px2.get() == 0);
  }

  {
    util::memory::strong_rc_ptr<void const volatile> px(new int);

    util::memory::strong_rc_ptr<void> px2 = util::memory::const_pointer_cast<void>(px);
    CASE_EXPECT_TRUE(px.get() == px2.get());
    CASE_EXPECT_TRUE(!(px < px2 || px2 < px));
    CASE_EXPECT_TRUE(px.use_count() == 2);
    CASE_EXPECT_TRUE(px2.use_count() == 2);
  }

  {
    util::memory::strong_rc_ptr<int const volatile> px(new int);

    util::memory::strong_rc_ptr<int> px2 = util::memory::const_pointer_cast<int>(px);
    CASE_EXPECT_TRUE(px.get() == px2.get());
    CASE_EXPECT_TRUE(!(px < px2 || px2 < px));
    CASE_EXPECT_TRUE(px.use_count() == 2);
    CASE_EXPECT_TRUE(px2.use_count() == 2);
  }
}

}  // namespace n_const_cast

#if defined(LIBATFRAME_UTILS_ENABLE_RTTI) && LIBATFRAME_UTILS_ENABLE_RTTI

namespace n_dynamic_cast {

struct V {
  virtual ~V() {}
};

struct W : public V {};

void test() {
  {
    util::memory::strong_rc_ptr<V> pv;
    util::memory::strong_rc_ptr<W> pw = util::memory::dynamic_pointer_cast<W>(pv);
    CASE_EXPECT_TRUE(pw.get() == 0);
  }

  {
    util::memory::strong_rc_ptr<V> pv(static_cast<V *>(0));

    util::memory::strong_rc_ptr<W> pw = util::memory::dynamic_pointer_cast<W>(pv);
    CASE_EXPECT_TRUE(pw.get() == 0);

    util::memory::strong_rc_ptr<V> pv2(pw);
    CASE_EXPECT_TRUE(!(pv < pv2 || pv2 < pv));
  }

  {
    util::memory::strong_rc_ptr<V> pv(static_cast<W *>(0));

    util::memory::strong_rc_ptr<W> pw = util::memory::dynamic_pointer_cast<W>(pv);
    CASE_EXPECT_TRUE(pw.get() == 0);

    util::memory::strong_rc_ptr<V> pv2(pw);
    CASE_EXPECT_TRUE(!(pv < pv2 || pv2 < pv));
  }

  {
    util::memory::strong_rc_ptr<V> pv(new V);

    util::memory::strong_rc_ptr<W> pw = util::memory::dynamic_pointer_cast<W>(pv);
    CASE_EXPECT_TRUE(pw.get() == 0);

    util::memory::strong_rc_ptr<V> pv2(pw);
    CASE_EXPECT_TRUE(pv < pv2 || pv2 < pv);
  }

  {
    util::memory::strong_rc_ptr<V> pv(new W);

    util::memory::strong_rc_ptr<W> pw = util::memory::dynamic_pointer_cast<W>(pv);
    CASE_EXPECT_TRUE(pw.get() == pv.get());
    CASE_EXPECT_TRUE(pv.use_count() == 2);
    CASE_EXPECT_TRUE(pw.use_count() == 2);

    util::memory::strong_rc_ptr<V> pv2(pw);
    CASE_EXPECT_TRUE(!(pv < pv2 || pv2 < pv));
  }
}

}  // namespace n_dynamic_cast

#endif

namespace n_map {

struct X {};

void test() {
  std::vector<util::memory::strong_rc_ptr<int> > vi;

  {
    util::memory::strong_rc_ptr<int> pi1(new int);
    util::memory::strong_rc_ptr<int> pi2(new int);
    util::memory::strong_rc_ptr<int> pi3(new int);

    vi.push_back(pi1);
    vi.push_back(pi1);
    vi.push_back(pi1);
    vi.push_back(pi2);
    vi.push_back(pi1);
    vi.push_back(pi2);
    vi.push_back(pi1);
    vi.push_back(pi3);
    vi.push_back(pi3);
    vi.push_back(pi2);
    vi.push_back(pi1);
  }

  std::vector<util::memory::strong_rc_ptr<X> > vx;

  {
    util::memory::strong_rc_ptr<X> px1(new X);
    util::memory::strong_rc_ptr<X> px2(new X);
    util::memory::strong_rc_ptr<X> px3(new X);

    vx.push_back(px2);
    vx.push_back(px2);
    vx.push_back(px1);
    vx.push_back(px2);
    vx.push_back(px1);
    vx.push_back(px1);
    vx.push_back(px1);
    vx.push_back(px2);
    vx.push_back(px1);
    vx.push_back(px3);
    vx.push_back(px2);
  }

  std::map<util::memory::strong_rc_ptr<void>, int64_t> m;

  {
    for (std::vector<util::memory::strong_rc_ptr<int> >::iterator i = vi.begin(); i != vi.end(); ++i) {
      ++m[*i];
    }
  }

  {
    for (std::vector<util::memory::strong_rc_ptr<X> >::iterator i = vx.begin(); i != vx.end(); ++i) {
      ++m[*i];
    }
  }

  {
    for (std::map<util::memory::strong_rc_ptr<void>, int64_t>::iterator i = m.begin(); i != m.end(); ++i) {
      CASE_EXPECT_TRUE(i->first.use_count() == static_cast<std::size_t>(i->second + 1));
    }
  }
}

}  // namespace n_map

namespace n_transitive {

struct X {
  X() : next() {}
  util::memory::strong_rc_ptr<X> next;
};

void test() {
  util::memory::strong_rc_ptr<X> p(new X);
  p->next = util::memory::strong_rc_ptr<X>(new X);
  CASE_EXPECT_TRUE(!p->next->next);
  p = p->next;
  CASE_EXPECT_TRUE(!p->next);
}

}  // namespace n_transitive

namespace n_report_1 {

class foo {
 public:
  foo() : m_self(this) {}

  void suicide() { m_self.reset(); }

 private:
  util::memory::strong_rc_ptr<foo> m_self;
};

void test() {
  foo *foo_ptr = new foo;
  foo_ptr->suicide();
}

}  // namespace n_report_1

// Test case by Per Kristensen
namespace n_report_2 {

class foo {
 public:
  void setWeak(util::memory::strong_rc_ptr<foo> s) { w = s; }

 private:
  util::memory::weak_rc_ptr<foo> w;
};

class deleter {
 public:
  deleter() : lock(0) {}

  ~deleter() { CASE_EXPECT_TRUE(lock == 0); }

  void operator()(foo *p) {
    ++lock;
    delete p;
    --lock;
  }

 private:
  int lock;
};

void test() {
  util::memory::strong_rc_ptr<foo> s(new foo, deleter());
  s->setWeak(s);
  s.reset();
}

}  // namespace n_report_2

namespace n_spt_incomplete {

class file;

util::memory::strong_rc_ptr<file> fopen(char const *name, char const *mode);
void fread(util::memory::strong_rc_ptr<file> f, void *data, int64_t size);

int file_instances = 0;

void test() {
  CASE_EXPECT_TRUE(file_instances == 0);

  {
    util::memory::strong_rc_ptr<file> pf = fopen("name", "mode");
    CASE_EXPECT_TRUE(file_instances == 1);
    fread(pf, 0, 17041);
  }

  CASE_EXPECT_TRUE(file_instances == 0);
}

}  // namespace n_spt_incomplete

namespace n_spt_pimpl {

class file {
 private:
  class impl;
  util::memory::strong_rc_ptr<impl> pimpl_;

 public:
  file(char const *name, char const *mode);

  // compiler generated members are fine and useful

  void read(void *data, int64_t size);

  int64_t total_size() const;
};

int file_instances = 0;

void test() {
  CASE_EXPECT_TRUE(file_instances == 0);

  {
    file f("name", "mode");
    CASE_EXPECT_TRUE(file_instances == 1);
    f.read(0, 152);

    file f2(f);
    CASE_EXPECT_TRUE(file_instances == 1);
    f2.read(0, 894);

    CASE_EXPECT_TRUE(f.total_size() == 152 + 894);

    {
      file f3("name2", "mode2");
      CASE_EXPECT_TRUE(file_instances == 2);
    }

    CASE_EXPECT_TRUE(file_instances == 1);
  }

  CASE_EXPECT_TRUE(file_instances == 0);
}

}  // namespace n_spt_pimpl

namespace n_spt_abstract {

class X {
 public:
  virtual void f(int) = 0;
  virtual int g() = 0;

 protected:
  virtual ~X() {}
};

util::memory::strong_rc_ptr<X> createX();

int X_instances = 0;

void test() {
  CASE_EXPECT_TRUE(X_instances == 0);

  {
    util::memory::strong_rc_ptr<X> px = createX();

    CASE_EXPECT_TRUE(X_instances == 1);

    px->f(18);
    px->f(152);

    CASE_EXPECT_TRUE(px->g() == 170);
  }

  CASE_EXPECT_TRUE(X_instances == 0);
}

}  // namespace n_spt_abstract

namespace n_spt_preventing_delete {

int X_instances = 0;

class X {
 private:
  X() { ++X_instances; }

  ~X() { --X_instances; }

  class deleter;
  friend class deleter;

  class deleter {
   public:
    void operator()(X *p) { delete p; }
  };

 public:
  static util::memory::strong_rc_ptr<X> create() {
    util::memory::strong_rc_ptr<X> px(new X, X::deleter());
    return px;
  }
};

void test() {
  CASE_EXPECT_TRUE(X_instances == 0);

  {
    util::memory::strong_rc_ptr<X> px = X::create();
    CASE_EXPECT_TRUE(X_instances == 1);
  }

  CASE_EXPECT_TRUE(X_instances == 0);
}

}  // namespace n_spt_preventing_delete

namespace n_spt_array {

int X_instances = 0;

struct X {
  X() { ++X_instances; }

  ~X() { --X_instances; }
};

void test() {
  CASE_EXPECT_TRUE(X_instances == 0);

  {
    util::memory::strong_rc_ptr<X> px(new X[4], checked_array_deleter<X>());
    CASE_EXPECT_TRUE(X_instances == 4);
  }

  CASE_EXPECT_TRUE(X_instances == 0);
}

}  // namespace n_spt_array

namespace n_spt_static {

class X {
 public:
  X() {}

 private:
  void operator delete(void *) {
    // Comeau 4.3.0.1 wants a definition
    CASE_EXPECT_ERROR("n_spt_static::X::operator delete() called.");
  }
};

struct null_deleter {
  void operator()(void const *) const {}
};

static X x;

void test() { util::memory::strong_rc_ptr<X> px(&x, null_deleter()); }

}  // namespace n_spt_static

namespace n_spt_intrusive {

int X_instances = 0;

struct X {
  int64_t count;

  X() : count(0) { ++X_instances; }

  ~X() { --X_instances; }
};

void intrusive_ptr_add_ref(X *p) { ++p->count; }

void intrusive_ptr_release(X *p) {
  if (--p->count == 0) delete p;
}

template <class T>
struct intrusive_deleter {
  void operator()(T *p) {
    if (p != 0) intrusive_ptr_release(p);
  }
};

util::memory::strong_rc_ptr<X> make_shared_from_intrusive(X *p) {
  if (p != 0) intrusive_ptr_add_ref(p);
  util::memory::strong_rc_ptr<X> px(p, intrusive_deleter<X>());
  return px;
}

void test() {
  CASE_EXPECT_TRUE(X_instances == 0);

  {
    X *p = new X;
    CASE_EXPECT_TRUE(X_instances == 1);
    CASE_EXPECT_TRUE(p->count == 0);
    util::memory::strong_rc_ptr<X> px = make_shared_from_intrusive(p);
    CASE_EXPECT_TRUE(px.get() == p);
    CASE_EXPECT_TRUE(p->count == 1);
    util::memory::strong_rc_ptr<X> px2(px);
    CASE_EXPECT_TRUE(px2.get() == p);
    CASE_EXPECT_TRUE(p->count == 1);
  }

  CASE_EXPECT_TRUE(X_instances == 0);
}

}  // namespace n_spt_intrusive

namespace n_spt_another_sp {

template <class T>
class another_ptr : private util::memory::strong_rc_ptr<T> {
 private:
  typedef util::memory::strong_rc_ptr<T> base_type;

 public:
  explicit another_ptr(T *p = 0) : base_type(p) {}

  void reset() { base_type::reset(); }

  T *get() const { return base_type::get(); }
};

class event_handler {
 public:
  virtual ~event_handler() {}
  virtual void begin() = 0;
  virtual void handle(int event) = 0;
  virtual void end() = 0;
};

int begin_called = 0;
int handle_called = 0;
int end_called = 0;

class event_handler_impl : public event_handler {
 public:
  virtual void begin() { ++begin_called; }

  virtual void handle(int event) { handle_called = event; }

  virtual void end() { ++end_called; }
};

another_ptr<event_handler> get_event_handler() {
  another_ptr<event_handler> p(new event_handler_impl);
  return p;
}

util::memory::strong_rc_ptr<event_handler> current_handler;

void install_event_handler(util::memory::strong_rc_ptr<event_handler> p) {
  p->begin();
  current_handler = p;
}

void handle_event(int event) { current_handler->handle(event); }

void remove_event_handler() {
  current_handler->end();
  current_handler.reset();
}

template <class P>
class smart_pointer_deleter {
 private:
  P p_;

 public:
  explicit smart_pointer_deleter(P const &p) : p_(p) {}

  void operator()(void const *) { p_.reset(); }
};

void test() {
  another_ptr<event_handler> p = get_event_handler();

  util::memory::strong_rc_ptr<event_handler> q(p.get(), smart_pointer_deleter<another_ptr<event_handler> >(p));

  p.reset();

  CASE_EXPECT_TRUE(begin_called == 0);

  install_event_handler(q);

  CASE_EXPECT_TRUE(begin_called == 1);

  CASE_EXPECT_TRUE(handle_called == 0);

  handle_event(17041);

  CASE_EXPECT_TRUE(handle_called == 17041);

  CASE_EXPECT_TRUE(end_called == 0);

  remove_event_handler();

  CASE_EXPECT_TRUE(end_called == 1);
}

}  // namespace n_spt_another_sp

namespace n_spt_shared_from_this {

class X {
 public:
  virtual void f() = 0;

 protected:
  virtual ~X() {}
};

class Y {
 public:
  virtual util::memory::strong_rc_ptr<X> getX() = 0;

 protected:
  virtual ~Y() {}
};

class impl : public X, public Y {
 private:
  util::memory::weak_rc_ptr<impl> weak_this;

  impl(impl const &);
  impl &operator=(impl const &);

  impl() {}

 public:
  static util::memory::strong_rc_ptr<impl> create() {
    util::memory::strong_rc_ptr<impl> pi(new impl);
    pi->weak_this = pi;
    return pi;
  }

  virtual void f() {}

  virtual util::memory::strong_rc_ptr<X> getX() {
    util::memory::strong_rc_ptr<X> px = weak_this.lock();
    return px;
  }
};

void test() {
  util::memory::strong_rc_ptr<Y> py = impl::create();
  CASE_EXPECT_TRUE(py.get() != 0);
  CASE_EXPECT_TRUE(py.use_count() == 1);

  util::memory::strong_rc_ptr<X> px = py->getX();
  CASE_EXPECT_TRUE(px.get() != 0);
  CASE_EXPECT_TRUE(py.use_count() == 2);

#if defined(LIBATFRAME_UTILS_ENABLE_RTTI) && LIBATFRAME_UTILS_ENABLE_RTTI
  util::memory::strong_rc_ptr<Y> py2 = util::memory::dynamic_pointer_cast<Y>(px);
  CASE_EXPECT_TRUE(py.get() == py2.get());
  CASE_EXPECT_TRUE(!(py < py2 || py2 < py));
  CASE_EXPECT_TRUE(py.use_count() == 3);
#endif
}

}  // namespace n_spt_shared_from_this

namespace n_spt_wrap {

void test() {}

}  // namespace n_spt_wrap

CASE_TEST(rc_ptr, strong_rc_basic) {
  n_element_type::test();
  n_constructors::test();
  n_assignment::test();
  n_reset::test();
  n_access::test();
  n_use_count::test();
  n_swap::test();
  n_comparison::test();
  n_static_cast::test();
  n_const_cast::test();
#if defined(LIBATFRAME_UTILS_ENABLE_RTTI) && LIBATFRAME_UTILS_ENABLE_RTTI
  n_dynamic_cast::test();
#endif

  n_map::test();

  n_transitive::test();
  n_report_1::test();
  n_report_2::test();

  n_spt_incomplete::test();
  n_spt_pimpl::test();
  n_spt_abstract::test();
  n_spt_preventing_delete::test();
  n_spt_array::test();
  n_spt_static::test();
  n_spt_intrusive::test();
  n_spt_another_sp::test();
  n_spt_shared_from_this::test();
  n_spt_wrap::test();
}

namespace n_spt_incomplete {

class file {
 public:
  file() : fread_called(false) { ++file_instances; }

  ~file() {
    CASE_EXPECT_TRUE(fread_called);
    --file_instances;
  }

  bool fread_called;
};

util::memory::strong_rc_ptr<file> fopen(char const *, char const *) {
  util::memory::strong_rc_ptr<file> pf(new file);
  return pf;
}

void fread(util::memory::strong_rc_ptr<file> pf, void *, int64_t) { pf->fread_called = true; }

}  // namespace n_spt_incomplete

namespace n_spt_pimpl {

class file::impl {
 private:
  impl(impl const &);
  impl &operator=(impl const &);

  int64_t total_size_;

 public:
  impl(char const *, char const *) : total_size_(0) { ++file_instances; }

  ~impl() { --file_instances; }

  void read(void *, int64_t size) { total_size_ += size; }

  int64_t total_size() const { return total_size_; }
};

file::file(char const *name, char const *mode) : pimpl_(new impl(name, mode)) {}

void file::read(void *data, int64_t size) { pimpl_->read(data, size); }

int64_t file::total_size() const { return pimpl_->total_size(); }

}  // namespace n_spt_pimpl

namespace n_spt_abstract {

class X_impl : public X {
 private:
  X_impl(X_impl const &);
  X_impl &operator=(X_impl const &);

  int n_;

 public:
  X_impl() : n_(0) { ++X_instances; }

  ~X_impl() { --X_instances; }

  virtual void f(int n) { n_ += n; }

  virtual int g() { return n_; }
};

util::memory::strong_rc_ptr<X> createX() {
  util::memory::strong_rc_ptr<X> px(new X_impl);
  return px;
}

}  // namespace n_spt_abstract
}  // namespace strong_rc

namespace strong_rc_fn {

static void f() {}

struct null_deleter {
  template <class Y>
  void operator()(Y *) {}
};

CASE_TEST(rc_ptr, strong_rc_fn) {
  util::memory::strong_rc_ptr<void()> pf(f, null_deleter());

  CASE_EXPECT_EQ(f, pf.get());

  util::memory::weak_rc_ptr<void()> wp(pf);

  CASE_EXPECT_EQ(wp.lock().get(), f);
  CASE_EXPECT_EQ(wp.use_count(), 1);

  pf.reset();

  CASE_EXPECT_EQ(wp.lock().get(), nullptr);
  CASE_EXPECT_EQ(wp.use_count(), 0);
}
}  // namespace strong_rc_fn

namespace strong_rc_move {

struct X {
  static int64_t instances;

  X() { ++instances; }

  ~X() { --instances; }

 private:
  X(X const &);
  X &operator=(X const &);
};

int64_t X::instances = 0;

CASE_TEST(rc_ptr, strong_rc_move) {
  CASE_EXPECT_TRUE(X::instances == 0);

  {
    util::memory::strong_rc_ptr<X> p(new X);
    CASE_EXPECT_TRUE(X::instances == 1);

    util::memory::strong_rc_ptr<X> p2(std::move(p));
    CASE_EXPECT_TRUE(X::instances == 1);
    CASE_EXPECT_TRUE(p.get() == 0);

    util::memory::strong_rc_ptr<void> p3(std::move(p2));
    CASE_EXPECT_TRUE(X::instances == 1);
    CASE_EXPECT_TRUE(p2.get() == 0);

    p3.reset();
    CASE_EXPECT_TRUE(X::instances == 0);
  }

  {
    util::memory::strong_rc_ptr<X> p(new X);
    CASE_EXPECT_TRUE(X::instances == 1);

    util::memory::strong_rc_ptr<X> p2;
    p2 = std::move(p);
    CASE_EXPECT_TRUE(X::instances == 1);
    CASE_EXPECT_TRUE(p.get() == 0);

    util::memory::strong_rc_ptr<void> p3;
    p3 = std::move(p2);
    CASE_EXPECT_TRUE(X::instances == 1);
    CASE_EXPECT_TRUE(p2.get() == 0);

    p3.reset();
    CASE_EXPECT_TRUE(X::instances == 0);
  }

  {
    util::memory::strong_rc_ptr<X> p(new X);
    CASE_EXPECT_TRUE(X::instances == 1);

    util::memory::strong_rc_ptr<X> p2(new X);
    CASE_EXPECT_TRUE(X::instances == 2);
    p2 = std::move(p);
    CASE_EXPECT_TRUE(X::instances == 1);
    CASE_EXPECT_TRUE(p.get() == 0);

    util::memory::strong_rc_ptr<void> p3(new X);
    CASE_EXPECT_TRUE(X::instances == 2);
    p3 = std::move(p2);
    CASE_EXPECT_TRUE(X::instances == 1);
    CASE_EXPECT_TRUE(p2.get() == 0);

    p3.reset();
    CASE_EXPECT_TRUE(X::instances == 0);
  }
}

}  // namespace strong_rc_move

namespace make_strong_rc {

class X {
 private:
  X(X const &);
  X &operator=(X const &);

  void *operator new(std::size_t n) {
    // lack of this definition causes link errors on Comeau C++
    CASE_EXPECT_ERROR("private X::new called");
    return ::operator new(n);
  }

  void operator delete(void *p) {
    // lack of this definition causes link errors on MSVC
    CASE_EXPECT_ERROR("private X::delete called");
    ::operator delete(p);
  }

 public:
  static int instances;

  int v;

  explicit X(int a1 = 0, int a2 = 0, int a3 = 0, int a4 = 0, int a5 = 0, int a6 = 0, int a7 = 0, int a8 = 0, int a9 = 0)
      : v(a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + a9) {
    ++instances;
  }

  ~X() { --instances; }
};

int X::instances = 0;

CASE_TEST(rc_ptr, make_strong_rc) {
  {
    util::memory::strong_rc_ptr<int> pi = util::memory::make_strong_rc<int>();

    CASE_EXPECT_TRUE(pi.get() != 0);
    CASE_EXPECT_TRUE(pi.use_count() == 1);

    CASE_EXPECT_TRUE(*pi == 0);
  }

  {
    util::memory::strong_rc_ptr<int> pi = util::memory::make_strong_rc<int>(5);

    CASE_EXPECT_TRUE(pi.get() != 0);
    CASE_EXPECT_TRUE(pi.use_count() == 1);

    CASE_EXPECT_TRUE(*pi == 5);
  }

  CASE_EXPECT_TRUE(X::instances == 0);

  {
    util::memory::strong_rc_ptr<X> pi = util::memory::make_strong_rc<X>();
    util::memory::weak_rc_ptr<X> wp(pi);

    CASE_EXPECT_TRUE(X::instances == 1);
    CASE_EXPECT_TRUE(pi.get() != 0);
    CASE_EXPECT_TRUE(pi.use_count() == 1);
    CASE_EXPECT_TRUE(pi->v == 0);

    pi.reset();

    CASE_EXPECT_TRUE(X::instances == 0);
  }

  {
    util::memory::strong_rc_ptr<X> pi = util::memory::make_strong_rc<X>(1);
    util::memory::weak_rc_ptr<X> wp(pi);

    CASE_EXPECT_TRUE(X::instances == 1);
    CASE_EXPECT_TRUE(pi.get() != 0);
    CASE_EXPECT_TRUE(pi.use_count() == 1);
    CASE_EXPECT_TRUE(pi->v == 1);

    pi.reset();

    CASE_EXPECT_TRUE(X::instances == 0);
  }

  {
    util::memory::strong_rc_ptr<X> pi = util::memory::make_strong_rc<X>(1, 2);
    util::memory::weak_rc_ptr<X> wp(pi);

    CASE_EXPECT_TRUE(X::instances == 1);
    CASE_EXPECT_TRUE(pi.get() != 0);
    CASE_EXPECT_TRUE(pi.use_count() == 1);
    CASE_EXPECT_TRUE(pi->v == 1 + 2);

    pi.reset();

    CASE_EXPECT_TRUE(X::instances == 0);
  }

  {
    util::memory::strong_rc_ptr<X> pi = util::memory::make_strong_rc<X>(1, 2, 3);
    util::memory::weak_rc_ptr<X> wp(pi);

    CASE_EXPECT_TRUE(X::instances == 1);
    CASE_EXPECT_TRUE(pi.get() != 0);
    CASE_EXPECT_TRUE(pi.use_count() == 1);
    CASE_EXPECT_TRUE(pi->v == 1 + 2 + 3);

    pi.reset();

    CASE_EXPECT_TRUE(X::instances == 0);
  }

  {
    util::memory::strong_rc_ptr<X> pi = util::memory::make_strong_rc<X>(1, 2, 3, 4);
    util::memory::weak_rc_ptr<X> wp(pi);

    CASE_EXPECT_TRUE(X::instances == 1);
    CASE_EXPECT_TRUE(pi.get() != 0);
    CASE_EXPECT_TRUE(pi.use_count() == 1);
    CASE_EXPECT_TRUE(pi->v == 1 + 2 + 3 + 4);

    pi.reset();

    CASE_EXPECT_TRUE(X::instances == 0);
  }

  {
    util::memory::strong_rc_ptr<X> pi = util::memory::make_strong_rc<X>(1, 2, 3, 4, 5);
    util::memory::weak_rc_ptr<X> wp(pi);

    CASE_EXPECT_TRUE(X::instances == 1);
    CASE_EXPECT_TRUE(pi.get() != 0);
    CASE_EXPECT_TRUE(pi.use_count() == 1);
    CASE_EXPECT_TRUE(pi->v == 1 + 2 + 3 + 4 + 5);

    pi.reset();

    CASE_EXPECT_TRUE(X::instances == 0);
  }

  {
    util::memory::strong_rc_ptr<X> pi = util::memory::make_strong_rc<X>(1, 2, 3, 4, 5, 6);
    util::memory::weak_rc_ptr<X> wp(pi);

    CASE_EXPECT_TRUE(X::instances == 1);
    CASE_EXPECT_TRUE(pi.get() != 0);
    CASE_EXPECT_TRUE(pi.use_count() == 1);
    CASE_EXPECT_TRUE(pi->v == 1 + 2 + 3 + 4 + 5 + 6);

    pi.reset();

    CASE_EXPECT_TRUE(X::instances == 0);
  }

  {
    util::memory::strong_rc_ptr<X> pi = util::memory::make_strong_rc<X>(1, 2, 3, 4, 5, 6, 7);
    util::memory::weak_rc_ptr<X> wp(pi);

    CASE_EXPECT_TRUE(X::instances == 1);
    CASE_EXPECT_TRUE(pi.get() != 0);
    CASE_EXPECT_TRUE(pi.use_count() == 1);
    CASE_EXPECT_TRUE(pi->v == 1 + 2 + 3 + 4 + 5 + 6 + 7);

    pi.reset();

    CASE_EXPECT_TRUE(X::instances == 0);
  }

  {
    util::memory::strong_rc_ptr<X> pi = util::memory::make_strong_rc<X>(1, 2, 3, 4, 5, 6, 7, 8);
    util::memory::weak_rc_ptr<X> wp(pi);

    CASE_EXPECT_TRUE(X::instances == 1);
    CASE_EXPECT_TRUE(pi.get() != 0);
    CASE_EXPECT_TRUE(pi.use_count() == 1);
    CASE_EXPECT_TRUE(pi->v == 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8);

    pi.reset();

    CASE_EXPECT_TRUE(X::instances == 0);
  }

  {
    util::memory::strong_rc_ptr<X> pi = util::memory::make_strong_rc<X>(1, 2, 3, 4, 5, 6, 7, 8, 9);
    util::memory::weak_rc_ptr<X> wp(pi);

    CASE_EXPECT_TRUE(X::instances == 1);
    CASE_EXPECT_TRUE(pi.get() != 0);
    CASE_EXPECT_TRUE(pi.use_count() == 1);
    CASE_EXPECT_TRUE(pi->v == 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9);

    pi.reset();

    CASE_EXPECT_TRUE(X::instances == 0);
  }
}
}  // namespace make_strong_rc

namespace weak_rc {

namespace n_element_type {

void f(int &) {}

void test() {
  typedef util::memory::weak_rc_ptr<int>::element_type T;
  T t;
  f(t);
}

}  // namespace n_element_type

class incomplete;

util::memory::strong_rc_ptr<incomplete> create_incomplete();

struct X {
  int dummy;
};

struct Y {
  int dummy2;
};

struct Z : public X, public virtual Y {};

namespace n_constructors {

void default_constructor() {
  {
    util::memory::weak_rc_ptr<int> wp;
    CASE_EXPECT_TRUE(wp.use_count() == 0);
  }

  {
    util::memory::weak_rc_ptr<void> wp;
    CASE_EXPECT_TRUE(wp.use_count() == 0);
  }

  {
    util::memory::weak_rc_ptr<incomplete> wp;
    CASE_EXPECT_TRUE(wp.use_count() == 0);
  }
}

void shared_ptr_constructor() {
  {
    util::memory::strong_rc_ptr<int> sp;

    util::memory::weak_rc_ptr<int> wp(sp);
    CASE_EXPECT_TRUE(wp.use_count() == sp.use_count());

    util::memory::weak_rc_ptr<void> wp2(sp);
    CASE_EXPECT_TRUE(wp2.use_count() == sp.use_count());
  }

  {
    util::memory::strong_rc_ptr<int> sp(static_cast<int *>(0));

    {
      util::memory::weak_rc_ptr<int> wp(sp);
      CASE_EXPECT_TRUE(wp.use_count() == sp.use_count());
      CASE_EXPECT_TRUE(wp.use_count() == 1);
      util::memory::strong_rc_ptr<int> sp2(wp);
      CASE_EXPECT_TRUE(wp.use_count() == 2);
      CASE_EXPECT_TRUE(!(sp < sp2 || sp2 < sp));
    }

    {
      util::memory::weak_rc_ptr<void> wp(sp);
      CASE_EXPECT_TRUE(wp.use_count() == sp.use_count());
      CASE_EXPECT_TRUE(wp.use_count() == 1);
      util::memory::strong_rc_ptr<void> sp2(wp);
      CASE_EXPECT_TRUE(wp.use_count() == 2);
      CASE_EXPECT_TRUE(!(sp < sp2 || sp2 < sp));
    }
  }

  {
    util::memory::strong_rc_ptr<int> sp(new int);

    {
      util::memory::weak_rc_ptr<int> wp(sp);
      CASE_EXPECT_TRUE(wp.use_count() == sp.use_count());
      CASE_EXPECT_TRUE(wp.use_count() == 1);
      util::memory::strong_rc_ptr<int> sp2(wp);
      CASE_EXPECT_TRUE(wp.use_count() == 2);
      CASE_EXPECT_TRUE(!(sp < sp2 || sp2 < sp));
    }

    {
      util::memory::weak_rc_ptr<void> wp(sp);
      CASE_EXPECT_TRUE(wp.use_count() == sp.use_count());
      CASE_EXPECT_TRUE(wp.use_count() == 1);
      util::memory::strong_rc_ptr<void> sp2(wp);
      CASE_EXPECT_TRUE(wp.use_count() == 2);
      CASE_EXPECT_TRUE(!(sp < sp2 || sp2 < sp));
    }
  }

  {
    util::memory::strong_rc_ptr<void> sp;

    util::memory::weak_rc_ptr<void> wp(sp);
    CASE_EXPECT_TRUE(wp.use_count() == sp.use_count());
  }

  {
    util::memory::strong_rc_ptr<void> sp(static_cast<int *>(0));

    util::memory::weak_rc_ptr<void> wp(sp);
    CASE_EXPECT_TRUE(wp.use_count() == sp.use_count());
    CASE_EXPECT_TRUE(wp.use_count() == 1);
    util::memory::strong_rc_ptr<void> sp2(wp);
    CASE_EXPECT_TRUE(wp.use_count() == 2);
    CASE_EXPECT_TRUE(!(sp < sp2 || sp2 < sp));
  }

  {
    util::memory::strong_rc_ptr<void> sp(new int);

    util::memory::weak_rc_ptr<void> wp(sp);
    CASE_EXPECT_TRUE(wp.use_count() == sp.use_count());
    CASE_EXPECT_TRUE(wp.use_count() == 1);
    util::memory::strong_rc_ptr<void> sp2(wp);
    CASE_EXPECT_TRUE(wp.use_count() == 2);
    CASE_EXPECT_TRUE(!(sp < sp2 || sp2 < sp));
  }

  {
    util::memory::strong_rc_ptr<incomplete> sp;

    util::memory::weak_rc_ptr<incomplete> wp(sp);
    CASE_EXPECT_TRUE(wp.use_count() == sp.use_count());

    util::memory::weak_rc_ptr<void> wp2(sp);
    CASE_EXPECT_TRUE(wp2.use_count() == sp.use_count());
  }

  {
    util::memory::strong_rc_ptr<incomplete> sp = create_incomplete();

    {
      util::memory::weak_rc_ptr<incomplete> wp(sp);
      CASE_EXPECT_TRUE(wp.use_count() == sp.use_count());
      CASE_EXPECT_TRUE(wp.use_count() == 1);
      util::memory::strong_rc_ptr<incomplete> sp2(wp);
      CASE_EXPECT_TRUE(wp.use_count() == 2);
      CASE_EXPECT_TRUE(!(sp < sp2 || sp2 < sp));
    }

    {
      util::memory::weak_rc_ptr<void> wp(sp);
      CASE_EXPECT_TRUE(wp.use_count() == sp.use_count());
      CASE_EXPECT_TRUE(wp.use_count() == 1);
      util::memory::strong_rc_ptr<void> sp2(wp);
      CASE_EXPECT_TRUE(wp.use_count() == 2);
      CASE_EXPECT_TRUE(!(sp < sp2 || sp2 < sp));
    }
  }

  {
    util::memory::strong_rc_ptr<void> sp = create_incomplete();

    util::memory::weak_rc_ptr<void> wp(sp);
    CASE_EXPECT_TRUE(wp.use_count() == sp.use_count());
    CASE_EXPECT_TRUE(wp.use_count() == 1);
    util::memory::strong_rc_ptr<void> sp2(wp);
    CASE_EXPECT_TRUE(wp.use_count() == 2);
    CASE_EXPECT_TRUE(!(sp < sp2 || sp2 < sp));
  }
}

void copy_constructor() {
  {
    util::memory::weak_rc_ptr<int> wp;
    util::memory::weak_rc_ptr<int> wp2(wp);
    CASE_EXPECT_TRUE(wp2.use_count() == wp.use_count());
    CASE_EXPECT_TRUE(wp2.use_count() == 0);
  }

  {
    util::memory::weak_rc_ptr<void> wp;
    util::memory::weak_rc_ptr<void> wp2(wp);
    CASE_EXPECT_TRUE(wp2.use_count() == wp.use_count());
    CASE_EXPECT_TRUE(wp2.use_count() == 0);
  }

  {
    util::memory::weak_rc_ptr<incomplete> wp;
    util::memory::weak_rc_ptr<incomplete> wp2(wp);
    CASE_EXPECT_TRUE(wp2.use_count() == wp.use_count());
    CASE_EXPECT_TRUE(wp2.use_count() == 0);
  }

  {
    util::memory::strong_rc_ptr<int> sp(static_cast<int *>(0));
    util::memory::weak_rc_ptr<int> wp(sp);

    util::memory::weak_rc_ptr<int> wp2(wp);
    CASE_EXPECT_TRUE(wp2.use_count() == wp.use_count());
    CASE_EXPECT_TRUE(wp2.use_count() == 1);
    // CASE_EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

    sp.reset();
    // CASE_EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

    util::memory::weak_rc_ptr<int> wp3(wp);
    CASE_EXPECT_TRUE(wp3.use_count() == wp.use_count());
    CASE_EXPECT_TRUE(wp3.use_count() == 0);
    // CASE_EXPECT_TRUE(!(wp < wp3 || wp3 < wp));
  }

  {
    util::memory::strong_rc_ptr<int> sp(new int);
    util::memory::weak_rc_ptr<int> wp(sp);

    util::memory::weak_rc_ptr<int> wp2(wp);
    CASE_EXPECT_TRUE(wp2.use_count() == wp.use_count());
    CASE_EXPECT_TRUE(wp2.use_count() == 1);
    // CASE_EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

    sp.reset();
    // CASE_EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

    util::memory::weak_rc_ptr<int> wp3(wp);
    CASE_EXPECT_TRUE(wp3.use_count() == wp.use_count());
    CASE_EXPECT_TRUE(wp3.use_count() == 0);
    // CASE_EXPECT_TRUE(!(wp < wp3 || wp3 < wp));
  }

  {
    util::memory::strong_rc_ptr<void> sp(static_cast<int *>(0));
    util::memory::weak_rc_ptr<void> wp(sp);

    util::memory::weak_rc_ptr<void> wp2(wp);
    CASE_EXPECT_TRUE(wp2.use_count() == wp.use_count());
    CASE_EXPECT_TRUE(wp2.use_count() == 1);
    // CASE_EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

    sp.reset();
    // CASE_EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

    util::memory::weak_rc_ptr<void> wp3(wp);
    CASE_EXPECT_TRUE(wp3.use_count() == wp.use_count());
    CASE_EXPECT_TRUE(wp3.use_count() == 0);
    // CASE_EXPECT_TRUE(!(wp < wp3 || wp3 < wp));
  }

  {
    util::memory::strong_rc_ptr<void> sp(new int);
    util::memory::weak_rc_ptr<void> wp(sp);

    util::memory::weak_rc_ptr<void> wp2(wp);
    CASE_EXPECT_TRUE(wp2.use_count() == wp.use_count());
    CASE_EXPECT_TRUE(wp2.use_count() == 1);
    // CASE_EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

    sp.reset();
    // CASE_EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

    util::memory::weak_rc_ptr<void> wp3(wp);
    CASE_EXPECT_TRUE(wp3.use_count() == wp.use_count());
    CASE_EXPECT_TRUE(wp3.use_count() == 0);
    // CASE_EXPECT_TRUE(!(wp < wp3 || wp3 < wp));
  }

  {
    util::memory::strong_rc_ptr<incomplete> sp = create_incomplete();
    util::memory::weak_rc_ptr<incomplete> wp(sp);

    util::memory::weak_rc_ptr<incomplete> wp2(wp);
    CASE_EXPECT_TRUE(wp2.use_count() == wp.use_count());
    CASE_EXPECT_TRUE(wp2.use_count() == 1);
    // CASE_EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

    sp.reset();
    // CASE_EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

    util::memory::weak_rc_ptr<incomplete> wp3(wp);
    CASE_EXPECT_TRUE(wp3.use_count() == wp.use_count());
    CASE_EXPECT_TRUE(wp3.use_count() == 0);
    // CASE_EXPECT_TRUE(!(wp < wp3 || wp3 < wp));
  }
}

void conversion_constructor() {
  {
    util::memory::weak_rc_ptr<int> wp;
    util::memory::weak_rc_ptr<void> wp2(wp);
    CASE_EXPECT_TRUE(wp2.use_count() == wp.use_count());
    CASE_EXPECT_TRUE(wp2.use_count() == 0);
  }

  {
    util::memory::weak_rc_ptr<incomplete> wp;
    util::memory::weak_rc_ptr<void> wp2(wp);
    CASE_EXPECT_TRUE(wp2.use_count() == wp.use_count());
    CASE_EXPECT_TRUE(wp2.use_count() == 0);
  }

  {
    util::memory::weak_rc_ptr<Z> wp;

    util::memory::weak_rc_ptr<X> wp2(wp);
    CASE_EXPECT_TRUE(wp2.use_count() == wp.use_count());
    CASE_EXPECT_TRUE(wp2.use_count() == 0);

    util::memory::weak_rc_ptr<Y> wp3(wp);
    CASE_EXPECT_TRUE(wp3.use_count() == wp.use_count());
    CASE_EXPECT_TRUE(wp3.use_count() == 0);
  }

  {
    util::memory::strong_rc_ptr<int> sp(static_cast<int *>(0));
    util::memory::weak_rc_ptr<int> wp(sp);

    util::memory::weak_rc_ptr<void> wp2(wp);
    CASE_EXPECT_TRUE(wp2.use_count() == wp.use_count());
    CASE_EXPECT_TRUE(wp2.use_count() == 1);
    // CASE_EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

    sp.reset();
    // CASE_EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

    util::memory::weak_rc_ptr<void> wp3(wp);
    CASE_EXPECT_TRUE(wp3.use_count() == wp.use_count());
    CASE_EXPECT_TRUE(wp3.use_count() == 0);
    // CASE_EXPECT_TRUE(!(wp < wp3 || wp3 < wp));
  }

  {
    util::memory::strong_rc_ptr<int> sp(new int);
    util::memory::weak_rc_ptr<int> wp(sp);

    util::memory::weak_rc_ptr<void> wp2(wp);
    CASE_EXPECT_TRUE(wp2.use_count() == wp.use_count());
    CASE_EXPECT_TRUE(wp2.use_count() == 1);
    // CASE_EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

    sp.reset();
    // CASE_EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

    util::memory::weak_rc_ptr<void> wp3(wp);
    CASE_EXPECT_TRUE(wp3.use_count() == wp.use_count());
    CASE_EXPECT_TRUE(wp3.use_count() == 0);
    // CASE_EXPECT_TRUE(!(wp < wp3 || wp3 < wp));
  }

  {
    util::memory::strong_rc_ptr<incomplete> sp = create_incomplete();
    util::memory::weak_rc_ptr<incomplete> wp(sp);

    util::memory::weak_rc_ptr<void> wp2(wp);
    CASE_EXPECT_TRUE(wp2.use_count() == wp.use_count());
    CASE_EXPECT_TRUE(wp2.use_count() == 1);
    // CASE_EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

    sp.reset();
    // CASE_EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

    util::memory::weak_rc_ptr<void> wp3(wp);
    CASE_EXPECT_TRUE(wp3.use_count() == wp.use_count());
    CASE_EXPECT_TRUE(wp3.use_count() == 0);
    // CASE_EXPECT_TRUE(!(wp < wp3 || wp3 < wp));
  }

  {
    util::memory::strong_rc_ptr<Z> sp(static_cast<Z *>(0));
    util::memory::weak_rc_ptr<Z> wp(sp);

    util::memory::weak_rc_ptr<X> wp2(wp);
    CASE_EXPECT_TRUE(wp2.use_count() == wp.use_count());
    CASE_EXPECT_TRUE(wp2.use_count() == 1);
    // CASE_EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

    sp.reset();
    // CASE_EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

    util::memory::weak_rc_ptr<X> wp3(wp);
    CASE_EXPECT_TRUE(wp3.use_count() == wp.use_count());
    CASE_EXPECT_TRUE(wp3.use_count() == 0);
    // CASE_EXPECT_TRUE(!(wp < wp3 || wp3 < wp));
  }

  {
    util::memory::strong_rc_ptr<Z> sp(static_cast<Z *>(0));
    util::memory::weak_rc_ptr<Z> wp(sp);

    util::memory::weak_rc_ptr<Y> wp2(wp);
    CASE_EXPECT_TRUE(wp2.use_count() == wp.use_count());
    CASE_EXPECT_TRUE(wp2.use_count() == 1);
    // CASE_EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

    sp.reset();
    // CASE_EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

    util::memory::weak_rc_ptr<Y> wp3(wp);
    CASE_EXPECT_TRUE(wp3.use_count() == wp.use_count());
    CASE_EXPECT_TRUE(wp3.use_count() == 0);
    // CASE_EXPECT_TRUE(!(wp < wp3 || wp3 < wp));
  }

  {
    util::memory::strong_rc_ptr<Z> sp(new Z);
    util::memory::weak_rc_ptr<Z> wp(sp);

    util::memory::weak_rc_ptr<X> wp2(wp);
    CASE_EXPECT_TRUE(wp2.use_count() == wp.use_count());
    CASE_EXPECT_TRUE(wp2.use_count() == 1);
    // CASE_EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

    sp.reset();
    // CASE_EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

    util::memory::weak_rc_ptr<X> wp3(wp);
    CASE_EXPECT_TRUE(wp3.use_count() == wp.use_count());
    CASE_EXPECT_TRUE(wp3.use_count() == 0);
    // CASE_EXPECT_TRUE(!(wp < wp3 || wp3 < wp));
  }

  {
    util::memory::strong_rc_ptr<Z> sp(new Z);
    util::memory::weak_rc_ptr<Z> wp(sp);

    util::memory::weak_rc_ptr<Y> wp2(wp);
    CASE_EXPECT_TRUE(wp2.use_count() == wp.use_count());
    CASE_EXPECT_TRUE(wp2.use_count() == 1);
    // CASE_EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

    sp.reset();
    // CASE_EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

    util::memory::weak_rc_ptr<Y> wp3(wp);
    CASE_EXPECT_TRUE(wp3.use_count() == wp.use_count());
    CASE_EXPECT_TRUE(wp3.use_count() == 0);
    // CASE_EXPECT_TRUE(!(wp < wp3 || wp3 < wp));
  }
}

void test() {
  default_constructor();
  shared_ptr_constructor();
  copy_constructor();
  conversion_constructor();
}

}  // namespace n_constructors

namespace n_assignment {

template <class T>
void copy_assignment(util::memory::strong_rc_ptr<T> &sp) {
  CASE_EXPECT_TRUE(sp.unique());

  util::memory::weak_rc_ptr<T> p1;

  p1 = p1;
  CASE_EXPECT_TRUE(p1.use_count() == 0);

  util::memory::weak_rc_ptr<T> p2;

  p1 = p2;
  CASE_EXPECT_TRUE(p1.use_count() == 0);

  util::memory::weak_rc_ptr<T> p3(p1);

  p1 = p3;
  CASE_EXPECT_TRUE(p1.use_count() == 0);

  util::memory::weak_rc_ptr<T> p4(sp);

  p4 = p4;
  CASE_EXPECT_TRUE(p4.use_count() == 1);

  p1 = p4;
  CASE_EXPECT_TRUE(p1.use_count() == 1);

  p4 = p2;
  CASE_EXPECT_TRUE(p4.use_count() == 0);

  sp.reset();

  p1 = p1;
  CASE_EXPECT_TRUE(p1.use_count() == 0);

  p4 = p1;
  CASE_EXPECT_TRUE(p4.use_count() == 0);
}

void conversion_assignment() {
  {
    util::memory::weak_rc_ptr<void> p1;

    util::memory::weak_rc_ptr<incomplete> p2;

    p1 = p2;
    CASE_EXPECT_TRUE(p1.use_count() == 0);

    util::memory::strong_rc_ptr<incomplete> sp = create_incomplete();
    util::memory::weak_rc_ptr<incomplete> p3(sp);

    p1 = p3;
    CASE_EXPECT_TRUE(p1.use_count() == 1);

    sp.reset();

    p1 = p3;
    CASE_EXPECT_TRUE(p1.use_count() == 0);

    p1 = p2;
    CASE_EXPECT_TRUE(p1.use_count() == 0);
  }

  {
    util::memory::weak_rc_ptr<X> p1;

    util::memory::weak_rc_ptr<Z> p2;

    p1 = p2;
    CASE_EXPECT_TRUE(p1.use_count() == 0);

    util::memory::strong_rc_ptr<Z> sp(new Z);
    util::memory::weak_rc_ptr<Z> p3(sp);

    p1 = p3;
    CASE_EXPECT_TRUE(p1.use_count() == 1);

    sp.reset();

    p1 = p3;
    CASE_EXPECT_TRUE(p1.use_count() == 0);

    p1 = p2;
    CASE_EXPECT_TRUE(p1.use_count() == 0);
  }

  {
    util::memory::weak_rc_ptr<Y> p1;

    util::memory::weak_rc_ptr<Z> p2;

    p1 = p2;
    CASE_EXPECT_TRUE(p1.use_count() == 0);

    util::memory::strong_rc_ptr<Z> sp(new Z);
    util::memory::weak_rc_ptr<Z> p3(sp);

    p1 = p3;
    CASE_EXPECT_TRUE(p1.use_count() == 1);

    sp.reset();

    p1 = p3;
    CASE_EXPECT_TRUE(p1.use_count() == 0);

    p1 = p2;
    CASE_EXPECT_TRUE(p1.use_count() == 0);
  }
}

template <class T, class U>
void shared_ptr_assignment(util::memory::strong_rc_ptr<U> &sp, T * = 0) {
  CASE_EXPECT_TRUE(sp.unique());

  util::memory::weak_rc_ptr<T> p1;
  util::memory::weak_rc_ptr<T> p2(p1);
  util::memory::weak_rc_ptr<T> p3(sp);
  util::memory::weak_rc_ptr<T> p4(p3);

  p1 = sp;
  CASE_EXPECT_TRUE(p1.use_count() == 1);

  p2 = sp;
  CASE_EXPECT_TRUE(p2.use_count() == 1);

  p3 = sp;
  CASE_EXPECT_TRUE(p3.use_count() == 1);

  p4 = sp;
  CASE_EXPECT_TRUE(p4.use_count() == 1);

  sp.reset();

  CASE_EXPECT_TRUE(p1.use_count() == 0);
  CASE_EXPECT_TRUE(p2.use_count() == 0);
  CASE_EXPECT_TRUE(p3.use_count() == 0);
  CASE_EXPECT_TRUE(p4.use_count() == 0);

  p1 = sp;
}

void test() {
  {
    util::memory::strong_rc_ptr<int> p(new int);
    copy_assignment(p);
  }

  {
    util::memory::strong_rc_ptr<X> p(new X);
    copy_assignment(p);
  }

  {
    util::memory::strong_rc_ptr<void> p(new int);
    copy_assignment(p);
  }

  {
    util::memory::strong_rc_ptr<incomplete> p = create_incomplete();
    copy_assignment(p);
  }

  conversion_assignment();

  {
    util::memory::strong_rc_ptr<int> p(new int);
    shared_ptr_assignment<int>(p);
  }

  {
    util::memory::strong_rc_ptr<int> p(new int);
    shared_ptr_assignment<void>(p);
  }

  {
    util::memory::strong_rc_ptr<X> p(new X);
    shared_ptr_assignment<X>(p);
  }

  {
    util::memory::strong_rc_ptr<X> p(new X);
    shared_ptr_assignment<void>(p);
  }

  {
    util::memory::strong_rc_ptr<void> p(new int);
    shared_ptr_assignment<void>(p);
  }

  {
    util::memory::strong_rc_ptr<incomplete> p = create_incomplete();
    shared_ptr_assignment<incomplete>(p);
  }

  {
    util::memory::strong_rc_ptr<incomplete> p = create_incomplete();
    shared_ptr_assignment<void>(p);
  }
}

}  // namespace n_assignment

namespace n_reset {

template <class T, class U>
void test2(util::memory::strong_rc_ptr<U> &sp, T * = 0) {
  CASE_EXPECT_TRUE(sp.unique());

  util::memory::weak_rc_ptr<T> p1;
  util::memory::weak_rc_ptr<T> p2(p1);
  util::memory::weak_rc_ptr<T> p3(sp);
  util::memory::weak_rc_ptr<T> p4(p3);
  util::memory::weak_rc_ptr<T> p5(sp);
  util::memory::weak_rc_ptr<T> p6(p5);

  p1.reset();
  CASE_EXPECT_TRUE(p1.use_count() == 0);

  p2.reset();
  CASE_EXPECT_TRUE(p2.use_count() == 0);

  p3.reset();
  CASE_EXPECT_TRUE(p3.use_count() == 0);

  p4.reset();
  CASE_EXPECT_TRUE(p4.use_count() == 0);

  sp.reset();

  p5.reset();
  CASE_EXPECT_TRUE(p5.use_count() == 0);

  p6.reset();
  CASE_EXPECT_TRUE(p6.use_count() == 0);
}

void test() {
  {
    util::memory::strong_rc_ptr<int> p(new int);
    test2<int>(p);
  }

  {
    util::memory::strong_rc_ptr<int> p(new int);
    test2<void>(p);
  }

  {
    util::memory::strong_rc_ptr<X> p(new X);
    test2<X>(p);
  }

  {
    util::memory::strong_rc_ptr<X> p(new X);
    test2<void>(p);
  }

  {
    util::memory::strong_rc_ptr<void> p(new int);
    test2<void>(p);
  }

  {
    util::memory::strong_rc_ptr<incomplete> p = create_incomplete();
    test2<incomplete>(p);
  }

  {
    util::memory::strong_rc_ptr<incomplete> p = create_incomplete();
    test2<void>(p);
  }
}

}  // namespace n_reset

namespace n_use_count {

void test() {
  {
    util::memory::weak_rc_ptr<X> wp;
    CASE_EXPECT_TRUE(wp.use_count() == 0);
    CASE_EXPECT_TRUE(wp.expired());

    util::memory::weak_rc_ptr<X> wp2;
    CASE_EXPECT_TRUE(wp.use_count() == 0);
    CASE_EXPECT_TRUE(wp.expired());

    util::memory::weak_rc_ptr<X> wp3(wp);
    CASE_EXPECT_TRUE(wp.use_count() == 0);
    CASE_EXPECT_TRUE(wp.expired());
    CASE_EXPECT_TRUE(wp3.use_count() == 0);
    CASE_EXPECT_TRUE(wp3.expired());
  }

  {
    util::memory::strong_rc_ptr<X> sp(static_cast<X *>(0));

    util::memory::weak_rc_ptr<X> wp(sp);
    CASE_EXPECT_TRUE(wp.use_count() == 1);
    CASE_EXPECT_TRUE(!wp.expired());

    util::memory::weak_rc_ptr<X> wp2(sp);
    CASE_EXPECT_TRUE(wp.use_count() == 1);
    CASE_EXPECT_TRUE(!wp.expired());

    util::memory::weak_rc_ptr<X> wp3(wp);
    CASE_EXPECT_TRUE(wp.use_count() == 1);
    CASE_EXPECT_TRUE(!wp.expired());
    CASE_EXPECT_TRUE(wp3.use_count() == 1);
    CASE_EXPECT_TRUE(!wp3.expired());

    util::memory::strong_rc_ptr<X> sp2(sp);

    CASE_EXPECT_TRUE(wp.use_count() == 2);
    CASE_EXPECT_TRUE(!wp.expired());
    CASE_EXPECT_TRUE(wp2.use_count() == 2);
    CASE_EXPECT_TRUE(!wp2.expired());
    CASE_EXPECT_TRUE(wp3.use_count() == 2);
    CASE_EXPECT_TRUE(!wp3.expired());

    util::memory::strong_rc_ptr<void> sp3(sp);

    CASE_EXPECT_TRUE(wp.use_count() == 3);
    CASE_EXPECT_TRUE(!wp.expired());
    CASE_EXPECT_TRUE(wp2.use_count() == 3);
    CASE_EXPECT_TRUE(!wp2.expired());
    CASE_EXPECT_TRUE(wp3.use_count() == 3);
    CASE_EXPECT_TRUE(!wp3.expired());

    sp.reset();

    CASE_EXPECT_TRUE(wp.use_count() == 2);
    CASE_EXPECT_TRUE(!wp.expired());
    CASE_EXPECT_TRUE(wp2.use_count() == 2);
    CASE_EXPECT_TRUE(!wp2.expired());
    CASE_EXPECT_TRUE(wp3.use_count() == 2);
    CASE_EXPECT_TRUE(!wp3.expired());

    sp2.reset();

    CASE_EXPECT_TRUE(wp.use_count() == 1);
    CASE_EXPECT_TRUE(!wp.expired());
    CASE_EXPECT_TRUE(wp2.use_count() == 1);
    CASE_EXPECT_TRUE(!wp2.expired());
    CASE_EXPECT_TRUE(wp3.use_count() == 1);
    CASE_EXPECT_TRUE(!wp3.expired());

    sp3.reset();

    CASE_EXPECT_TRUE(wp.use_count() == 0);
    CASE_EXPECT_TRUE(wp.expired());
    CASE_EXPECT_TRUE(wp2.use_count() == 0);
    CASE_EXPECT_TRUE(wp2.expired());
    CASE_EXPECT_TRUE(wp3.use_count() == 0);
    CASE_EXPECT_TRUE(wp3.expired());
  }
}

}  // namespace n_use_count

namespace n_swap {

void test() {
  {
    util::memory::weak_rc_ptr<X> wp;
    util::memory::weak_rc_ptr<X> wp2;

    wp.swap(wp2);

    CASE_EXPECT_TRUE(wp.use_count() == 0);
    CASE_EXPECT_TRUE(wp2.use_count() == 0);

    using std::swap;
    swap(wp, wp2);

    CASE_EXPECT_TRUE(wp.use_count() == 0);
    CASE_EXPECT_TRUE(wp2.use_count() == 0);
  }

  {
    util::memory::strong_rc_ptr<X> sp(new X);
    util::memory::weak_rc_ptr<X> wp;
    util::memory::weak_rc_ptr<X> wp2(sp);
    util::memory::weak_rc_ptr<X> wp3(sp);

    wp.swap(wp2);

    CASE_EXPECT_TRUE(wp.use_count() == 1);
    CASE_EXPECT_TRUE(wp2.use_count() == 0);
    // CASE_EXPECT_TRUE(!(wp < wp3 || wp3 < wp));

    using std::swap;
    swap(wp, wp2);

    CASE_EXPECT_TRUE(wp.use_count() == 0);
    CASE_EXPECT_TRUE(wp2.use_count() == 1);
    // CASE_EXPECT_TRUE(!(wp2 < wp3 || wp3 < wp2));

    sp.reset();

    wp.swap(wp2);

    CASE_EXPECT_TRUE(wp.use_count() == 0);
    CASE_EXPECT_TRUE(wp2.use_count() == 0);
    // CASE_EXPECT_TRUE(!(wp < wp3 || wp3 < wp));

    swap(wp, wp2);

    CASE_EXPECT_TRUE(wp.use_count() == 0);
    CASE_EXPECT_TRUE(wp2.use_count() == 0);
    // CASE_EXPECT_TRUE(!(wp2 < wp3 || wp3 < wp2));
  }

  {
    util::memory::strong_rc_ptr<X> sp(new X);
    util::memory::strong_rc_ptr<X> sp2(new X);
    util::memory::weak_rc_ptr<X> wp(sp);
    util::memory::weak_rc_ptr<X> wp2(sp2);
    util::memory::weak_rc_ptr<X> wp3(sp2);

    wp.swap(wp2);

    CASE_EXPECT_TRUE(wp.use_count() == 1);
    CASE_EXPECT_TRUE(wp2.use_count() == 1);
    // CASE_EXPECT_TRUE(!(wp < wp3 || wp3 < wp));

    using std::swap;
    swap(wp, wp2);

    CASE_EXPECT_TRUE(wp.use_count() == 1);
    CASE_EXPECT_TRUE(wp2.use_count() == 1);
    // CASE_EXPECT_TRUE(!(wp2 < wp3 || wp3 < wp2));

    sp.reset();

    wp.swap(wp2);

    CASE_EXPECT_TRUE(wp.use_count() == 1);
    CASE_EXPECT_TRUE(wp2.use_count() == 0);
    // CASE_EXPECT_TRUE(!(wp < wp3 || wp3 < wp));

    swap(wp, wp2);

    CASE_EXPECT_TRUE(wp.use_count() == 0);
    CASE_EXPECT_TRUE(wp2.use_count() == 1);
    // CASE_EXPECT_TRUE(!(wp2 < wp3 || wp3 < wp2));

    sp2.reset();

    wp.swap(wp2);

    CASE_EXPECT_TRUE(wp.use_count() == 0);
    CASE_EXPECT_TRUE(wp2.use_count() == 0);
    // CASE_EXPECT_TRUE(!(wp < wp3 || wp3 < wp));

    swap(wp, wp2);

    CASE_EXPECT_TRUE(wp.use_count() == 0);
    CASE_EXPECT_TRUE(wp2.use_count() == 0);
    // CASE_EXPECT_TRUE(!(wp2 < wp3 || wp3 < wp2));
  }
}

}  // namespace n_swap

namespace n_lock {

void test() {}

}  // namespace n_lock

CASE_TEST(rc_ptr, weak_rc_ptr_basic) {
  n_element_type::test();
  n_constructors::test();
  n_assignment::test();
  n_reset::test();
  n_use_count::test();
  n_swap::test();
  n_lock::test();
}

class incomplete {};

util::memory::strong_rc_ptr<incomplete> create_incomplete() {
  util::memory::strong_rc_ptr<incomplete> px(new incomplete);
  return px;
}

}  // namespace weak_rc

namespace weak_rc_ptr_move {

struct X {
  static int64_t instances;

  X() { ++instances; }

  ~X() { --instances; }

 private:
  X(X const &);
  X &operator=(X const &);
};

int64_t X::instances = 0;

CASE_TEST(rc_ptr, weak_rc_ptr_move) {
  CASE_EXPECT_TRUE(X::instances == 0);

  {
    util::memory::strong_rc_ptr<X> p_(new X);
    util::memory::weak_rc_ptr<X> p(p_);
    CASE_EXPECT_TRUE(X::instances == 1);
    CASE_EXPECT_TRUE(p.use_count() == 1);

    util::memory::weak_rc_ptr<X> p2(std::move(p));
    CASE_EXPECT_TRUE(X::instances == 1);
    CASE_EXPECT_TRUE(p2.use_count() == 1);
    CASE_EXPECT_TRUE(p.expired());

    util::memory::weak_rc_ptr<void> p3(std::move(p2));
    CASE_EXPECT_TRUE(X::instances == 1);
    CASE_EXPECT_TRUE(p3.use_count() == 1);
    CASE_EXPECT_TRUE(p2.expired());

    p_.reset();
    CASE_EXPECT_TRUE(X::instances == 0);
    CASE_EXPECT_TRUE(p3.expired());
  }

  {
    util::memory::strong_rc_ptr<X> p_(new X);
    util::memory::weak_rc_ptr<X> p(p_);
    CASE_EXPECT_TRUE(X::instances == 1);
    CASE_EXPECT_TRUE(p.use_count() == 1);

    util::memory::weak_rc_ptr<X> p2;
    p2 = static_cast<util::memory::weak_rc_ptr<X> &&>(p);
    CASE_EXPECT_TRUE(X::instances == 1);
    CASE_EXPECT_TRUE(p2.use_count() == 1);
    CASE_EXPECT_TRUE(p.expired());

    util::memory::weak_rc_ptr<void> p3;
    p3 = std::move(p2);
    CASE_EXPECT_TRUE(X::instances == 1);
    CASE_EXPECT_TRUE(p3.use_count() == 1);
    CASE_EXPECT_TRUE(p2.expired());

    p_.reset();
    CASE_EXPECT_TRUE(X::instances == 0);
    CASE_EXPECT_TRUE(p3.expired());
  }

  {
    util::memory::strong_rc_ptr<X> p_(new X);
    util::memory::weak_rc_ptr<X> p(p_);
    CASE_EXPECT_TRUE(X::instances == 1);
    CASE_EXPECT_TRUE(p.use_count() == 1);

    util::memory::strong_rc_ptr<X> p_2(new X);
    util::memory::weak_rc_ptr<X> p2(p_2);
    CASE_EXPECT_TRUE(X::instances == 2);
    p2 = std::move(p);
    CASE_EXPECT_TRUE(X::instances == 2);
    CASE_EXPECT_TRUE(p2.use_count() == 1);
    CASE_EXPECT_TRUE(p.expired());
    CASE_EXPECT_TRUE(p2.lock() != p_2);

    util::memory::strong_rc_ptr<void> p_3(new X);
    util::memory::weak_rc_ptr<void> p3(p_3);
    CASE_EXPECT_TRUE(X::instances == 3);
    p3 = std::move(p2);
    CASE_EXPECT_TRUE(X::instances == 3);
    CASE_EXPECT_TRUE(p3.use_count() == 1);
    CASE_EXPECT_TRUE(p2.expired());
    CASE_EXPECT_TRUE(p3.lock() != p_3);
  }
}
}  // namespace weak_rc_ptr_move

template <class L, class R>
static bool owner_before_comp(const L &l, const R &r) noexcept {
  return l.owner_before(r);
}

CASE_TEST(rc_ptr, owner_before) {
  {
    util::memory::strong_rc_ptr<int> x;
    util::memory::strong_rc_ptr<int> y;
    util::memory::weak_rc_ptr<int> w;
    CASE_EXPECT_TRUE(!(owner_before_comp(x, w) || owner_before_comp(w, x)));
  }
  {
    util::memory::strong_rc_ptr<int> z(reinterpret_cast<int *>(0));
    util::memory::weak_rc_ptr<int> w;
    CASE_EXPECT_TRUE(owner_before_comp(z, w) || owner_before_comp(w, z));
    {
      util::memory::strong_rc_ptr<int> zz(z);
      w = util::memory::weak_rc_ptr<int>(zz);
      CASE_EXPECT_TRUE(!(owner_before_comp(z, zz) || owner_before_comp(z, zz)));
      CASE_EXPECT_TRUE(!(owner_before_comp(z, w) || owner_before_comp(z, w)));
    }
    CASE_EXPECT_TRUE(!(owner_before_comp(z, w) || owner_before_comp(w, z)));
  }
  {
    util::memory::strong_rc_ptr<int> x;
    util::memory::strong_rc_ptr<int> z(reinterpret_cast<int *>(0));
    CASE_EXPECT_TRUE(owner_before_comp(x, z) || owner_before_comp(z, x));
  }
  {
    util::memory::strong_rc_ptr<int> a(reinterpret_cast<int *>(0));
    util::memory::strong_rc_ptr<int> b(reinterpret_cast<int *>(0));
    CASE_EXPECT_TRUE(owner_before_comp(a, b) || owner_before_comp(b, a));
    util::memory::weak_rc_ptr<int> w(a);
    CASE_EXPECT_TRUE(!(owner_before_comp(a, w) || owner_before_comp(w, a)));
    CASE_EXPECT_TRUE(owner_before_comp(b, w) || owner_before_comp(w, b));
  }

  {
    util::memory::strong_rc_ptr<int> a(reinterpret_cast<int *>(0));
    util::memory::weak_rc_ptr<int> wa(a);
    util::memory::strong_rc_ptr<int> b(reinterpret_cast<int *>(0));
    util::memory::weak_rc_ptr<int> wb(b);
    CASE_EXPECT_TRUE(!(owner_before_comp(a, wa) || owner_before_comp(wa, a)));
    CASE_EXPECT_TRUE(!(owner_before_comp(b, wb) || owner_before_comp(wb, b)));
    CASE_EXPECT_TRUE(owner_before_comp(wa, wb) || owner_before_comp(wb, wa));
    CASE_EXPECT_TRUE(owner_before_comp(wa, b) || owner_before_comp(b, wa));
  }
}

template <class L, class R>
static bool owner_equal_comp(const L &l, const R &r) noexcept {
  return l.owner_equal(r);
}

CASE_TEST(rc_ptr, owner_equal) {
  {
    util::memory::strong_rc_ptr<int> p1(new int);
    util::memory::strong_rc_ptr<int> p2(p1);

    CASE_EXPECT_TRUE(owner_equal_comp(p1, p2));
    CASE_EXPECT_TRUE(owner_equal_comp(p2, p1));

    util::memory::strong_rc_ptr<int> p3(new int);

    CASE_EXPECT_TRUE(!owner_equal_comp(p1, p3));
    CASE_EXPECT_TRUE(!owner_equal_comp(p3, p1));

    util::memory::strong_rc_ptr<int> p4;
    util::memory::strong_rc_ptr<int> p5;

    CASE_EXPECT_TRUE(owner_equal_comp(p4, p5));
    CASE_EXPECT_TRUE(owner_equal_comp(p5, p4));

    CASE_EXPECT_TRUE(!owner_equal_comp(p4, p3));
    CASE_EXPECT_TRUE(!owner_equal_comp(p3, p4));

    util::memory::strong_rc_ptr<int> p6(static_cast<int *>(0));

    CASE_EXPECT_TRUE(!owner_equal_comp(p4, p6));
    CASE_EXPECT_TRUE(!owner_equal_comp(p6, p4));

    util::memory::strong_rc_ptr<void> p7(p1);

    CASE_EXPECT_TRUE(owner_equal_comp(p1, p7));
    CASE_EXPECT_TRUE(owner_equal_comp(p7, p1));

    util::memory::strong_rc_ptr<void> p8;

    CASE_EXPECT_TRUE(!owner_equal_comp(p1, p8));
    CASE_EXPECT_TRUE(!owner_equal_comp(p8, p1));

    CASE_EXPECT_TRUE(owner_equal_comp(p4, p8));
    CASE_EXPECT_TRUE(owner_equal_comp(p8, p4));

    util::memory::weak_rc_ptr<int> q1(p1);

    CASE_EXPECT_TRUE(owner_equal_comp(p1, q1));
    CASE_EXPECT_TRUE(owner_equal_comp(q1, p1));

    util::memory::weak_rc_ptr<int> q2(p1);

    CASE_EXPECT_TRUE(owner_equal_comp(q1, q2));
    CASE_EXPECT_TRUE(owner_equal_comp(q2, q1));

    util::memory::weak_rc_ptr<int> q3(p3);

    CASE_EXPECT_TRUE(!owner_equal_comp(p1, q3));
    CASE_EXPECT_TRUE(!owner_equal_comp(q3, p1));

    CASE_EXPECT_TRUE(!owner_equal_comp(q1, q3));
    CASE_EXPECT_TRUE(!owner_equal_comp(q3, q1));

    util::memory::weak_rc_ptr<int> q4;

    CASE_EXPECT_TRUE(owner_equal_comp(p4, q4));
    CASE_EXPECT_TRUE(owner_equal_comp(q4, p4));

    CASE_EXPECT_TRUE(!owner_equal_comp(q1, q4));
    CASE_EXPECT_TRUE(!owner_equal_comp(q4, q1));

    util::memory::weak_rc_ptr<void> q5;

    CASE_EXPECT_TRUE(owner_equal_comp(q4, q5));
    CASE_EXPECT_TRUE(owner_equal_comp(q5, q4));

    util::memory::weak_rc_ptr<void> q7(p7);

    CASE_EXPECT_TRUE(owner_equal_comp(p1, q7));
    CASE_EXPECT_TRUE(owner_equal_comp(q7, p1));

    CASE_EXPECT_TRUE(owner_equal_comp(q1, q7));
    CASE_EXPECT_TRUE(owner_equal_comp(q7, q1));

    p1.reset();
    p2.reset();
    p3.reset();
    p7.reset();

    CASE_EXPECT_TRUE(q1.expired());
    CASE_EXPECT_TRUE(q2.expired());
    CASE_EXPECT_TRUE(q3.expired());
    CASE_EXPECT_TRUE(q7.expired());

    CASE_EXPECT_TRUE(owner_equal_comp(q1, q2));
    CASE_EXPECT_TRUE(owner_equal_comp(q2, q1));

    CASE_EXPECT_TRUE(owner_equal_comp(q1, q7));
    CASE_EXPECT_TRUE(owner_equal_comp(q7, q1));

    CASE_EXPECT_TRUE(!owner_equal_comp(q1, q3));
    CASE_EXPECT_TRUE(!owner_equal_comp(q3, q1));

    CASE_EXPECT_TRUE(!owner_equal_comp(q1, q4));
    CASE_EXPECT_TRUE(!owner_equal_comp(q4, q1));
  }
}

template <class P>
static std::size_t get_owner_hash(const P &l) noexcept {
  return l.owner_hash();
}

CASE_TEST(rc_ptr, owner_hash) {
  util::memory::strong_rc_ptr<int> p1(new int);
  util::memory::strong_rc_ptr<int> p2(p1);

  CASE_EXPECT_EQ(get_owner_hash(p1), get_owner_hash(p2));

  util::memory::strong_rc_ptr<int> p3(new int);

  CASE_EXPECT_NE(get_owner_hash(p1), get_owner_hash(p3));

  util::memory::strong_rc_ptr<int> p4;
  util::memory::strong_rc_ptr<int> p5;

  CASE_EXPECT_EQ(get_owner_hash(p4), get_owner_hash(p5));
  CASE_EXPECT_NE(get_owner_hash(p4), get_owner_hash(p3));

  util::memory::strong_rc_ptr<int> p6(static_cast<int *>(0));

  CASE_EXPECT_NE(get_owner_hash(p4), get_owner_hash(p6));

  util::memory::strong_rc_ptr<void> p7(p1);

  CASE_EXPECT_EQ(get_owner_hash(p1), get_owner_hash(p7));

  util::memory::strong_rc_ptr<void> p8;

  CASE_EXPECT_NE(get_owner_hash(p1), get_owner_hash(p8));
  CASE_EXPECT_EQ(get_owner_hash(p4), get_owner_hash(p8));

  util::memory::weak_rc_ptr<int> q1(p1);

  CASE_EXPECT_EQ(get_owner_hash(p1), get_owner_hash(q1));

  util::memory::weak_rc_ptr<int> q2(p1);

  CASE_EXPECT_EQ(get_owner_hash(q1), get_owner_hash(q2));

  util::memory::weak_rc_ptr<int> q3(p3);

  CASE_EXPECT_NE(get_owner_hash(p1), get_owner_hash(q3));
  CASE_EXPECT_NE(get_owner_hash(q1), get_owner_hash(q3));

  util::memory::weak_rc_ptr<int> q4;

  CASE_EXPECT_EQ(get_owner_hash(p4), get_owner_hash(q4));
  CASE_EXPECT_NE(get_owner_hash(q1), get_owner_hash(q4));

  util::memory::weak_rc_ptr<void> q5;

  CASE_EXPECT_EQ(get_owner_hash(q4), get_owner_hash(q5));

  util::memory::weak_rc_ptr<void> q7(p7);

  CASE_EXPECT_EQ(get_owner_hash(p1), get_owner_hash(q7));
  CASE_EXPECT_EQ(get_owner_hash(q1), get_owner_hash(q7));

  p1.reset();
  p2.reset();
  p3.reset();
  p7.reset();

  CASE_EXPECT_TRUE(q1.expired());
  CASE_EXPECT_TRUE(q2.expired());
  CASE_EXPECT_TRUE(q3.expired());
  CASE_EXPECT_TRUE(q7.expired());

  CASE_EXPECT_EQ(get_owner_hash(q1), get_owner_hash(q2));
  CASE_EXPECT_EQ(get_owner_hash(q1), get_owner_hash(q7));
  CASE_EXPECT_NE(get_owner_hash(q1), get_owner_hash(q3));
  CASE_EXPECT_NE(get_owner_hash(q1), get_owner_hash(q4));
}

namespace shared_from_this {

class X {
 public:
  virtual void f() = 0;

 protected:
  ~X() {}
};

class Y {
 public:
  virtual util::memory::strong_rc_ptr<X> getX() = 0;

 protected:
  ~Y() {}
};

util::memory::strong_rc_ptr<Y> createY();

void test() {
  util::memory::strong_rc_ptr<Y> py = createY();
  CASE_EXPECT_TRUE(py.get() != 0);
  CASE_EXPECT_TRUE(py.use_count() == 1);

#if defined(LIBATFRAME_UTILS_ENABLE_EXCEPTION) && LIBATFRAME_UTILS_ENABLE_EXCEPTION
  try {
    util::memory::strong_rc_ptr<X> px = py->getX();
    CASE_EXPECT_TRUE(px.get() != 0);
    CASE_EXPECT_TRUE(py.use_count() == 2);

    px->f();

#  if defined(LIBATFRAME_UTILS_ENABLE_RTTI) && LIBATFRAME_UTILS_ENABLE_RTTI
    util::memory::strong_rc_ptr<Y> py2 = util::memory::dynamic_pointer_cast<Y>(px);
    CASE_EXPECT_TRUE(py.get() == py2.get());
    CASE_EXPECT_TRUE(!(py < py2 || py2 < py));
    CASE_EXPECT_TRUE(py.use_count() == 3);
#  endif
  } catch (std::bad_weak_ptr const &) {
    CASE_EXPECT_ERROR("py->getX() failed");
  }
#endif
}

void test2();
void test3();

CASE_TEST(rc_ptr, shared_from_this) {
  test();
  test2();
  test3();
}

// virtual inheritance to stress the implementation
// (prevents Y* -> impl*, enable_shared_rc_from_this<impl>* -> impl* casts)

class impl : public X, public virtual Y, public virtual util::memory::enable_shared_rc_from_this<impl> {
 public:
  virtual void f() {}

  virtual util::memory::strong_rc_ptr<X> getX() {
    util::memory::strong_rc_ptr<impl> pi = shared_from_this();
    CASE_EXPECT_TRUE(pi.get() == this);
    return pi;
  }
};

// intermediate impl2 to stress the implementation

class impl2 : public impl {};

util::memory::strong_rc_ptr<Y> createY() {
  util::memory::strong_rc_ptr<Y> pi(new impl2);
  return pi;
}

void test2() { util::memory::strong_rc_ptr<Y> pi(static_cast<impl2 *>(0)); }

//

struct V : public util::memory::enable_shared_rc_from_this<V> {};

void test3() {
  util::memory::strong_rc_ptr<V> p(new V);

#if defined(LIBATFRAME_UTILS_ENABLE_EXCEPTION) && LIBATFRAME_UTILS_ENABLE_EXCEPTION
  try {
    util::memory::strong_rc_ptr<V> q = p->shared_from_this();
    CASE_EXPECT_TRUE(p == q);
    CASE_EXPECT_TRUE(!(p < q) && !(q < p));
  } catch (std::bad_weak_ptr const &) {
    CASE_EXPECT_ERROR("p->shared_from_this() failed");
  }

  V v2(*p);

  try {
    util::memory::strong_rc_ptr<V> r = v2.shared_from_this();
    CASE_EXPECT_ERROR("v2.shared_from_this() failed to throw");
  } catch (std::bad_weak_ptr const &) {
  }

  try {
    *p = V();
    util::memory::strong_rc_ptr<V> r = p->shared_from_this();
    CASE_EXPECT_ERROR("*p = V() failed to throw");
  } catch (std::bad_weak_ptr const &) {
  }
#endif
}

}  // namespace shared_from_this

#if defined(__GNUC__) && !defined(__clang__) && !defined(__apple_build_version__)
#  if (__GNUC__ * 100 + __GNUC_MINOR__ * 10) >= 460
#    pragma GCC diagnostic pop
#  endif
#elif defined(__clang__) || defined(__apple_build_version__)
#  pragma clang diagnostic pop
#endif
