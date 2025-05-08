// Copyright 2023 atframework
// Created by owent on 2015/6/2.
//

#include "common/file_system.h"

#include <std/explicit_declare.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <sstream>

#include "common/compiler_message.h"

#ifdef UTIL_FS_WINDOWS_API

#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif

#  include <Windows.h>
#  include <direct.h>
#  include <io.h>

#  ifdef UNICODE
#    include <atlconv.h>

#    define VC_TEXT(x) A2W(x)
#  else
#    define VC_TEXT(x) x
#  endif

#  define FUNC_ACCESS(x) _access(x, 0)
#  define SAFE_STRTOK_S(...) strtok_s(__VA_ARGS__)
#  define FUNC_MKDIR(path, mode) _mkdir(path)

#else

#  include <dirent.h>
#  include <errno.h>
#  include <fcntl.h>
#  include <sys/types.h>
#  include <unistd.h>

#  define FUNC_ACCESS(x) access(x, F_OK)
#  define SAFE_STRTOK_S(...) strtok_r(__VA_ARGS__)
#  define FUNC_MKDIR(path, mode) ::mkdir(path, mode)

#endif

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN

#if !((defined(__cplusplus) && __cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L))
ATFRAMEWORK_UTILS_API constexpr const char file_system::DIRECTORY_SEPARATOR;
ATFRAMEWORK_UTILS_API constexpr const size_t file_system::MAX_PATH_LEN;
#endif

ATFRAMEWORK_UTILS_API bool file_system::get_file_content(std::string &out, const char *file_path, bool is_binary) {
  FILE *f = nullptr;
  if (is_binary) {
    UTIL_FS_OPEN(error_code, f, file_path, "rb");
    COMPILER_UNUSED(error_code);
  } else {
    UTIL_FS_OPEN(error_code, f, file_path, "r");
    COMPILER_UNUSED(error_code);
  }

  if (nullptr == f) {
    return false;
  }

  fseek(f, 0, SEEK_END);
  auto len = ftell(f);
  fseek(f, 0, SEEK_SET);

  bool ret = true;
  if (len > 0) {
    out.resize(static_cast<size_t>(len));
    size_t real_read_sz = fread(const_cast<char *>(out.data()), sizeof(char), static_cast<size_t>(len), f);
    if (real_read_sz < out.size()) {
      out.resize(real_read_sz);
      // CLRF maybe converted into CL or RF on text mode
      ret = !is_binary;
    }
  } else {
    // fclose(f);
    // f = nullptr;
    // if (is_binary) {
    //     UTIL_FS_OPEN(error_code, f, file_path, "rb");
    //     COMPILER_UNUSED(error_code);
    // } else {
    //     UTIL_FS_OPEN(error_code, f, file_path, "r");
    //     COMPILER_UNUSED(error_code);
    // }

    // if (nullptr == f) {
    //     return false;
    // }

    // 虚拟文件ftell(f)会拿不到长度，只能按流来读
    char buf[4096];  // 4K for each block
    std::stringstream ss;
    while (true) {
      size_t read_sz = fread(buf, 1, sizeof(buf), f);
      ss.write(buf, static_cast<std::streamsize>(read_sz));
      if (read_sz < sizeof(buf)) {
        break;
      }
    }

    ss.str().swap(out);
  }

  fclose(f);
  return ret;
}

ATFRAMEWORK_UTILS_API bool file_system::split_path(std::vector<std::string> &out, const char *path, bool compact) {
  if (nullptr == path) {
    return false;
  }

  char opr_path[MAX_PATH_LEN];

#if defined(UTIL_FS_C11_API)
  strncpy_s(opr_path, sizeof(opr_path), path, strlen(path));
#else
  strncpy(opr_path, path, sizeof(opr_path) - 1);
#endif

  char *saveptr = nullptr;
  char *token = SAFE_STRTOK_S(opr_path, "\\/", &saveptr);
  while (nullptr != token) {
    if (0 != strlen(token)) {
      if (compact) {
        // 紧缩路径
        if (0 == strcmp("..", token)) {
          if (!out.empty() && out.back() != "..") {
            out.pop_back();
          } else {
            out.push_back(token);
          }
        } else if (0 != strcmp(".", token)) {
          out.push_back(token);
        }
      } else {
        out.push_back(token);
      }
    }
    token = SAFE_STRTOK_S(nullptr, "\\/", &saveptr);
  }

  return !out.empty();
}

