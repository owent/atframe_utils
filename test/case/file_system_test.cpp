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