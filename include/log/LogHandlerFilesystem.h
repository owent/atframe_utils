#pragma once

#include <cstdlib>
#include <cstdio>
#include <stdint.h>
#include <string>
#include <vector>
#include <memory>
#include <inttypes.h>
#include <ctime>
#include "std/smart_ptr.h"

#include "LogWrapper.h"

namespace util {
    namespace log {

        class LogHandlerFilesystem
        {
        public:
            LogHandlerFilesystem();
            LogHandlerFilesystem(const std::string& file_name_pattern, std::string suffix = ".%d.log");
            ~LogHandlerFilesystem();

        public:
            void setFilePattern(const std::string& file_name_pattern, std::string suffix = ".%d.log");

            void operator()(LogWrapper::level_t::type level_id, const char* level, const char* content);

            inline const time_t& getCheckInterval() const {
                return check_interval_;
            }

            inline LogHandlerFilesystem& setCheckInterval(time_t check_interval) {
                check_interval_ = check_interval;
                return *this;
            }

            inline const bool& getEnableBuffer() const {
                return enable_buffer_;
            }

            inline LogHandlerFilesystem& setEnableBuffer(bool enable_buffer) {
                enable_buffer_ = enable_buffer;
                return *this;
            }

            inline const size_t& getMaxFileSize() const {
                return max_file_size_;
            }

            inline LogHandlerFilesystem& setMaxFileSize(size_t max_file_size) {
                max_file_size_ = max_file_size;
                return *this;
            }

            inline const size_t& getMaxFileNumber() const {
                return max_file_number_;
            }

            inline LogHandlerFilesystem& setMaxFileNumber(size_t max_file_number) {
                max_file_number_ = max_file_number;
                return *this;
            }

        private:

        private:
            void init();

            std::shared_ptr<FILE*> open_log_file();

            std::string get_log_file();

            static const tm* get_tm();

            static void delete_file(FILE** f);
        private:
            std::vector<std::string> dirs_pattern_;
            std::string log_file_path_;
            std::string log_file_suffix_;   // 文件后缀，必须有一个%d参数用于传入文件分割编号

            time_t check_interval_;
            // 更换文件或目录的检查周期
            time_t last_check_point_;       // 更换文件或目录的检查周期

            bool enable_buffer_;            // 启用缓冲区
            bool inited_;
            size_t max_file_size_;
            size_t max_file_number_;
            size_t open_file_index_;
            std::shared_ptr<FILE*> opened_file_;
        };

    }
}