ATFRAMEWORK_UTILS_API bool file_system::is_exist(const char *file_path) { return 0 == FUNC_ACCESS(file_path); }

ATFRAMEWORK_UTILS_API bool file_system::file_size(const char *file_path, size_t &sz) {
  FILE *f = nullptr;
  UTIL_FS_OPEN(error_code, f, file_path, "rb");
  COMPILER_UNUSED(error_code);

  if (nullptr == f) {
    return false;
  }

  fseek(f, 0, SEEK_END);
  sz = static_cast<size_t>(ftell(f));
  fclose(f);

  return true;
}

ATFRAMEWORK_UTILS_API bool file_system::mkdir(const char *dir_path, bool recursion,
                                              ATFW_EXPLICIT_UNUSED_ATTR int mode) {
#ifndef UTIL_FS_WINDOWS_API
  if (0 == mode) {
    mode = S_IRWXU | S_IRWXG | S_IRWXO;
  }
#endif
  if (!recursion) {
    return 0 == FUNC_MKDIR(dir_path, static_cast<mode_t>(mode));
  }

  std::vector<std::string> path_segs;
  split_path(path_segs, dir_path, true);

  if (path_segs.empty()) {
    return false;
  }

  std::string now_path;
  // 初始路径在Unix系统下会被转为相对路径
  if (nullptr != dir_path && ('/' == *dir_path || '\\' == *dir_path)) {
    // 留一个\0和一个分隔符位
    now_path.reserve(strlen(dir_path) + 4);

    now_path = *dir_path;

    // NFS 支持
    char next_char = *(dir_path + 1);
    if ('/' == next_char || '\\' == next_char) {
      now_path += next_char;
    }
  }

  for (size_t i = 0; i < path_segs.size(); ++i) {
    now_path += path_segs[i];

    if (false == is_exist(now_path.c_str())) {
      if (0 != FUNC_MKDIR(now_path.c_str(), static_cast<mode_t>(mode))) {
        return false;
      }
    }

    now_path += DIRECTORY_SEPARATOR;
  }

  return true;
}

ATFRAMEWORK_UTILS_API bool file_system::dirname(const char *file_path, size_t sz, std::string &dir, int depth) {
  if (nullptr == file_path || 0 == file_path[0]) {
    return false;
  }

  dir.clear();
  if (0 == sz) {
    sz = strlen(file_path);
  }
  --sz;

  while (sz > 0 && ('/' == file_path[sz] || '\\' == file_path[sz])) {
    --sz;
  }

  while (sz > 0 && depth > 0) {
    if ('/' == file_path[sz] || '\\' == file_path[sz]) {
      --depth;
    }

    if (depth <= 0) {
      break;
    }
    --sz;
  }

  dir.assign(file_path, sz);
  return true;
}

ATFRAMEWORK_UTILS_API std::string file_system::get_cwd() {
  std::string ret;
  char *res = nullptr;
#ifdef UTIL_FS_WINDOWS_API
  res = _getcwd(nullptr, 0);
#else
  res = getcwd(nullptr, 0);
#endif

  if (nullptr != res) {
    ret = res;
    free(res);
  }
  return ret;
}

ATFRAMEWORK_UTILS_API std::string file_system::get_abs_path(const char *dir_path) {
  if (is_abs_path(dir_path)) {
    return dir_path;
  }

  std::string ret;
  ret.reserve(MAX_PATH_LEN);

  std::vector<std::string> out;

  std::string cwd = get_cwd();
  split_path(out, (cwd + DIRECTORY_SEPARATOR + dir_path).c_str(), true);

  if ('\\' == cwd[0] || '/' == cwd[0]) {
    ret += DIRECTORY_SEPARATOR;
  }

  if (!out.empty()) {
    ret += out[0];
  }

  for (size_t i = 1; i < out.size(); ++i) {
    ret += DIRECTORY_SEPARATOR;
    ret += out[i];
  }

  return ret;
}

ATFRAMEWORK_UTILS_API bool file_system::rename(const char *from, const char *to) { return 0 == ::rename(from, to); }

ATFRAMEWORK_UTILS_API bool file_system::remove(const char *path) { return 0 == ::remove(path); }

