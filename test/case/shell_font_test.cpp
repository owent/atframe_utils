// Copyright 2026 atframework

#include <sstream>
#include <string>

#include "cli/shell_font.h"
#include "frame/test_macros.h"

CASE_TEST(shell_font, get_style_code_bold) {
  std::string code = atfw::util::cli::shell_font::GetStyleCode(atfw::util::cli::shell_font_style::SHELL_FONT_SPEC_BOLD);
  CASE_EXPECT_FALSE(code.empty());
  // ANSI code should start with ESC[ and end with 'm'
  CASE_EXPECT_EQ('\033', code[0]);
  CASE_EXPECT_EQ('[', code[1]);
  CASE_EXPECT_EQ('m', code[code.size() - 1]);
  // Bold is code 1
  CASE_EXPECT_TRUE(code.find('1') != std::string::npos);
}

CASE_TEST(shell_font, get_style_code_underline) {
  std::string code =
      atfw::util::cli::shell_font::GetStyleCode(atfw::util::cli::shell_font_style::SHELL_FONT_SPEC_UNDERLINE);
  CASE_EXPECT_FALSE(code.empty());
  // Underline is code 4
  CASE_EXPECT_TRUE(code.find('4') != std::string::npos);
}

CASE_TEST(shell_font, get_style_code_flash) {
  std::string code =
      atfw::util::cli::shell_font::GetStyleCode(atfw::util::cli::shell_font_style::SHELL_FONT_SPEC_FLASH);
  CASE_EXPECT_FALSE(code.empty());
  // Flash is code 5
  CASE_EXPECT_TRUE(code.find('5') != std::string::npos);
}

CASE_TEST(shell_font, get_style_code_dark) {
  std::string code = atfw::util::cli::shell_font::GetStyleCode(atfw::util::cli::shell_font_style::SHELL_FONT_SPEC_DARK);
  CASE_EXPECT_FALSE(code.empty());
  // Dark is code 2
  CASE_EXPECT_TRUE(code.find('2') != std::string::npos);
}

CASE_TEST(shell_font, get_style_code_foreground_colors) {
  // Test each foreground color
  int colors[] = {
      atfw::util::cli::shell_font_style::SHELL_FONT_COLOR_BLACK,
      atfw::util::cli::shell_font_style::SHELL_FONT_COLOR_RED,
      atfw::util::cli::shell_font_style::SHELL_FONT_COLOR_GREEN,
      atfw::util::cli::shell_font_style::SHELL_FONT_COLOR_YELLOW,
      atfw::util::cli::shell_font_style::SHELL_FONT_COLOR_BLUE,
      atfw::util::cli::shell_font_style::SHELL_FONT_COLOR_MAGENTA,
      atfw::util::cli::shell_font_style::SHELL_FONT_COLOR_CYAN,
      atfw::util::cli::shell_font_style::SHELL_FONT_COLOR_WHITE,
  };

  for (int color : colors) {
    std::string code = atfw::util::cli::shell_font::GetStyleCode(color);
    CASE_EXPECT_FALSE(code.empty());
    CASE_EXPECT_EQ('\033', code[0]);
    CASE_EXPECT_EQ('m', code[code.size() - 1]);
  }
}

CASE_TEST(shell_font, get_style_code_background_colors) {
  int colors[] = {
      atfw::util::cli::shell_font_style::SHELL_FONT_BACKGROUND_COLOR_BLACK,
      atfw::util::cli::shell_font_style::SHELL_FONT_BACKGROUND_COLOR_RED,
      atfw::util::cli::shell_font_style::SHELL_FONT_BACKGROUND_COLOR_GREEN,
      atfw::util::cli::shell_font_style::SHELL_FONT_BACKGROUND_COLOR_YELLOW,
      atfw::util::cli::shell_font_style::SHELL_FONT_BACKGROUND_COLOR_BLUE,
      atfw::util::cli::shell_font_style::SHELL_FONT_BACKGROUND_COLOR_MAGENTA,
      atfw::util::cli::shell_font_style::SHELL_FONT_BACKGROUND_COLOR_CYAN,
      atfw::util::cli::shell_font_style::SHELL_FONT_BACKGROUND_COLOR_WHITE,
  };

  for (int color : colors) {
    std::string code = atfw::util::cli::shell_font::GetStyleCode(color);
    CASE_EXPECT_FALSE(code.empty());
  }
}

