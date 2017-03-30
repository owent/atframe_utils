/**
 * @file http_request.h
 * @brief 对CURL的封装
 *
 * @version 1.0
 * @author owent
 * @date 2016.07.14
 *
 */

#ifndef _UTILS_NETWORK_HTTP_REQUEST_H_
#define _UTILS_NETWORK_HTTP_REQUEST_H_

#pragma once

#include <cstddef>
#include <cstdio>
#include <memory>
#include <sstream>
#include <string>


#include "design_pattern/noncopyable.h"
#include "std/functional.h"
#include "std/smart_ptr.h"
#include "string/tquerystring.h"


#include "config/atframe_utils_build_feature.h"

#if defined(NETWORK_EVPOLL_ENABLE_LIBUV) && defined(NETWORK_ENABLE_CURL)
#if NETWORK_ENABLE_CURL && NETWORK_EVPOLL_ENABLE_LIBUV

extern "C" {
#include <curl/curl.h>
#include <uv.h>
}

namespace util {
    namespace network {

        /**
         * http_request using libuv
         * @see https://curl.haxx.se/libcurl/c/multi-uv.html
         */
        class http_request : public std::enable_shared_from_this<http_request>, public ::util::design_pattern::noncopyable {
        public:
            /**
             * @brief types
             */
            typedef http_request self_type;
            typedef std::shared_ptr<self_type> ptr_t;

            struct method_t {
                enum type { EN_MT_GET = 0, EN_MT_POST, EN_MT_PUT, EN_MT_DELETE, EN_MT_TRACE };
            };

            /**
             * @brief common http status code
             * @see https://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
             */
            struct status_code_t {
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

            struct flag_t {
                enum type {
                    EN_FT_CURL_MULTI_HANDLE = 0x01,
                    EN_FT_RUNNING = 0x02,
                    EN_FT_CLEANING = 0x04,
                };
            };

            struct curl_m_bind_t {
                uv_loop_t *ev_loop;
                CURLM *curl_multi;
                uv_timer_t ev_timeout;

                curl_m_bind_t();
                std::shared_ptr<curl_m_bind_t> self_holder;
            };
            typedef std::shared_ptr<curl_m_bind_t> curl_m_bind_ptr_t;

            struct curl_poll_context_t {
                curl_m_bind_t *bind_multi;
                uv_poll_t poll_object;
                curl_socket_t sockfd;
            };

            typedef std::function<int(http_request &)> on_error_fn_t;
            typedef std::function<int(http_request &)> on_success_fn_t;
            typedef std::function<int(http_request &)> on_complete_fn_t;

            struct progress_t {
                double dltotal; /** total download size **/
                double dlnow;   /** already downloaded size **/
                double ultotal; /** total upload size **/
                double ulnow;   /** already uploaded size **/
            };
            typedef std::function<int(http_request &, const progress_t &)> on_progress_fn_t;
            /** parameters: http_request, key, key length, value, value length **/
            typedef std::function<int(http_request &, const char *, size_t, const char *, size_t)> on_header_fn_t;

        public:
            static ptr_t create(curl_m_bind_t *, const std::string &url);

            static ptr_t create(curl_m_bind_t *);

            static int get_status_code_group(int code);

            http_request(curl_m_bind_t *curl_multi);
            ~http_request();

            static int create_curl_multi(uv_loop_t *evloop, std::shared_ptr<curl_m_bind_t> &manager);
            static int destroy_curl_multi(std::shared_ptr<curl_m_bind_t> &manager);

            /**
             * @brief start a http request
             * @param wait if true, waiting for request finished
             * @return 0 or error code, curl's error code is greater than 0, and this system's error code will be less than 0
             */
            int start(method_t::type method = method_t::EN_MT_GET, bool wait = false);

            int stop();

            void remove_curl_request();

            void cleanup();

            void set_url(const std::string &v);
            const std::string &get_url() const;

            void set_user_agent(const std::string &v);
            const std::string &get_user_agent() const;

            std::string &post_data();
            const std::string &post_data() const;

            int get_response_code() const;

            int get_error_code() const;

            inline const char *get_error_msg() const { return error_buffer_; }

            inline std::stringstream &get_response_stream() { return response_; }
            inline const std::stringstream &get_response_stream() const { return response_; }

            int add_form_file(const std::string &fieldname, const char *filename);

            int add_form_file(const std::string &fieldname, const char *filename, const char *content_type, const char *new_filename);

            int add_form_file(const std::string &fieldname, const char *filename, const char *content_type);

            int add_form_field(const std::string &fieldname, const std::string &fieldvalue);

