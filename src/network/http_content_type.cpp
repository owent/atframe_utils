#include <cstdlib>
#include <cstring>

#include <common/string_oprs.h>
#include <config/compile_optimize.h>
#include <network/http_content_type.h>

LIBATFRAME_UTILS_NAMESPACE_BEGIN
namespace network {
namespace http_content_type {
LIBATFRAME_UTILS_API const char *get_type(main_type mt) {
  static const char *ret[EN_HCT_MT_MAX] = {nullptr};
  UTIL_UNLIKELY_IF(nullptr == ret[EN_HCT_MT_DISCRETE_TEXT]) {
    ret[EN_HCT_MT_DISCRETE_TEXT] = "text";
    ret[EN_HCT_MT_DISCRETE_IMAGE] = "image";
    ret[EN_HCT_MT_DISCRETE_AUDIO] = "audio";
    ret[EN_HCT_MT_DISCRETE_VIDEO] = "video";
    ret[EN_HCT_MT_DISCRETE_APPLICATION] = "application";

    ret[EN_HCT_MT_COMPOSITE_MESSAGE] = "message";
    ret[EN_HCT_MT_COMPOSITE_MULTIPART] = "multipart";
  }

  if (mt >= EN_HCT_MT_MAX) {
    return nullptr;
  }

  return ret[mt];
}

LIBATFRAME_UTILS_API const char *get_subtype(sub_type st) {
  static const char *ret[EN_HCT_ST_MAX] = {nullptr};
  UTIL_UNLIKELY_IF(nullptr == ret[EN_HCT_ST_TEXT_PLAIN]) {
    ret[EN_HCT_ST_TEXT_PLAIN] = "plain";

    ret[EN_HCT_ST_AUDIO_BASIC] = "basic";

    ret[EN_HCT_ST_APPLICATION_OCTET_STREAM] = "octet-stream";
    ret[EN_HCT_ST_APPLICATION_POSTSCRIPT] = "postscript";
    ret[EN_HCT_ST_APPLICATION_X_WWW_FORM_URLENCODED] = "x-www-form-urlencoded";
    ret[EN_HCT_ST_APPLICATION_MULTIPART_FORM_DATA] = "multipart-formdata";

    ret[EN_HCT_ST_MESSAGE_RFC822] = "rfc822";

    ret[EN_HCT_ST_MULTIPART_MIXED] = "mixed";
    ret[EN_HCT_ST_MULTIPART_DIGEST] = "digest";
    ret[EN_HCT_ST_MULTIPART_ALTERNATIVE] = "alternative";
    ret[EN_HCT_ST_MULTIPART_FORM_DATA] = "form-data";
  }

  if (st >= EN_HCT_ST_MAX) {
    return nullptr;
  }

  return ret[st];
}

LIBATFRAME_UTILS_API int make_content_type(char *dst, size_t dst_sz, const char *type, const char *subtype,
                                           const char *parameter_key[], const char *parameter_value[],
                                           size_t parameter_sz) {
  if (nullptr == dst || dst_sz < 14) {
    return -1;
  }

  if (nullptr == type || nullptr == subtype) {
    return -2;
  }

  int res = UTIL_STRFUNC_SNPRINTF(dst, dst_sz, "Content-Type: %s/%s", type, subtype);
  if (res < 0) {
    return res;
  }

  int used = res;
  if (parameter_sz > 0 && nullptr != parameter_key) {
    for (size_t i = 0; i < parameter_sz && parameter_key[i]; ++i) {
      if (parameter_value && nullptr != parameter_value[i]) {
        res = UTIL_STRFUNC_SNPRINTF(dst + used, static_cast<int>(dst_sz) - used, "; %s=\"%s\"", parameter_key[i],
                                    parameter_value[i]);
      } else {
        res = UTIL_STRFUNC_SNPRINTF(dst + used, static_cast<int>(dst_sz) - used, "; %s=", parameter_key[i]);
      }
      if (res < 0) {
        return res;
      }

      used += res;
    }
  }

  return res;
}

LIBATFRAME_UTILS_API int make_content_type(char *dst, size_t dst_sz, main_type mt, const char *st,
                                           const char *parameter_key[], const char *parameter_value[],
                                           size_t parameter_sz) {
  const char *mt_str = get_type(mt);
  if (nullptr == mt_str) {
    return -11;
  }

  return make_content_type(dst, dst_sz, mt_str, st, parameter_key, parameter_value, parameter_sz);
}

LIBATFRAME_UTILS_API int make_content_type(char *dst, size_t dst_sz, const char *type, sub_type st,
                                           const char *parameter_key[], const char *parameter_value[],
                                           size_t parameter_sz) {
  const char *st_str = get_subtype(st);
  if (nullptr == st_str) {
    return -21;
  }

  return make_content_type(dst, dst_sz, type, st_str, parameter_key, parameter_value, parameter_sz);
}

LIBATFRAME_UTILS_API int make_content_type(char *dst, size_t dst_sz, main_type mt, sub_type st,
                                           const char *parameter_key[], const char *parameter_value[],
                                           size_t parameter_sz) {
  const char *st_str = get_subtype(st);
  if (nullptr == st_str) {
    return -21;
  }

  return make_content_type(dst, dst_sz, mt, st_str, parameter_key, parameter_value, parameter_sz);
}

LIBATFRAME_UTILS_API int make_content_type(char *dst, size_t dst_sz, easy_type et, const char *parameter_key[],
                                           const char *parameter_value[], size_t parameter_sz) {
  switch (et) {
    case EN_HCP_ET_APPLICATION_OCTET_STREAM:
      return make_content_type(dst, dst_sz, EN_HCT_MT_DISCRETE_APPLICATION, EN_HCT_ST_APPLICATION_OCTET_STREAM,
                               parameter_key, parameter_value, parameter_sz);
    case EN_HCP_ET_APPLICATION_X_WWW_FORM_URLENCODED:
      return make_content_type(dst, dst_sz, EN_HCT_MT_DISCRETE_APPLICATION, EN_HCT_ST_APPLICATION_X_WWW_FORM_URLENCODED,
                               parameter_key, parameter_value, parameter_sz);
    case EN_HCT_ET_APPLICATION_MULTIPART_FORM_DATA:
      return make_content_type(dst, dst_sz, EN_HCT_MT_DISCRETE_APPLICATION, EN_HCT_ST_APPLICATION_MULTIPART_FORM_DATA,
                               parameter_key, parameter_value, parameter_sz);
    case EN_HCP_ET_MULTIPART_FORM_DATA:
      return make_content_type(dst, dst_sz, EN_HCT_MT_COMPOSITE_MULTIPART, EN_HCT_ST_MULTIPART_FORM_DATA, parameter_key,
                               parameter_value, parameter_sz);
    default:
      return -31;
  }
}
}  // namespace http_content_type
}  // namespace network
LIBATFRAME_UTILS_NAMESPACE_END
