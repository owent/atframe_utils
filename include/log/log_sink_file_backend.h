/**
 * @file log_sink_file_backend.h
 * @brief 日志文件后端
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author owent
 * @date 2016-03-31
 * @history
 */

#ifndef UTIL_LOG_LOG_SINK_FILE_BACKEND_H
#define UTIL_LOG_LOG_SINK_FILE_BACKEND_H

#pragma once

#include "lock/spin_lock.h"
#include "std/smart_ptr.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <stdint.h>
#include <string>
#include <vector>


#include "log_formatter.h"

namespace util {
    namespace log {
        /**
         * @brief 文件日志后端
         */
        class log_sink_file_backend {
        public:
            log_sink_file_backend();
            log_sink_file_backend(const std::string &file_name_pattern);
            log_sink_file_backend(const log_sink_file_backend &other);
            ~log_sink_file_backend();

        public:
            void set_file_pattern(const std::string &file_name_pattern);

            void operator()(const log_formatter::caller_info_t &caller, const char *content, size_t content_size);

            inline time_t get_check_interval() const { return check_interval_; }

            inline log_sink_file_backend &set_check_interval(time_t check_interval) {
                check_interval_ = check_interval;
                return *this;
            }

            inline uint32_t get_auto_flush() const { return log_file_.auto_flush; }

            inline log_sink_file_backend &set_auto_flush(uint32_t flush_level) {
                log_file_.auto_flush = flush_level;
                return *this;
            }

            inline size_t get_max_file_size() const { return max_file_size_; }

            inline log_sink_file_backend &set_max_file_size(size_t max_file_size) {
                max_file_size_ = max_file_size;
                return *this;
            }

            inline uint32_t get_rotate_size() const { return rotation_size_; }

            inline log_sink_file_backend &set_rotate_size(uint32_t sz) {
                // 轮训sz不能为0
                if (sz <= 1) {
                    sz = 1;
                }
                rotation_size_ = sz;
                return *this;
            }

        private:
            void init();

            std::shared_ptr<std::ofstream> open_log_file(bool destroy_content);

            void rotate_log();

            void check_update();

            void reset_log_file();

        private:
            // 第一个first表示是否需要format
            std::string path_pattern_;

            uint32_t rotation_size_; // 轮询滚动size
            size_t max_file_size_;   // log文件size限制


            time_t check_interval_;     // 更换文件或目录的检查周期
            time_t check_expire_point_; // 更换文件或目录的检查周期
            bool inited_;
            lock::spin_lock fs_lock_;


            struct file_impl_t {
                uint32_t auto_flush; // 当日记级别高于或等于这个时，将会强制执行一次flush
                uint32_t rotation_index;
                size_t written_size;
                std::shared_ptr<std::ofstream> opened_file;
                std::string file_path;
            };
            file_impl_t log_file_;
        };
    }
}

#endif
