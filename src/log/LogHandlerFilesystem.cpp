#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <iostream>

#include "log/LogHandlerFilesystem.h"

#ifdef _MSC_VER
#include <io.h>
#include <direct.h>
#define FUNC_MKDIR(x) _mkdir(x)
#define FUNC_ACCESS(x) _access(x, 0)

#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#define FUNC_MKDIR(x) mkdir(x, S_IRWXU | S_IRWXG | S_IRGRP | S_IWGRP | S_IROTH)
#define FUNC_ACCESS(x) access(x, F_OK)

#endif

// 默认文件大小是256KB
#define DEFAULT_FILE_SIZE 256 * 1024

namespace util {
    namespace log {

        LogHandlerFilesystem::LogHandlerFilesystem()
            : check_interval_(60), // 默认文件切换检查周期为60秒
              last_check_point_(0), enable_buffer_(false), inited_(false), max_file_size_(DEFAULT_FILE_SIZE), max_file_number_(10),
              open_file_index_(0) {
            dirs_pattern_.push_back("%Y-%m-%d"); // 默认文件名规则
            log_file_suffix_ = ".%d.log";
        }

        LogHandlerFilesystem::LogHandlerFilesystem(const std::string &file_name_pattern, std::string suffix)
            : check_interval_(60), // 默认文件切换检查周期为60秒
              last_check_point_(0), enable_buffer_(false), inited_(false), max_file_size_(DEFAULT_FILE_SIZE), max_file_number_(10),
              open_file_index_(0) {

            setFilePattern(file_name_pattern, suffix);
        }

        LogHandlerFilesystem::~LogHandlerFilesystem() {}

        void LogHandlerFilesystem::setFilePattern(const std::string &file_name_pattern, std::string suffix) {
            using std::strtok;
            dirs_pattern_.clear();
            // 保证必须有一个 %d
            if (std::string::npos == suffix.find("%d")) {
                suffix.append(".%d");
            }
            log_file_suffix_ = suffix;

            char *paths = static_cast<char *>(malloc(file_name_pattern.size() + 1));
            strncpy(paths, file_name_pattern.c_str(), file_name_pattern.size());
            paths[file_name_pattern.size()] = '\0';

            char *token = strtok(paths, "\\/");
            while (NULL != token) {
                if (0 != strlen(token)) {
                    dirs_pattern_.push_back(token);
                }
                token = strtok(NULL, "\\/");
            }

            free(paths);
        }

        void LogHandlerFilesystem::operator()(log_wrapper::level_t::type level_id, const char *level, const char *content) {
            if (!inited_) {
                init();
            }

            std::shared_ptr<FILE *> f = open_log_file();
            if (f && NULL == *f) {
                return;
            }

            fputs(content, *f);
            fputs("\r\n", *f);
            if (!enable_buffer_) {
                fflush(*f);
            }
        }

        void LogHandlerFilesystem::init() {
            inited_ = true;
            if (!opened_file_) {
                typedef FILE *ft;
                opened_file_ = std::shared_ptr<ft>(new ft(), delete_file);
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

        std::shared_ptr<FILE *> LogHandlerFilesystem::open_log_file() {
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

        std::string LogHandlerFilesystem::get_log_file() {
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

        const tm *LogHandlerFilesystem::get_tm() { return log_wrapper::Instance()->getLogTm(); }

        void LogHandlerFilesystem::delete_file(FILE **f) {
            if (f) {
                if (NULL != *f) {
                    fflush(*f);
                    fclose(*f);
                    *f = NULL;
                }
                delete f;
            }
        }
    }
}
