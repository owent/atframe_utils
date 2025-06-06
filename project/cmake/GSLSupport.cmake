include_guard(GLOBAL)

if(NOT TARGET Microsoft.GSL::GSL
   AND NOT TARGET gsl::gsl-lite-v1
   AND NOT TARGET gsl::gsl-lite)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "8")
      project_third_party_include_port("gsl/ms-gsl.cmake")
    endif()
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "10")
      project_third_party_include_port("gsl/ms-gsl.cmake")
    endif()
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "10.3")
      project_third_party_include_port("gsl/ms-gsl.cmake")
    endif()
  elseif(MSVC)
    if(MSVC_VERSION GREATER_EQUAL 1916)
      project_third_party_include_port("gsl/ms-gsl.cmake")
    endif()
  endif()
  if(NOT TARGET Microsoft.GSL::GSL)
    project_third_party_include_port("gsl/gsl-lite.cmake")
  endif()
endif()

if(TARGET Microsoft.GSL::GSL)
  set(ATFRAMEWORK_UTILS_ENABLE_GSL_WITH_MS_GSL ON)
  list(APPEND PROJECT_ATFRAME_UTILS_PUBLIC_LINK_NAMES Microsoft.GSL::GSL)
endif()

if(TARGET gsl::gsl-lite-v1)
  set(ATFRAMEWORK_UTILS_ENABLE_GSL_WITH_GSL_LITE ON)
  list(APPEND PROJECT_ATFRAME_UTILS_PUBLIC_LINK_NAMES gsl::gsl-lite-v1)
elseif(TARGET gsl::gsl-lite)
  set(ATFRAMEWORK_UTILS_ENABLE_GSL_WITH_GSL_LITE ON)
  list(APPEND PROJECT_ATFRAME_UTILS_PUBLIC_LINK_NAMES gsl::gsl-lite)
endif()
project_build_tools_patch_default_imported_config(Microsoft.GSL::GSL gsl::gsl-lite-v1 gsl::gsl-lite)

set(ATFRAMEWORK_UTILS_TEST_GSL_SUPPORT_BACKUP_CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES})
set(CMAKE_REQUIRED_LIBRARIES ${ATFRAMEWORK_CMAKE_TOOLSET_SYSTEM_LINKS})

check_cxx_source_compiles(
  "
#include <iostream>
#include <string_view>
int main() {
std::string_view unicode[] {\"▀▄─\", \"▄▀─\", \"▀─▄\", \"▄─▀\"};

for (int y{}, p{}; y != 6; ++y, p = ((p + 1) % 4)) {
  for (int x{}; x != 16; ++x)
    std::cout << unicode[p];
  std::cout << std::endl;
}
return 0;
}"
  ATFRAMEWORK_UTILS_GSL_TEST_STL_STRING_VIEW)
set(ATFRAMEWORK_UTILS_ENABLE_GSL_STRING_VIEW_FROM_STD ${ATFRAMEWORK_UTILS_ENABLE_CXX_GSL_STD_STRING_VIEW})

if(NOT TARGET Microsoft.GSL::GSL AND NOT TARGET gsl::gsl-lite)
  check_cxx_source_compiles(
    "
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <span>
template<class T, std::size_t N> [[nodiscard]]
constexpr auto slide(std::span<T,N> s, std::size_t offset, std::size_t width) {
  return s.subspan(offset, offset + width <= s.size() ? width : 0U);
}
int main() {
  constexpr int a[] { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
  constexpr int b[] { 8, 7, 6 };
  for (std::size_t offset{}; ; ++offset) {
    constexpr std::size_t width{6};
    auto s = slide(std::span{a}, offset, width);
    if (s.empty())
      break;
  }
  return 0;
}"
    ATFRAMEWORK_UTILS_GSL_TEST_FALLBACK_STL_SPAN)
  check_cxx_source_compiles(
    "
#include <iostream>
#include <cstddef>

int main() {
  std::byte b{42};
  return 0;
}"
    ATFRAMEWORK_UTILS_GSL_TEST_FALLBACK_STL_BYTE)
  if(ATFRAMEWORK_UTILS_GSL_TEST_STL_STRING_VIEW AND ATFRAMEWORK_UTILS_GSL_TEST_FALLBACK_STL_SPAN)
    set(ATFRAMEWORK_UTILS_ENABLE_GSL_WITH_FALLBACK_STL ON)
  endif()
endif()

set(CMAKE_REQUIRED_LIBRARIES ${ATFRAMEWORK_UTILS_TEST_GSL_SUPPORT_BACKUP_CMAKE_REQUIRED_LIBRARIES})
unset(ATFRAMEWORK_UTILS_TEST_GSL_SUPPORT_BACKUP_CMAKE_REQUIRED_LIBRARIES)