ATFRAMEWORK_UTILS_API std::string file_system::getenv(const char *name) {
  std::string ret;
#if defined(UTIL_FS_C11_API)
  size_t len = 0;
  if (0 != getenv_s(&len, nullptr, 0, name)) {
    return ret;
  }
  if (len == 0) {
    return ret;
  }

  ret.resize(len, 0);
  if (0 != getenv_s(&len, &ret[0], ret.size(), name)) {
    ret.clear();
    return ret;
  }

  while (!ret.empty() && ret[ret.size() - 1] == '\0') {
    ret.pop_back();
  }

  return ret;
#else
  char *val = ::getenv(name);
  if (nullptr != val) {
    ret = val;
  }

  while (!ret.empty() && ret[ret.size() - 1] == '\0') {
    ret.pop_back();
  }
  return ret;
#endif
}

ATFRAMEWORK_UTILS_API FILE *file_system::open_tmp_file() {
#if defined(UTIL_FS_C11_API)
  FILE *ret = nullptr;
  if (0 == tmpfile_s(&ret)) {
    return ret;
  }

  return nullptr;
#else
  return tmpfile();
#endif
}

ATFRAMEWORK_UTILS_API bool file_system::generate_tmp_file_name(std::string &inout) {
#if (defined(ATFRAMEWORK_UTILS_ENABLE_WINDOWS_MKTEMP) && ATFRAMEWORK_UTILS_ENABLE_WINDOWS_MKTEMP) || \
    (defined(ATFRAMEWORK_UTILS_ENABLE_POSIX_MKSTEMP) && ATFRAMEWORK_UTILS_ENABLE_POSIX_MKSTEMP)
  // 适配环境变量设置
  if (inout.empty()) {
#  ifdef WIN32
    inout = file_system::getenv("TMP");
    if (inout.empty()) {
      inout = file_system::getenv("TEMP");
    }
    if (inout.empty()) {
      inout = file_system::getenv("tmp");
    }
    if (inout.empty()) {
      inout = file_system::getenv("temp");
    }

    if (inout.empty()) {
      inout = "temp";
    } else {
      inout += DIRECTORY_SEPARATOR;
    }
#  else
    inout = "/tmp/";
#  endif
  }

  inout.reserve(inout.size() + 8);
  inout += "XXXXXX";
  inout.resize(inout.size() + 1, 0);

#  if (defined(ATFRAMEWORK_UTILS_ENABLE_WINDOWS_MKTEMP) && ATFRAMEWORK_UTILS_ENABLE_WINDOWS_MKTEMP)
#    if defined(UTIL_FS_C11_API)
  // Windows实现 - C11 API
  if (0 == _mktemp_s(&inout[0], inout.size())) {
    inout.pop_back();
    return true;
  } else {
    inout.clear();
    return false;
  }
#    else
  // Windows实现 - 传统 API
  if (nullptr != _mktemp(&inout[0])) {
    inout.pop_back();
    return true;
  } else {
    inout.clear();
    return false;
  }
#    endif
#  else
  // 传统 C API
  int tmp_fd = mkstemp(&inout[0]);
  if (-1 == tmp_fd) {
    inout.clear();
    return false;
  }

  inout.pop_back();
  close(tmp_fd);
  if (is_exist(inout.c_str())) {
    remove(inout.c_str());
  }
  return true;
#  endif

#else

#  if defined(__GNUC__) && !defined(__clang__) && !defined(__apple_build_version__)
#    if (__GNUC__ * 100 + __GNUC_MINOR__ * 10) >= 460
#      pragma GCC diagnostic push
#    endif
#    pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#  elif defined(__clang__) || defined(__apple_build_version__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wdeprecated-declarations"
#  endif

  // Posix实现 - C11 API
#  if defined(UTIL_FS_C11_API)
#    if defined(L_tmpnam_s)
  char path_buffer[L_tmpnam_s + 1] = {0};
  path_buffer[L_tmpnam_s] = 0;
#    else
  char path_buffer[ATFRAMEWORK_UTILS_NAMESPACE_ID::file_system::MAX_PATH_LEN + 1];
  path_buffer[ATFRAMEWORK_UTILS_NAMESPACE_ID::file_system::MAX_PATH_LEN] = 0;
#    endif
  if (0 == tmpnam_s(path_buffer, sizeof(path_buffer) - 1)) {
    inout = &path_buffer[0];
    return true;
  } else {
    return false;
  }
#  else
// Posix实现 - 传统 API
#    if defined(L_tmpnam)
  char path_buffer[L_tmpnam + 1] = {0};
  path_buffer[L_tmpnam] = 0;
#    else
  char path_buffer[ATFRAMEWORK_UTILS_NAMESPACE_ID::file_system::MAX_PATH_LEN + 1];
  path_buffer[ATFRAMEWORK_UTILS_NAMESPACE_ID::file_system::MAX_PATH_LEN] = 0;
#    endif
  if (nullptr != tmpnam(path_buffer)) {
    inout = &path_buffer[0];
    return true;
  } else {
    return false;
  }
#  endif

#  if defined(__GNUC__) && !defined(__clang__) && !defined(__apple_build_version__)
#    if (__GNUC__ * 100 + __GNUC_MINOR__ * 10) >= 460
#      pragma GCC diagnostic pop
#    endif
#  elif defined(__clang__) || defined(__apple_build_version__)
#    pragma clang diagnostic pop
#  endif

#endif
}

