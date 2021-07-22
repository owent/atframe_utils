#include <assert.h>
#include <cstring>

#include <std/explicit_declare.h>

#include <common/file_system.h>
#include <string/tquerystring.h>

// #include <log/log_wrapper.h>

#include "network/http_request.h"

#if defined(NETWORK_EVPOLL_ENABLE_LIBUV) && defined(NETWORK_ENABLE_CURL)
#  if NETWORK_ENABLE_CURL && NETWORK_EVPOLL_ENABLE_LIBUV

#    if defined(UTIL_CONFIG_COMPILER_CXX_STATIC_ASSERT) && UTIL_CONFIG_COMPILER_CXX_STATIC_ASSERT
#      include <type_traits>
#      if (defined(__cplusplus) && __cplusplus >= 201402L) || ((defined(_MSVC_LANG) && _MSVC_LANG >= 201402L))
UTIL_CONFIG_STATIC_ASSERT(std::is_trivially_copyable<util::network::http_request::curl_poll_context_t>::value);
#      elif (defined(__cplusplus) && __cplusplus >= 201103L) || ((defined(_MSVC_LANG) && _MSVC_LANG >= 201103L))
UTIL_CONFIG_STATIC_ASSERT(std::is_trivial<util::network::http_request::curl_poll_context_t>::value);
#      else
UTIL_CONFIG_STATIC_ASSERT(std::is_pod<util::network::http_request::curl_poll_context_t>::value);
#      endif
#    endif

#    define CHECK_FLAG(f, v) !!((f) & (v))
#    define SET_FLAG(f, v) (f) |= (v)
#    define UNSET_FLAG(f, v) (f) &= (~(v))

