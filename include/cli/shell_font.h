// Copyright 2022 atframework
// Created on: 2014-3-11 by owent

#pragma once

#include <config/atframe_utils_build_feature.h>
#include <config/compiler_features.h>

#include <iostream>
#include <map>
#include <string>

#if defined(__GNUC__) && !defined(__clang__) && !defined(__apple_build_version__)
#  if (__GNUC__ * 100 + __GNUC_MINOR__ * 10) >= 460
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Waddress"

#    if (__GNUC__ * 100 + __GNUC_MINOR__ * 10) >= 600
#      pragma GCC diagnostic ignored "-Wnonnull-compare"
#    endif
#  endif
#elif defined(__clang__) || defined(__apple_build_version__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Waddress"
#endif

/**
 * Window 控制台相关
 * @see https://msdn.microsoft.com/zh-cn/windows/apps/ms686047%28v=vs.100%29.aspx
 * @see https://github.com/owent-utils/python/blob/master/print_color.py
 */
#ifdef _MSC_VER

#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif

#  include <Windows.h>

#  define SHELL_FONT_USING_WIN32_CONSOLE

#endif

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace cli {

// 下面是编码表
//
// 编码    颜色/动作
//  0   重新设置属性到缺省设置
//  1   设置粗体
//  2   设置一半亮度（模拟彩色显示器的颜色）
//  4   设置下划线（模拟彩色显示器的颜色）
//  5   设置闪烁
//  7   设置反向图象
//  22  设置一般密度
//  24  关闭下划线
//  25  关闭闪烁
//  27  关闭反向图象
//  30  设置黑色前景
//  31  设置红色前景
//  32  设置绿色前景
//  33  设置棕色前景
//  34  设置蓝色前景
//  35  设置紫色前景
//  36  设置青色前景
//  37  设置白色前景
//  38  在缺省的前景颜色上设置下划线
//  39  在缺省的前景颜色上关闭下划线
//  40  设置黑色背景
//  41  设置红色背景
//  42  设置绿色背景
//  43  设置棕色背景
//  44  设置蓝色背景
//  45  设置紫色背景
//  46  设置青色背景
//  47  设置白色背景
//  49  设置缺省黑色背景

struct ATFRAMEWORK_UTILS_API shell_font_style {
  enum shell_font_spec {
    SHELL_FONT_SPEC_NULL = 0x00,
    SHELL_FONT_SPEC_BOLD = 0x01,
    SHELL_FONT_SPEC_UNDERLINE = 0x02,
    SHELL_FONT_SPEC_FLASH = 0x04,
    SHELL_FONT_SPEC_DARK = 0x08,
  };

  enum shell_font_color {
    SHELL_FONT_COLOR_BLACK = 0x0100,  // 30
    SHELL_FONT_COLOR_RED = 0x0200,
    SHELL_FONT_COLOR_GREEN = 0x0400,
    SHELL_FONT_COLOR_YELLOW = 0x0800,
    SHELL_FONT_COLOR_BLUE = 0x1000,
    SHELL_FONT_COLOR_MAGENTA = 0x2000,
    SHELL_FONT_COLOR_CYAN = 0x4000,
    SHELL_FONT_COLOR_WHITE = 0x8000,
  };

  enum shell_font_background_color {
    SHELL_FONT_BACKGROUND_COLOR_BLACK = 0x010000,  // 40
    SHELL_FONT_BACKGROUND_COLOR_RED = 0x020000,
    SHELL_FONT_BACKGROUND_COLOR_GREEN = 0x040000,
    SHELL_FONT_BACKGROUND_COLOR_YELLOW = 0x080000,
    SHELL_FONT_BACKGROUND_COLOR_BLUE = 0x100000,
    SHELL_FONT_BACKGROUND_COLOR_MAGENTA = 0x200000,
    SHELL_FONT_BACKGROUND_COLOR_CYAN = 0x400000,
    SHELL_FONT_BACKGROUND_COLOR_WHITE = 0x800000,
  };
};

class ATFRAMEWORK_UTILS_API shell_font {
 private:
  int m_iFlag;

 public:
  /**
   * 字体信息
   * @param iFlag
   */
  explicit shell_font(int iFlag = 0);

  virtual ~shell_font();

  /**
   * 生成带样式的文本
   * @param [in] strInput 原始文本
   * @return 生成带样式的文本
   */
  std::string GenerateString(const std::string &strInput);

  /**
   * 生成带样式的文本
   * @param [in] strInput 原始文本
   * @param [in] iFlag 样式
   * @return 生成带样式的文本
   */
  static std::string GenerateString(const std::string &strInput, int iFlag);

  /**
   * 获取样式的生成命令
   * @param [in] iFlag 样式
   * @return 样式的生成命令
   */
  static std::string GetStyleCode(int iFlag);

  /**
   * 获取样式的生成命令
   * @return 样式的生成命令
   */
  std::string GetStyleCode();

  /**
   * 获取样式的关闭命令
   * @return 样式的关闭命令
   */
  static std::string GetStyleCloseCode();
};

class UTIL_SYMBOL_VISIBLE shell_stream {
 public:
  using stream_t = std::ostream;
  class UTIL_SYMBOL_VISIBLE shell_stream_opr {
   public:
    using self_t = shell_stream_opr;

   private:
    stream_t *pOs;
#ifdef SHELL_FONT_USING_WIN32_CONSOLE
    HANDLE hOsHandle;
#endif
    mutable int flag;

    // 进允许内部复制构造
    ATFRAMEWORK_UTILS_API shell_stream_opr(const shell_stream_opr &);
    ATFRAMEWORK_UTILS_API shell_stream_opr &operator=(const shell_stream_opr &);

    friend class shell_stream;

   public:
    ATFRAMEWORK_UTILS_API shell_stream_opr(stream_t *os);
    ATFRAMEWORK_UTILS_API ~shell_stream_opr();

    template <typename Ty>
    ATFRAMEWORK_UTILS_API_HEAD_ONLY const shell_stream_opr &operator<<(const Ty &v) const {
      close();
      (*pOs) << v;
      return (*this);
    }

    ATFRAMEWORK_UTILS_API const shell_stream_opr &operator<<(std::nullptr_t) const;
    ATFRAMEWORK_UTILS_API const shell_stream_opr &operator<<(shell_font_style::shell_font_spec style) const;
    ATFRAMEWORK_UTILS_API const shell_stream_opr &operator<<(shell_font_style::shell_font_color style) const;
    ATFRAMEWORK_UTILS_API const shell_stream_opr &operator<<(shell_font_style::shell_font_background_color style) const;
    ATFRAMEWORK_UTILS_API const shell_stream_opr &operator<<(stream_t &(*fn)(stream_t &)) const;

    ATFRAMEWORK_UTILS_API const shell_stream_opr &open(int flag) const;

    ATFRAMEWORK_UTILS_API void close() const;

    ATFRAMEWORK_UTILS_API void reset() const;

    UTIL_FORCEINLINE operator stream_t &() const { return *pOs; }

    UTIL_FORCEINLINE operator const stream_t &() const { return *pOs; }
  };

 public:
  ATFRAMEWORK_UTILS_API shell_stream(stream_t &stream = std::cout);
  ATFRAMEWORK_UTILS_API shell_stream_opr operator()() const;

 private:
  stream_t *m_pOs;
};

}  // namespace cli
ATFRAMEWORK_UTILS_NAMESPACE_END

#if defined(__GNUC__) && !defined(__clang__) && !defined(__apple_build_version__)
#  if (__GNUC__ * 100 + __GNUC_MINOR__ * 10) >= 460
#    pragma GCC diagnostic pop
#  endif
#elif defined(__clang__) || defined(__apple_build_version__)
#  pragma clang diagnostic pop
#endif
