#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <iostream>

#include "lock/lock_holder.h"
#include "common/file_system.h"

#include "log/log_sink_file_backend.h"

// 默认文件大小是256KB
#define DEFAULT_FILE_SIZE 256 * 1024

namespace util {
    namespace log {

        log_sink_file_backend::log_sink_file_backend()
            : rotation_size_(10),    // 默认10个文件
            max_file_size_(DEFAULT_FILE_SIZE), // 默认文件大小
            check_interval_(60), // 默认文件切换检查周期为60秒
            check_expire_point_(0), inited_(false) {

            path_pattern_ = "%Y-%m-%d.%N.log";// 默认文件名规则
            log_file_.auto_flush = false;
            log_file_.rotation_index = 0;
            log_file_.written_size = 0;
        }

        log_sink_file_backend::log_sink_file_backend(const std::string &file_name_pattern)
            : rotation_size_(10),    // 默认10个文件
            max_file_size_(DEFAULT_FILE_SIZE), // 默认文件大小
            check_interval_(60), // 默认文件切换检查周期为60秒
            check_expire_point_(0), inited_(false) {

            path_pattern_ = "%Y-%m-%d.%N.log";// 默认文件名规则
            log_file_.auto_flush = false;
            log_file_.rotation_index = 0;
            log_file_.written_size = 0;

            set_file_pattern(file_name_pattern);
        }

        log_sink_file_backend::~log_sink_file_backend() {}

        void log_sink_file_backend::set_file_pattern(const std::string &file_name_pattern) {
            // 设置文件路径模式， 如果文件已打开，需要重新执行初始化流程
            if (log_file_.opened_file) {
                inited_ = false;
                init();
            }
        }

        void log_sink_file_backend::operator()(const log_formatter::caller_info_t &caller, const char *content, size_t content_size) {
            if (!inited_) {
                init();
            }

            std::shared_ptr<std::ofstream> f = open_log_file();
            if (!f) {
                return;
            }

            f->write(content, content_size);
            f->put('\n');
            if (log_file_.auto_flush) {
                f->flush();
            }
        }

        void log_sink_file_backend::init() {
            lock::lock_holder<lock::spin_lock> lkholder(fs_lock_);
            if (inited_) {
                return;
            }

            inited_ = true;

            log_file_.rotation_index = 0;
            log_file_.written_size = 0;
            log_file_.opened_file.reset();
            log_file_.file_path.clear();

            log_formatter::caller_info_t caller;
            char log_file[file_system::MAX_PATH_LEN];

            for (size_t i = 0; max_file_size_ > 0 && i < rotation_size_; ++i) {
                caller.rotate_index = (log_file_.rotation_index + i) % rotation_size_;
                size_t fsz = 0;
                log_formatter::format(log_file, sizeof(log_file), path_pattern_.c_str(), path_pattern_.size(), caller);
                file_system::file_size(log_file, fsz);

                // 文件不存在fsz也是0
                if (fsz < max_file_size_) {
                    log_file_.rotation_index = caller.rotate_index;
                }
                
            }

            for (open_file_index_ = 0; open_file_index_ < max_file_number_; ++open_file_index_) {
                std::string real_path = get_log_file();

                if (NULL != (*opened_file_)) {
                    fclose(*opened_file_);
                }

                (*opened_file_) = fopen(real_path.c_str(), "a+");
                if (NULL != (*opened_file_)) {
                    fseek((*opened_file_), 0, SEEK_END);
                    size_t file_size = ftell((*opened_file_));
                    if (file_size < max_file_size_) {
                        log_file_path_ = real_path;
                        break;
                    }
                }
            }

            open_file_index_ = open_file_index_ % max_file_number_;
        }

        std::shared_ptr<FILE *> log_sink_file_backend::open_log_file() {
            std::string real_path;

            size_t file_size = max_file_size_;
            if (opened_file_ && NULL != (*opened_file_)) {
                file_size = static_cast<size_t>(ftell(*opened_file_));
            }

            if (opened_file_ && NULL != (*opened_file_) && file_size < max_file_size_) {
                time_t now = log_wrapper::getLogTime();
                time_t cp = now >= last_check_point_ ? now - last_check_point_ : last_check_point_ - now;

                if (cp < check_interval_) {
                    return opened_file_;
                }

                last_check_point_ = now;
                real_path = get_log_file();
                // 文件目录不变
                if (real_path == log_file_path_) {
                    return opened_file_;
                }
            }

            // open new file
            {
                open_file_index_ = (open_file_index_ + 1) % max_file_number_;

                typedef FILE *ft;
                opened_file_ = std::shared_ptr<ft>(new ft(NULL), delete_file);

                real_path.clear(); // 文件名失效
            }

            // 重算文件名
            real_path = get_log_file();

            FILE *clear_fd = fopen(real_path.c_str(), "w");
            if (NULL != clear_fd) {
                fclose(clear_fd);
            }

            *opened_file_ = fopen(real_path.c_str(), "a");
            if (NULL == *opened_file_) {
                std::cerr << "[LOG INIT.ERR] open log file " << real_path << " failed." << std::endl;
                return opened_file_;
            }

            log_file_path_ = real_path;
            return opened_file_;
        }

        std::string log_sink_file_backend::get_log_file() {
            std::string real_path;

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

            char os_name[MAX_PATH];
            real_path.reserve(MAX_PATH);

            for (size_t i = 0; i < dirs_pattern_.size(); ++i) {
                size_t len = strftime(os_name, sizeof(os_name), dirs_pattern_[i].c_str(), get_tm());
                if (!real_path.empty()) {
#ifdef WIN32
                    real_path += "\\";
#else
                    real_path += "/";
#endif
                }

                real_path.append(os_name, len);
                // 目录，递归创建
                if (i != dirs_pattern_.size() - 1) {
                    if (FUNC_ACCESS(real_path.c_str())) {
                        if (FUNC_MKDIR(real_path.c_str())) {
                            std::cerr << "[LOG INIT.ERR] create directory " << real_path << " failed." << std::endl;
                            return real_path;
                        } else {
                            // 只要换目录，一定从0重新开始
                            open_file_index_ = 0;
                        }
                    }
                }
            }

            size_t len = sprintf(os_name, log_file_suffix_.c_str(), open_file_index_);
            real_path.append(os_name, len);

            return real_path;
        }
    }
}
