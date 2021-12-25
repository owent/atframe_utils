/**
 * @file http_content_type.h
 * @brief HTTP Content Type的规范定义和辅助函数的简化实现
 *
 * @see http://www.ietf.org/rfc/rfc2045.txt
 * @see http://www.ietf.org/rfc/rfc2046.txt
 * @see https://www.w3.org/TR/html401/references.html#ref-IANA
 * @version 1.0
 * @author owent
 * @date 2016.11.30
 *
 */

#ifndef UTILS_NETWORK_HTTP_CONTENT_TYPE_H
#define UTILS_NETWORK_HTTP_CONTENT_TYPE_H

#pragma once

#include <stdint.h>
#include <cstddef>

#include <config/atframe_utils_build_feature.h>

LIBATFRAME_UTILS_NAMESPACE_BEGIN
namespace network {
namespace http_content_type {
enum main_type {
  EN_HCT_MT_CUSTOM = 0,
  // discrete type
  EN_HCT_MT_DISCRETE_TEXT,
  EN_HCT_MT_DISCRETE_IMAGE,  // 不识别的图片应该使用application/octet-stream
  EN_HCT_MT_DISCRETE_AUDIO,  // 不识别的音频应该使用application/octet-stream
  EN_HCT_MT_DISCRETE_VIDEO,  // 不识别的视频应该使用application/octet-stream
  EN_HCT_MT_DISCRETE_APPLICATION,
  EN_HCT_MT_DISCRETE_EXTENSION_TOKEN,  // ietf-token / x-token, not support now

  // composite-type
  EN_HCT_MT_COMPOSITE_MESSAGE,
  EN_HCT_MT_COMPOSITE_MULTIPART,
  EN_HCT_MT_COMPOSITE_EXTENSION_TOKEN,  // ietf-token / x-token / iana-token, not support now

  EN_HCT_MT_MAX
};

enum sub_type {
  // discrete type
  EN_HCT_ST_CUSTOM = 0,

  // text
  EN_HCT_ST_TEXT_PLAIN,

  // audio
  EN_HCT_ST_AUDIO_BASIC,

  // application
  EN_HCT_ST_APPLICATION_OCTET_STREAM,           // application/octet-stream
  EN_HCT_ST_APPLICATION_POSTSCRIPT,             // application/postscript
  EN_HCT_ST_APPLICATION_X_WWW_FORM_URLENCODED,  // application/x-www-form-urlencoded
  EN_HCT_ST_APPLICATION_MULTIPART_FORM_DATA,    // application/multipart-formdata

  // message
  EN_HCT_ST_MESSAGE_RFC822,

  // multupart
  EN_HCT_ST_MULTIPART_MIXED,
  EN_HCT_ST_MULTIPART_DIGEST,
  EN_HCT_ST_MULTIPART_ALTERNATIVE,
  EN_HCT_ST_MULTIPART_FORM_DATA,  // multipart/form-data

  EN_HCT_ST_MAX
};

enum easy_type {
  EN_HCP_ET_APPLICATION_OCTET_STREAM = 0,
  EN_HCP_ET_APPLICATION_X_WWW_FORM_URLENCODED,
  EN_HCT_ET_APPLICATION_MULTIPART_FORM_DATA,
  EN_HCP_ET_MULTIPART_FORM_DATA,
  EN_HCP_ET_MAX
};

LIBATFRAME_UTILS_API const char *get_type(main_type mt);

LIBATFRAME_UTILS_API const char *get_subtype(sub_type st);

LIBATFRAME_UTILS_API int make_content_type(char *dst, size_t dst_sz, const char *type, const char *subtype,
                                           const char *parameter_key[], const char *parameter_value[],
                                           size_t parameter_sz);

LIBATFRAME_UTILS_API int make_content_type(char *dst, size_t dst_sz, main_type mt, const char *subtype,
                                           const char *parameter_key[], const char *parameter_value[],
                                           size_t parameter_sz);

LIBATFRAME_UTILS_API int make_content_type(char *dst, size_t dst_sz, const char *type, sub_type st,
                                           const char *parameter_key[], const char *parameter_value[],
                                           size_t parameter_sz);

LIBATFRAME_UTILS_API int make_content_type(char *dst, size_t dst_sz, main_type mt, sub_type st,
                                           const char *parameter_key[], const char *parameter_value[],
                                           size_t parameter_sz);

LIBATFRAME_UTILS_API int make_content_type(char *dst, size_t dst_sz, easy_type et, const char *parameter_key[],
                                           const char *parameter_value[], size_t parameter_sz);
};  // namespace http_content_type
struct LIBATFRAME_UTILS_API http_request_content_type_t {
  enum type {
    EN_HRCT_DEFAULT = 0,                              // auto content type
    EN_HRCT_CUSTOM = 1,                               // custom content type
    EN_HRCT_APPLICATION_X_WWW_FORM_URLENCODED = 100,  // application/x-www-form-urlencoded
    EN_HRCT_MULTIPART_FORM_DATA = 200,                // multipart/form-data

    EN_HRCT_MAX
  };
};

struct LIBATFRAME_UTILS_API http_response_content_type_t {
  enum type {};
};
}  // namespace network
LIBATFRAME_UTILS_NAMESPACE_END
#endif
