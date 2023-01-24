/**
 * @file http_request.h
 * @brief 对CURL的封装
 *
 * @version 1.0
 * @author owent
 * @date 2016.07.14
 *
 */

#ifndef UTILS_NETWORK_HTTP_REQUEST_H
#define UTILS_NETWORK_HTTP_REQUEST_H

#pragma once

#include <cstddef>
#include <cstdio>
#include <functional>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>

#include "design_pattern/noncopyable.h"
#include "string/tquerystring.h"

#include "config/atframe_utils_build_feature.h"

#if defined(NETWORK_EVPOLL_ENABLE_LIBUV) && defined(NETWORK_ENABLE_CURL)
#  if NETWORK_ENABLE_CURL && NETWORK_EVPOLL_ENABLE_LIBUV

extern "C" {
#    include <curl/curl.h>
#    include <uv.h>
}

LIBATFRAME_UTILS_NAMESPACE_BEGIN
namespace network {

/**
 * http_request using libuv
 * @see https://curl.haxx.se/libcurl/c/multi-uv.html
 */
class http_request : public std::enable_shared_from_this<http_request>,
                     public LIBATFRAME_UTILS_NAMESPACE_ID::design_pattern::noncopyable {
 public:
  /**
   * @brief types
   */
  using self_type = http_request;
  using ptr_t = std::shared_ptr<self_type>;

  struct LIBATFRAME_UTILS_API method_t {
    enum type { EN_MT_GET = 0, EN_MT_POST, EN_MT_PUT, EN_MT_DELETE, EN_MT_TRACE };
  };

  /**
   * @brief common http status code
   * @see https://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
   */
  struct LIBATFRAME_UTILS_API status_code_t {
    enum group {
      EN_ECG_INFOMATION = 1,
      EN_ECG_SUCCESS = 2,
      EN_ECG_REDIRECTION = 3,
      EN_ECG_CLIENT_ERROR = 4,
      EN_ECG_SERVER_ERROR = 5,
    };

    enum type {
      EN_SCT_CONTINUE = 100,
      EN_SCT_SWITCHING_PROTOCOL = 101,
      EN_SCT_OK = 200,
      EN_SCT_CREATED = 201,
      EN_SCT_ACCPTED = 202,
      EN_SCT_NO_AUTH_INFO = 203,
      EN_SCT_NO_CONTENT = 204,
      EN_SCT_RESET_CONTENT = 205,
      EN_SCT_PARTIAL_CONTENT = 206,
      EN_SCT_MULTIPLE_CHOICES = 300,
      EN_SCT_MOVE_PERMANENTLY = 301,
      EN_SCT_FOUND = 302,
      EN_SCT_SEE_OTHER = 303,
      EN_SCT_NOT_MODIFIED = 304,
      EN_SCT_USE_PROXY = 305,
      EN_SCT_UNUSED = 306,
      EN_SCT_TEMPORARY_REDIRECT = 307,
      EN_SCT_BAD_REQUEST = 400,
      EN_SCT_UNAUTHORIZED = 401,
      EN_SCT_PAYMENT_REQUIRED = 402,
      EN_SCT_FORBIDDEN = 403,
      EN_SCT_NOT_FOUND = 404,
      EN_SCT_METHOD_NOT_ALLOWED = 405,
      EN_SCT_NOT_ACCEPTABLE = 406,
      EN_SCT_PROXY_AUTHENTICATION_REQUIRED = 407,
      EN_SCT_REQUEST_TIMEOUT = 408,
      EN_SCT_CONFLICT = 409,
      EN_SCT_GONE = 410,
      EN_SCT_LENGTH_REQUIRED = 411,
      EN_SCT_PRECONDITION_FAILED = 412,
      EN_SCT_REQUEST_ENTITY_TOO_LARGE = 413,
      EN_SCT_REQUEST_URI_TOO_LONG = 414,
      EN_SCT_UNSUPPORTED_MEDIA_TYPE = 415,
      EN_SCT_REQUEST_RANGE_NOT_SATISFIABLE = 416,
      EN_SCT_EXPECTATION_FAILED = 417,
      EN_SCT_NOT_IMPLEMENTED = 501,
      EN_SCT_BAD_GATEWAY = 502,
      EN_SCT_SERVICE_UNAVAILABLE = 503,
      EN_SCT_GATEWAY_TIMEOUT = 504,
      EN_SCT_HTTP_VERSION_NOT_SUPPORTED = 505,
    };
  };

  struct LIBATFRAME_UTILS_API flag_t {
    enum type {
      EN_FT_CURL_MULTI_HANDLE = 0x01,
      EN_FT_RUNNING = 0x02,
      EN_FT_CLEANING = 0x04,
      EN_FT_STOPING = 0x08,
    };
  };

  struct LIBATFRAME_UTILS_API_HEAD_ONLY curl_share_options {
    bool enable_multi_thread;
    bool share_cookie;
    bool share_dns;
    bool share_ssl_session;  // curl 7.23.0+

    // Connections that are used for HTTP/1.1 Pipelining or HTTP/2 multiplexing only get additional transfers added to
    //   them if the existing connection is held by the same multi or easy handle. libcurl does not support doing HTTP/2
    //   streams in different threads using a shared connection.
    // Note that due to a known bug, it is not safe to share connections this way between multiple concurrent threads.
    // Note that when you use the multi interface, all easy handles added to the same multi handle will share connection
    //   cache by default without using this option.
    bool share_connection;  // curl 7.57.0+

    bool share_psl;   // Public Suffix List, curl 7.61.0+
    bool share_hsts;  // The in-memory HSTS cache, curl 7.88.0+

    inline curl_share_options() noexcept
        : enable_multi_thread(true),
          share_cookie(true),
          share_dns(true),
#    if LIBCURL_VERSION_NUM >= 0x071700
          share_ssl_session(true),
#    else
          share_ssl_session(false),
#    endif
          share_connection(true),
#    if LIBCURL_VERSION_NUM >= 0x073d00
          share_psl(true),
#    else
          share_psl(false),
#    endif
#    if LIBCURL_VERSION_NUM >= 0x075800
          share_hsts(true)
#    else
          share_hsts(false)
#    endif
    {
    }
  };

  struct curl_share_context {
   private:
    curl_share_context(const curl_share_context &) = delete;
    curl_share_context(curl_share_context &&) = delete;

    curl_share_context &operator=(const curl_share_context &) = delete;
    curl_share_context &operator=(curl_share_context &&) = delete;

   public:
    LIBATFRAME_UTILS_API curl_share_context(CURLSH *share) noexcept;
    LIBATFRAME_UTILS_API ~curl_share_context();

    UTIL_FORCEINLINE CURLSH *get_share_handle() noexcept { return curl_share_; }
    UTIL_FORCEINLINE const CURLSH *get_share_handle() const noexcept { return curl_share_; }

   private:
    friend class http_request;

    CURLSH *curl_share_;
    bool enable_lock_;
    std::recursive_mutex global_lock_;
    std::unordered_map<int32_t, std::shared_ptr<std::mutex>> data_locks_;
  };
  using curl_share_context_ptr_type = std::shared_ptr<curl_share_context>;

  struct LIBATFRAME_UTILS_API_HEAD_ONLY curl_multi_options {
    uv_loop_t *ev_loop;
    curl_share_context_ptr_type share_context;

    inline curl_multi_options() noexcept : ev_loop(nullptr) {}
    inline curl_multi_options(uv_loop_t *loop, curl_share_context_ptr_type share) noexcept
        : ev_loop(loop), share_context(share) {}
  };

  class curl_multi_context {
   private:
    curl_multi_context(const curl_multi_context &) = delete;
    curl_multi_context(curl_multi_context &&) = delete;

    curl_multi_context &operator=(const curl_multi_context &) = delete;
    curl_multi_context &operator=(curl_multi_context &&) = delete;

   public:
    LIBATFRAME_UTILS_API curl_multi_context() noexcept;

    UTIL_FORCEINLINE uv_loop_t *get_evloop() noexcept { return ev_loop_; }
    UTIL_FORCEINLINE const uv_loop_t *get_evloop() const noexcept { return ev_loop_; }

    UTIL_FORCEINLINE CURLM *get_multi_handle() noexcept { return curl_multi_; }
    UTIL_FORCEINLINE const CURLM *get_multi_handle() const noexcept { return curl_multi_; }

   private:
    friend class http_request;

    uv_loop_t *ev_loop_;
    CURLM *curl_multi_;
    uv_timer_t ev_timeout_;
    std::shared_ptr<curl_multi_context> self_holder_;
    curl_share_context_ptr_type share_context_;
  };

  using curl_multi_context_ptr_type = std::shared_ptr<curl_multi_context>;
  using curl_m_bind_t = curl_multi_context;
  using curl_m_bind_ptr_t = curl_multi_context_ptr_type;

  struct LIBATFRAME_UTILS_API curl_poll_context_t {
    curl_multi_context *bind_multi;
    uv_poll_t poll_object;
    curl_socket_t sockfd;
    bool is_removed;
  };

  using on_error_fn_t = std::function<int(http_request &)>;
  using on_success_fn_t = std::function<int(http_request &)>;
  using on_complete_fn_t = std::function<int(http_request &)>;

  struct LIBATFRAME_UTILS_API progress_t {
    size_t dltotal; /** total download size **/
    size_t dlnow;   /** already downloaded size **/
    size_t ultotal; /** total upload size **/
    size_t ulnow;   /** already uploaded size **/
  };

  using on_progress_fn_t = std::function<int(http_request &, const progress_t &)>;
  /** parameters: http_request, key, key length, value, value length **/
  using on_header_fn_t = std::function<int(http_request &, const char *, size_t, const char *, size_t)>;
  /**
   * parameters: http_request, origin write data address, origin write data size, real write data address, real write
   * data size
   */
  using on_write_fn_t = std::function<int(http_request &, const char *, size_t, const char *&,
                                          size_t &)>;  // it's useful if header is Transfer-Encoding: chunked

  using on_verbose_fn_t = std::function<int(http_request &, curl_infotype type, char *data,
                                            size_t size)>;  // it's useful if we want to debug and show verbose info

 public:
  LIBATFRAME_UTILS_API static ptr_t create(curl_multi_context *, const std::string &url);

  LIBATFRAME_UTILS_API static ptr_t create(curl_multi_context *);

  LIBATFRAME_UTILS_API static ptr_t create(const curl_share_context_ptr_type &, const std::string &url);

  LIBATFRAME_UTILS_API static ptr_t create(const curl_share_context_ptr_type &);

  LIBATFRAME_UTILS_API static int get_status_code_group(int code);

  LIBATFRAME_UTILS_API http_request(curl_multi_context *curl_multi, const curl_share_context_ptr_type & share_context);

  LIBATFRAME_UTILS_API ~http_request();

  static LIBATFRAME_UTILS_API int create_curl_share(const curl_share_options &options,
                                                    std::shared_ptr<curl_share_context> &output);

  static LIBATFRAME_UTILS_API int create_curl_multi(uv_loop_t *evloop, std::shared_ptr<curl_multi_context> &manager);
  static LIBATFRAME_UTILS_API int create_curl_multi(const curl_multi_options &options,
                                                    std::shared_ptr<curl_multi_context> &manager);
  static LIBATFRAME_UTILS_API int destroy_curl_multi(std::shared_ptr<curl_multi_context> &manager);

  /**
   * @brief start a http request
   * @param wait if true, waiting for request finished
   * @return 0 or error code, curl's error code is greater than 0, and this system's error code will be less than 0
   */
  LIBATFRAME_UTILS_API int start(method_t::type method = method_t::EN_MT_GET, bool wait = false);

  LIBATFRAME_UTILS_API int stop();

  LIBATFRAME_UTILS_API void set_url(const std::string &v);
  LIBATFRAME_UTILS_API const std::string &get_url() const;

  LIBATFRAME_UTILS_API void set_user_agent(const std::string &v);
  LIBATFRAME_UTILS_API const std::string &get_user_agent() const;

  LIBATFRAME_UTILS_API std::string &post_data();
  LIBATFRAME_UTILS_API const std::string &post_data() const;

  LIBATFRAME_UTILS_API int get_response_code() const;

  LIBATFRAME_UTILS_API int get_error_code() const;

  LIBATFRAME_UTILS_API const char *get_error_msg() const;

  LIBATFRAME_UTILS_API std::stringstream &get_response_stream();
  LIBATFRAME_UTILS_API const std::stringstream &get_response_stream() const;

  LIBATFRAME_UTILS_API int add_form_file(const std::string &fieldname, const char *filename);

  LIBATFRAME_UTILS_API int add_form_file(const std::string &fieldname, const char *filename, const char *content_type,
                                         const char *new_filename);

  LIBATFRAME_UTILS_API int add_form_file(const std::string &fieldname, const char *filename, const char *content_type);

  LIBATFRAME_UTILS_API int add_form_field(const std::string &fieldname, const std::string &fieldvalue);

  template <typename T>
  LIBATFRAME_UTILS_API_HEAD_ONLY int add_form_field(const std::string &fieldname, const T &fieldvalue) {
    std::stringstream ss;
    ss << fieldvalue;

    std::string val;
    ss.str().swap(val);
    return add_form_field(fieldname, val);
  }

  LIBATFRAME_UTILS_API void set_priv_data(void *v);
  LIBATFRAME_UTILS_API void *get_priv_data() const;

  // ======== set options of libcurl @see https://curl.haxx.se/libcurl/c/curl_easy_setopt.html for detail ========
  LIBATFRAME_UTILS_API void set_opt_bool(CURLoption k, bool v);

  LIBATFRAME_UTILS_API void set_opt_string(CURLoption k, const char *v);

  template <typename T>
  LIBATFRAME_UTILS_API_HEAD_ONLY void set_opt_long(CURLoption k, T v) {
    if (nullptr == mutable_request()) {
      return;
    }

    long val = static_cast<long>(v);
    curl_easy_setopt(mutable_request(), k, val);
  }

  LIBATFRAME_UTILS_API void set_opt_ssl_verify_peer(bool v);

  LIBATFRAME_UTILS_API void set_opt_no_signal(bool v);

  LIBATFRAME_UTILS_API void set_opt_follow_location(bool v);

  /**
   * @brief use set_on_verbose instead
   */
  LIBATFRAME_UTILS_API void set_opt_verbose(bool v);

  /**
   * @brief set accept encoding for this request
   * @param enc pass empty string("") to use all built-in supported encodings, and nullptr to disable it
   */
  LIBATFRAME_UTILS_API void set_opt_accept_encoding(const char *enc);

  LIBATFRAME_UTILS_API void set_opt_http_content_decoding(bool v);

  /**
   * @brief set keepalive option for libcurl
   * @param idle should be greater than interval @see CURLOPT_TCP_KEEPIDLE
   * @param interval @see CURLOPT_TCP_KEEPINTVL
   * @note set idle and interval into 0 to disable keepalive
   * @return true if it's supported
   */
  LIBATFRAME_UTILS_API bool set_opt_keepalive(time_t idle, time_t interval);

  LIBATFRAME_UTILS_API void set_opt_timeout(time_t timeout_ms);

  LIBATFRAME_UTILS_API void set_opt_connect_timeout(time_t timeout_ms);

  LIBATFRAME_UTILS_API void set_opt_reuse_connection(bool v);

  LIBATFRAME_UTILS_API void set_libcurl_no_expect();

  LIBATFRAME_UTILS_API void set_libcurl_allow_expect_100_continue();

  LIBATFRAME_UTILS_API void append_http_header(const char *http_header);
  // -------- set options of libcurl @see https://curl.haxx.se/libcurl/c/curl_easy_setopt.html for detail --------

  LIBATFRAME_UTILS_API const on_progress_fn_t &get_on_progress() const;
  LIBATFRAME_UTILS_API void set_on_progress(on_progress_fn_t fn);

  LIBATFRAME_UTILS_API const on_header_fn_t &get_on_header() const;
  LIBATFRAME_UTILS_API void set_on_header(on_header_fn_t fn);

  LIBATFRAME_UTILS_API const on_success_fn_t &get_on_success() const;
  LIBATFRAME_UTILS_API void set_on_success(on_success_fn_t fn);

  LIBATFRAME_UTILS_API const on_error_fn_t &get_on_error() const;
  LIBATFRAME_UTILS_API void set_on_error(on_error_fn_t fn);

  LIBATFRAME_UTILS_API const on_complete_fn_t &get_on_complete() const;
  LIBATFRAME_UTILS_API void set_on_complete(on_complete_fn_t fn);

  LIBATFRAME_UTILS_API const on_write_fn_t &get_on_write() const;
  LIBATFRAME_UTILS_API void set_on_write(on_write_fn_t fn);

  LIBATFRAME_UTILS_API const on_verbose_fn_t &get_on_verbose() const;
  LIBATFRAME_UTILS_API void set_on_verbose(on_verbose_fn_t fn);

  LIBATFRAME_UTILS_API bool is_running() const;

 private:
  LIBATFRAME_UTILS_API void remove_curl_request();

  LIBATFRAME_UTILS_API void cleanup();

  LIBATFRAME_UTILS_API void finish_req_rsp();

  LIBATFRAME_UTILS_API CURL *mutable_request();

  LIBATFRAME_UTILS_API void build_http_form(method_t::type method);

  static LIBATFRAME_UTILS_API curl_poll_context_t *malloc_poll(http_request *req, curl_socket_t sockfd);
  static LIBATFRAME_UTILS_API void free_poll(curl_poll_context_t *);

  static LIBATFRAME_UTILS_API void check_multi_info(CURLM *curl_handle);

  static LIBATFRAME_UTILS_API void ev_callback_on_timer_closed(uv_handle_t *handle);
  static LIBATFRAME_UTILS_API void ev_callback_on_poll_closed(uv_handle_t *handle);
  static LIBATFRAME_UTILS_API void ev_callback_on_timeout(uv_timer_t *handle);
  static LIBATFRAME_UTILS_API void ev_callback_curl_perform(uv_poll_t *req, int status, int events);

  static LIBATFRAME_UTILS_API int curl_callback_start_timer(CURLM *multi, long timeout_ms, void *userp);
  static LIBATFRAME_UTILS_API int curl_callback_handle_socket(CURL *easy, curl_socket_t s, int action, void *userp,
                                                              void *socketp);
  static LIBATFRAME_UTILS_API size_t curl_callback_on_write(char *ptr, size_t size, size_t nmemb, void *userdata);
#    if LIBCURL_VERSION_NUM >= 0x072000
  static LIBATFRAME_UTILS_API int curl_callback_on_progress(void *clientp, curl_off_t dltotal, curl_off_t dlnow,
                                                            curl_off_t ultotal, curl_off_t ulnow);
#    else
  static LIBATFRAME_UTILS_API int curl_callback_on_progress(void *clientp, double dltotal, double dlnow, double ultotal,
                                                            double ulnow);
#    endif
  static LIBATFRAME_UTILS_API size_t curl_callback_on_read(char *buffer, size_t size, size_t nitems, void *instream);
  static LIBATFRAME_UTILS_API size_t curl_callback_on_header(char *buffer, size_t size, size_t nitems, void *userdata);
  static LIBATFRAME_UTILS_API int curl_callback_on_verbose(CURL *handle, curl_infotype type, char *data, size_t size,
                                                           void *userptr);

  static LIBATFRAME_UTILS_API void curl_share_callback_on_lock(CURL *handle, curl_lock_data data,
                                                               curl_lock_access access, void *userptr);
  static LIBATFRAME_UTILS_API void curl_share_callback_on_unlock(CURL *handle, curl_lock_data data, void *userptr);

 private:
  // event dispatcher
  time_t timeout_ms_;
  time_t connect_timeout_ms_;

  // curl resource
  curl_multi_context *bind_multi_;
  CURL *request_;
  uint32_t flags_;
  curl_share_context_ptr_type share_context_;

  // data and resource
  std::string url_;
  std::string post_data_;
  std::stringstream response_;
  mutable int response_code_;
  int last_error_code_;
  void *priv_data_;
  std::string useragent_;

  struct form_list_t {
    curl_httppost *begin;
    curl_httppost *end;
    curl_slist *headerlist;
    LIBATFRAME_UTILS_NAMESPACE_ID::tquerystring qs_fields;

    size_t posted_size;
    FILE *uploaded_file;
    int flags;

    enum flag_t {
      EN_FLFT_HAS_FORM_FILE = 0x01,
      EN_FLFT_HAS_FORM_FIELD = 0x02,
      EN_FLFT_WRITE_FORM_USE_FUNC = 0x04,
      EN_FLFT_LIBCURL_NO_EXPECT = 0x08,
      EN_FLFT_LIBCURL_ALLOW_EXPECT_100_CONTINUE = 0x10,
    };
  };
  form_list_t http_form_;

  char error_buffer_[CURL_ERROR_SIZE];

  // callbacks
  on_error_fn_t on_error_fn_;
  on_success_fn_t on_success_fn_;
  on_complete_fn_t on_complete_fn_;
  on_progress_fn_t on_progress_fn_;
  on_header_fn_t on_header_fn_;
  on_write_fn_t on_write_fn_;
  on_verbose_fn_t on_verbose_fn_;
};
}  // namespace network
LIBATFRAME_UTILS_NAMESPACE_END
#  endif

#endif

#endif
