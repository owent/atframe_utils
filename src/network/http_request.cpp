#include <assert.h>
#include <cstring>

#include <std/explicit_declare.h>

#include <common/file_system.h>
#include <string/tquerystring.h>

// #include <log/log_wrapper.h>

#include "network/http_request.h"

#if defined(ATFRAMEWORK_UTILS_NETWORK_EVPOLL_ENABLE_LIBUV) && defined(ATFRAMEWORK_UTILS_NETWORK_ENABLE_CURL)
#  if ATFRAMEWORK_UTILS_NETWORK_ENABLE_CURL && ATFRAMEWORK_UTILS_NETWORK_EVPOLL_ENABLE_LIBUV

#    if defined(UTIL_CONFIG_COMPILER_CXX_STATIC_ASSERT) && UTIL_CONFIG_COMPILER_CXX_STATIC_ASSERT
#      include <type_traits>
#      if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201402L)) ||                       \
          (defined(__cplusplus) && __cplusplus >= 201402L &&                        \
           !(!defined(__clang__) && defined(__GNUC__) && defined(__GNUC_MINOR__) && \
             __GNUC__ * 100 + __GNUC_MINOR__ <= 409))
UTIL_CONFIG_STATIC_ASSERT(
    std::is_trivially_copyable<ATFRAMEWORK_UTILS_NAMESPACE_ID::network::http_request::curl_poll_context_t>::value);
#      elif (defined(__cplusplus) && __cplusplus >= 201103L) || ((defined(_MSVC_LANG) && _MSVC_LANG >= 201103L))
UTIL_CONFIG_STATIC_ASSERT(
    std::is_trivial<ATFRAMEWORK_UTILS_NAMESPACE_ID::network::http_request::curl_poll_context_t>::value);
#      else
UTIL_CONFIG_STATIC_ASSERT(
    std::is_pod<ATFRAMEWORK_UTILS_NAMESPACE_ID::network::http_request::curl_poll_context_t>::value);
#      endif
#    endif

