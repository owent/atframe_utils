// Copyright 2026 atframework

#include <string>
#include <typeinfo>

#include "common/demangle.h"

#include "frame/test_macros.h"

namespace {
struct demangle_test_struct {};
class demangle_test_class {
 public:
  virtual ~demangle_test_class() {}
};
}  // namespace

CASE_TEST(demangle, demangle_basic) {
  std::string result = atfw::util::demangle(typeid(int).name());
  CASE_EXPECT_FALSE(result.empty());
  // On most platforms, "int" should appear in the demangled name
  CASE_MSG_INFO() << "demangle(int): " << result << std::endl;
}

CASE_TEST(demangle, demangle_struct) {
  std::string result = atfw::util::demangle(typeid(demangle_test_struct).name());
  CASE_EXPECT_FALSE(result.empty());
  CASE_MSG_INFO() << "demangle(demangle_test_struct): " << result << std::endl;
}

CASE_TEST(demangle, demangle_class) {
  std::string result = atfw::util::demangle(typeid(demangle_test_class).name());
  CASE_EXPECT_FALSE(result.empty());
  CASE_MSG_INFO() << "demangle(demangle_test_class): " << result << std::endl;
}

CASE_TEST(demangle, demangle_std_string) {
  std::string result = atfw::util::demangle(typeid(std::string).name());
  CASE_EXPECT_FALSE(result.empty());
  CASE_MSG_INFO() << "demangle(std::string): " << result << std::endl;
}

CASE_TEST(demangle, demangle_alloc_nullptr) {
  // demangle_alloc handles nullptr gracefully
  const char *result = atfw::util::demangle_alloc(nullptr);
  CASE_EXPECT_TRUE(nullptr == result);
}

CASE_TEST(demangle, demangle_empty_string) {
  std::string result = atfw::util::demangle("");
  // Should not crash
}

CASE_TEST(demangle, demangle_alloc_free) {
  const char *name = typeid(int).name();
  const char *demangled = atfw::util::demangle_alloc(name);
  CASE_EXPECT_TRUE(nullptr != demangled);
  if (nullptr != demangled) {
    CASE_MSG_INFO() << "demangle_alloc(int): " << demangled << std::endl;
    atfw::util::demangle_free(demangled);
  }
}

CASE_TEST(demangle, demangle_free_nullptr) {
  // Should not crash
  atfw::util::demangle_free(nullptr);
}

CASE_TEST(demangle, scoped_demangled_name_basic) {
  const char *name = typeid(double).name();
  atfw::util::scoped_demangled_name scoped(name);
  CASE_EXPECT_TRUE(nullptr != scoped.get());
  if (nullptr != scoped.get()) {
    CASE_MSG_INFO() << "scoped_demangled_name(double): " << scoped.get() << std::endl;
  }
}

CASE_TEST(demangle, scoped_demangled_name_move) {
  const char *name = typeid(float).name();
  atfw::util::scoped_demangled_name scoped1(name);
  const char *ptr1 = scoped1.get();
  CASE_EXPECT_TRUE(nullptr != ptr1);

  // Move constructor
  atfw::util::scoped_demangled_name scoped2(std::move(scoped1));
  CASE_EXPECT_TRUE(nullptr != scoped2.get());
  CASE_EXPECT_TRUE(nullptr == scoped1.get());
}

CASE_TEST(demangle, scoped_demangled_name_move_assign) {
  const char *name1 = typeid(int).name();
  const char *name2 = typeid(double).name();

  atfw::util::scoped_demangled_name scoped1(name1);
  atfw::util::scoped_demangled_name scoped2(name2);

  const char *ptr2 = scoped2.get();
  CASE_EXPECT_TRUE(nullptr != ptr2);

  // Move assignment swaps internal pointers
  scoped1 = std::move(scoped2);
  CASE_EXPECT_TRUE(nullptr != scoped1.get());
  // scoped2 now holds scoped1's old pointer (swap semantics)
  CASE_EXPECT_TRUE(nullptr != scoped2.get());
}

CASE_TEST(demangle, scoped_demangled_name_nullptr) {
  atfw::util::scoped_demangled_name scoped(nullptr);
  // Should not crash, get() may return nullptr
}

CASE_TEST(demangle, demangle_template_type) {
  std::string result = atfw::util::demangle(typeid(std::vector<int>).name());
  CASE_EXPECT_FALSE(result.empty());
  CASE_MSG_INFO() << "demangle(vector<int>): " << result << std::endl;
}
