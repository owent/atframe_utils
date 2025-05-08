/**
 * Copyright 2024 atframework
 * @file file_system.h
 * @note 文件系统统一接口，主要针对跨平台（Windows,Linux,macOS等）提供尽可能统一且行为一致的接口
 * @note 允许混合使用 "/" 或 "\" 作为路径分隔符，支持Mingw、Cygwin
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author OWenT
 * @date 2014.12.15
 *
 * @history
 *
 *
 */

#ifndef UTIL_COMMON__FILESYSTEM_H
#define UTIL_COMMON__FILESYSTEM_H

#pragma once

#include <config/atframe_utils_build_feature.h>
#include <config/compile_optimize.h>
#include <std/explicit_declare.h>

#include <climits>
#include <cstdio>
#include <list>
#include <string>
#include <vector>

#if defined(__CYGWIN__)  // Windows Cygwin
#  define UTIL_FS_POSIX_API
#elif defined(_WIN32)  // Windows default, including MinGW
#  define UTIL_FS_WINDOWS_API
#else
#  define UTIL_FS_POSIX_API
#endif

#ifdef UTIL_FS_WINDOWS_API
#  include <direct.h>
#  include <io.h>

#else
#  include <sys/stat.h>
#  include <sys/types.h>
#  include <unistd.h>

#  if defined(__ANDROID__)
#    define UTIL_FS_DISABLE_LINK 1
#  elif defined(__APPLE__)
#    if __dest_os != __mac_os_x
#      define UTIL_FS_DISABLE_LINK 1
#    endif
#  endif

#endif

#if (defined(_MSC_VER) && _MSC_VER >= 1600) || (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L)
#  define UTIL_FS_OPEN(e, f, path, mode) ATFW_EXPLICIT_UNUSED_ATTR errno_t e = fopen_s(&f, path, mode)
#  define UTIL_FS_CLOSE(f) fclose(f)
#  define UTIL_FS_C11_API
#else
#  include <errno.h>
#  define UTIL_FS_OPEN(e, f, path, mode) \
    f = fopen(path, mode);               \
    ATFW_EXPLICIT_UNUSED_ATTR int e = errno
#  define UTIL_FS_CLOSE(f) fclose(f)
#endif

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
class file_system {
 public:
#if ((defined(__cplusplus) && __cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L))
  static constexpr const char DIRECTORY_SEPARATOR =
#else
  ATFRAMEWORK_UTILS_API static constexpr const char DIRECTORY_SEPARATOR =
#endif
#ifdef _WIN32
      '\\';
#else
      '/';
#endif

  // When LongPathsEnabled on Windows, it allow 32767 characters in a absolute path.But it still only allow 260
  // characters in a relative path.
  // See https://docs.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation
#if ((defined(__cplusplus) && __cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L))
  static constexpr const size_t MAX_PATH_LEN =
#else
  ATFRAMEWORK_UTILS_API static constexpr const size_t MAX_PATH_LEN =
#endif
#if defined(MAX_PATH)
      MAX_PATH;
#elif defined(_MAX_PATH)
      _MAX_PATH;
#elif defined(PATH_MAX)
  PATH_MAX;
#else
  260;
#endif

  struct dir_opt_t {
    enum type {
      EN_DOT_ABSP = 0x0001,  // 转换为绝对路径
      EN_DOT_SELF = 0x0002,  // 包含.和..
      EN_DOT_RLNK = 0x0004,  // 解析符号链接
      EN_DOT_RECU = 0x0010,  // 对目录递归扫描而不是列举出出目录名

      EN_DOT_TDIR = 0x0100,   // 包含目录
      EN_DOT_TREG = 0x0200,   // 包含文件
      EN_DOT_TLNK = 0x0400,   // 包含符号链接
      EN_DOT_TSOCK = 0x0800,  // 包含Unix Sock
      EN_DOT_TOTH = 0x1000,   // 其他类型

      EN_DOT_DAFAULT = 0xFF00,  // 默认规则
    };
  };

  struct link_opt_t {
    enum type {
      EN_LOT_DEFAULT = 0x00,         // hard link for default
      EN_LOT_SYMBOLIC_LINK = 0x01,   // or soft link
      EN_LOT_DIRECTORY_LINK = 0x02,  // it's used only for windows
      EN_LOT_FORCE_REWRITE = 0x04,   // delete the old file if it's exists
    };
  };