ATFRAMEWORK_UTILS_API int file_system::scan_dir(const char *dir_path, std::list<std::string> &out, int options) {
  int ret = 0;
  std::string base_dir = dir_path ? dir_path : "";

  // 转为绝对路径
  if ((options & dir_opt_t::EN_DOT_ABSP) && false == is_abs_path(base_dir.c_str())) {
    if (base_dir.empty()) {
      base_dir = get_cwd();
    } else {
      base_dir = get_abs_path(base_dir.c_str());
    }
  }

#ifdef UTIL_FS_WINDOWS_API

  // Windows 选项转换
  if (!base_dir.empty()) {
    base_dir += DIRECTORY_SEPARATOR;
  }

  _finddata_t child_node;
  intptr_t cache = _findfirst((base_dir + "*").c_str(), &child_node);

  if (-1 == cache) {
    return errno;
  }

  do {
    std::string child_path;
    child_path.reserve(MAX_PATH_LEN);
    child_path = base_dir;
    child_path += child_node.name;

    int accept = 0;
    bool is_link = false;

    // Windows 版本暂不支持软链接
    if (_A_SUBDIR & child_node.attrib) {
      accept = options & dir_opt_t::EN_DOT_TDIR;
    } else if ((_A_NORMAL | _A_RDONLY | _A_HIDDEN | _A_SYSTEM | _A_ARCH) & child_node.attrib) {
      accept = options & dir_opt_t::EN_DOT_TREG;
    } else {
      accept = options & dir_opt_t::EN_DOT_TOTH;
    }

#  if defined(UNICODE)
    USES_CONVERSION;
#  endif

    DWORD flag = GetFileAttributes(VC_TEXT(child_path.c_str()));
    if (FILE_ATTRIBUTE_REPARSE_POINT & flag) {
      accept = options & dir_opt_t::EN_DOT_TLNK;
      is_link = true;
    }

    // 类型不符合则跳过
    if (0 == accept) {
      continue;
    }

    // 是否排除 . 和 ..
    if (0 == strcmp(".", child_node.name) || 0 == strcmp("..", child_node.name)) {
      if (!(options & dir_opt_t::EN_DOT_SELF)) {
        continue;
      }
    } else {
      // 递归扫描（软链接不扫描，防止死循环）
      if (!is_link && (_A_SUBDIR & child_node.attrib) && (options & dir_opt_t::EN_DOT_RECU)) {
        scan_dir(child_path.c_str(), out, options & (~dir_opt_t::EN_DOT_SELF));
        continue;
      }

      // 解析软链接
      if ((FILE_ATTRIBUTE_REPARSE_POINT & flag) && (options & dir_opt_t::EN_DOT_RLNK)) {
        child_path = get_abs_path(child_path.c_str());
      }
    }

    // 普通追加目录
    out.push_back(child_path);
  } while ((ret = _findnext(cache, &child_node)) == 0);

  _findclose(cache);

  if (ENOENT == errno) {
    return 0;
  }
#else
  DIR *dir = nullptr;
  if (base_dir.empty()) {
    dir = opendir(".");
  } else {
    dir = opendir(base_dir.c_str());
  }
  if (nullptr == dir) {
    return errno;
  }

  do {
    struct dirent *child_node = readdir(dir);
    if (nullptr == child_node) {
      break;
    }

    int accept = 0;
    switch (child_node->d_type) {
      case DT_DIR: {
        accept = options & dir_opt_t::EN_DOT_TDIR;
        break;
      }

      case DT_REG: {
        accept = options & dir_opt_t::EN_DOT_TREG;
        break;
      }

      case DT_LNK: {
        accept = options & dir_opt_t::EN_DOT_TLNK;
        break;
      }

      case DT_SOCK: {
        accept = options & dir_opt_t::EN_DOT_TSOCK;
        break;
      }

      default: {
        accept = options & dir_opt_t::EN_DOT_TOTH;
        break;
      }
    }

    // 类型不符合则跳过
    if (DT_UNKNOWN != child_node->d_type && 0 == accept) {
      continue;
    }

    std::string child_path;
    child_path.reserve(MAX_PATH_LEN);
    if (!base_dir.empty()) {
      child_path += base_dir + DIRECTORY_SEPARATOR;
    }
    child_path += child_node->d_name;

    // @see http://man7.org/linux/man-pages/man3/readdir.3.html
    // some file system do not support d_type
    if (DT_UNKNOWN == child_node->d_type && 0 == accept) {
      struct stat child_stat;
      memset(&child_stat, 0, sizeof(struct stat));
      if (0 != lstat(child_path.c_str(), &child_stat)) {
        continue;
      }

      if (S_ISDIR(child_stat.st_mode)) {
        accept = options & dir_opt_t::EN_DOT_TDIR;
      } else if (S_ISREG(child_stat.st_mode)) {
        accept = options & dir_opt_t::EN_DOT_TREG;
      } else if (S_ISLNK(child_stat.st_mode)) {
        accept = options & dir_opt_t::EN_DOT_TLNK;
      } else if (S_ISSOCK(child_stat.st_mode)) {
        accept = options & dir_opt_t::EN_DOT_TSOCK;
      } else {
        accept = options & dir_opt_t::EN_DOT_TOTH;
      }
    }

    if (0 == accept) {
      continue;
    }
    // 是否排除 . 和 ..
    if (0 == strcmp(".", child_node->d_name) || 0 == strcmp("..", child_node->d_name)) {
      if (!(options & dir_opt_t::EN_DOT_SELF)) {
        continue;
      }
    } else {
      // 递归扫描（软链接不扫描，防止死循环）
      if (DT_DIR == child_node->d_type && (options & dir_opt_t::EN_DOT_RECU)) {
        scan_dir(child_path.c_str(), out, options & (~dir_opt_t::EN_DOT_SELF));
        continue;
      }

      // 解析软链接
      if (DT_LNK == child_node->d_type && (options & dir_opt_t::EN_DOT_RLNK)) {
        child_path = get_abs_path(child_path.c_str());
      }
    }

    // 普通追加目录
    out.push_back(child_path);
  } while (true);

  closedir(dir);

#endif

  return ret;
}