#    define CHECK_FLAG(f, v) !!((f) & static_cast<uint32_t>(v))
#    define SET_FLAG(f, v) (f) |= static_cast<uint32_t>(v)
#    define UNSET_FLAG(f, v) (f) &= (~static_cast<uint32_t>(v))

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace network {
namespace detail {
/* initialize custom header list (stating that Expect: 100-continue is not wanted */
static constexpr const char custom_no_expect_header[] = "Expect:";
static constexpr const char content_type_multipart_post[] = "Content-Type: application/x-www-form-urlencoded";
// static constexpr const char content_type_multipart_form_data[] = "Content-Type: multipart/form-data";
}  // namespace detail

ATFRAMEWORK_UTILS_API http_request::ptr_t http_request::create(curl_multi_context *curl_multi, gsl::string_view url) {
  ptr_t ret = create(curl_multi);
  if (ret) {
    ret->set_url(url);
  }

  return ret;
}

ATFRAMEWORK_UTILS_API http_request::ptr_t http_request::create(curl_multi_context *curl_multi) {
  ptr_t ret = std::make_shared<http_request>(curl_multi, nullptr);
  if (ret->mutable_request()) {
    return ret;
  }

  return ptr_t();
}

ATFRAMEWORK_UTILS_API http_request::ptr_t http_request::create(const curl_share_context_ptr_type &share_context,
                                                               gsl::string_view url) {
  ptr_t ret = create(share_context);
  if (ret) {
    ret->set_url(url);
  }

  return ret;
}

ATFRAMEWORK_UTILS_API http_request::ptr_t http_request::create(const curl_share_context_ptr_type &share_context) {
  ptr_t ret = std::make_shared<http_request>(nullptr, share_context);
  if (ret->mutable_request()) {
    return ret;
  }

  return ptr_t();
}

ATFRAMEWORK_UTILS_API int http_request::get_status_code_group(int code) { return code / 100; }

ATFRAMEWORK_UTILS_API http_request::http_request(curl_multi_context *curl_multi,
                                                 const curl_share_context_ptr_type &share_context)
    : timeout_ms_(0),
      connect_timeout_ms_(0),
      bind_multi_(curl_multi),
      request_(nullptr),
      flags_(0),
      share_context_(share_context),
      response_code_(0),
      last_error_code_(0),
      priv_data_(nullptr) {
#    if LIBCURL_VERSION_NUM >= 0x073800
  http_form_.multipart = nullptr;
#    else
  http_form_.begin = nullptr;
  http_form_.end = nullptr;
#    endif
  http_form_.headerlist = nullptr;
  http_form_.posted_size = 0;
  http_form_.flags = 0;
  set_user_agent("libcurl");

  if (!share_context_ && curl_multi != nullptr) {
    share_context_ = bind_multi_->share_context_;
  }
}

ATFRAMEWORK_UTILS_API http_request::~http_request() { cleanup(); }

ATFRAMEWORK_UTILS_API int http_request::start(method_t::type method, bool wait) {
  CURL *req = mutable_request();
  if (nullptr == req) {
    return -1;
  }

  if (!wait && (nullptr == bind_multi_ || nullptr == bind_multi_->curl_multi_)) {
    return -1;
  }

  if (CHECK_FLAG(flags_, flag_t::EN_FT_CURL_MULTI_HANDLE)) {
    return -1;
  }

  // HTTP Method转换
  switch (method) {
    case method_t::EN_MT_GET:
      curl_easy_setopt(req, CURLOPT_HTTPGET, 1L);
      break;
    case method_t::EN_MT_POST:
      curl_easy_setopt(req, CURLOPT_POST, 1L);
      break;
    case method_t::EN_MT_PUT: {
#    if LIBCURL_VERSION_NUM < 0x075700
      curl_easy_setopt(req, CURLOPT_PUT, 1L);
#    endif
      curl_easy_setopt(req, CURLOPT_UPLOAD, 1L);
      break;
    }
    case method_t::EN_MT_DELETE:
      curl_easy_setopt(req, CURLOPT_CUSTOMREQUEST, "DELETE");
      break;
    case method_t::EN_MT_TRACE:
      curl_easy_setopt(req, CURLOPT_CUSTOMREQUEST, "TRACE");
      break;
    default:
      break;
  }

  last_error_code_ = CURLE_OK;
  // 构建提交表单缓存，维持生命周期
  build_http_form(method);

  // 常见选项的跨版本兼容性适配
#    if LIBCURL_VERSION_NUM >= 0x073800
  if (nullptr != http_form_.multipart) {
#    else
  if (nullptr != http_form_.begin) {
#    endif
    if (0 == (http_form_.flags & form_list_t::EN_FLFT_LIBCURL_ALLOW_EXPECT_100_CONTINUE)) {
      set_libcurl_no_expect();
    }
#    if LIBCURL_VERSION_NUM >= 0x073800
    curl_easy_setopt(req, CURLOPT_MIMEPOST, http_form_.multipart);
#    else
    curl_easy_setopt(req, CURLOPT_HTTPPOST, http_form_.begin);
#    endif
    // curl_easy_setopt(req, CURLOPT_VERBOSE, 1L);
  }
  if (!post_data_.empty()) {
    set_opt_long(CURLOPT_POSTFIELDSIZE, post_data_.size());
    curl_easy_setopt(req, CURLOPT_POSTFIELDS, post_data_.c_str());
    // curl_easy_setopt(req, CURLOPT_COPYPOSTFIELDS, post_data_.c_str());
  }

  if (nullptr != http_form_.headerlist) {
    curl_easy_setopt(req, CURLOPT_HTTPHEADER, http_form_.headerlist);
  }

  if (http_form_.flags & form_list_t::EN_FLFT_WRITE_FORM_USE_FUNC) {
    curl_easy_setopt(req, CURLOPT_POSTFIELDS, nullptr);
    curl_easy_setopt(req, CURLOPT_READFUNCTION, curl_callback_on_read);
    curl_easy_setopt(req, CURLOPT_READDATA, this);

#    if LIBCURL_VERSION_NUM >= 0x071700
    using infile_size_type = curl_off_t;
#    else
    using infile_size_type = long;
#    endif
    infile_size_type infile_size = static_cast<infile_size_type>(post_data_.size());
#    if LIBCURL_VERSION_NUM >= 0x071700
    curl_easy_setopt(req, CURLOPT_INFILESIZE_LARGE, infile_size);
#    else
    curl_easy_setopt(req, CURLOPT_INFILESIZE, infile_size);
#    endif
  }

  if (timeout_ms_ > 0) {
    if (connect_timeout_ms_ > 0 && connect_timeout_ms_ < timeout_ms_) {
      set_opt_long(CURLOPT_CONNECTTIMEOUT_MS, connect_timeout_ms_);
    } else {
      set_opt_long(CURLOPT_CONNECTTIMEOUT_MS, timeout_ms_);
    }
    set_opt_long(CURLOPT_TIMEOUT_MS, timeout_ms_);
  } else if (connect_timeout_ms_ > 0) {
    set_opt_long(CURLOPT_CONNECTTIMEOUT_MS, connect_timeout_ms_);
  }

  // 绑定到共享的EventLoop驱动层
  if (share_context_ && nullptr != share_context_->get_share_handle()) {
    curl_easy_setopt(req, CURLOPT_SHARE, share_context_->get_share_handle());
  }

  int perform_result;
  // 同时支持同步模式和异步模式
  if (wait) {
    SET_FLAG(flags_, flag_t::EN_FT_RUNNING);
    perform_result = curl_easy_perform(req);
    UNSET_FLAG(flags_, flag_t::EN_FT_RUNNING);
    finish_req_rsp();
  } else {
    SET_FLAG(flags_, flag_t::EN_FT_RUNNING);
    SET_FLAG(flags_, flag_t::EN_FT_CURL_MULTI_HANDLE);
    perform_result = curl_multi_add_handle(bind_multi_->curl_multi_, req);
    if (perform_result != CURLM_OK) {
      UNSET_FLAG(flags_, flag_t::EN_FT_RUNNING);
      cleanup();
    }
  }

  last_error_code_ = perform_result;
  return last_error_code_;
}

ATFRAMEWORK_UTILS_API int http_request::stop() {
  if (nullptr == request_) {
    return -1;
  }

  // if not set on_progress_fn_, setup process function to abort transfer
  // @see https://curl.haxx.se/libcurl/c/CURLOPT_PROGRESSFUNCTION.html
  // @see https://curl.haxx.se/libcurl/c/CURLOPT_XFERINFOFUNCTION.html
  if (!on_progress_fn_) {
    set_opt_bool(CURLOPT_NOPROGRESS, false);
#    if LIBCURL_VERSION_NUM >= 0x072000
    curl_easy_setopt(request_, CURLOPT_XFERINFOFUNCTION, curl_callback_on_progress);
    curl_easy_setopt(request_, CURLOPT_XFERINFODATA, this);
#    else
    curl_easy_setopt(request_, CURLOPT_PROGRESSFUNCTION, curl_callback_on_progress);
    curl_easy_setopt(request_, CURLOPT_PROGRESSDATA, this);
#    endif
  }
  SET_FLAG(flags_, flag_t::EN_FT_STOPING);

  return 0;
}

ATFRAMEWORK_UTILS_API void http_request::set_url(gsl::string_view v) {
  if (nullptr == mutable_request()) {
    return;
  }

  url_ = static_cast<std::string>(v);
  curl_easy_setopt(mutable_request(), CURLOPT_URL, url_.c_str());
}

ATFRAMEWORK_UTILS_API const std::string &http_request::get_url() const { return url_; }

ATFRAMEWORK_UTILS_API void http_request::set_user_agent(gsl::string_view v) {
  if (nullptr == mutable_request()) {
    return;
  }

  useragent_ = static_cast<std::string>(v);
  curl_easy_setopt(mutable_request(), CURLOPT_USERAGENT, url_.c_str());
}

ATFRAMEWORK_UTILS_API const std::string &http_request::get_user_agent() const { return useragent_; }

ATFRAMEWORK_UTILS_API std::string &http_request::post_data() { return post_data_; }
ATFRAMEWORK_UTILS_API const std::string &http_request::post_data() const { return post_data_; }

ATFRAMEWORK_UTILS_API int http_request::get_response_code() const {
  if (0 != response_code_) {
    return response_code_;
  }

  if (nullptr != request_) {
    long rsp_code = 0;
    curl_easy_getinfo(request_, CURLINFO_RESPONSE_CODE, &rsp_code);
    response_code_ = static_cast<int>(rsp_code);
  }

  return response_code_;
}

ATFRAMEWORK_UTILS_API int http_request::get_error_code() const { return last_error_code_; }

ATFRAMEWORK_UTILS_API const char *http_request::get_error_msg() const { return error_buffer_; }

ATFRAMEWORK_UTILS_API std::stringstream &http_request::get_response_stream() { return response_; }
ATFRAMEWORK_UTILS_API const std::stringstream &http_request::get_response_stream() const { return response_; }

ATFRAMEWORK_UTILS_API int http_request::add_form_file(const std::string &fieldname, const char *filename) {
  if (CHECK_FLAG(flags_, flag_t::EN_FT_CLEANING)) {
    return -1;
  }

  if (!file_system::is_exist(filename)) {
    return -1;
  }

#    if LIBCURL_VERSION_NUM >= 0x073800
  curl_mimepart *part = curl_mime_addpart(mutable_multipart());
  if (nullptr == part) {
    return -1;
  }
  int ret = curl_mime_name(part, fieldname.c_str());
  if (ret != CURLE_OK) {
    return ret;
  }
  ret = curl_mime_filedata(part, filename);
#    else
  int ret = curl_formadd(&http_form_.begin, &http_form_.end, CURLFORM_COPYNAME, fieldname.c_str(), CURLFORM_NAMELENGTH,
                         static_cast<long>(fieldname.size()), CURLFORM_FILE, filename, CURLFORM_END);
#    endif
  if (0 == ret) {
    http_form_.flags |= form_list_t::EN_FLFT_HAS_FORM_FILE;
  }
  return ret;
}

ATFRAMEWORK_UTILS_API int http_request::add_form_file(const std::string &fieldname, const char *filename,
                                                      const char *content_type, const char *new_filename) {
  if (CHECK_FLAG(flags_, flag_t::EN_FT_CLEANING)) {
    return -1;
  }

  if (!file_system::is_exist(filename)) {
    return -1;
  }

#    if LIBCURL_VERSION_NUM >= 0x073800
  curl_mimepart *part = curl_mime_addpart(mutable_multipart());
  if (nullptr == part) {
    return -1;
  }
  int ret = curl_mime_name(part, fieldname.c_str());
  if (ret != CURLE_OK) {
    return ret;
  }
  ret = curl_mime_filedata(part, filename);
  curl_mime_type(part, content_type);
  curl_mime_filename(part, new_filename);
#    else
  int ret = curl_formadd(&http_form_.begin, &http_form_.end, CURLFORM_COPYNAME, fieldname.c_str(), CURLFORM_NAMELENGTH,
                         static_cast<long>(fieldname.size()), CURLFORM_FILE, filename, CURLFORM_CONTENTTYPE,
                         content_type, CURLFORM_FILENAME, new_filename, CURLFORM_END);
#    endif
  if (0 == ret) {
    http_form_.flags |= form_list_t::EN_FLFT_HAS_FORM_FILE;
  }
  return ret;
}

ATFRAMEWORK_UTILS_API int http_request::add_form_file(const std::string &fieldname, const char *filename,
                                                      const char *content_type) {
  if (CHECK_FLAG(flags_, flag_t::EN_FT_CLEANING)) {
    return -1;
  }

  if (!file_system::is_exist(filename)) {
    return -1;
  }

#    if LIBCURL_VERSION_NUM >= 0x073800
  curl_mimepart *part = curl_mime_addpart(mutable_multipart());
  if (nullptr == part) {
    return -1;
  }
  int ret = curl_mime_name(part, fieldname.c_str());
  if (ret != CURLE_OK) {
    return ret;
  }
  ret = curl_mime_filedata(part, filename);
  curl_mime_type(part, content_type);
#    else
  int ret = curl_formadd(&http_form_.begin, &http_form_.end, CURLFORM_COPYNAME, fieldname.c_str(), CURLFORM_NAMELENGTH,
                         static_cast<long>(fieldname.size()), CURLFORM_FILE, filename, CURLFORM_CONTENTTYPE,
                         content_type, CURLFORM_END);
#    endif
  if (0 == ret) {
    http_form_.flags |= form_list_t::EN_FLFT_HAS_FORM_FILE;
  }
  return ret;
}

ATFRAMEWORK_UTILS_API int http_request::add_form_field(const std::string &fieldname, const std::string &fieldvalue) {
  if (CHECK_FLAG(flags_, flag_t::EN_FT_CLEANING)) {
    return -1;
  }

  http_form_.qs_fields.set(fieldname, fieldvalue);
  http_form_.flags |= form_list_t::EN_FLFT_HAS_FORM_FIELD;
  // int ret = add_form_field(fieldname, fieldvalue.c_str(), fieldvalue.size());
  return 0;
}

ATFRAMEWORK_UTILS_API void http_request::set_priv_data(void *v) { priv_data_ = v; }
ATFRAMEWORK_UTILS_API void *http_request::get_priv_data() const { return priv_data_; }

ATFRAMEWORK_UTILS_API void http_request::set_opt_bool(CURLoption k, bool v) {
  if (nullptr == mutable_request()) {
    return;
  }

  long val = v ? 1L : 0L;
  curl_easy_setopt(mutable_request(), k, val);
}

ATFRAMEWORK_UTILS_API void http_request::set_opt_string(CURLoption k, const char *v) {
  if (nullptr == mutable_request()) {
    return;
  }

  curl_easy_setopt(mutable_request(), k, v);
}

ATFRAMEWORK_UTILS_API void http_request::set_opt_ssl_verify_peer(bool v) { set_opt_bool(CURLOPT_SSL_VERIFYPEER, v); }

ATFRAMEWORK_UTILS_API void http_request::set_opt_no_signal(bool v) { set_opt_bool(CURLOPT_NOSIGNAL, v); }

ATFRAMEWORK_UTILS_API void http_request::set_opt_follow_location(bool v) { set_opt_bool(CURLOPT_FOLLOWLOCATION, v); }

ATFRAMEWORK_UTILS_API void http_request::set_opt_verbose(bool v) { set_opt_bool(CURLOPT_VERBOSE, v); }

ATFRAMEWORK_UTILS_API void http_request::set_opt_accept_encoding(const char *enc) {
#    if LIBCURL_VERSION_NUM >= 0x071506
  set_opt_string(CURLOPT_ACCEPT_ENCODING, enc);
#    else
  set_opt_string(CURLOPT_ENCODING, enc);
#    endif
}

ATFRAMEWORK_UTILS_API void http_request::set_opt_http_content_decoding(bool v) {
  set_opt_bool(CURLOPT_HTTP_CONTENT_DECODING, v);
}

ATFRAMEWORK_UTILS_API bool http_request::set_opt_keepalive(ATFW_EXPLICIT_UNUSED_ATTR time_t idle,
                                                           ATFW_EXPLICIT_UNUSED_ATTR time_t interval) {
#    if LIBCURL_VERSION_NUM >= 0x071900
  if (0 == idle && 0 == interval) {
    set_opt_bool(CURLOPT_TCP_KEEPALIVE, false);
  } else {
    set_opt_bool(CURLOPT_TCP_KEEPALIVE, true);
  }

  set_opt_long(CURLOPT_TCP_KEEPIDLE, idle);
  set_opt_long(CURLOPT_TCP_KEEPINTVL, interval);

  return true;
#    else
  return false;
#    endif
}

ATFRAMEWORK_UTILS_API void http_request::set_opt_reuse_connection(bool v) {
  if (v) {
    set_opt_long(CURLOPT_FRESH_CONNECT, 0);
    set_opt_long(CURLOPT_FORBID_REUSE, 0);
  } else {
    set_opt_long(CURLOPT_FRESH_CONNECT, 1);
    set_opt_long(CURLOPT_FORBID_REUSE, 1);
  }
}

// void http_request::set_opt_connect_timeout(time_t timeout_ms) {
//    set_opt_long(CURLOPT_CONNECTTIMEOUT_MS, timeout_ms);
//}

// void http_request::set_opt_request_timeout(time_t timeout_ms) {
//    set_opt_long(CURLOPT_TIMEOUT_MS, timeout_ms);
//}

ATFRAMEWORK_UTILS_API void http_request::set_opt_timeout(time_t timeout_ms) {
  timeout_ms_ = timeout_ms;
  // set_opt_connect_timeout(timeout_ms);
  // set_opt_request_timeout(timeout_ms);
}

ATFRAMEWORK_UTILS_API void http_request::set_opt_connect_timeout(time_t timeout_ms) {
  connect_timeout_ms_ = timeout_ms;
}

ATFRAMEWORK_UTILS_API void http_request::set_libcurl_no_expect() {
  if (http_form_.flags & form_list_t::EN_FLFT_LIBCURL_NO_EXPECT) {
    return;
  }
  http_form_.flags |= form_list_t::EN_FLFT_LIBCURL_NO_EXPECT;

  append_http_header(ATFRAMEWORK_UTILS_NAMESPACE_ID::network::detail::custom_no_expect_header);
}

ATFRAMEWORK_UTILS_API void http_request::set_libcurl_allow_expect_100_continue() {
  if (http_form_.flags & form_list_t::EN_FLFT_LIBCURL_ALLOW_EXPECT_100_CONTINUE) {
    return;
  }
  http_form_.flags |= form_list_t::EN_FLFT_LIBCURL_ALLOW_EXPECT_100_CONTINUE;
}

ATFRAMEWORK_UTILS_API void http_request::append_http_header(const char *http_header) {
  http_form_.headerlist = curl_slist_append(http_form_.headerlist, http_header);
}

ATFRAMEWORK_UTILS_API const http_request::on_progress_fn_t &http_request::get_on_progress() const {
  return on_progress_fn_;
}
ATFRAMEWORK_UTILS_API void http_request::set_on_progress(on_progress_fn_t fn) {
  on_progress_fn_ = fn;

  CURL *req = mutable_request();
  if (nullptr == req) {
    return;
  }

  if (on_progress_fn_) {
    set_opt_bool(CURLOPT_NOPROGRESS, false);
#    if LIBCURL_VERSION_NUM >= 0x072000
    curl_easy_setopt(req, CURLOPT_XFERINFOFUNCTION, curl_callback_on_progress);
    curl_easy_setopt(req, CURLOPT_XFERINFODATA, this);
#    else
    curl_easy_setopt(req, CURLOPT_PROGRESSFUNCTION, curl_callback_on_progress);
    curl_easy_setopt(req, CURLOPT_PROGRESSDATA, this);
#    endif
  } else if (false == CHECK_FLAG(flags_, flag_t::EN_FT_STOPING)) {
    set_opt_bool(CURLOPT_NOPROGRESS, true);
#    if LIBCURL_VERSION_NUM >= 0x072000
    curl_easy_setopt(req, CURLOPT_XFERINFOFUNCTION, nullptr);
    curl_easy_setopt(req, CURLOPT_XFERINFODATA, nullptr);
#    else
    curl_easy_setopt(req, CURLOPT_PROGRESSFUNCTION, nullptr);
    curl_easy_setopt(req, CURLOPT_PROGRESSDATA, nullptr);
#    endif
  }
}

ATFRAMEWORK_UTILS_API const http_request::on_header_fn_t &http_request::get_on_header() const { return on_header_fn_; }
ATFRAMEWORK_UTILS_API void http_request::set_on_header(on_header_fn_t fn) {
  on_header_fn_ = fn;

  CURL *req = mutable_request();
  if (nullptr == req) {
    return;
  }

  if (on_header_fn_) {
    curl_easy_setopt(req, CURLOPT_HEADERFUNCTION, curl_callback_on_header);
    curl_easy_setopt(req, CURLOPT_HEADERDATA, this);
  } else {
    curl_easy_setopt(req, CURLOPT_HEADERFUNCTION, nullptr);
    curl_easy_setopt(req, CURLOPT_HEADERDATA, nullptr);
  }
}

ATFRAMEWORK_UTILS_API const http_request::on_success_fn_t &http_request::get_on_success() const {
  return on_success_fn_;
}
ATFRAMEWORK_UTILS_API void http_request::set_on_success(on_success_fn_t fn) { on_success_fn_ = fn; }

ATFRAMEWORK_UTILS_API const http_request::on_error_fn_t &http_request::get_on_error() const { return on_error_fn_; }
ATFRAMEWORK_UTILS_API void http_request::set_on_error(on_error_fn_t fn) { on_error_fn_ = fn; }

ATFRAMEWORK_UTILS_API const http_request::on_complete_fn_t &http_request::get_on_complete() const {
  return on_complete_fn_;
}
ATFRAMEWORK_UTILS_API void http_request::set_on_complete(on_complete_fn_t fn) { on_complete_fn_ = fn; }

ATFRAMEWORK_UTILS_API const http_request::on_write_fn_t &http_request::get_on_write() const { return on_write_fn_; }
ATFRAMEWORK_UTILS_API void http_request::set_on_write(on_write_fn_t fn) { on_write_fn_ = fn; }

ATFRAMEWORK_UTILS_API const http_request::on_verbose_fn_t &http_request::get_on_verbose() const {
  return on_verbose_fn_;
}

ATFRAMEWORK_UTILS_API void http_request::set_on_verbose(on_verbose_fn_t fn) {
  on_verbose_fn_ = fn;

  CURL *req = mutable_request();
  if (nullptr == req) {
    return;
  }

  if (on_verbose_fn_) {
    curl_easy_setopt(req, CURLOPT_DEBUGFUNCTION, curl_callback_on_verbose);
    curl_easy_setopt(req, CURLOPT_DEBUGDATA, this);
    set_opt_verbose(true);
  } else {
    curl_easy_setopt(req, CURLOPT_DEBUGFUNCTION, nullptr);
    curl_easy_setopt(req, CURLOPT_DEBUGDATA, nullptr);
    set_opt_verbose(false);
  }
}

ATFRAMEWORK_UTILS_API bool http_request::is_running() const { return CHECK_FLAG(flags_, flag_t::EN_FT_RUNNING); }

ATFRAMEWORK_UTILS_API void http_request::remove_curl_request() {
  CURL *req = request_;
  request_ = nullptr;
  if (nullptr != req) {
    if (nullptr != bind_multi_ && CHECK_FLAG(flags_, flag_t::EN_FT_CURL_MULTI_HANDLE)) {
      // can not be called inside socket callback
      last_error_code_ = curl_multi_remove_handle(bind_multi_->curl_multi_, req);
      UNSET_FLAG(flags_, flag_t::EN_FT_CURL_MULTI_HANDLE);
    }

    if (share_context_) {
      curl_easy_setopt(req, CURLOPT_SHARE, nullptr);
    }

    curl_easy_cleanup(req);
  }
  UNSET_FLAG(flags_, flag_t::EN_FT_STOPING);
  UNSET_FLAG(flags_, flag_t::EN_FT_RUNNING);
#    if LIBCURL_VERSION_NUM >= 0x073800
  if (nullptr != http_form_.multipart) {
    curl_mime_free(http_form_.multipart);
    http_form_.multipart = nullptr;
  }
#    else
  if (nullptr != http_form_.begin) {
    curl_formfree(http_form_.begin);
    http_form_.begin = nullptr;
    http_form_.end = nullptr;
  }
#    endif
  http_form_.qs_fields.clear();

  if (nullptr != http_form_.headerlist) {
    curl_slist_free_all(http_form_.headerlist);
    http_form_.headerlist = nullptr;
  }

  http_form_.posted_size = 0;
  post_data_.clear();

  http_form_.flags = 0;
}

ATFRAMEWORK_UTILS_API void http_request::cleanup() {
  if (CHECK_FLAG(flags_, flag_t::EN_FT_CLEANING)) {
    return;
  }
  SET_FLAG(flags_, flag_t::EN_FT_CLEANING);

  remove_curl_request();

  UNSET_FLAG(flags_, flag_t::EN_FT_CLEANING);
}

ATFRAMEWORK_UTILS_API void http_request::finish_req_rsp() {
  UNSET_FLAG(flags_, flag_t::EN_FT_RUNNING);

  {
    long rsp_code = 0;
    curl_easy_getinfo(request_, CURLINFO_RESPONSE_CODE, &rsp_code);
    response_code_ = static_cast<int>(rsp_code);
  }

  size_t err_len = strlen(error_buffer_);
  if (err_len > 0) {
    if (on_error_fn_) {
      on_error_fn_(*this);
    }
  } else {
    if (on_success_fn_) {
      on_success_fn_(*this);
    }
  }

  if (on_complete_fn_) {
    on_complete_fn_(*this);
  }
}

ATFRAMEWORK_UTILS_API CURL *http_request::mutable_request() {
  if (nullptr != request_) {
    return request_;
  }

  request_ = curl_easy_init();
  if (nullptr != request_) {
    curl_easy_setopt(request_, CURLOPT_PRIVATE, this);
    curl_easy_setopt(request_, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(request_, CURLOPT_WRITEFUNCTION, curl_callback_on_write);
    curl_easy_setopt(request_, CURLOPT_ERRORBUFFER, error_buffer_);
    error_buffer_[0] = 0;
    error_buffer_[sizeof(error_buffer_) - 1] = 0;
  }

  return request_;
}

#    if LIBCURL_VERSION_NUM >= 0x073800
ATFRAMEWORK_UTILS_API curl_mime *http_request::mutable_multipart() {
  UTIL_UNLIKELY_IF (nullptr == http_form_.multipart) {
    http_form_.multipart = curl_mime_init(mutable_request());
  }

  return http_form_.multipart;
}
#    endif

ATFRAMEWORK_UTILS_API void http_request::build_http_form(method_t::type method) {
  if (method_t::EN_MT_PUT == method) {
    http_form_.posted_size = 0;

    // if using put method, CURLOPT_POST* will have no effect
    // we must use CURLOPT_READFUNCTION to upload the data
    // @see https://curl.haxx.se/libcurl/c/CURLOPT_PUT.html
    // @see https://curl.haxx.se/libcurl/c/CURLOPT_UPLOAD.html
    // @see https://curl.haxx.se/libcurl/c/CURLOPT_READDATA.html
    // @see https://curl.haxx.se/libcurl/c/CURLOPT_READFUNCTION.html
    // @see https://curl.haxx.se/libcurl/c/CURLOPT_INFILESIZE.html
    // @see https://curl.haxx.se/libcurl/c/CURLOPT_INFILESIZE_LARGE.html
    if (0 == (http_form_.flags & form_list_t::EN_FLFT_LIBCURL_ALLOW_EXPECT_100_CONTINUE)) {
      set_libcurl_no_expect();
    }
    if (!http_form_.qs_fields.empty()) {
      append_http_header(ATFRAMEWORK_UTILS_NAMESPACE_ID::network::detail::content_type_multipart_post);
      http_form_.qs_fields.to_string().swap(post_data_);
      http_form_.flags |= form_list_t::EN_FLFT_WRITE_FORM_USE_FUNC;
    } else {
      // Using custom post_data_
      http_form_.flags |= form_list_t::EN_FLFT_WRITE_FORM_USE_FUNC;
    }
  } else if (!http_form_.qs_fields.empty()) {
    if (0 == (http_form_.flags & form_list_t::EN_FLFT_LIBCURL_ALLOW_EXPECT_100_CONTINUE)) {
      set_libcurl_no_expect();
    }

    for (ATFRAMEWORK_UTILS_NAMESPACE_ID::tquerystring::data_const_iterator iter = http_form_.qs_fields.data().begin();
         iter != http_form_.qs_fields.data().end(); ++iter) {
      if (ATFRAMEWORK_UTILS_NAMESPACE_ID::types::ITEM_TYPE_STRING == iter->second->type()) {
#    if defined(ATFRAMEWORK_UTILS_ENABLE_RTTI) && ATFRAMEWORK_UTILS_ENABLE_RTTI
        ATFRAMEWORK_UTILS_NAMESPACE_ID::types::item_string *val =
            dynamic_cast<ATFRAMEWORK_UTILS_NAMESPACE_ID::types::item_string *>(iter->second.get());
#    else
        ATFRAMEWORK_UTILS_NAMESPACE_ID::types::item_string *val =
            static_cast<ATFRAMEWORK_UTILS_NAMESPACE_ID::types::item_string *>(iter->second.get());
#    endif

        if (nullptr != val) {
#    if LIBCURL_VERSION_NUM >= 0x073800
          curl_mimepart *part = curl_mime_addpart(mutable_multipart());
          if (part != nullptr) {
            curl_mime_name(part, iter->first.c_str());
            curl_mime_data(part, val->data().c_str(), val->data().size());
          }
#    else
          curl_formadd(&http_form_.begin, &http_form_.end, CURLFORM_PTRNAME, iter->first.c_str(), CURLFORM_NAMELENGTH,
                       static_cast<long>(iter->first.size()), CURLFORM_PTRCONTENTS, val->data().c_str(),
#      if LIBCURL_VERSION_NUM >= 0x072e00
                       CURLFORM_CONTENTLEN, static_cast<curl_off_t>(val->data().size()),
#      else
                       CURLFORM_CONTENTSLENGTH, static_cast<long>(val->data().size()),
#      endif
                       CURLFORM_END);
#    endif
        }
      } else {
        std::string val = iter->second->to_string();
#    if LIBCURL_VERSION_NUM >= 0x073800
        curl_mimepart *part = curl_mime_addpart(mutable_multipart());
        if (part != nullptr) {
          curl_mime_name(part, iter->first.c_str());
          curl_mime_data(part, val.c_str(), val.size());
        }
#    else
        curl_formadd(&http_form_.begin, &http_form_.end, CURLFORM_PTRNAME, iter->first.c_str(), CURLFORM_NAMELENGTH,
                     static_cast<long>(iter->first.size()), CURLFORM_COPYCONTENTS, val.c_str(), CURLFORM_CONTENTSLENGTH,
                     static_cast<long>(val.size()), CURLFORM_END);
#    endif
      }
    }
  }
}

ATFRAMEWORK_UTILS_API http_request::curl_poll_context_t *http_request::malloc_poll(http_request *req,
                                                                                   curl_socket_t sockfd) {
  if (nullptr == req) {
    abort();
  }

  assert(req->bind_multi_);
  assert(req->bind_multi_->ev_loop_);

  curl_poll_context_t *ret = reinterpret_cast<curl_poll_context_t *>(malloc(sizeof(curl_poll_context_t)));
  if (nullptr == ret) {
    return ret;
  }

  ret->sockfd = sockfd;
  ret->bind_multi = req->bind_multi_;
  uv_poll_init_socket(req->bind_multi_->ev_loop_, &ret->poll_object, sockfd);
  ret->poll_object.data = ret;
  ret->is_removed = false;

  return ret;
}

ATFRAMEWORK_UTILS_API void http_request::free_poll(curl_poll_context_t *p) { free(p); }

ATFRAMEWORK_UTILS_API void http_request::check_multi_info(CURLM *curl_handle) {
  CURLMsg *message;
  int pending;

  for (message = curl_multi_info_read(curl_handle, &pending); nullptr != message;
       message = curl_multi_info_read(curl_handle, &pending)) {
    switch (message->msg) {
      case CURLMSG_DONE: {
        http_request *req = nullptr;
        curl_easy_getinfo(message->easy_handle, CURLINFO_PRIVATE, &req);
        assert(req);

        if (nullptr != req) {
          http_request::ptr_t req_p = req->shared_from_this();
          req->last_error_code_ = message->data.result;
          // this may cause req not available any more
          req_p->finish_req_rsp();

          // this function will probably call curl_multi_remove_handle
          // which may destroy message, so message can not be used any more
          req_p->remove_curl_request();
        }
        break;
      }
      default:
        break;
    }
  }
}

ATFRAMEWORK_UTILS_API http_request::curl_share_context::curl_share_context(CURLSH *share) noexcept
    : curl_share_(share), enable_lock_(false) {}

ATFRAMEWORK_UTILS_API http_request::curl_share_context::~curl_share_context() {
  std::unordered_map<int32_t, std::shared_ptr<std::mutex>> data_locks;

  if (enable_lock_) {
    std::lock_guard<std::recursive_mutex> guard{global_lock_};
    data_locks.swap(data_locks_);
  }

  if (nullptr != curl_share_) {
    curl_share_setopt(curl_share_, CURLSHOPT_USERDATA, nullptr);
    curl_share_cleanup(curl_share_);
  }

  // Unlock all locks
  for (auto &lock : data_locks) {
    if (lock.second) {
      lock.second->unlock();
    }
  }
}

ATFRAMEWORK_UTILS_API http_request::curl_multi_context::curl_multi_context() noexcept
    : ev_loop_(nullptr), curl_multi_(nullptr) {}

ATFRAMEWORK_UTILS_API int http_request::create_curl_share(const curl_share_options &options,
                                                          std::shared_ptr<curl_share_context> &output) {
  CURLSH *handle = curl_share_init();
  if (nullptr == handle) {
    return -1;
  }

  if (output) {
    output.reset();
  }

  output = std::make_shared<curl_share_context>(handle);
  if (!output) {
    curl_share_cleanup(handle);
    return -1;
  }
  curl_share_setopt(handle, CURLSHOPT_USERDATA, output.get());

  if (options.enable_multi_thread) {
    curl_share_setopt(handle, CURLSHOPT_LOCKFUNC, curl_share_callback_on_lock);
    curl_share_setopt(handle, CURLSHOPT_UNLOCKFUNC, curl_share_callback_on_unlock);
    output->enable_lock_ = true;
  }

  if (options.share_cookie) {
    curl_share_setopt(handle, CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);
  } else {
    curl_share_setopt(handle, CURLSHOPT_UNSHARE, CURL_LOCK_DATA_COOKIE);
  }

  if (options.share_dns) {
    curl_share_setopt(handle, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
  } else {
    curl_share_setopt(handle, CURLSHOPT_UNSHARE, CURL_LOCK_DATA_DNS);
  }

#    if LIBCURL_VERSION_NUM >= 0x071700
  if (options.share_ssl_session) {
    curl_share_setopt(handle, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION);
  } else {
    curl_share_setopt(handle, CURLSHOPT_UNSHARE, CURL_LOCK_DATA_SSL_SESSION);
  }
#    endif

#    if LIBCURL_VERSION_NUM >= 0x073900
  // Note that when you use the multi interface, all easy handles added to the same multi handle will share connection
  //   cache by default without using this option. So this option can be set unshare only when disabled.
  if (!options.share_connection) {
    curl_share_setopt(handle, CURLSHOPT_UNSHARE, CURL_LOCK_DATA_CONNECT);
  }
#    endif

#    if LIBCURL_VERSION_NUM >= 0x073d00
  if (options.share_psl) {
    curl_share_setopt(handle, CURLSHOPT_SHARE, CURL_LOCK_DATA_PSL);
  } else {
    curl_share_setopt(handle, CURLSHOPT_UNSHARE, CURL_LOCK_DATA_PSL);
  }
#    endif

#    if LIBCURL_VERSION_NUM >= 0x075800
  if (options.share_hsts) {
    curl_share_setopt(handle, CURLSHOPT_SHARE, CURL_LOCK_DATA_HSTS);
  } else {
    curl_share_setopt(handle, CURLSHOPT_UNSHARE, CURL_LOCK_DATA_HSTS);
  }
#    endif

  return 0;
}

ATFRAMEWORK_UTILS_API int http_request::create_curl_multi(uv_loop_t *evloop,
                                                          std::shared_ptr<curl_multi_context> &manager) {
  return create_curl_multi(curl_multi_options{evloop, nullptr}, manager);
}

ATFRAMEWORK_UTILS_API int http_request::create_curl_multi(const curl_multi_options &options,
                                                          std::shared_ptr<curl_multi_context> &manager) {
  uv_loop_t *evloop = options.ev_loop;
  if (evloop == nullptr) {
    evloop = uv_default_loop();
  }

  if (manager) {
    destroy_curl_multi(manager);
  }
  manager = std::make_shared<curl_multi_context>();
  if (!manager) {
    return -1;
  }

  manager->curl_multi_ = curl_multi_init();
  if (nullptr == manager->curl_multi_) {
    manager.reset();
    return -1;
  }

  manager->ev_loop_ = evloop;
  if (0 != uv_timer_init(evloop, &manager->ev_timeout_)) {
    curl_multi_cleanup(manager->curl_multi_);
    manager.reset();
    return -1;
  }
  manager->ev_timeout_.data = manager.get();

  manager->share_context_ = options.share_context;

  int ret = curl_multi_setopt(manager->curl_multi_, CURLMOPT_SOCKETFUNCTION, http_request::curl_callback_handle_socket);
  ret = (CURLE_OK != ret) || curl_multi_setopt(manager->curl_multi_, CURLMOPT_SOCKETDATA, manager.get());
  ret = (CURLE_OK != ret) ||
        curl_multi_setopt(manager->curl_multi_, CURLMOPT_TIMERFUNCTION, http_request::curl_callback_start_timer);
  ret = (CURLE_OK != ret) || curl_multi_setopt(manager->curl_multi_, CURLMOPT_TIMERDATA, manager.get());

  return ret;
}

ATFRAMEWORK_UTILS_API int http_request::destroy_curl_multi(std::shared_ptr<curl_multi_context> &manager) {
  if (!manager) {
    return 0;
  }

  assert(manager->ev_loop_);
  assert(manager->curl_multi_);

  int ret = curl_multi_cleanup(manager->curl_multi_);
  manager->curl_multi_ = nullptr;

  // hold self in case of timer in libuv invalid
  manager->self_holder_ = manager;
  uv_timer_stop(&manager->ev_timeout_);
  uv_close(reinterpret_cast<uv_handle_t *>(&manager->ev_timeout_), http_request::ev_callback_on_timer_closed);

  manager.reset();
  return ret;
}

ATFRAMEWORK_UTILS_API void http_request::ev_callback_on_timer_closed(uv_handle_t *handle) {
  curl_multi_context *bind = reinterpret_cast<curl_multi_context *>(handle->data);
  assert(bind);

  // release self holder
  if (nullptr != bind) {
    bind->self_holder_.reset();
  }
}

ATFRAMEWORK_UTILS_API void http_request::ev_callback_on_poll_closed(uv_handle_t *handle) {
  curl_poll_context_t *context = reinterpret_cast<curl_poll_context_t *>(handle->data);
  assert(context);

  free_poll(context);
}

ATFRAMEWORK_UTILS_API void http_request::ev_callback_on_timeout(uv_timer_t *handle) {
  curl_multi_context *bind = reinterpret_cast<curl_multi_context *>(handle->data);
  assert(bind);
  if (nullptr == bind) {
    return;
  }

  int running_handles = 0;
  curl_multi_socket_action(bind->curl_multi_, CURL_SOCKET_TIMEOUT, 0, &running_handles);
  check_multi_info(bind->curl_multi_);
}

ATFRAMEWORK_UTILS_API void http_request::ev_callback_curl_perform(uv_poll_t *req, int, int events) {
  assert(req && req->data);
  if (nullptr == req || nullptr == req->data) {
    return;
  }
  curl_poll_context_t *context = reinterpret_cast<curl_poll_context_t *>(req->data);
  assert(context);

  int running_handles;
  int flags = 0;

  // WLOGWARNING(" ====== [HTTP] ====== sock %d evpoll %d for %p start", context->sockfd, events, context);
  if (false == context->is_removed) {
    if (events & UV_READABLE) {
      flags |= CURL_CSELECT_IN;
    }
    if (events & UV_WRITABLE) {
      flags |= CURL_CSELECT_OUT;
    }

    curl_multi_socket_action(context->bind_multi->curl_multi_, context->sockfd, flags, &running_handles);
  }
  // WLOGWARNING(" ====== [HTTP] ====== sock %d evpoll %d for %p after socket action", context->sockfd, events,
  // context);
  check_multi_info(context->bind_multi->curl_multi_);
  // WLOGWARNING(" ====== [HTTP] ====== sock %d evpoll %d for %p done", context->sockfd, events, context);
}

ATFRAMEWORK_UTILS_API int http_request::curl_callback_start_timer(CURLM *, long timeout_ms, void *userp) {
  curl_multi_context *bind = reinterpret_cast<curl_multi_context *>(userp);
  assert(bind);

  // @see https://curl.haxx.se/libcurl/c/evhiperfifo.html
  // @see https://gist.github.com/clemensg/5248927
  // @see https://curl.haxx.se/libcurl/c/multi-uv.html
  if (timeout_ms < 0) {
    uv_timer_stop(&bind->ev_timeout_);
  } else {
    if (timeout_ms == 0) {
      timeout_ms = 1;
    }

    uv_timer_start(&bind->ev_timeout_, http_request::ev_callback_on_timeout, static_cast<uint64_t>(timeout_ms), 0);
  }

  return 0;
}

ATFRAMEWORK_UTILS_API int http_request::curl_callback_handle_socket(CURL *easy, curl_socket_t s, int action, void *,
                                                                    void *socketp) {
  curl_poll_context_t *context = reinterpret_cast<curl_poll_context_t *>(socketp);
  if (action == CURL_POLL_IN || action == CURL_POLL_OUT || action == CURL_POLL_INOUT) {
    if (nullptr == context) {
      http_request *req;
      curl_easy_getinfo(easy, CURLINFO_PRIVATE, &req);
      if (nullptr != req && nullptr != req->bind_multi_) {
        assert(req->bind_multi_);
        assert(req->bind_multi_->curl_multi_);

        context = malloc_poll(req, s);
        req->last_error_code_ = curl_multi_assign(req->bind_multi_->curl_multi_, s, context);
      }
    }
  }

  if (nullptr == context) {
    return 0;
  }

  int res = 0;
  switch (action) {
    case CURL_POLL_IN: {
      // WLOGWARNING(" ====== [HTTP] ====== sock %d poll in %p", s, context);
      res = uv_poll_start(&context->poll_object, UV_READABLE, ev_callback_curl_perform);
      break;
    }
    case CURL_POLL_OUT: {
      // WLOGWARNING(" ====== [HTTP] ====== sock %d poll out %p", s, context);
      res = uv_poll_start(&context->poll_object, UV_WRITABLE, ev_callback_curl_perform);
      break;
    }
    case CURL_POLL_INOUT: {
      // WLOGWARNING(" ====== [HTTP] ====== sock %d poll inout %p", s, context);
      res = uv_poll_start(&context->poll_object, UV_READABLE | UV_WRITABLE, ev_callback_curl_perform);
      break;
    }
    case CURL_POLL_REMOVE: {
      // WLOGWARNING(" ====== [HTTP] ====== sock %d poll remove %p", s, context);
      if (context) {
        CURLM *curl_multi = context->bind_multi->curl_multi_;

        // set removed first, or libuv may call poll callback and cause a coredump
        context->is_removed = true;

        // already removed by libcurl
        uv_poll_stop(&context->poll_object);
        uv_close(reinterpret_cast<uv_handle_t *>(&context->poll_object), ev_callback_on_poll_closed);
        curl_multi_assign(curl_multi, s, nullptr);
      }
      break;
    }
    default: {
      break;
    }
  }

  return res;
}

ATFRAMEWORK_UTILS_API size_t http_request::curl_callback_on_write(char *ptr, size_t size, size_t nmemb,
                                                                  void *userdata) {
  http_request *self = reinterpret_cast<http_request *>(userdata);
  assert(self);
  if (nullptr == self) {
    return 0;
  }

  const char *data = ptr;
  size_t data_len = size * nmemb;
  if (self->on_write_fn_) {
    self->on_write_fn_(*self, data, data_len, data, data_len);
  }

  if (nullptr != data && data_len > 0) {
    self->response_.write(data, static_cast<std::streamsize>(data_len));
  }

  return size * nmemb;
}

#    if LIBCURL_VERSION_NUM >= 0x072000
ATFRAMEWORK_UTILS_API int http_request::curl_callback_on_progress(void *clientp, curl_off_t dltotal, curl_off_t dlnow,
                                                                  curl_off_t ultotal, curl_off_t ulnow) {
#    else
ATFRAMEWORK_UTILS_API int http_request::curl_callback_on_progress(void *clientp, double dltotal, double dlnow,
                                                                  double ultotal, double ulnow) {
#    endif
  http_request *self = reinterpret_cast<http_request *>(clientp);
  int ret = 0;
  if (nullptr == self) {
    return ret;
  }

  if (self->on_progress_fn_) {
    progress_t progress;
    progress.dltotal = static_cast<size_t>(dltotal);
    progress.dlnow = static_cast<size_t>(dlnow);
    progress.ultotal = static_cast<size_t>(ultotal);
    progress.ulnow = static_cast<size_t>(ulnow);

    ret = self->on_progress_fn_(*self, progress);
  }

  if (0 == ret && CHECK_FLAG(self->flags_, flag_t::EN_FT_STOPING)) {
    ret = -1;
  }

  return ret;
}  // namespace network

ATFRAMEWORK_UTILS_API size_t http_request::curl_callback_on_read(char *buffer, size_t size, size_t nitems,
                                                                 void *instream) {
  http_request *self = reinterpret_cast<http_request *>(instream);
  assert(self);
  if (nullptr == self) {
    return 0;
  }

  if (self->post_data_.size() <= self->http_form_.posted_size) {
    return 0;
  }

  size_t nwrite = size * nitems;
  if (nwrite > self->post_data_.size() - self->http_form_.posted_size) {
    nwrite = self->post_data_.size() - self->http_form_.posted_size;
  }

  memcpy(buffer, self->post_data_.data() + self->http_form_.posted_size, nwrite);
  self->http_form_.posted_size += nwrite;
  return nwrite;
}

ATFRAMEWORK_UTILS_API size_t http_request::curl_callback_on_header(char *buffer, size_t size, size_t nitems,
                                                                   void *userdata) {
  http_request *self = reinterpret_cast<http_request *>(userdata);
  assert(self);

  size_t nwrite = size * nitems;

  while (nwrite > 0) {
    if ('\r' != buffer[nwrite - 1] && '\n' != buffer[nwrite - 1]) {
      break;
    }

    --nwrite;
  }

  const char *key = buffer;
  size_t keylen = 0;
  const char *val = nullptr;

  for (; keylen < nwrite; ++keylen) {
    if (':' == key[keylen]) {
      val = &key[keylen + 1];
      break;
    }
  }

  for (; val && static_cast<size_t>(val - buffer) < nwrite; ++val) {
    if (' ' != *val && '\t' != *val) {
      break;
    }
  }

  if (keylen > 0 && nullptr != self && self->on_header_fn_) {
    if (static_cast<size_t>(val - buffer) < nwrite) {
      self->on_header_fn_(*self, key, keylen, val, nwrite - static_cast<size_t>(val - key));
    } else {
      self->on_header_fn_(*self, key, keylen, nullptr, 0);
    }
  }

  // Transfer-Encoding: chunked

  return size * nitems;
}

ATFRAMEWORK_UTILS_API int http_request::curl_callback_on_verbose(CURL *, curl_infotype type, char *data, size_t size,
                                                                 void *userptr) {
  http_request *self = reinterpret_cast<http_request *>(userptr);
  assert(self);

  if (self && self->on_verbose_fn_) {
    return self->on_verbose_fn_(*self, type, data, size);
  }

  return 0;
}

ATFRAMEWORK_UTILS_API void http_request::curl_share_callback_on_lock(CURL *, curl_lock_data data, curl_lock_access,
                                                                     void *userptr) {
  curl_share_context *share = reinterpret_cast<curl_share_context *>(userptr);
  if (nullptr == share) {
    return;
  }

  std::shared_ptr<std::mutex> lock;
  {
    std::lock_guard<std::recursive_mutex> guard{share->global_lock_};
    auto iter = share->data_locks_.find(data);
    if (iter != share->data_locks_.end()) {
      lock = iter->second;
    } else {
      lock = std::make_shared<std::mutex>();
      share->data_locks_[data] = lock;
    }
  }

  if (lock) {
    lock->lock();
  }
}

ATFRAMEWORK_UTILS_API void http_request::curl_share_callback_on_unlock(CURL *, curl_lock_data data, void *userptr) {
  curl_share_context *share = reinterpret_cast<curl_share_context *>(userptr);
  if (nullptr == share) {
    return;
  }

  std::shared_ptr<std::mutex> lock;
  {
    std::lock_guard<std::recursive_mutex> guard{share->global_lock_};
    auto iter = share->data_locks_.find(data);
    if (iter != share->data_locks_.end()) {
      lock = iter->second;
    }
  }

  if (lock) {
    lock->unlock();
  }
}

}  // namespace network
ATFRAMEWORK_UTILS_NAMESPACE_END

#  endif

#endif
