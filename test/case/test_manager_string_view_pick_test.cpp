// Copyright 2026 atframework

#include <array>
#include <initializer_list>
#include <string>
#include <type_traits>
#include <vector>

#include "frame/test_macros.h"
#include "frame/test_manager.h"
#include "nostd/string_view.h"

namespace {

template <class T>
struct tm_picked_sv {
  using type = typename test_manager::try_pick_basic_string_view<T>::type;
};

struct tm_custom_char_buffer {
  using value_type = char;
  char buf[3];

  const char *data() const { return buf; }
  size_t size() const { return 3; }
};

}  // namespace

CASE_TEST(test_manager, pick_basic_string_view_from_string_like) {
  {
    using picked = tm_picked_sv<std::string>::type;
    static_assert(std::is_same<picked, ::atfw::util::nostd::basic_string_view<char>>::value,
                  "std::string should map to basic_string_view<char>");
  }

  {
    using picked = tm_picked_sv<::atfw::util::nostd::string_view>::type;
    static_assert(std::is_same<picked, ::atfw::util::nostd::basic_string_view<char>>::value,
                  "nostd::string_view should map to basic_string_view<char>");
  }

  {
    // From data()/size() container
    std::vector<char> buf;
    buf.push_back('a');
    buf.push_back('b');
    buf.push_back('c');

    using conv_t =
        test_manager::try_convert_to_string_view<std::vector<char> &,
                                                 test_manager::try_pick_basic_string_view<std::vector<char> &>::value>;
    using picked = typename conv_t::value_type;
    static_assert(std::is_same<picked, ::atfw::util::nostd::basic_string_view<char>>::value,
                  "vector<char> should map to basic_string_view<char>");

    auto sv = conv_t::pick(buf);
    CASE_EXPECT_EQ(static_cast<size_t>(3), sv.size());
    CASE_EXPECT_EQ('a', sv[0]);
    CASE_EXPECT_EQ('b', sv[1]);
    CASE_EXPECT_EQ('c', sv[2]);
  }

  {
    // From data()/size() array
    std::array<char, 4> arr{{'x', 'y', 'z', 0}};
    using conv_t = test_manager::try_convert_to_string_view<
        std::array<char, 4> &, test_manager::try_pick_basic_string_view<std::array<char, 4> &>::value>;
    auto sv = conv_t::pick(arr);
    CASE_EXPECT_EQ(static_cast<size_t>(4), sv.size());
    CASE_EXPECT_EQ('x', sv[0]);
    CASE_EXPECT_EQ('y', sv[1]);
    CASE_EXPECT_EQ('z', sv[2]);
  }

#ifdef __cpp_unicode_characters
  {
    using picked = tm_picked_sv<std::u16string>::type;
    static_assert(std::is_same<picked, ::atfw::util::nostd::basic_string_view<char16_t>>::value,
                  "std::u16string should map to basic_string_view<char16_t>");

    std::u16string s = u"hi";
    using conv_t =
        test_manager::try_convert_to_string_view<std::u16string &,
                                                 test_manager::try_pick_basic_string_view<std::u16string &>::value>;
    auto sv = conv_t::pick(s);
    CASE_EXPECT_EQ(static_cast<size_t>(2), sv.size());
    CASE_EXPECT_EQ(static_cast<uint32_t>('h'), static_cast<uint32_t>(sv[0]));
    CASE_EXPECT_EQ(static_cast<uint32_t>('i'), static_cast<uint32_t>(sv[1]));
  }

  {
    using picked = tm_picked_sv<std::u32string>::type;
    static_assert(std::is_same<picked, ::atfw::util::nostd::basic_string_view<char32_t>>::value,
                  "std::u32string should map to basic_string_view<char32_t>");
  }
#endif

#ifdef __cpp_char8_t
  {
    using picked = tm_picked_sv<std::u8string>::type;
    static_assert(std::is_same<picked, ::atfw::util::nostd::basic_string_view<char8_t>>::value,
                  "std::u8string should map to basic_string_view<char8_t>");

    std::u8string s = u8"ok";
    using conv_t =
        test_manager::try_convert_to_string_view<std::u8string &,
                                                 test_manager::try_pick_basic_string_view<std::u8string &>::value>;
    auto sv = conv_t::pick(s);
    CASE_EXPECT_EQ(static_cast<size_t>(2), sv.size());
  }
#endif
}

CASE_TEST(test_manager, convert_string_view_from_pointers_and_arrays) {
  const char *cstr1 = "hello";
  const char *cstr2 = "world";

  // Make sure we compare content rather than pointer addresses.
  CASE_EXPECT_EQ(cstr1, std::string("hello"));
  CASE_EXPECT_NE(cstr1, cstr2);
  CASE_EXPECT_EQ(std::string("hello"), cstr1);

  // Array types should be supported too.
  const char carr1[] = "abc";
  const char carr2[] = "abc";
  const char carr3[] = "abd";
  CASE_EXPECT_EQ(carr1, carr2);
  CASE_EXPECT_NE(carr1, carr3);

  // Ensure try_pick_basic_string_view works for const char* and arrays.
  {
    using picked_ptr = tm_picked_sv<const char *>::type;
    static_assert(std::is_same<picked_ptr, ::atfw::util::nostd::basic_string_view<char>>::value,
                  "const char* should map to basic_string_view<char>");
  }

  {
    using picked_arr = tm_picked_sv<decltype(carr1)>::type;
    static_assert(std::is_same<picked_arr, ::atfw::util::nostd::basic_string_view<char>>::value,
                  "const char[N] should map to basic_string_view<char>");
  }
}

