#include "algorithm/crypto_dh.h"
#include "common/file_system.h"
#include "frame/test_macros.h"
#include <cstring>

#ifdef CRYPTO_DH_ENABLED

CASE_TEST(crypto_dh, dh) {
    int test_times = 32;
    // 单元测试多次以定位openssl是否内存泄漏的问题
    // 单元测试发现openssl内部有11处still reachable内存大约72KB。并且不随测试次数增加而增加。初步判断为openssl内部分配的全局数据未释放

    while (test_times-- > 0) {
        // client shared context & dh
        util::crypto::dh cli_dh;

        // server shared context & dh
        util::crypto::dh svr_dh;

        // server - init: read and setup server dh params
        {
            util::crypto::dh::shared_context::ptr_t svr_shctx = util::crypto::dh::shared_context::create();

            std::string dir;
            CASE_EXPECT_TRUE(util::file_system::dirname(__FILE__, 0, dir, 2));
            dir += util::file_system::DIRECTORY_SEPARATOR;
            dir += "resource";
            dir += util::file_system::DIRECTORY_SEPARATOR;
            dir += "test-dhparam.pem";
            CASE_EXPECT_EQ(0, svr_shctx->init(dir.c_str()));
            CASE_EXPECT_EQ(0, svr_dh.init(svr_shctx));
        }

        // client - init: read and setup client shared context
        {
            util::crypto::dh::shared_context::ptr_t cli_shctx = util::crypto::dh::shared_context::create();
            CASE_EXPECT_EQ(0, cli_shctx->init(util::crypto::dh::method_t::EN_CDT_DH));
            CASE_EXPECT_EQ(0, cli_dh.init(cli_shctx));
        }

        std::vector<unsigned char> switch_params;
        std::vector<unsigned char> switch_public;
        std::vector<unsigned char> cli_secret;
        std::vector<unsigned char> svr_secret;

        // step 1 - server: make private key and public key
        CASE_EXPECT_EQ(0, svr_dh.make_params(switch_params));

        // step 2 - client: read dhparam and public key of server
        CASE_EXPECT_EQ(0, cli_dh.read_params(switch_params.data(), switch_params.size()));

        // step 3 - client: make public key
        CASE_EXPECT_EQ(0, cli_dh.make_public(switch_public));

        // step 4 - client: calculate secret
        CASE_EXPECT_EQ(0, cli_dh.calc_secret(cli_secret));

        // step 5 - server: read public key of client
        CASE_EXPECT_EQ(0, svr_dh.read_public(switch_public.data(), switch_public.size()));

        // step 6 - server: calculate secret
        CASE_EXPECT_EQ(0, svr_dh.calc_secret(svr_secret));

        // DH process done
        CASE_EXPECT_EQ(cli_secret.size(), svr_secret.size());
        if (cli_secret.size() == svr_secret.size()) {
            CASE_EXPECT_EQ(0, memcmp(cli_secret.data(), svr_secret.data(), svr_secret.size()));
        }
    }
}

#endif