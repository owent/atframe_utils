// Copyright 2021 atframework
// Licensed under the MIT licenses.
// Created by owent on 2016-03-31

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <iostream>
#include <sstream>

#include "common/file_system.h"
#include "common/string_oprs.h"
#include "lock/lock_holder.h"
#include "time/time_utility.h"

#include "log/log_sink_file_backend.h"

// 默认文件大小是256KB
#define DEFAULT_FILE_SIZE 256 * 1024

namespace util {
namespace log {

LIBATFRAME_UTILS_API log_sink_file_backend::log_sink_file_backend()
    : rotation_size_(10),                 // 默认10个文件
      max_file_size_(DEFAULT_FILE_SIZE),  // 默认文件大小
      check_interval_(0),                 // 默认文件切换检查周期
      flush_interval_(0),                 // 默认关闭定时刷入
      inited_(false) {
  log_file_.opened_file_point_ = 0;
  log_file_.last_flush_timepoint_ = 0;
  log_file_.auto_flush = log_formatter::level_t::LOG_LW_DISABLED;
  log_file_.rotation_index = 0;
  log_file_.written_size = 0;

  set_file_pattern("%Y-%m-%d.%N.log");  // 默认文件名规则
}

LIBATFRAME_UTILS_API log_sink_file_backend::log_sink_file_backend(const std::string &file_name_pattern)
    : rotation_size_(10),                 // 默认10个文件
      max_file_size_(DEFAULT_FILE_SIZE),  // 默认文件大小
      check_interval_(0),                 // 默认文件切换检查周期
      flush_interval_(0),                 // 默认关闭定时刷入
      inited_(false) {
  log_file_.opened_file_point_ = 0;
  log_file_.last_flush_timepoint_ = 0;
  log_file_.auto_flush = log_formatter::level_t::LOG_LW_DISABLED;
  log_file_.rotation_index = 0;
  log_file_.written_size = 0;

  set_file_pattern(file_name_pattern);
}

LIBATFRAME_UTILS_API log_sink_file_backend::log_sink_file_backend(const log_sink_file_backend &other)
    : rotation_size_(other.rotation_size_),    // 默认文件数量
      max_file_size_(other.max_file_size_),    // 默认文件大小
      check_interval_(other.check_interval_),  // 默认文件切换检查周期
      flush_interval_(other.flush_interval_),  // 默认定时刷入周期
      inited_(false) {
  log_file_.opened_file_point_ = other.log_file_.opened_file_point_;
  log_file_.last_flush_timepoint_ = other.log_file_.last_flush_timepoint_;
  set_file_pattern(other.path_pattern_);
  alias_writing_pattern_ = other.alias_writing_pattern_;

  log_file_.auto_flush = other.log_file_.auto_flush;

  // 其他的部分都要重新初始化，不能复制
}

LIBATFRAME_UTILS_API log_sink_file_backend::~log_sink_file_backend() {
  if (log_file_.opened_file && log_file_.opened_file->is_open() && !log_file_.opened_file->bad()) {
    log_file_.opened_file->flush();
  }
}

LIBATFRAME_UTILS_API void log_sink_file_backend::set_file_pattern(const std::string &file_name_pattern) {
  // 计算按时间切换的检测间隔
  static time_t check_interval[128] = {0};
  // @see log_formatter::format
  // 计算检查周期，考虑到某些地区有夏令时，所以最大是小时。Unix时间戳会抹平闰秒，所以可以不考虑闰秒
  if (check_interval[(int)'S'] == 0) {
    check_interval[(int)'f'] = 1;
    check_interval[(int)'R'] = util::time::time_utility::MINITE_SECONDS;
    check_interval[(int)'T'] = 1;
    check_interval[(int)'F'] = util::time::time_utility::HOUR_SECONDS;
    check_interval[(int)'S'] = 1;
    check_interval[(int)'M'] = util::time::time_utility::MINITE_SECONDS;
    check_interval[(int)'I'] = util::time::time_utility::HOUR_SECONDS;
    check_interval[(int)'H'] = util::time::time_utility::HOUR_SECONDS;
    check_interval[(int)'w'] = util::time::time_utility::HOUR_SECONDS;
    check_interval[(int)'d'] = util::time::time_utility::HOUR_SECONDS;
    check_interval[(int)'j'] = util::time::time_utility::HOUR_SECONDS;
    check_interval[(int)'m'] = util::time::time_utility::HOUR_SECONDS;
    check_interval[(int)'y'] = util::time::time_utility::HOUR_SECONDS;
    check_interval[(int)'Y'] = util::time::time_utility::HOUR_SECONDS;
  }

  {
    // 改这个成员也要加锁。stl是非线程安全的
    lock::lock_holder<lock::spin_lock> lkholder(init_lock_);

    // 计算检查周期，考虑到某些地区有夏令时，所以最大是小时。Unix时间戳会抹平闰秒，所以可以不考虑闰秒
    check_interval_ = 0;
    for (size_t i = 0; i + 1 < file_name_pattern.size(); ++i) {
      if (file_name_pattern[i] == '%') {
        int checked = file_name_pattern[i + 1];
        if (checked > 0 && checked < 128 && check_interval[checked] > 0) {
          if (0 == check_interval_ || check_interval[checked] < check_interval_) {
            check_interval_ = check_interval[checked];
          }
        }
      }
    }

    path_pattern_ = file_name_pattern;
  }

  // 设置文件路径模式， 如果文件已打开，需要重新执行初始化流程
  if (log_file_.opened_file) {
    inited_ = false;
    init();
  }
}

LIBATFRAME_UTILS_API void log_sink_file_backend::set_writing_alias_pattern(const std::string &file_name_pattern) {
  alias_writing_pattern_ = file_name_pattern;
}

LIBATFRAME_UTILS_API const std::string &log_sink_file_backend::get_writing_alias_pattern(
    const std::string &file_name_pattern) {
  return file_name_pattern;
}

LIBATFRAME_UTILS_API void log_sink_file_backend::operator()(const log_formatter::caller_info_t &caller,
                                                            const char *content, size_t content_size) {
  if (!inited_) {
    init();
  }

  if (log_file_.written_size > 0 && log_file_.written_size >= max_file_size_) {
    rotate_log();
  }
  check_update();

  std::shared_ptr<std::ofstream> f = open_log_file(true);

  if (!f) {
    return;
  }

  f->write(content, content_size);
  f->put('\n');
  time_t now = util::time::time_utility::get_sys_now();

  // 日志级别高于指定级别，需要刷入
  if (static_cast<uint32_t>(caller.level_id) <= log_file_.auto_flush) {
    log_file_.last_flush_timepoint_ = now;
    f->flush();
  }

  // 定期刷入
  if (flush_interval_ > 0 && (log_file_.last_flush_timepoint_ > now  // 说明系统时间被改小了
                              || log_file_.last_flush_timepoint_ + flush_interval_ <= now)) {
    log_file_.last_flush_timepoint_ = now;
    f->flush();
  }

  log_file_.written_size += content_size + 1;
}

LIBATFRAME_UTILS_API time_t log_sink_file_backend::get_check_interval() const { return check_interval_; }

LIBATFRAME_UTILS_API log_sink_file_backend &log_sink_file_backend::set_check_interval(time_t check_interval) {
  check_interval_ = check_interval;
  return *this;
}

LIBATFRAME_UTILS_API time_t log_sink_file_backend::get_flush_interval() const { return flush_interval_; }

LIBATFRAME_UTILS_API log_sink_file_backend &log_sink_file_backend::set_flush_interval(time_t v) {
  flush_interval_ = v;
  return *this;
}

LIBATFRAME_UTILS_API uint32_t log_sink_file_backend::get_auto_flush() const { return log_file_.auto_flush; }

LIBATFRAME_UTILS_API log_sink_file_backend &log_sink_file_backend::set_auto_flush(uint32_t flush_level) {
  log_file_.auto_flush = flush_level;
  return *this;
}

LIBATFRAME_UTILS_API size_t log_sink_file_backend::get_max_file_size() const { return max_file_size_; }

LIBATFRAME_UTILS_API log_sink_file_backend &log_sink_file_backend::set_max_file_size(size_t max_file_size) {
  max_file_size_ = max_file_size;
  return *this;
}

LIBATFRAME_UTILS_API uint32_t log_sink_file_backend::get_rotate_size() const { return rotation_size_; }

LIBATFRAME_UTILS_API log_sink_file_backend &log_sink_file_backend::set_rotate_size(uint32_t sz) {
  // 轮训sz不能为0
  if (sz <= 1) {
    sz = 1;
  }
  rotation_size_ = sz;
  return *this;
}

LIBATFRAME_UTILS_API void log_sink_file_backend::init() {
  if (inited_) {
    return;
  }
  // 双检锁，初始化加锁
  lock::lock_holder<lock::spin_lock> lkholder(init_lock_);
  if (inited_) {
    return;
  }

  inited_ = true;

  log_file_.rotation_index = 0;
  reset_log_file();

  log_formatter::caller_info_t caller;
  char log_file[file_system::MAX_PATH_LEN];

  for (size_t i = 0; max_file_size_ > 0 && i < rotation_size_; ++i) {
    caller.rotate_index = static_cast<uint32_t>((log_file_.rotation_index + i) % rotation_size_);
    size_t fsz = 0;
    log_formatter::format(log_file, sizeof(log_file), path_pattern_.c_str(), path_pattern_.size(), caller);
    file_system::file_size(log_file, fsz);

    // 文件不存在fsz也是0
    if (fsz < max_file_size_) {
      log_file_.rotation_index = caller.rotate_index;
      break;
    }
  }

  open_log_file(false);
}

LIBATFRAME_UTILS_API std::shared_ptr<std::ofstream> log_sink_file_backend::open_log_file(bool destroy_content) {
  if (log_file_.opened_file && log_file_.opened_file->good()) {
    return log_file_.opened_file;
  }

  reset_log_file();

  // 打开新文件要加锁
  lock::lock_holder<lock::spin_lock> lkholder(fs_lock_);

  char log_file[file_system::MAX_PATH_LEN + 1];
  log_formatter::caller_info_t caller;
  caller.rotate_index = log_file_.rotation_index;
  size_t file_path_len =
      log_formatter::format(log_file, sizeof(log_file), path_pattern_.c_str(), path_pattern_.size(), caller);
  if (file_path_len <= 0) {
    std::cerr << "log.format " << path_pattern_ << " failed" << std::endl;
    return std::shared_ptr<std::ofstream>();
  }
  if (file_path_len < sizeof(log_file)) {
    log_file[file_path_len] = 0;
  }

  std::shared_ptr<std::ofstream> of = std::make_shared<std::ofstream>();
  if (!of) {
    std::cerr << "log.file malloc failed: " << path_pattern_ << std::endl;
    return std::shared_ptr<std::ofstream>();
  }

  std::string dir_name;
  util::file_system::dirname(log_file, file_path_len, dir_name);
  if (!dir_name.empty() && !util::file_system::is_exist(dir_name.c_str())) {
    util::file_system::mkdir(dir_name.c_str(), true);
  }

  // 销毁原先的内容
  if (destroy_content) {
    of->open(log_file, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!of->is_open()) {
      std::cerr << "log.file open " << static_cast<const char *>(log_file) << " failed: " << path_pattern_ << std::endl;
      return std::shared_ptr<std::ofstream>();
    }
    of->close();
  }

  of->open(log_file, std::ios::binary | std::ios::out | std::ios::app);
  if (!of->is_open()) {
    std::cerr << "log.file open " << static_cast<const char *>(log_file) << " failed: " << path_pattern_ << std::endl;
    return std::shared_ptr<std::ofstream>();
  }

  of->seekp(0, std::ios_base::end);
  log_file_.written_size = static_cast<size_t>(of->tellp());

  log_file_.opened_file = of;
  log_file_.opened_file_point_ = util::time::time_utility::get_sys_now();
  log_file_.file_path.assign(log_file, file_path_len);

  // 硬链接别名
#if !defined(UTIL_FS_DISABLE_LINK)
  if (!alias_writing_pattern_.empty()) {
    char alias_log_file[file_system::MAX_PATH_LEN + 1];
    file_path_len = log_formatter::format(alias_log_file, sizeof(alias_log_file), alias_writing_pattern_.c_str(),
                                          alias_writing_pattern_.size(), caller);
    if (file_path_len <= 0) {
      std::cerr << "log.format for writing alias " << alias_writing_pattern_ << " failed" << std::endl;
      return log_file_.opened_file;
    }

    if (file_path_len < sizeof(alias_log_file)) {
      alias_log_file[file_path_len] = 0;
    }

    if (0 == UTIL_STRFUNC_STRNCASE_CMP(log_file, alias_log_file, sizeof(alias_log_file))) {
      return log_file_.opened_file;
    }

    int res = util::file_system::link(log_file, alias_log_file, util::file_system::link_opt_t::EN_LOT_FORCE_REWRITE);
    if (res != 0) {
      std::cerr << "link(" << log_file << ", " << alias_log_file << ") failed, errno: " << res << std::endl;
#  ifdef UTIL_FS_WINDOWS_API
      std::cerr
          << "    you can use FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM"
          << " | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, " << res << ", MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), "
          << "(LPTSTR) &lpMsgBuf, 0, nullptr) to get the error message, see "
          << "https://docs.microsoft.com/en-us/windows/desktop/api/WinBase/nf-winbase-formatmessage and "
          << "https://docs.microsoft.com/en-us/windows/desktop/Debug/retrieving-the-last-error-code for more details"
          << std::endl;
#  else
      std::cerr << "    you can use strerror(" << res << ") to get the error message, see "
                << "http://man7.org/linux/man-pages/man3/strerror.3.html or "
                << "https://linux.die.net/man/3/strerror for more details" << std::endl;
#  endif
      return log_file_.opened_file;
    }
  }
#endif

  return log_file_.opened_file;
}

LIBATFRAME_UTILS_API void log_sink_file_backend::rotate_log() {
  if (rotation_size_ > 0) {
    log_file_.rotation_index = (log_file_.rotation_index + 1) % rotation_size_;
  } else {
    log_file_.rotation_index = 0;
  }
  reset_log_file();
}

LIBATFRAME_UTILS_API void log_sink_file_backend::check_update() {
  if (0 != log_file_.opened_file_point_) {
    if (0 == check_interval_ ||
        util::time::time_utility::get_sys_now() / check_interval_ == log_file_.opened_file_point_ / check_interval_) {
      return;
    }
  }

  char log_file[file_system::MAX_PATH_LEN];
  log_formatter::caller_info_t caller;
  caller.rotate_index = log_file_.rotation_index;
  size_t file_path_len =
      log_formatter::format(log_file, sizeof(log_file), path_pattern_.c_str(), path_pattern_.size(), caller);
  if (file_path_len <= 0) {
    return;
  }

  std::string new_file_path;
  std::string old_file_path;

  {
    // 短时间加锁，防止文件路径变更
    lock::lock_holder<lock::spin_lock> lkholder(fs_lock_);
    old_file_path = log_file_.file_path;
  }

  new_file_path.assign(log_file, file_path_len);
  if (new_file_path == old_file_path) {
    // 本次刷新周期内的文件名未变化，说明检测周期小于实际周期，所以这个周期内不需要再检测了
    // 因为考虑到夏时令，只要大于小时的配置检测周期都设置为小时，必然会小于实际周期然后走到这里
    log_file_.opened_file_point_ = util::time::time_utility::get_sys_now();
    return;
  }

  std::string new_dir;
  std::string old_dir;
  util::file_system::dirname(new_file_path.c_str(), new_file_path.size(), new_dir);
  util::file_system::dirname(old_file_path.c_str(), old_file_path.size(), old_dir);

  // 如果目录变化则重置序号
  if (new_dir != old_dir) {
    log_file_.rotation_index = 0;
  }

  reset_log_file();
}

LIBATFRAME_UTILS_API void log_sink_file_backend::reset_log_file() {
  // 更换日志文件需要加锁
  lock::lock_holder<lock::spin_lock> lkholder(fs_lock_);

  // 必须依赖析构来关闭文件，以防这个文件正在其他地方被引用
  log_file_.opened_file.reset();
  log_file_.opened_file_point_ = 0;
  log_file_.written_size = 0;
  // log_file_.file_path.clear(); // 保留上一个文件路径，即便已被关闭。用于rotate后的目录变更判定
}
}  // namespace log
}  // namespace util
