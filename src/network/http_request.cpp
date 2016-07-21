#include <assert.h>
#include <cstring>

#include <common/file_system.h>
#include <string/tquerystring.h>

#include "network/http_request.h"

#if defined(NETWORK_EVPOLL_ENABLE_LIBUV) && defined(NETWORK_ENABLE_CURL)

#if __cplusplus >= 201103L || (defined(_MSC_VER) && _MSC_VER >= 1800)
#include <type_traits>

static_assert(std::is_pod<util::network::http_request::curl_poll_context_t>::value, "curl_poll_context_t must be a POD type");

#endif

#define CHECK_FLAG(f, v) !!((f) & (v))
#define SET_FLAG(f, v) (f) |= (v)
#define UNSET_FLAG(f, v) (f) &= (~(v))

namespace util {
    namespace network {
        namespace detail {
            /* initialize custom header list (stating that Expect: 100-continue is not wanted */
            static const char custom_no_expect_header[] = "Expect:";
            static const char content_type_multipart_post[] = "Content-Type: application/x-www-form-urlencoded";
            // static const char content_type_multipart_form_data[] = "Content-Type: multipart/form-data";
        }

        http_request::ptr_t http_request::create(curl_m_bind_t *curl_multi, const std::string &url) {
            ptr_t ret = create(curl_multi);
            if (ret) {
                ret->set_url(url);
            }

            return ret;
        }

        http_request::ptr_t http_request::create(curl_m_bind_t *curl_multi) {
            ptr_t ret = std::make_shared<http_request>(curl_multi);
            if (ret->mutable_request()) {
                return ret;
            }

            return ptr_t();
        }

        int http_request::get_status_code_group(int code) { return code / 100; }

        http_request::http_request(curl_m_bind_t *curl_multi)
            : timeout_ms_(0), bind_m_(curl_multi), request_(NULL), flags_(0), response_code_(0), last_error_code_(0), priv_data_(NULL) {
            http_form_.begin = NULL;
            http_form_.end = NULL;
            http_form_.headerlist = NULL;
            http_form_.posted_size = 0;
            http_form_.uploaded_file = NULL;
            http_form_.flags = 0;
            set_user_agent("libcurl");
        }

        http_request::~http_request() { cleanup(); }