namespace util {
namespace network {
namespace detail {
/* initialize custom header list (stating that Expect: 100-continue is not wanted */
static constexpr const char custom_no_expect_header[] = "Expect:";
static constexpr const char content_type_multipart_post[] = "Content-Type: application/x-www-form-urlencoded";
// static constexpr const char content_type_multipart_form_data[] = "Content-Type: multipart/form-data";
}  // namespace detail

LIBATFRAME_UTILS_API http_request::ptr_t http_request::create(curl_m_bind_t *curl_multi, const std::string &url) {
  ptr_t ret = create(curl_multi);
  if (ret) {
    ret->set_url(url);
  }

  return ret;
}

LIBATFRAME_UTILS_API http_request::ptr_t http_request::create(curl_m_bind_t *curl_multi) {
  ptr_t ret = std::make_shared<http_request>(curl_multi);
  if (ret->mutable_request()) {
    return ret;
  }

  return ptr_t();
}

LIBATFRAME_UTILS_API int http_request::get_status_code_group(int code) { return code / 100; }

LIBATFRAME_UTILS_API http_request::http_request(curl_m_bind_t *curl_multi)
    : timeout_ms_(0),
      bind_m_(curl_multi),
      request_(nullptr),
      flags_(0),
      response_code_(0),
      last_error_code_(0),
      priv_data_(nullptr) {
  http_form_.begin = nullptr;
  http_form_.end = nullptr;
  http_form_.headerlist = nullptr;
  http_form_.posted_size = 0;
  http_form_.uploaded_file = nullptr;
  http_form_.flags = 0;
  set_user_agent("libcurl");
}

LIBATFRAME_UTILS_API http_request::~http_request() { cleanup(); }

LIBATFRAME_UTILS_API int http_request::start(method_t::type method, bool wait) {
  CURL *req = mutable_request();
  if (nullptr == req) {
    return -1;
  }

  if (!wait && (nullptr == bind_m_ || nullptr == bind_m_->curl_multi)) {
    return -1;
  }

  if (CHECK_FLAG(flags_, flag_t::EN_FT_CURL_MULTI_HANDLE)) {
    return -1;
  }

  switch (method) {
    case method_t::EN_MT_GET:
      curl_easy_setopt(req, CURLOPT_HTTPGET, 1L);
      break;
    case method_t::EN_MT_POST:
      curl_easy_setopt(req, CURLOPT_POST, 1L);
      break;
    case method_t::EN_MT_PUT: {
      curl_easy_setopt(req, CURLOPT_PUT, 1L);
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
  build_http_form(method);

  if (nullptr != http_form_.begin) {
    set_libcurl_no_expect();
    curl_easy_setopt(req, CURLOPT_HTTPPOST, http_form_.begin);
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
    curl_easy_setopt(req, CURLOPT_READFUNCTION, curl_callback_on_read);
    curl_easy_setopt(req, CURLOPT_READDATA, this);

    long infile_size = 0;
    if (nullptr != http_form_.uploaded_file) {
      fseek(http_form_.uploaded_file, 0, SEEK_END);
      infile_size = ftell(http_form_.uploaded_file);
      fseek(http_form_.uploaded_file, 0, SEEK_SET);
    } else if (!post_data_.empty()) {
      infile_size = static_cast<long>(post_data_.size());
    }
    curl_easy_setopt(req, CURLOPT_INFILESIZE, infile_size);
  }

  if (timeout_ms_ > 0) {
    set_opt_long(CURLOPT_CONNECTTIMEOUT_MS, timeout_ms_);
    set_opt_long(CURLOPT_TIMEOUT_MS, timeout_ms_);
  }

  if (wait) {
    SET_FLAG(flags_, flag_t::EN_FT_RUNNING);
    last_error_code_ = curl_easy_perform(req);
    UNSET_FLAG(flags_, flag_t::EN_FT_RUNNING);
    finish_req_rsp();
  } else {
    SET_FLAG(flags_, flag_t::EN_FT_RUNNING);
    SET_FLAG(flags_, flag_t::EN_FT_CURL_MULTI_HANDLE);
    last_error_code_ = curl_multi_add_handle(bind_m_->curl_multi, req);
    if (last_error_code_ != CURLM_OK) {
      UNSET_FLAG(flags_, flag_t::EN_FT_RUNNING);
      cleanup();
    }
  }

  return last_error_code_;
}

LIBATFRAME_UTILS_API int http_request::stop() {
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

LIBATFRAME_UTILS_API void http_request::set_url(const std::string &v) {
  if (nullptr == mutable_request()) {
    return;
  }

  url_ = v;
  curl_easy_setopt(mutable_request(), CURLOPT_URL, url_.c_str());
}

LIBATFRAME_UTILS_API const std::string &http_request::get_url() const { return url_; }

LIBATFRAME_UTILS_API void http_request::set_user_agent(const std::string &v) {
  if (nullptr == mutable_request()) {
    return;
  }

  useragent_ = v;
  curl_easy_setopt(mutable_request(), CURLOPT_USERAGENT, url_.c_str());
}

LIBATFRAME_UTILS_API const std::string &http_request::get_user_agent() const { return useragent_; }

LIBATFRAME_UTILS_API std::string &http_request::post_data() { return post_data_; }
LIBATFRAME_UTILS_API const std::string &http_request::post_data() const { return post_data_; }

LIBATFRAME_UTILS_API int http_request::get_response_code() const {
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

LIBATFRAME_UTILS_API int http_request::get_error_code() const { return last_error_code_; }

LIBATFRAME_UTILS_API const char *http_request::get_error_msg() const { return error_buffer_; }

LIBATFRAME_UTILS_API std::stringstream &http_request::get_response_stream() { return response_; }
LIBATFRAME_UTILS_API const std::stringstream &http_request::get_response_stream() const { return response_; }

LIBATFRAME_UTILS_API int http_request::add_form_file(const std::string &fieldname, const char *filename) {
  if (CHECK_FLAG(flags_, flag_t::EN_FT_CLEANING)) {
    return -1;
  }

  if (nullptr != http_form_.uploaded_file) {
    fclose(http_form_.uploaded_file);
    http_form_.uploaded_file = nullptr;
  }

  UTIL_FS_OPEN(res, http_form_.uploaded_file, filename, "rb");
  if (0 != res || nullptr == http_form_.uploaded_file) {
    if (nullptr != http_form_.uploaded_file) {
      fclose(http_form_.uploaded_file);
    }
    return res;
  }

  int ret = curl_formadd(&http_form_.begin, &http_form_.end, CURLFORM_COPYNAME, fieldname.c_str(), CURLFORM_NAMELENGTH,
                         static_cast<long>(fieldname.size()), CURLFORM_FILE, filename, CURLFORM_END);
  if (0 == ret) {
    http_form_.flags |= form_list_t::EN_FLFT_HAS_FORM_FILE;
  } else {
    fclose(http_form_.uploaded_file);
    http_form_.uploaded_file = nullptr;
  }
  return ret;
}

LIBATFRAME_UTILS_API int http_request::add_form_file(const std::string &fieldname, const char *filename,
                                                     const char *content_type, const char *new_filename) {
  if (CHECK_FLAG(flags_, flag_t::EN_FT_CLEANING)) {
    return -1;
  }

  if (nullptr != http_form_.uploaded_file) {
    fclose(http_form_.uploaded_file);
    http_form_.uploaded_file = nullptr;
  }

  UTIL_FS_OPEN(res, http_form_.uploaded_file, filename, "rb");
  if (0 != res || nullptr == http_form_.uploaded_file) {
    if (nullptr != http_form_.uploaded_file) {
      fclose(http_form_.uploaded_file);
    }
    return res;
  }

  int ret = curl_formadd(&http_form_.begin, &http_form_.end, CURLFORM_COPYNAME, fieldname.c_str(), CURLFORM_NAMELENGTH,
                         static_cast<long>(fieldname.size()), CURLFORM_FILE, filename, CURLFORM_CONTENTTYPE,
                         content_type, CURLFORM_FILENAME, new_filename, CURLFORM_END);
  if (0 == ret) {
    http_form_.flags |= form_list_t::EN_FLFT_HAS_FORM_FILE;
  } else {
    fclose(http_form_.uploaded_file);
    http_form_.uploaded_file = nullptr;
  }
  return ret;
}

LIBATFRAME_UTILS_API int http_request::add_form_file(const std::string &fieldname, const char *filename,
                                                     const char *content_type) {
  if (CHECK_FLAG(flags_, flag_t::EN_FT_CLEANING)) {
    return -1;
  }

  if (nullptr != http_form_.uploaded_file) {
    fclose(http_form_.uploaded_file);
    http_form_.uploaded_file = nullptr;
  }

  UTIL_FS_OPEN(res, http_form_.uploaded_file, filename, "rb");
  if (0 != res || nullptr == http_form_.uploaded_file) {
    if (nullptr != http_form_.uploaded_file) {
      fclose(http_form_.uploaded_file);
    }
    return res;
  }

  int ret = curl_formadd(&http_form_.begin, &http_form_.end, CURLFORM_COPYNAME, fieldname.c_str(), CURLFORM_NAMELENGTH,
                         static_cast<long>(fieldname.size()), CURLFORM_FILE, filename, CURLFORM_CONTENTTYPE,
                         content_type, CURLFORM_END);
  if (0 == ret) {
    http_form_.flags |= form_list_t::EN_FLFT_HAS_FORM_FILE;
  } else {
    fclose(http_form_.uploaded_file);
    http_form_.uploaded_file = nullptr;
  }
  return ret;
}

LIBATFRAME_UTILS_API int http_request::add_form_field(const std::string &fieldname, const std::string &fieldvalue) {
  if (CHECK_FLAG(flags_, flag_t::EN_FT_CLEANING)) {
    return -1;
  }

  http_form_.qs_fields.set(fieldname, fieldvalue);
  http_form_.flags |= form_list_t::EN_FLFT_HAS_FORM_FIELD;
  // int ret = add_form_field(fieldname, fieldvalue.c_str(), fieldvalue.size());
  return 0;
}

LIBATFRAME_UTILS_API void http_request::set_priv_data(void *v) { priv_data_ = v; }
LIBATFRAME_UTILS_API void *http_request::get_priv_data() const { return priv_data_; }

LIBATFRAME_UTILS_API void http_request::set_opt_bool(CURLoption k, bool v) {
  if (nullptr == mutable_request()) {
    return;
  }

  long val = v ? 1L : 0L;
  curl_easy_setopt(mutable_request(), k, val);
}

LIBATFRAME_UTILS_API void http_request::set_opt_string(CURLoption k, const char *v) {
  if (nullptr == mutable_request()) {
    return;
  }

  curl_easy_setopt(mutable_request(), k, v);
}

LIBATFRAME_UTILS_API void http_request::set_opt_ssl_verify_peer(bool v) { set_opt_bool(CURLOPT_SSL_VERIFYPEER, v); }

LIBATFRAME_UTILS_API void http_request::set_opt_no_signal(bool v) { set_opt_bool(CURLOPT_NOSIGNAL, v); }

LIBATFRAME_UTILS_API void http_request::set_opt_follow_location(bool v) { set_opt_bool(CURLOPT_FOLLOWLOCATION, v); }

LIBATFRAME_UTILS_API void http_request::set_opt_verbose(bool v) { set_opt_bool(CURLOPT_VERBOSE, v); }

LIBATFRAME_UTILS_API void http_request::set_opt_accept_encoding(const char *enc) {
#    if LIBCURL_VERSION_NUM >= 0x071506
  set_opt_string(CURLOPT_ACCEPT_ENCODING, enc);
#    else
  set_opt_string(CURLOPT_ENCODING, enc);
#    endif
}

LIBATFRAME_UTILS_API void http_request::set_opt_http_content_decoding(bool v) {
  set_opt_bool(CURLOPT_HTTP_CONTENT_DECODING, v);
}

LIBATFRAME_UTILS_API bool http_request::set_opt_keepalive(EXPLICIT_UNUSED_ATTR time_t idle,
                                                          EXPLICIT_UNUSED_ATTR time_t interval) {
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

LIBATFRAME_UTILS_API void http_request::set_opt_reuse_connection(bool v) {
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

LIBATFRAME_UTILS_API void http_request::set_opt_timeout(time_t timeout_ms) {
  timeout_ms_ = timeout_ms;
  // set_opt_connect_timeout(timeout_ms);
  // set_opt_request_timeout(timeout_ms);
}

LIBATFRAME_UTILS_API void http_request::set_libcurl_no_expect() {
  if (http_form_.flags & form_list_t::EN_FLFT_LIBCURL_NO_EXPECT) {
    return;
  }
  http_form_.flags |= form_list_t::EN_FLFT_LIBCURL_NO_EXPECT;

  append_http_header(::util::network::detail::custom_no_expect_header);
}

LIBATFRAME_UTILS_API void http_request::append_http_header(const char *http_header) {
  http_form_.headerlist = curl_slist_append(http_form_.headerlist, http_header);
}

LIBATFRAME_UTILS_API const http_request::on_progress_fn_t &http_request::get_on_progress() const {
  return on_progress_fn_;
}
LIBATFRAME_UTILS_API void http_request::set_on_progress(on_progress_fn_t fn) {
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

LIBATFRAME_UTILS_API const http_request::on_header_fn_t &http_request::get_on_header() const { return on_header_fn_; }
LIBATFRAME_UTILS_API void http_request::set_on_header(on_header_fn_t fn) {
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

LIBATFRAME_UTILS_API const http_request::on_success_fn_t &http_request::get_on_success() const {
  return on_success_fn_;
}
LIBATFRAME_UTILS_API void http_request::set_on_success(on_success_fn_t fn) { on_success_fn_ = fn; }

LIBATFRAME_UTILS_API const http_request::on_error_fn_t &http_request::get_on_error() const { return on_error_fn_; }
LIBATFRAME_UTILS_API void http_request::set_on_error(on_error_fn_t fn) { on_error_fn_ = fn; }

LIBATFRAME_UTILS_API const http_request::on_complete_fn_t &http_request::get_on_complete() const {
  return on_complete_fn_;
}
LIBATFRAME_UTILS_API void http_request::set_on_complete(on_complete_fn_t fn) { on_complete_fn_ = fn; }

LIBATFRAME_UTILS_API const http_request::on_write_fn_t &http_request::get_on_write() const { return on_write_fn_; }
LIBATFRAME_UTILS_API void http_request::set_on_write(on_write_fn_t fn) { on_write_fn_ = fn; }

LIBATFRAME_UTILS_API const http_request::on_verbose_fn_t &http_request::get_on_verbose() const {
  return on_verbose_fn_;
}

LIBATFRAME_UTILS_API void http_request::set_on_verbose(on_verbose_fn_t fn) {
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

LIBATFRAME_UTILS_API bool http_request::is_running() const { return CHECK_FLAG(flags_, flag_t::EN_FT_RUNNING); }

LIBATFRAME_UTILS_API void http_request::remove_curl_request() {
  CURL *req = request_;
  request_ = nullptr;
  if (nullptr != req) {
    if (nullptr != bind_m_ && CHECK_FLAG(flags_, flag_t::EN_FT_CURL_MULTI_HANDLE)) {
      // can not be called inside socket callback
      last_error_code_ = curl_multi_remove_handle(bind_m_->curl_multi, req);
      UNSET_FLAG(flags_, flag_t::EN_FT_CURL_MULTI_HANDLE);
    }

    curl_easy_cleanup(req);
  }
  UNSET_FLAG(flags_, flag_t::EN_FT_STOPING);
  UNSET_FLAG(flags_, flag_t::EN_FT_RUNNING);

  if (nullptr != http_form_.begin) {
    curl_formfree(http_form_.begin);
    http_form_.begin = nullptr;
    http_form_.end = nullptr;
    http_form_.qs_fields.clear();
  }

  if (nullptr != http_form_.headerlist) {
    curl_slist_free_all(http_form_.headerlist);
    http_form_.headerlist = nullptr;
  }

  http_form_.posted_size = 0;

  if (nullptr != http_form_.uploaded_file) {
    fclose(http_form_.uploaded_file);
    http_form_.uploaded_file = nullptr;
  }
  http_form_.flags = 0;

  post_data_.clear();
}

LIBATFRAME_UTILS_API void http_request::cleanup() {
  if (CHECK_FLAG(flags_, flag_t::EN_FT_CLEANING)) {
    return;
  }
  SET_FLAG(flags_, flag_t::EN_FT_CLEANING);

  remove_curl_request();

  UNSET_FLAG(flags_, flag_t::EN_FT_CLEANING);
}

LIBATFRAME_UTILS_API void http_request::finish_req_rsp() {
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

LIBATFRAME_UTILS_API CURL *http_request::mutable_request() {
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

LIBATFRAME_UTILS_API void http_request::build_http_form(method_t::type method) {
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
    if (!http_form_.qs_fields.empty()) {
      set_libcurl_no_expect();
      append_http_header(::util::network::detail::content_type_multipart_post);
      http_form_.qs_fields.to_string().swap(post_data_);
      http_form_.flags |= form_list_t::EN_FLFT_WRITE_FORM_USE_FUNC;

      if (nullptr != http_form_.uploaded_file) {
        fclose(http_form_.uploaded_file);
        http_form_.uploaded_file = nullptr;
      }
    } else if (nullptr != http_form_.uploaded_file) {
      set_libcurl_no_expect();
    }
  } else if (!http_form_.qs_fields.empty()) {
    set_libcurl_no_expect();

    for (util::tquerystring::data_const_iterator iter = http_form_.qs_fields.data().begin();
         iter != http_form_.qs_fields.data().end(); ++iter) {
      if (util::types::ITEM_TYPE_STRING == iter->second->type()) {
#    if defined(LIBATFRAME_UTILS_ENABLE_RTTI) && LIBATFRAME_UTILS_ENABLE_RTTI
        util::types::item_string *val = dynamic_cast<util::types::item_string *>(iter->second.get());
#    else
        util::types::item_string *val = static_cast<util::types::item_string *>(iter->second.get());
#    endif

        if (nullptr != val) {
          curl_formadd(&http_form_.begin, &http_form_.end, CURLFORM_PTRNAME, iter->first.c_str(), CURLFORM_NAMELENGTH,
                       static_cast<long>(iter->first.size()), CURLFORM_PTRCONTENTS, val->data().c_str(),
#    if LIBCURL_VERSION_NUM >= 0x072e00
                       CURLFORM_CONTENTLEN, static_cast<curl_off_t>(val->data().size()),
#    else
                       CURLFORM_CONTENTSLENGTH, static_cast<long>(val->data().size()),
#    endif
                       CURLFORM_END);
        }
      } else {
        std::string val = iter->second->to_string();
        curl_formadd(&http_form_.begin, &http_form_.end, CURLFORM_PTRNAME, iter->first.c_str(), CURLFORM_NAMELENGTH,
                     static_cast<long>(iter->first.size()), CURLFORM_COPYCONTENTS, val.c_str(), CURLFORM_CONTENTSLENGTH,
                     static_cast<long>(val.size()), CURLFORM_END);
      }
    }

    if (nullptr != http_form_.uploaded_file) {
      fclose(http_form_.uploaded_file);
      http_form_.uploaded_file = nullptr;
    }
  }
}

LIBATFRAME_UTILS_API http_request::curl_poll_context_t *http_request::malloc_poll(http_request *req,
                                                                                  curl_socket_t sockfd) {
  if (nullptr == req) {
    abort();
  }

  assert(req->bind_m_);
  assert(req->bind_m_->ev_loop);

  curl_poll_context_t *ret = reinterpret_cast<curl_poll_context_t *>(malloc(sizeof(curl_poll_context_t)));
  if (nullptr == ret) {
    return ret;
  }

  ret->sockfd = sockfd;
  ret->bind_multi = req->bind_m_;
  uv_poll_init_socket(req->bind_m_->ev_loop, &ret->poll_object, sockfd);
  ret->poll_object.data = ret;
  ret->is_removed = false;

  return ret;
}

LIBATFRAME_UTILS_API void http_request::free_poll(curl_poll_context_t *p) { free(p); }

LIBATFRAME_UTILS_API void http_request::check_multi_info(CURLM *curl_handle) {
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

LIBATFRAME_UTILS_API http_request::curl_m_bind_t::curl_m_bind_t() : ev_loop(nullptr), curl_multi(nullptr) {}

LIBATFRAME_UTILS_API int http_request::create_curl_multi(uv_loop_t *evloop, std::shared_ptr<curl_m_bind_t> &manager) {
  assert(evloop);
  if (manager) {
    destroy_curl_multi(manager);
  }
  manager = std::make_shared<curl_m_bind_t>();
  if (!manager) {
    return -1;
  }

  manager->curl_multi = curl_multi_init();
  if (nullptr == manager->curl_multi) {
    manager.reset();
    return -1;
  }

  manager->ev_loop = evloop;
  if (0 != uv_timer_init(evloop, &manager->ev_timeout)) {
    curl_multi_cleanup(manager->curl_multi);
    manager.reset();
    return -1;
  }
  manager->ev_timeout.data = manager.get();

  int ret = curl_multi_setopt(manager->curl_multi, CURLMOPT_SOCKETFUNCTION, http_request::curl_callback_handle_socket);
  ret = (CURLE_OK != ret) || curl_multi_setopt(manager->curl_multi, CURLMOPT_SOCKETDATA, manager.get());
  ret = (CURLE_OK != ret) ||
        curl_multi_setopt(manager->curl_multi, CURLMOPT_TIMERFUNCTION, http_request::curl_callback_start_timer);
  ret = (CURLE_OK != ret) || curl_multi_setopt(manager->curl_multi, CURLMOPT_TIMERDATA, manager.get());

  return ret;
}

LIBATFRAME_UTILS_API int http_request::destroy_curl_multi(std::shared_ptr<curl_m_bind_t> &manager) {
  if (!manager) {
    return 0;
  }

  assert(manager->ev_loop);
  assert(manager->curl_multi);

  int ret = curl_multi_cleanup(manager->curl_multi);
  manager->curl_multi = nullptr;

  // hold self in case of timer in libuv invalid
  manager->self_holder = manager;
  uv_timer_stop(&manager->ev_timeout);
  uv_close(reinterpret_cast<uv_handle_t *>(&manager->ev_timeout), http_request::ev_callback_on_timer_closed);

  manager.reset();
  return ret;
}

LIBATFRAME_UTILS_API void http_request::ev_callback_on_timer_closed(uv_handle_t *handle) {
  curl_m_bind_t *bind = reinterpret_cast<curl_m_bind_t *>(handle->data);
  assert(bind);

  // release self holder
  if (nullptr != bind) {
    bind->self_holder.reset();
  }
}

LIBATFRAME_UTILS_API void http_request::ev_callback_on_poll_closed(uv_handle_t *handle) {
  curl_poll_context_t *context = reinterpret_cast<curl_poll_context_t *>(handle->data);
  assert(context);

  free_poll(context);
}

LIBATFRAME_UTILS_API void http_request::ev_callback_on_timeout(uv_timer_t *handle) {
  curl_m_bind_t *bind = reinterpret_cast<curl_m_bind_t *>(handle->data);
  assert(bind);
  if (nullptr == bind) {
    return;
  }

  int running_handles = 0;
  curl_multi_socket_action(bind->curl_multi, CURL_SOCKET_TIMEOUT, 0, &running_handles);
  check_multi_info(bind->curl_multi);
}

LIBATFRAME_UTILS_API void http_request::ev_callback_curl_perform(uv_poll_t *req, int, int events) {
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

    curl_multi_socket_action(context->bind_multi->curl_multi, context->sockfd, flags, &running_handles);
  }
  // WLOGWARNING(" ====== [HTTP] ====== sock %d evpoll %d for %p after socket action", context->sockfd, events,
  // context);
  check_multi_info(context->bind_multi->curl_multi);
  // WLOGWARNING(" ====== [HTTP] ====== sock %d evpoll %d for %p done", context->sockfd, events, context);
}

LIBATFRAME_UTILS_API int http_request::curl_callback_start_timer(CURLM *, long timeout_ms, void *userp) {
  curl_m_bind_t *bind = reinterpret_cast<curl_m_bind_t *>(userp);
  assert(bind);

  // @see https://curl.haxx.se/libcurl/c/evhiperfifo.html
  // @see https://gist.github.com/clemensg/5248927
  // @see https://curl.haxx.se/libcurl/c/multi-uv.html
  if (timeout_ms < 0) {
    uv_timer_stop(&bind->ev_timeout);
  } else {
    if (timeout_ms == 0) {
      timeout_ms = 1;
    }

    uv_timer_start(&bind->ev_timeout, http_request::ev_callback_on_timeout, static_cast<uint64_t>(timeout_ms), 0);
  }

  return 0;
}

LIBATFRAME_UTILS_API int http_request::curl_callback_handle_socket(CURL *easy, curl_socket_t s, int action, void *,
                                                                   void *socketp) {
  curl_poll_context_t *context = reinterpret_cast<curl_poll_context_t *>(socketp);
  if (action == CURL_POLL_IN || action == CURL_POLL_OUT || action == CURL_POLL_INOUT) {
    if (nullptr == context) {
      http_request *req;
      curl_easy_getinfo(easy, CURLINFO_PRIVATE, &req);
      if (nullptr != req && nullptr != req->bind_m_) {
        assert(req->bind_m_);
        assert(req->bind_m_->curl_multi);

        context = malloc_poll(req, s);
        req->last_error_code_ = curl_multi_assign(req->bind_m_->curl_multi, s, context);
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
        CURLM *curl_multi = context->bind_multi->curl_multi;

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

LIBATFRAME_UTILS_API size_t http_request::curl_callback_on_write(char *ptr, size_t size, size_t nmemb, void *userdata) {
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
    self->response_.write(data, data_len);
  }

  return size * nmemb;
}

#    if LIBCURL_VERSION_NUM >= 0x072000
LIBATFRAME_UTILS_API int http_request::curl_callback_on_progress(void *clientp, curl_off_t dltotal, curl_off_t dlnow,
                                                                 curl_off_t ultotal, curl_off_t ulnow) {
#    else
LIBATFRAME_UTILS_API int http_request::curl_callback_on_progress(void *clientp, double dltotal, double dlnow,
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

LIBATFRAME_UTILS_API size_t http_request::curl_callback_on_read(char *buffer, size_t size, size_t nitems,
                                                                void *instream) {
  http_request *self = reinterpret_cast<http_request *>(instream);
  assert(self);
  if (nullptr == self) {
    return 0;
  }

  if (self->post_data_.empty() && nullptr != self->http_form_.uploaded_file) {
    return fread(buffer, size, nitems, self->http_form_.uploaded_file);
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

LIBATFRAME_UTILS_API size_t http_request::curl_callback_on_header(char *buffer, size_t size, size_t nitems,
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
      self->on_header_fn_(*self, key, keylen, val, nwrite - (val - key));
    } else {
      self->on_header_fn_(*self, key, keylen, nullptr, 0);
    }
  }

  // Transfer-Encoding: chunked

  return size * nitems;
}

LIBATFRAME_UTILS_API int http_request::curl_callback_on_verbose(CURL *, curl_infotype type, char *data, size_t size,
                                                                void *userptr) {
  http_request *self = reinterpret_cast<http_request *>(userptr);
  assert(self);

  if (self && self->on_verbose_fn_) {
    return self->on_verbose_fn_(*self, type, data, size);
  }

  return 0;
}
}  // namespace network
}  // namespace util

#  endif

#endif