 public:
  /**
   * @brief 获取文件内容
   * @param out [OUT] 输出变量
   * @param file_path [IN] 文件路径
   * @param is_binary [IN] 是否是二进制
   * @return 成功返回true
   */
  static ATFRAMEWORK_UTILS_API bool get_file_content(std::string &out, const char *file_path, bool is_binary = false);

  /**
   * @brief 获取文件内容
   * @param out [OUT] 输出变量
   * @param path [IN] 路径
   * @param compact [IN] 是否精简路径（这个功能会尽量移除路径中的.和..）
   * @return 成功返回true
   */
  static ATFRAMEWORK_UTILS_API bool split_path(std::vector<std::string> &out, const char *path, bool compact = false);

  /**
   * @brief 检查文件是否存在
   * @param file_path [IN] 文件路径
   * @return 存在且有权限返回true
   */
  static ATFRAMEWORK_UTILS_API bool is_exist(const char *file_path);

  /**
   * @brief 返回文件的大小
   * @param file_path [IN] 文件路径
   * @param sz [OUT] 文件大小
   * @return 无法打开(不存在，权限不足)返回false, 其他情况返回true
   */
  static ATFRAMEWORK_UTILS_API bool file_size(const char *file_path, size_t &sz);

  /**
   * @brief 创建目录
   * @param dir_path [IN] 目录路径
   * @param recursion [IN] 是否递归创建
   * @param mode [IN] 目录权限（Windows下会被忽略）
   * @return 创建成功返回true
   */
  static ATFRAMEWORK_UTILS_API bool mkdir(const char *dir_path, bool recursion = false, int mode = 0);

  /**
   * @brief 获取目录路径
   * @param file_path [IN] 文件/目录路径
   * @param sz [IN] 文件/目录路径长度
   * @param dir [OUT] 父级目录路径
   * @param depth [IN] 深度，默认1，表示1级父级目录
   * @return 成功返回true
   */
  static ATFRAMEWORK_UTILS_API bool dirname(const char *file_path, size_t sz, std::string &dir, int depth = 1);

  /**
   * @brief 获取当前运行目录
   * @return 当前运行目录
   */
  static ATFRAMEWORK_UTILS_API std::string get_cwd();

  /**
   * @brief 获取绝对路径
   * @param dir_path 相对路径
   * @return 当前运行目录
   */
  static ATFRAMEWORK_UTILS_API std::string get_abs_path(const char *dir_path);

  /**
   * @brief 移动或重命名文件/目录
   * @param from 原始路径
   * @param to 目标路径
   * @return 成功返回true
   */
  static ATFRAMEWORK_UTILS_API bool rename(const char *from, const char *to);

  /**
   * @brief 移除文件/目录
   * @param path 路径
   * @return 成功返回true
   */
  static ATFRAMEWORK_UTILS_API bool remove(const char *path);

  /**
   * @brief 打开一个临时文件
   * @return 临时文件
   */
  static ATFRAMEWORK_UTILS_API std::string getenv(const char *name);

  /**
   * @brief 打开一个临时文件
   * @return 临时文件
   */
  static ATFRAMEWORK_UTILS_API FILE *open_tmp_file();

  /**
   * @brief 生成一个临时文件名
   * @param inout 输入前缀(如果支持)，输出生成的文件名
   * @return 成功返回true，失败返回false
   */
  static ATFRAMEWORK_UTILS_API bool generate_tmp_file_name(std::string &inout);

  /**
   * @brief 列举目录下所有文件
   * @param dir_path 目录路径
   * @param out 录下所有文件路径
   * @return 成功返回0，错误返回错误码(不同平台错误码不同)
   */
  static ATFRAMEWORK_UTILS_API int scan_dir(const char *dir_path, std::list<std::string> &out,
                                            int options = dir_opt_t::EN_DOT_DAFAULT);

  /**
   * @brief 判断是否是绝对路径
   * @param dir_path 目录路径
   * @return 是绝对路径返回true
   */
  static ATFRAMEWORK_UTILS_API bool is_abs_path(const char *dir_path);

#if !defined(UTIL_FS_DISABLE_LINK)
  /**
   * @brief 创建链接
   * @param oldpath 老的文件/目录路径
   * @param newpath 新的文件/目录路径
   * @param options 链接选项
   * @return 成功返回0，错误返回错误码(不同平台错误码不同)
   */
  static ATFRAMEWORK_UTILS_API int link(const char *oldpath, const char *newpath,
                                        int options = link_opt_t::EN_LOT_DEFAULT);
#endif
};
ATFRAMEWORK_UTILS_NAMESPACE_END

#endif  // UTIL_COMMON__FILESYSTEM_H