        int http_request::start(method_t::type method, bool wait) {
            CURL *req = mutable_request();
            if (NULL == req) {
                return -1;
            }

            if (!wait && (NULL == bind_m_ || NULL == bind_m_->curl_multi)) {
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

            if (NULL != http_form_.begin) {
                http_form_.headerlist = curl_slist_append(http_form_.headerlist, ::util::network::detail::custom_no_expect_header);
                curl_easy_setopt(req, CURLOPT_HTTPPOST, http_form_.begin);
                // curl_easy_setopt(req, CURLOPT_VERBOSE, 1L);
            }
            if (!post_data_.empty()) {
                set_opt_long(CURLOPT_POSTFIELDSIZE, post_data_.size());
                curl_easy_setopt(req, CURLOPT_POSTFIELDS, post_data_.c_str());
                // curl_easy_setopt(req, CURLOPT_COPYPOSTFIELDS, post_data_.c_str());
            }

            if (NULL != http_form_.headerlist) {
                curl_easy_setopt(req, CURLOPT_HTTPHEADER, http_form_.headerlist);
            }

            if (http_form_.flags & form_list_t::EN_FLFT_WRITE_FORM_USE_FUNC) {
                curl_easy_setopt(req, CURLOPT_READFUNCTION, curl_callback_on_read);
                curl_easy_setopt(req, CURLOPT_READDATA, this);

                long infile_size = 0;
                if (NULL != http_form_.uploaded_file) {
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

        int http_request::stop() {
            if (NULL == request_) {
                return -1;
            }

            cleanup();
            return 0;
        }

        void http_request::remove_curl_request() {
            CURL *req = request_;
            request_ = NULL;
            if (NULL != req) {
                if (NULL != bind_m_ && CHECK_FLAG(flags_, flag_t::EN_FT_CURL_MULTI_HANDLE)) {
                    last_error_code_ = curl_multi_remove_handle(bind_m_->curl_multi, req);
                    UNSET_FLAG(flags_, flag_t::EN_FT_CURL_MULTI_HANDLE);
                }

                curl_easy_cleanup(req);
            }

            if (NULL != http_form_.begin) {
                curl_formfree(http_form_.begin);
                http_form_.begin = NULL;
                http_form_.end = NULL;
                http_form_.qs_fields.clear();
            }

            if (NULL != http_form_.headerlist) {
                curl_slist_free_all(http_form_.headerlist);
                http_form_.headerlist = NULL;
            }

            http_form_.posted_size = 0;

            if (NULL != http_form_.uploaded_file) {
                fclose(http_form_.uploaded_file);
                http_form_.uploaded_file = NULL;
            }
            http_form_.flags = 0;

            post_data_.clear();
        }

        void http_request::cleanup() {
            if (CHECK_FLAG(flags_, flag_t::EN_FT_CLEANING)) {
                return;
            }
            SET_FLAG(flags_, flag_t::EN_FT_CLEANING);

            remove_curl_request();

            UNSET_FLAG(flags_, flag_t::EN_FT_CLEANING);
        }

        void http_request::set_url(const std::string &v) {
            if (NULL == mutable_request()) {
                return;
            }

            url_ = v;
            curl_easy_setopt(mutable_request(), CURLOPT_URL, url_.c_str());
        }

        const std::string &http_request::get_url() const { return url_; }

        void http_request::set_user_agent(const std::string &v) {
            if (NULL == mutable_request()) {
                return;
            }

            useragent_ = v;
            curl_easy_setopt(mutable_request(), CURLOPT_USERAGENT, url_.c_str());
        }

        const std::string &http_request::get_user_agent() const { return useragent_; }

        std::string &http_request::post_data() { return post_data_; }
        const std::string &http_request::post_data() const { return post_data_; }

        int http_request::get_response_code() const {
            if (0 != response_code_) {
                return response_code_;
            }

            if (NULL != request_) {
                long rsp_code = 0;
                curl_easy_getinfo(request_, CURLINFO_RESPONSE_CODE, &rsp_code);
                response_code_ = static_cast<int>(rsp_code);
            }

            return response_code_;
        }

        int http_request::get_error_code() const { return last_error_code_; }

        int http_request::add_form_file(const std::string &fieldname, const char *filename) {
            if (NULL != http_form_.uploaded_file) {
                fclose(http_form_.uploaded_file);
                http_form_.uploaded_file = NULL;
            }

            UTIL_FS_OPEN(res, http_form_.uploaded_file, filename, "rb");
            if (0 != res) {
                return res;
            }

            int ret = curl_formadd(&http_form_.begin, &http_form_.end, CURLFORM_COPYNAME, fieldname.c_str(), CURLFORM_NAMELENGTH,
                                   static_cast<long>(fieldname.size()), CURLFORM_FILE, filename, CURLFORM_END);
            if (0 == ret) {
                http_form_.flags |= form_list_t::EN_FLFT_HAS_FORM_FILE;
            } else {
                fclose(http_form_.uploaded_file);
                http_form_.uploaded_file = NULL;
            }
            return ret;
        }

        int http_request::add_form_file(const std::string &fieldname, const char *filename, const char *content_type,
                                        const char *new_filename) {
            if (NULL != http_form_.uploaded_file) {
                fclose(http_form_.uploaded_file);
                http_form_.uploaded_file = NULL;
            }

            UTIL_FS_OPEN(res, http_form_.uploaded_file, filename, "rb");
            if (0 != res) {
                return res;
            }

            int ret = curl_formadd(&http_form_.begin, &http_form_.end, CURLFORM_COPYNAME, fieldname.c_str(), CURLFORM_NAMELENGTH,
                                   static_cast<long>(fieldname.size()), CURLFORM_FILE, filename, CURLFORM_CONTENTTYPE, content_type,
                                   CURLFORM_FILENAME, new_filename, CURLFORM_END);
            if (0 == ret) {
                http_form_.flags |= form_list_t::EN_FLFT_HAS_FORM_FILE;
            } else {
                fclose(http_form_.uploaded_file);
                http_form_.uploaded_file = NULL;
            }
            return ret;
        }

        int http_request::add_form_file(const std::string &fieldname, const char *filename, const char *content_type) {
            if (NULL != http_form_.uploaded_file) {
                fclose(http_form_.uploaded_file);
                http_form_.uploaded_file = NULL;
            }

            UTIL_FS_OPEN(res, http_form_.uploaded_file, filename, "rb");
            if (0 != res) {
                return res;
            }

            int ret = curl_formadd(&http_form_.begin, &http_form_.end, CURLFORM_COPYNAME, fieldname.c_str(), CURLFORM_NAMELENGTH,
                                   static_cast<long>(fieldname.size()), CURLFORM_FILE, filename, CURLFORM_CONTENTTYPE, content_type,
                                   CURLFORM_END);
            if (0 == ret) {
                http_form_.flags |= form_list_t::EN_FLFT_HAS_FORM_FILE;
            } else {
                fclose(http_form_.uploaded_file);
                http_form_.uploaded_file = NULL;
            }
            return ret;
        }

        int http_request::add_form_field(const std::string &fieldname, const std::string &fieldvalue) {
            http_form_.qs_fields.set(fieldname, fieldvalue);
            http_form_.flags |= form_list_t::EN_FLFT_HAS_FORM_FIELD;
            // int ret = add_form_field(fieldname, fieldvalue.c_str(), fieldvalue.size());
            return 0;
        }

        void http_request::set_opt_ssl_verify_peer(bool v) { set_opt_bool(CURLOPT_SSL_VERIFYPEER, v); }

        void http_request::set_opt_no_signal(bool v) { set_opt_bool(CURLOPT_NOSIGNAL, v); }

        void http_request::set_opt_follow_location(bool v) { set_opt_bool(CURLOPT_FOLLOWLOCATION, v); }

        void http_request::set_opt_verbose(bool v) { set_opt_bool(CURLOPT_VERBOSE, v); }

        void http_request::set_opt_http_content_decoding(bool v) { set_opt_bool(CURLOPT_HTTP_CONTENT_DECODING, v); }

        void http_request::set_opt_keepalive(time_t idle, time_t interval) {
            if (0 == idle && 0 == interval) {
                set_opt_bool(CURLOPT_TCP_KEEPALIVE, false);
            } else {
                set_opt_bool(CURLOPT_TCP_KEEPALIVE, true);
            }

            set_opt_long(CURLOPT_TCP_KEEPIDLE, idle);
            set_opt_long(CURLOPT_TCP_KEEPINTVL, interval);
        }

        // void http_request::set_opt_connect_timeout(time_t timeout_ms) {
        //    set_opt_long(CURLOPT_CONNECTTIMEOUT_MS, timeout_ms);
        //}

        // void http_request::set_opt_request_timeout(time_t timeout_ms) {
        //    set_opt_long(CURLOPT_TIMEOUT_MS, timeout_ms);
        //}

        void http_request::set_opt_timeout(time_t timeout_ms) {
            timeout_ms_ = timeout_ms;
            // set_opt_connect_timeout(timeout_ms);
            // set_opt_request_timeout(timeout_ms);
        }

        const http_request::on_progress_fn_t &http_request::get_on_progress() const { return on_progress_fn_; }
        void http_request::set_on_progress(on_progress_fn_t fn) {
            on_progress_fn_ = fn;

            CURL *req = mutable_request();
            if (NULL == req) {
                return;
            }

            if (on_progress_fn_) {
                set_opt_bool(CURLOPT_NOPROGRESS, false);
                curl_easy_setopt(req, CURLOPT_PROGRESSFUNCTION, curl_callback_on_progress);
                curl_easy_setopt(req, CURLOPT_PROGRESSDATA, this);
            } else {
                set_opt_bool(CURLOPT_NOPROGRESS, true);
                curl_easy_setopt(req, CURLOPT_PROGRESSFUNCTION, NULL);
                curl_easy_setopt(req, CURLOPT_PROGRESSDATA, NULL);
            }
        }

        const http_request::on_header_fn_t &http_request::get_on_header() const { return on_header_fn_; }
        void http_request::set_on_header(on_header_fn_t fn) {
            on_header_fn_ = fn;

            CURL *req = mutable_request();
            if (NULL == req) {
                return;
            }

            if (on_header_fn_) {
                curl_easy_setopt(req, CURLOPT_HEADERFUNCTION, curl_callback_on_header);
                curl_easy_setopt(req, CURLOPT_HEADERDATA, this);
            } else {
                curl_easy_setopt(req, CURLOPT_HEADERFUNCTION, NULL);
                curl_easy_setopt(req, CURLOPT_HEADERDATA, NULL);
            }
        }

        const http_request::on_success_fn_t &http_request::get_on_success() const { return on_success_fn_; }
        void http_request::set_on_success(on_success_fn_t fn) { on_success_fn_ = fn; }

        const http_request::on_error_fn_t &http_request::get_on_error() const { return on_error_fn_; }
        void http_request::set_on_error(on_error_fn_t fn) { on_error_fn_ = fn; }

        const http_request::on_complete_fn_t &http_request::get_on_complete() const { return on_complete_fn_; }
        void http_request::set_on_complete(on_complete_fn_t fn) { on_complete_fn_ = fn; }

        bool http_request::is_running() const { return CHECK_FLAG(flags_, flag_t::EN_FT_RUNNING); }

        void http_request::finish_req_rsp() {
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

        CURL *http_request::mutable_request() {
            if (NULL != request_) {
                return request_;
            }

            request_ = curl_easy_init();
            if (NULL != request_) {
                curl_easy_setopt(request_, CURLOPT_PRIVATE, this);
                curl_easy_setopt(request_, CURLOPT_WRITEDATA, this);
                curl_easy_setopt(request_, CURLOPT_WRITEFUNCTION, curl_callback_on_write);
                curl_easy_setopt(request_, CURLOPT_ERRORBUFFER, error_buffer_);
                error_buffer_[0] = 0;
                error_buffer_[sizeof(error_buffer_) - 1] = 0;
            }

            return request_;
        }

        void http_request::build_http_form(method_t::type method) {
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
                    http_form_.headerlist = curl_slist_append(http_form_.headerlist, ::util::network::detail::custom_no_expect_header);
                    http_form_.headerlist = curl_slist_append(http_form_.headerlist, ::util::network::detail::content_type_multipart_post);
                    http_form_.qs_fields.to_string().swap(post_data_);
                    http_form_.flags |= form_list_t::EN_FLFT_WRITE_FORM_USE_FUNC;

                    if (NULL != http_form_.uploaded_file) {
                        fclose(http_form_.uploaded_file);
                        http_form_.uploaded_file = NULL;
                    }
                } else if (NULL != http_form_.uploaded_file) {
                    http_form_.headerlist = curl_slist_append(http_form_.headerlist, ::util::network::detail::custom_no_expect_header);
                }
            } else if (!http_form_.qs_fields.empty()) {
                http_form_.headerlist = curl_slist_append(http_form_.headerlist, ::util::network::detail::custom_no_expect_header);

                for (util::tquerystring::data_const_iterator iter = http_form_.qs_fields.data().begin();
                     iter != http_form_.qs_fields.data().end(); ++iter) {

                    if (util::types::ITEM_TYPE_STRING == iter->second->type()) {
                        util::types::item_string *val = dynamic_cast<util::types::item_string *>(iter->second.get());

                        if (NULL != val) {
                            curl_formadd(&http_form_.begin, &http_form_.end, CURLFORM_PTRNAME, iter->first.c_str(), CURLFORM_NAMELENGTH,
                                         static_cast<long>(iter->first.size()), CURLFORM_PTRCONTENTS, val->data().c_str(),
#if LIBCURL_VERSION_MAJOR > 7 || (7 == LIBCURL_VERSION_MAJOR && LIBCURL_VERSION_MINOR >= 46)
                                         CURLFORM_CONTENTLEN, static_cast<curl_off_t>(val->data().size()),
#else
                                         CURLFORM_CONTENTSLENGTH, static_cast<long>(val->data().size()),
#endif
                                         CURLFORM_END);
                        }
                    } else {
                        std::string val = iter->second->to_string();
                        curl_formadd(&http_form_.begin, &http_form_.end, CURLFORM_PTRNAME, iter->first.c_str(), CURLFORM_NAMELENGTH,
                                     static_cast<long>(iter->first.size()), CURLFORM_COPYCONTENTS, val.c_str(), CURLFORM_CONTENTSLENGTH,
                                     static_cast<long>(val.size()), CURLFORM_END);
                    }
                }

                if (NULL != http_form_.uploaded_file) {
                    fclose(http_form_.uploaded_file);
                    http_form_.uploaded_file = NULL;
                }
            }
        }

        http_request::curl_poll_context_t *http_request::malloc_poll(http_request *req, curl_socket_t sockfd) {
            if (NULL == req) {
                abort();
            }

            assert(req->bind_m_);
            assert(req->bind_m_->ev_loop);

            curl_poll_context_t *ret = reinterpret_cast<curl_poll_context_t *>(malloc(sizeof(curl_poll_context_t)));
            if (NULL == ret) {
                return ret;
            }

            ret->sockfd = sockfd;
            ret->bind_multi = req->bind_m_;
            uv_poll_init_socket(req->bind_m_->ev_loop, &ret->poll_object, sockfd);
            ret->poll_object.data = ret;

            return ret;
        }

        void http_request::free_poll(curl_poll_context_t *p) { free(p); }

        void http_request::check_multi_info(CURLM *curl_handle) {
            CURLMsg *message;
            int pending;

            while ((message = curl_multi_info_read(curl_handle, &pending))) {
                switch (message->msg) {
                case CURLMSG_DONE: {
                    http_request *req = NULL;
                    curl_easy_getinfo(message->easy_handle, CURLINFO_PRIVATE, &req);
                    assert(req);

                    http_request::ptr_t req_p = req->shared_from_this();
                    req->last_error_code_ = message->data.result;
                    // this may cause req not available any more
                    req_p->finish_req_rsp();
                    req_p->remove_curl_request();
                    break;
                }
                default:
                    break;
                }
            }
        }

        http_request::curl_m_bind_t::curl_m_bind_t() : ev_loop(NULL), curl_multi(NULL) {}

        int http_request::create_curl_multi(uv_loop_t *evloop, std::shared_ptr<curl_m_bind_t> &manager) {
            assert(evloop);
            if (manager) {
                destroy_curl_multi(manager);
            }
            manager = std::make_shared<curl_m_bind_t>();
            if (!manager) {
                return -1;
            }

            manager->curl_multi = curl_multi_init();
            if (NULL == manager->curl_multi) {
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
            ret = curl_multi_setopt(manager->curl_multi, CURLMOPT_SOCKETDATA, manager.get());
            ret = curl_multi_setopt(manager->curl_multi, CURLMOPT_TIMERFUNCTION, http_request::curl_callback_start_timer);
            ret = curl_multi_setopt(manager->curl_multi, CURLMOPT_TIMERDATA, manager.get());

            return ret;
        }

        int http_request::destroy_curl_multi(std::shared_ptr<curl_m_bind_t> &manager) {
            if (!manager) {
                return 0;
            }

            assert(manager->ev_loop);
            assert(manager->curl_multi);

            int ret = curl_multi_cleanup(manager->curl_multi);
            manager->curl_multi = NULL;

            // hold self in case of timer in libuv invalid
            manager->self_holder = manager;
            uv_timer_stop(&manager->ev_timeout);
            uv_close(reinterpret_cast<uv_handle_t *>(&manager->ev_timeout), http_request::ev_callback_on_timer_closed);

            manager.reset();
            return ret;
        }

        void http_request::ev_callback_on_timer_closed(uv_handle_t *handle) {
            curl_m_bind_t *bind = reinterpret_cast<curl_m_bind_t *>(handle->data);
            assert(bind);

            // release self holder
            bind->self_holder.reset();
        }

        void http_request::ev_callback_on_poll_closed(uv_handle_t *handle) {
            curl_poll_context_t *context = reinterpret_cast<curl_poll_context_t *>(handle->data);
            assert(context);

            free_poll(context);
        }

        void http_request::ev_callback_on_timeout(uv_timer_t *handle) {
            curl_m_bind_t *bind = reinterpret_cast<curl_m_bind_t *>(handle->data);
            assert(bind);

            int running_handles = 0;
            curl_multi_socket_action(bind->curl_multi, CURL_SOCKET_TIMEOUT, 0, &running_handles);
            check_multi_info(bind->curl_multi);
        }

        void http_request::ev_callback_curl_perform(uv_poll_t *req, int status, int events) {
            curl_poll_context_t *context = reinterpret_cast<curl_poll_context_t *>(req->data);
            assert(context);

            uv_timer_stop(&context->bind_multi->ev_timeout);
            int running_handles;
            int flags = 0;

            if (status < 0) {
                flags |= CURL_CSELECT_ERR;
            }
            if (events & UV_READABLE) {
                flags |= CURL_CSELECT_IN;
            }
            if (events & UV_WRITABLE) {
                flags |= CURL_CSELECT_OUT;
            }

            if (0 == flags) {
                return;
            }

            curl_multi_socket_action(context->bind_multi->curl_multi, context->sockfd, flags, &running_handles);
            check_multi_info(context->bind_multi->curl_multi);
        }

        void http_request::curl_callback_start_timer(CURLM *multi, long timeout_ms, void *userp) {
            curl_m_bind_t *bind = reinterpret_cast<curl_m_bind_t *>(userp);
            assert(bind);

            // @see https://curl.haxx.se/libcurl/c/evhiperfifo.html
            // @see https://gist.github.com/clemensg/5248927
            // @see https://curl.haxx.se/libcurl/c/multi-uv.html
            if (timeout_ms <= 0) {
                timeout_ms = 1;
            }

            uv_timer_start(&bind->ev_timeout, http_request::ev_callback_on_timeout, static_cast<uint64_t>(timeout_ms), 0);
        }

        int http_request::curl_callback_handle_socket(CURL *easy, curl_socket_t s, int action, void *userp, void *socketp) {
            curl_poll_context_t *context = reinterpret_cast<curl_poll_context_t *>(socketp);
            if (action == CURL_POLL_IN || action == CURL_POLL_OUT || action == CURL_POLL_INOUT) {
                if (NULL == context) {
                    http_request *req;
                    curl_easy_getinfo(easy, CURLINFO_PRIVATE, &req);
                    assert(req->bind_m_);
                    assert(req->bind_m_->curl_multi);

                    context = malloc_poll(req, s);
                    req->last_error_code_ = curl_multi_assign(req->bind_m_->curl_multi, s, context);
                }
            }

            int res = 0;
            switch (action) {
            case CURL_POLL_IN: {
                res = uv_poll_start(&context->poll_object, UV_READABLE, ev_callback_curl_perform);
                break;
            }
            case CURL_POLL_OUT: {
                res = uv_poll_start(&context->poll_object, UV_WRITABLE, ev_callback_curl_perform);
                break;
            }
            case CURL_POLL_INOUT: {
                res = uv_poll_start(&context->poll_object, UV_READABLE | UV_WRITABLE, ev_callback_curl_perform);
                break;
            }
            case CURL_POLL_REMOVE: {
                if (context) {
                    CURLM *curl_multi = context->bind_multi->curl_multi;

                    // already removed by libcurl
                    uv_poll_stop(&context->poll_object);
                    uv_close(reinterpret_cast<uv_handle_t *>(&context->poll_object), ev_callback_on_poll_closed);
                    curl_multi_assign(curl_multi, s, NULL);
                }
                break;
            }
            default: { break; }
            }

            return res;
        }

        size_t http_request::curl_callback_on_write(char *ptr, size_t size, size_t nmemb, void *userdata) {
            http_request *self = reinterpret_cast<http_request *>(userdata);
            assert(self);
            self->response_.write(ptr, size * nmemb);
            return size * nmemb;
        }

        int http_request::curl_callback_on_progress(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
            http_request *self = reinterpret_cast<http_request *>(clientp);
            assert(self);
            if (!self->on_progress_fn_) {
                return 0;
            }

            progress_t progress;
            progress.dltotal = dltotal;
            progress.dlnow = dlnow;
            progress.ultotal = ultotal;
            progress.ulnow = ulnow;

            return self->on_progress_fn_(*self, progress);
        }

        size_t http_request::curl_callback_on_read(char *buffer, size_t size, size_t nitems, void *instream) {
            http_request *self = reinterpret_cast<http_request *>(instream);
            assert(self);

            if (self->post_data_.empty() && NULL != self->http_form_.uploaded_file) {
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

        size_t http_request::curl_callback_on_header(char *buffer, size_t size, size_t nitems, void *userdata) {
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
            const char *val = NULL;

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

            if (keylen > 0 && self->on_header_fn_) {
                if (static_cast<size_t>(val - buffer) < nwrite) {
                    self->on_header_fn_(*self, key, keylen, val, nwrite - (val - key));
                } else {
                    self->on_header_fn_(*self, key, keylen, NULL, 0);
                }
            }

            return size * nitems;
        }
    }
}

#endif