CASE_TEST(shell_font, get_style_code_combined) {
  // Test combining spec + foreground + background
  int flag = static_cast<int>(atfw::util::cli::shell_font_style::SHELL_FONT_SPEC_BOLD) |
             static_cast<int>(atfw::util::cli::shell_font_style::SHELL_FONT_COLOR_RED) |
             static_cast<int>(atfw::util::cli::shell_font_style::SHELL_FONT_BACKGROUND_COLOR_WHITE);

  std::string code = atfw::util::cli::shell_font::GetStyleCode(flag);
  CASE_EXPECT_FALSE(code.empty());
  // Should contain semicolons separating codes
  CASE_EXPECT_TRUE(code.find(';') != std::string::npos);
}

CASE_TEST(shell_font, get_style_close_code) {
  std::string code = atfw::util::cli::shell_font::GetStyleCloseCode();
  CASE_EXPECT_FALSE(code.empty());
  CASE_EXPECT_EQ('\033', code[0]);
}

CASE_TEST(shell_font, instance_methods) {
  atfw::util::cli::shell_font font(atfw::util::cli::shell_font_style::SHELL_FONT_COLOR_GREEN);

  std::string code = font.GetStyleCode();
  CASE_EXPECT_FALSE(code.empty());

  std::string result = font.GenerateString("test");
  // GenerateString may or may not add codes depending on terminal detection
  CASE_EXPECT_TRUE(result.find("test") != std::string::npos);
}

CASE_TEST(shell_font, generate_string_static) {
  std::string result =
      atfw::util::cli::shell_font::GenerateString("colored", atfw::util::cli::shell_font_style::SHELL_FONT_COLOR_RED);
  CASE_EXPECT_TRUE(result.find("colored") != std::string::npos);
}

CASE_TEST(shell_font, generate_string_no_flag) {
  // With flag=0, should return input unchanged
  std::string result = atfw::util::cli::shell_font::GenerateString("plain", 0);
  CASE_EXPECT_EQ("plain", result);
}

CASE_TEST(shell_font, shell_stream_basic) {
  std::ostringstream oss;
  // shell_stream wraps an ostream
  atfw::util::cli::shell_stream ss(oss);

  ss() << "hello";
  CASE_EXPECT_TRUE(oss.str().find("hello") != std::string::npos);
}

CASE_TEST(shell_font, shell_stream_with_color) {
  std::ostringstream oss;
  atfw::util::cli::shell_stream ss(oss);

  ss() << atfw::util::cli::shell_font_style::SHELL_FONT_COLOR_RED << "red text";
  CASE_EXPECT_TRUE(oss.str().find("red text") != std::string::npos);
}

CASE_TEST(shell_font, shell_stream_with_spec) {
  std::ostringstream oss;
  atfw::util::cli::shell_stream ss(oss);

  ss() << atfw::util::cli::shell_font_style::SHELL_FONT_SPEC_BOLD << "bold text";
  CASE_EXPECT_TRUE(oss.str().find("bold text") != std::string::npos);
}

CASE_TEST(shell_font, shell_stream_with_background) {
  std::ostringstream oss;
  atfw::util::cli::shell_stream ss(oss);

  ss() << atfw::util::cli::shell_font_style::SHELL_FONT_BACKGROUND_COLOR_BLUE << "bg text";
  CASE_EXPECT_TRUE(oss.str().find("bg text") != std::string::npos);
}

CASE_TEST(shell_font, shell_stream_nullptr) {
  std::ostringstream oss;
  atfw::util::cli::shell_stream ss(oss);

  ss() << nullptr;
  CASE_EXPECT_TRUE(oss.str().find("nullptr") != std::string::npos);
}

CASE_TEST(shell_font, shell_stream_endl) {
  std::ostringstream oss;
  atfw::util::cli::shell_stream ss(oss);

  ss() << "line" << std::endl;
  CASE_EXPECT_TRUE(oss.str().find("line") != std::string::npos);
  CASE_EXPECT_TRUE(oss.str().find('\n') != std::string::npos);
}

CASE_TEST(shell_font, shell_stream_null_reset) {
  std::ostringstream oss;
  atfw::util::cli::shell_stream ss(oss);

  // Open with SPEC_NULL should trigger reset
  ss() << atfw::util::cli::shell_font_style::SHELL_FONT_SPEC_NULL << "text";
  CASE_EXPECT_TRUE(oss.str().find("text") != std::string::npos);
}

CASE_TEST(shell_font, constructor_destructor) {
  // Test constructor and destructor
  {
    int flag = static_cast<int>(atfw::util::cli::shell_font_style::SHELL_FONT_SPEC_BOLD) |
               static_cast<int>(atfw::util::cli::shell_font_style::SHELL_FONT_COLOR_GREEN);
    atfw::util::cli::shell_font font(flag);
    std::string code = font.GetStyleCode();
    CASE_EXPECT_FALSE(code.empty());
  }
  // Destructor should not crash
}
