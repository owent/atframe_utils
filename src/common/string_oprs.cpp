#include <common/string_oprs.h>
#include <sstream>

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace string {
ATFRAMEWORK_UTILS_API gsl::string_view trim_string(gsl::string_view input, bool trim_left, bool trim_right) {
  if (input.empty()) {
    return input;
  }

  auto ret = trim(input.data(), input.size(), trim_left, trim_right);
  return gsl::string_view{ret.first, ret.second};
}

ATFRAMEWORK_UTILS_API const char *version_tok(const char *v, int64_t &out) {
  if (nullptr == v) {
    out = 0;
    return v;
  }

  while (*v && (*v == ' ' || *v == '\t' || *v == '\r' || *v == '\n')) {
    ++v;
  }

  if (*v) {
    out = to_int<int64_t>(v);
  } else {
    out = 0;
  }

  // skip digital
  while (*v && *v != '.' && *v != '-') {
    ++v;
  }

  // skip dot or minus
  if (*v) {
    ++v;
  }

  return v;
}

ATFRAMEWORK_UTILS_API int version_compare(const char *l, const char *r) {
  while ((l && *l) || (r && *r)) {
    int64_t lver = 0;
    int64_t rver = 0;

    l = version_tok(l, lver);
    r = version_tok(r, rver);

    if (lver != rver) {
      return lver < rver ? -1 : 1;
    }
  }

  return 0;
}

ATFRAMEWORK_UTILS_API std::string version_normalize(const char *v) {
  std::stringstream ss;

  bool need_dot = false;
  while (v && *v) {
    if (need_dot) {
      ss << '.';
    } else {
      need_dot = true;
    }

    int64_t n = 0;

    v = version_tok(v, n);
    ss << n;
  }

  std::string ret = ss.str();
  size_t resize = ret.size();
  while (resize > 2 && ret[resize - 2] == '.' && ret[resize - 1] == '0') {
    resize -= 2;
  }
  if (resize != ret.size()) {
    ret.resize(resize);
  }

  if (ret.empty()) {
    ret = "0";
  }

  return ret;
}

}  // namespace string
ATFRAMEWORK_UTILS_NAMESPACE_END

ATFRAMEWORK_UTILS_API_C(const char *) util_string_version_tok(const char *v, int64_t &out) {
  return ATFRAMEWORK_UTILS_NAMESPACE_ID::string::version_tok(v, out);
}

ATFRAMEWORK_UTILS_API_C(int) util_string_version_compare(const char *l, const char *r) {
  return ATFRAMEWORK_UTILS_NAMESPACE_ID::string::version_compare(l, r);
}

ATFRAMEWORK_UTILS_API std::string util_string_version_normalize(const char *v) {
  return ATFRAMEWORK_UTILS_NAMESPACE_ID::string::version_normalize(v);
}
