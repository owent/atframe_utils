#include "common/file_system.h"
#include "frame/test_macros.h"

CASE_TEST(file_system, dirname) {
    std::string dir;

    CASE_EXPECT_TRUE(util::file_system::dirname(__FILE__, 0, dir));
    CASE_MSG_INFO() << "Current dir: " << dir << std::endl;

    CASE_EXPECT_TRUE(util::file_system::dirname(__FILE__, 0, dir, 2));
    CASE_MSG_INFO() << "Test dir: " << dir << std::endl;

    CASE_EXPECT_TRUE(util::file_system::dirname(__FILE__, 0, dir, 3));
    CASE_MSG_INFO() << "Project dir: " << dir << std::endl;
}

CASE_TEST(file_system, scan_dir) {
    std::string dir;
    CASE_EXPECT_TRUE(util::file_system::dirname(__FILE__, 0, dir, 2));

    std::list<std::string> out;
    CASE_EXPECT_EQ(0,
                   util::file_system::scan_dir(dir.c_str(), out,
                                               util::file_system::dir_opt_t::EN_DOT_DAFAULT | util::file_system::dir_opt_t::EN_DOT_ABSP |
                                                   util::file_system::dir_opt_t::EN_DOT_RLNK | util::file_system::dir_opt_t::EN_DOT_RECU));

    CASE_MSG_INFO() << "All files in " << dir << std::endl;
    for (std::list<std::string>::iterator iter = out.begin(); iter != out.end(); ++iter) {
        CASE_MSG_INFO() << "\t" << *iter << std::endl;
    }
}

CASE_TEST(file_system, get_cwd) {
    std::string dir;
    CASE_MSG_INFO() << "Working dir: " << util::file_system::get_cwd() << std::endl;
}


CASE_TEST(file_system, get_file_content) {
    std::string content;
#if defined(__unix__)
    const char *virtual_file = "/proc/cpuinfo";
    CASE_EXPECT_TRUE(util::file_system::get_file_content(content, virtual_file));
    CASE_MSG_INFO() << "Cpu Info: " << std::endl << content << std::endl;
#endif

    CASE_EXPECT_TRUE(util::file_system::get_file_content(content, __FILE__));
}

CASE_TEST(file_system, mkdir_remove) {
    if (!util::file_system::is_exist("test-mkdir/relpath")) {
        CASE_EXPECT_TRUE(util::file_system::mkdir("test-mkdir/relpath", true));
        CASE_EXPECT_TRUE(util::file_system::is_exist("test-mkdir/relpath"));
    }

    // remove
    CASE_EXPECT_TRUE(util::file_system::remove("test-mkdir/relpath"));
    CASE_EXPECT_FALSE(util::file_system::is_exist("test-mkdir/relpath"));

    std::string dirpath = util::file_system::get_cwd() + "/test-mkdir/abspath";
    if (!util::file_system::is_exist(dirpath.c_str())) {
        CASE_EXPECT_TRUE(util::file_system::mkdir(dirpath.c_str(), true));
        CASE_EXPECT_TRUE(util::file_system::is_exist(dirpath.c_str()));
    }

    // remove & rename
    CASE_EXPECT_TRUE(util::file_system::rename(dirpath.c_str(), "test-mkdir/relpath"));
    CASE_EXPECT_TRUE(util::file_system::is_exist("test-mkdir/relpath"));
    CASE_EXPECT_FALSE(util::file_system::is_exist(dirpath.c_str()));
    CASE_EXPECT_TRUE(util::file_system::remove("test-mkdir/relpath"));
}

CASE_TEST(file_system, file_size) {
    size_t sz = 0;
    CASE_EXPECT_TRUE(util::file_system::file_size(__FILE__, sz));
    CASE_EXPECT_GT(sz, 0);
}