CASE_TEST(test_manager, convert_string_view_from_initializer_list_and_custom_data_size) {
  // initializer_list is not directly convertible to string_view, but should be constructible via data()/size().
  std::initializer_list<char> il = {'a', 'b', 'c'};

  using conv_il_t = test_manager::try_convert_to_string_view<
      std::initializer_list<char> &, test_manager::try_pick_basic_string_view<std::initializer_list<char> &>::value>;
  using picked_il = typename conv_il_t::value_type;
  static_assert(std::is_same<picked_il, ::atfw::util::nostd::basic_string_view<char>>::value,
                "initializer_list<char> should map to basic_string_view<char>");

  auto sv_il = conv_il_t::pick(il);
  CASE_EXPECT_EQ(static_cast<size_t>(3), sv_il.size());
  CASE_EXPECT_EQ('a', sv_il[0]);
  CASE_EXPECT_EQ('b', sv_il[1]);
  CASE_EXPECT_EQ('c', sv_il[2]);

  // Note: avoid passing initializer_list into CASE_EXPECT_* directly.
  // The test framework prints operands on failure, and initializer_list is not streamable.
  CASE_EXPECT_EQ(sv_il, ::atfw::util::nostd::string_view{"abc"});
  CASE_EXPECT_NE(sv_il, ::atfw::util::nostd::string_view{"ab"});

  // Custom type: only provides value_type + data()/size().
  tm_custom_char_buffer custom{{'x', 'y', 'z'}};
  using conv_custom_t = test_manager::try_convert_to_string_view<
      tm_custom_char_buffer &, test_manager::try_pick_basic_string_view<tm_custom_char_buffer &>::value>;
  using picked_custom = typename conv_custom_t::value_type;
  static_assert(std::is_same<picked_custom, ::atfw::util::nostd::basic_string_view<char>>::value,
                "custom buffer should map to basic_string_view<char>");

  auto sv_custom = conv_custom_t::pick(custom);
  CASE_EXPECT_EQ(static_cast<size_t>(3), sv_custom.size());
  CASE_EXPECT_EQ('x', sv_custom[0]);
  CASE_EXPECT_EQ('y', sv_custom[1]);
  CASE_EXPECT_EQ('z', sv_custom[2]);
  CASE_EXPECT_EQ(sv_custom, ::atfw::util::nostd::string_view{"xyz"});
}

CASE_TEST(test_manager, convert_string_view_from_wide_types) {
  const wchar_t *wcstr = L"wide";
  std::wstring wstr = L"wide";

  // Direct conversion paths.
  // Note: avoid passing wide types into CASE_EXPECT_* directly.
  // The test framework prints operands on failure, but the logger streams to std::ostream (char),
  // so wchar_t/std::wstring are not safely streamable here.
  {
    using conv_wcstr_t =
        test_manager::try_convert_to_string_view<const wchar_t *,
                                                 test_manager::try_pick_basic_string_view<const wchar_t *>::value>;
    using conv_wstr_t =
        test_manager::try_convert_to_string_view<std::wstring &,
                                                 test_manager::try_pick_basic_string_view<std::wstring &>::value>;

    auto sv_wcstr = conv_wcstr_t::pick(wcstr);
    auto sv_wstr = conv_wstr_t::pick(wstr);

    CASE_EXPECT_EQ(sv_wcstr.size(), sv_wstr.size());
    CASE_EXPECT_EQ(static_cast<size_t>(4), sv_wcstr.size());
    CASE_EXPECT_EQ(static_cast<uint32_t>(L'w'), static_cast<uint32_t>(sv_wcstr[0]));
    CASE_EXPECT_EQ(static_cast<uint32_t>(L'i'), static_cast<uint32_t>(sv_wcstr[1]));
    CASE_EXPECT_EQ(static_cast<uint32_t>(L'd'), static_cast<uint32_t>(sv_wcstr[2]));
    CASE_EXPECT_EQ(static_cast<uint32_t>(L'e'), static_cast<uint32_t>(sv_wcstr[3]));

    CASE_EXPECT_EQ(static_cast<uint32_t>(sv_wcstr[0]), static_cast<uint32_t>(sv_wstr[0]));
    CASE_EXPECT_EQ(static_cast<uint32_t>(sv_wcstr[1]), static_cast<uint32_t>(sv_wstr[1]));
    CASE_EXPECT_EQ(static_cast<uint32_t>(sv_wcstr[2]), static_cast<uint32_t>(sv_wstr[2]));
    CASE_EXPECT_EQ(static_cast<uint32_t>(sv_wcstr[3]), static_cast<uint32_t>(sv_wstr[3]));
  }

  // data()/size() container path.
  std::vector<wchar_t> wbuf;
  wbuf.push_back(L'a');
  wbuf.push_back(L'b');
  wbuf.push_back(L'c');

  using conv_wbuf_t =
      test_manager::try_convert_to_string_view<std::vector<wchar_t> &,
                                               test_manager::try_pick_basic_string_view<std::vector<wchar_t> &>::value>;
  using picked_wbuf = typename conv_wbuf_t::value_type;
  static_assert(std::is_same<picked_wbuf, ::atfw::util::nostd::basic_string_view<wchar_t>>::value,
                "vector<wchar_t> should map to basic_string_view<wchar_t>");

  auto sv_wbuf = conv_wbuf_t::pick(wbuf);
  CASE_EXPECT_EQ(static_cast<size_t>(3), sv_wbuf.size());
  CASE_EXPECT_EQ(static_cast<uint32_t>(L'a'), static_cast<uint32_t>(sv_wbuf[0]));
  CASE_EXPECT_EQ(static_cast<uint32_t>(L'b'), static_cast<uint32_t>(sv_wbuf[1]));
  CASE_EXPECT_EQ(static_cast<uint32_t>(L'c'), static_cast<uint32_t>(sv_wbuf[2]));
}