ATFRAMEWORK_UTILS_API bool file_system::is_abs_path(const char *dir_path) {
  if (nullptr == dir_path) {
    return false;
  }

  // NFS may has a path of "\\DOMAIN\PATH"
  if (dir_path[0] == '/' || dir_path[0] == '\\') {
    return true;
  }

#ifdef _WIN32
  if (((dir_path[0] >= 'a' && dir_path[0] <= 'z') || (dir_path[0] >= 'A' && dir_path[0] <= 'Z')) &&
      dir_path[1] == ':') {
    return true;
  }
#endif

  return false;
}

#if !defined(UTIL_FS_DISABLE_LINK)
ATFRAMEWORK_UTILS_API int file_system::link(const char *oldpath, const char *newpath, int options) {
  if ((options & link_opt_t::EN_LOT_FORCE_REWRITE) && is_exist(newpath)) {
    remove(newpath);
  }

#  if defined(UTIL_FS_WINDOWS_API)

#    if defined(UNICODE)
  USES_CONVERSION;
#    endif

  if (options & link_opt_t::EN_LOT_SYMBOLIC_LINK) {
    DWORD dwFlags = 0;
    if (options & link_opt_t::EN_LOT_DIRECTORY_LINK) {
      dwFlags |= SYMBOLIC_LINK_FLAG_DIRECTORY;
#    if defined(SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE)
      dwFlags |= SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE;
#    endif
    }

    if (CreateSymbolicLink(VC_TEXT(newpath), VC_TEXT(oldpath), dwFlags)) {
      return 0;
    }

    return static_cast<int>(GetLastError());
  } else {
    if (CreateHardLink(VC_TEXT(newpath), VC_TEXT(oldpath), nullptr)) {
      return 0;
    }

    return static_cast<int>(GetLastError());
  }

#  else
  int opts = 0;
  if (options & link_opt_t::EN_LOT_SYMBOLIC_LINK) {
    opts = AT_SYMLINK_FOLLOW;
  }

  int res = ::linkat(AT_FDCWD, oldpath, AT_FDCWD, newpath, opts);
  if (0 == res) {
    return 0;
  }

  return errno;

#  endif
}
#endif

ATFRAMEWORK_UTILS_NAMESPACE_END