            template <typename T>
            int add_form_field(const std::string &fieldname, const T &fieldvalue) {
                std::stringstream ss;
                ss << fieldvalue;

                std::string val;
                ss.str().swap(val);
                return add_form_field(fieldname, val);
            }

            inline void set_priv_data(void *v) { priv_data_ = v; }
            inline void *get_priv_data() const { return priv_data_; }

            // ======== set options of libcurl @see https://curl.haxx.se/libcurl/c/curl_easy_setopt.html for detail ========
            inline void set_opt_bool(CURLoption k, bool v) {
                if (NULL == mutable_request()) {
                    return;
                }

                long val = v ? 1L : 0L;
                curl_easy_setopt(mutable_request(), k, val);
            }

            inline void set_opt_string(CURLoption k, const char *v) {
                if (NULL == mutable_request()) {
                    return;
                }

                curl_easy_setopt(mutable_request(), k, v);
            }

            template <typename T>
            void set_opt_long(CURLoption k, T v) {
                if (NULL == mutable_request()) {
                    return;
                }

                long val = static_cast<long>(v);
                curl_easy_setopt(mutable_request(), k, val);
            }

            void set_opt_ssl_verify_peer(bool v);

            void set_opt_no_signal(bool v);

            void set_opt_follow_location(bool v);

            void set_opt_verbose(bool v);

            void set_opt_http_content_decoding(bool v);

            void set_opt_keepalive(time_t idle, time_t interval);

            void set_opt_timeout(time_t timeout_ms);

            void set_libcurl_no_expect();

            void append_http_header(const char *http_header);
            // -------- set options of libcurl @see https://curl.haxx.se/libcurl/c/curl_easy_setopt.html for detail --------

            const on_progress_fn_t &get_on_progress() const;
            void set_on_progress(on_progress_fn_t fn);

            const on_header_fn_t &get_on_header() const;
            void set_on_header(on_header_fn_t fn);

            const on_success_fn_t &get_on_success() const;
            void set_on_success(on_success_fn_t fn);

            const on_error_fn_t &get_on_error() const;
            void set_on_error(on_error_fn_t fn);

            const on_complete_fn_t &get_on_complete() const;
            void set_on_complete(on_complete_fn_t fn);

            bool is_running() const;

        private:
            void finish_req_rsp();

            CURL *mutable_request();

            void build_http_form(method_t::type method);

            static curl_poll_context_t *malloc_poll(http_request *req, curl_socket_t sockfd);
            static void free_poll(curl_poll_context_t *);

            static void check_multi_info(CURLM *curl_handle);

            static void ev_callback_on_timer_closed(uv_handle_t *handle);
            static void ev_callback_on_poll_closed(uv_handle_t *handle);
            static void ev_callback_on_timeout(uv_timer_t *handle);
            static void ev_callback_curl_perform(uv_poll_t *req, int status, int events);

            static void curl_callback_start_timer(CURLM *multi, long timeout_ms, void *userp);
            static int curl_callback_handle_socket(CURL *easy, curl_socket_t s, int action, void *userp, void *socketp);
            static size_t curl_callback_on_write(char *ptr, size_t size, size_t nmemb, void *userdata);
            static int curl_callback_on_progress(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);
            static size_t curl_callback_on_read(char *buffer, size_t size, size_t nitems, void *instream);
            static size_t curl_callback_on_header(char *buffer, size_t size, size_t nitems, void *userdata);

        private:
            // event dispatcher
            time_t timeout_ms_;

            // curl resource
            curl_m_bind_t *bind_m_;
            CURL *request_;
            int flags_;

            // data and resource
            std::string url_;
            std::string post_data_;
            std::stringstream response_;
            mutable int response_code_;
            int last_error_code_;
            void *priv_data_;
            std::string useragent_;

            typedef struct {
                curl_httppost *begin;
                curl_httppost *end;
                curl_slist *headerlist;
                util::tquerystring qs_fields;

                size_t posted_size;
                FILE *uploaded_file;
                int flags;

                enum flag_t {
                    EN_FLFT_HAS_FORM_FILE = 0x01,
                    EN_FLFT_HAS_FORM_FIELD = 0x02,
                    EN_FLFT_WRITE_FORM_USE_FUNC = 0x04,
                    EN_FLFT_LIBCURL_NO_EXPECT = 0x08,
                };
            } form_list_t;
            form_list_t http_form_;

            char error_buffer_[CURL_ERROR_SIZE];

            // callbacks
            on_error_fn_t on_error_fn_;
            on_success_fn_t on_success_fn_;
            on_complete_fn_t on_complete_fn_;
            on_progress_fn_t on_progress_fn_;
            on_header_fn_t on_header_fn_;
        };
    }
}
#endif

#endif

#endif
