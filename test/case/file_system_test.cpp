// Copyright 2021 atframework

#include <stdio.h>

#include "common/file_system.h"
#include "frame/test_macros.h"

#ifndef UNUSED
#  define UNUSED(x) ((void)x)
#endif

CASE_TEST(file_system, dirname) {
  std::string dir;

  CASE_EXPECT_TRUE(atfw::util::file_system::dirname(__FILE__, 0, dir));
  CASE_MSG_INFO() << "Current dir: " << dir << std::endl;

  CASE_EXPECT_TRUE(atfw::util::file_system::dirname(__FILE__, 0, dir, 2));
  CASE_MSG_INFO() << "Test dir: " << dir << std::endl;

  CASE_EXPECT_TRUE(atfw::util::file_system::dirname(__FILE__, 0, dir, 3));
  CASE_MSG_INFO() << "Project dir: " << dir << std::endl;
}

CASE_TEST(file_system, scan_dir) {
  std::string dir;
  CASE_EXPECT_TRUE(atfw::util::file_system::dirname(__FILE__, 0, dir, 2));

  std::list<std::string> out;
  CASE_EXPECT_EQ(0, atfw::util::file_system::scan_dir(dir.c_str(), out,
                                                      atfw::util::file_system::dir_opt_t::EN_DOT_DAFAULT |
                                                          atfw::util::file_system::dir_opt_t::EN_DOT_ABSP |
                                                          atfw::util::file_system::dir_opt_t::EN_DOT_RLNK |
                                                          atfw::util::file_system::dir_opt_t::EN_DOT_RECU));

  CASE_MSG_INFO() << "All files in " << dir << std::endl;
  for (std::list<std::string>::iterator iter = out.begin(); iter != out.end(); ++iter) {
    CASE_MSG_INFO() << "\t" << *iter << std::endl;
  }
}

CASE_TEST(file_system, get_cwd) {
  std::string dir;
  CASE_MSG_INFO() << "Working dir: " << atfw::util::file_system::get_cwd() << std::endl;
}

CASE_TEST(file_system, getenv) {
  CASE_MSG_INFO() << "Env OS: " << atfw::util::file_system::getenv("OS") << std::endl;
  CASE_MSG_INFO() << "Env USER: " << atfw::util::file_system::getenv("USER") << std::endl;
  CASE_MSG_INFO() << "Env HOME: " << atfw::util::file_system::getenv("HOME") << std::endl;
  CASE_MSG_INFO() << "Env USERNAME: " << atfw::util::file_system::getenv("USERNAME") << std::endl;
  CASE_MSG_INFO() << "Env HOMEPATH: " << atfw::util::file_system::getenv("HOMEPATH") << std::endl;
  CASE_MSG_INFO() << "Env PATH: " << atfw::util::file_system::getenv("PATH") << std::endl;
}

CASE_TEST(file_system, get_file_content) {
  std::string content;
#if defined(__unix__)
  const char* virtual_file = "/proc/cpuinfo";
  CASE_EXPECT_TRUE(atfw::util::file_system::get_file_content(content, virtual_file));
  CASE_MSG_INFO() << "Cpu Info: " << std::endl << content << std::endl;
#endif

  CASE_EXPECT_TRUE(atfw::util::file_system::get_file_content(content, __FILE__));
}

CASE_TEST(file_system, mkdir_remove) {
  if (!atfw::util::file_system::is_exist("test-mkdir/relpath")) {
    CASE_EXPECT_TRUE(atfw::util::file_system::mkdir("test-mkdir/relpath", true));
    CASE_EXPECT_TRUE(atfw::util::file_system::is_exist("test-mkdir/relpath"));
  }

  // remove, only POSIX can remove(directory)
#if defined(__unix__)
  CASE_EXPECT_TRUE(atfw::util::file_system::remove("test-mkdir/relpath"));
  CASE_EXPECT_FALSE(atfw::util::file_system::is_exist("test-mkdir/relpath"));
#endif

  std::string dirpath = atfw::util::file_system::get_cwd() + "/test-mkdir/abspath";
  if (!atfw::util::file_system::is_exist(dirpath.c_str())) {
    CASE_EXPECT_TRUE(atfw::util::file_system::mkdir(dirpath.c_str(), true));
    CASE_EXPECT_TRUE(atfw::util::file_system::is_exist(dirpath.c_str()));
  }

// remove & rename, only POSIX can remove(directory)
#if defined(__unix__)
  CASE_EXPECT_TRUE(atfw::util::file_system::rename(dirpath.c_str(), "test-mkdir/relpath"));
  CASE_EXPECT_TRUE(atfw::util::file_system::is_exist("test-mkdir/relpath"));
  CASE_EXPECT_FALSE(atfw::util::file_system::is_exist(dirpath.c_str()));
  CASE_EXPECT_TRUE(atfw::util::file_system::remove("test-mkdir/relpath"));
#endif
}

CASE_TEST(file_system, file_size) {
  size_t sz = 0;
  CASE_EXPECT_TRUE(atfw::util::file_system::file_size(__FILE__, sz));
  CASE_EXPECT_GT(sz, 0);
}

CASE_TEST(file_system, open_tmp_file) {
  FILE* f = atfw::util::file_system::open_tmp_file();
  CASE_EXPECT_NE(f, nullptr);

  if (nullptr != f) {
    UTIL_FS_CLOSE(f);
  }

  std::string fname;
  atfw::util::file_system::generate_tmp_file_name(fname);
  CASE_MSG_INFO() << "Tmp file path: " << fname << std::endl;
  CASE_EXPECT_FALSE(fname.empty());

  if (fname.empty()) {
    UTIL_FS_OPEN(open_res, f, fname.c_str(), "w");
    UNUSED(open_res);  // For old compiler
    CASE_EXPECT_NE(f, nullptr);
    if (nullptr != f) {
      UTIL_FS_CLOSE(f);
    }
  }
}
