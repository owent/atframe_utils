#include "algorithm/crypto_dh.h"
#include <cstring>
#include "algorithm/crypto_cipher.h"
#include "common/file_system.h"
#include "frame/test_macros.h"

#ifdef CRYPTO_DH_ENABLED

#  if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
struct openssl_test_init_wrapper_for_dh {
  openssl_test_init_wrapper_for_dh() { util::crypto::cipher::init_global_algorithm(); }

  ~openssl_test_init_wrapper_for_dh() { util::crypto::cipher::cleanup_global_algorithm(); }
};

static std::shared_ptr<openssl_test_init_wrapper_for_dh> openssl_test_inited_for_dh;

#  endif

CASE_TEST(crypto_dh, get_all_curve_names) {
#  if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
  if (!openssl_test_inited_for_dh) {
    openssl_test_inited_for_dh = std::make_shared<openssl_test_init_wrapper_for_dh>();
  }
#  endif
  const std::vector<std::string> &all_curves = util::crypto::dh::get_all_curve_names();
  std::stringstream ss;
  for (size_t i = 0; i < all_curves.size(); ++i) {
    if (i) {
      ss << ",";
    }

    ss << all_curves[i];
  }

  CASE_MSG_INFO() << "All curves: " << ss.str() << std::endl;
#  if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
  // Openssl 1.0.1 or lower do not support ECDH
#    if (defined(OPENSSL_API_COMPAT) && OPENSSL_API_COMPAT >= 0x10002000L) ||    \
        (defined(OPENSSL_API_LEVEL) && OPENSSL_API_LEVEL >= 10002) ||            \
        (!defined(LIBRESSL_VERSION_NUMBER) && defined(OPENSSL_VERSION_NUMBER) && \
         OPENSSL_VERSION_NUMBER >= 0x10002000L)
  CASE_EXPECT_NE(0, all_curves.size());
#    endif
#  endif
}

CASE_TEST(crypto_dh, dh) {
#  if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
  if (!openssl_test_inited_for_dh) {
    openssl_test_inited_for_dh = std::make_shared<openssl_test_init_wrapper_for_dh>();
  }
#  endif

  int test_times = 32;
  int left_times = test_times;
  size_t key_bits = 0;
  // 单元测试多次以定位openssl是否内存泄漏的问题
  // 单元测试发现openssl内部有11处still
  // reachable内存大约72KB。并且不随测试次数增加而增加。初步判断为openssl内部分配的全局数据未释放

  while (left_times-- > 0) {
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
    CASE_EXPECT_GT(cli_secret.size(), 0);
    key_bits = cli_secret.size();
  }

  CASE_MSG_INFO() << "Test DH algorithm " << test_times << " times, key len " << key_bits << " bits. " << std::endl;
}

CASE_TEST(crypto_dh, ecdh) {
#  if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
  if (!openssl_test_inited_for_dh) {
    openssl_test_inited_for_dh = std::make_shared<openssl_test_init_wrapper_for_dh>();
  }
#  endif

  int test_times = 16;
  // 单元测试多次以定位openssl是否内存泄漏的问题
  const std::vector<std::string> &all_curves = util::crypto::dh::get_all_curve_names();

  clock_t min_cost_clock = 0;
  clock_t max_cost_clock = 0;
  size_t min_cost_idx = 0;
  size_t min_cost_bits = 0;
  size_t max_cost_idx = 0;
  size_t max_cost_bits = 0;
  for (size_t curve_idx = 0; curve_idx < all_curves.size(); ++curve_idx) {
    CASE_MSG_INFO() << "Test ECDH algorithm " << all_curves[curve_idx] << std::endl;
    clock_t beg_time_clk = clock();
    int left_times = test_times;
    size_t secret_bits = 0;
    while (left_times-- > 0) {
      // client shared context & dh
      util::crypto::dh cli_dh;

      // server shared context & dh
      util::crypto::dh svr_dh;

      // server - init: read and setup server dh params
      {
        util::crypto::dh::shared_context::ptr_t svr_shctx = util::crypto::dh::shared_context::create();
        CASE_EXPECT_EQ(0, svr_shctx->init(all_curves[curve_idx].c_str()));
        CASE_EXPECT_EQ(0, svr_dh.init(svr_shctx));
      }

      // client - init: read and setup client shared context
      {
        util::crypto::dh::shared_context::ptr_t cli_shctx = util::crypto::dh::shared_context::create();
        CASE_EXPECT_EQ(0, cli_shctx->init(util::crypto::dh::method_t::EN_CDT_ECDH));
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
      secret_bits = cli_secret.size();
      CASE_EXPECT_GT(secret_bits, 0);
    }

    clock_t end_time_clk = clock();
    if (0 == curve_idx) {
      min_cost_clock = end_time_clk - beg_time_clk;
      max_cost_clock = end_time_clk - beg_time_clk;
      min_cost_idx = 0;
      max_cost_idx = 0;
      min_cost_bits = secret_bits * 8;
      max_cost_bits = secret_bits * 8;
    } else {
      clock_t off_clk = end_time_clk - beg_time_clk;
      if (off_clk > max_cost_clock) {
        max_cost_clock = off_clk;
        max_cost_idx = curve_idx;
        max_cost_bits = secret_bits * 8;
      }
      if (off_clk < min_cost_clock) {
        min_cost_clock = off_clk;
        min_cost_idx = curve_idx;
        min_cost_bits = secret_bits * 8;
      }
    }
  }

  CASE_MSG_INFO() << "Test ECDH algorithm " << test_times << " times for " << all_curves.size() << " curves done. "
                  << std::endl;
  if (!all_curves.empty()) {
    CASE_MSG_INFO() << "  Fastest => " << all_curves[min_cost_idx] << " cost "
                    << (1000.0 * min_cost_clock / CLOCKS_PER_SEC / test_times) << "ms(avg.) key len " << min_cost_bits
                    << " bits. " << std::endl;
    CASE_MSG_INFO() << "  Slowest => " << all_curves[max_cost_idx] << " cost "
                    << (1000.0 * max_cost_clock / CLOCKS_PER_SEC / test_times) << "ms(avg.) key len " << max_cost_bits
                    << " bits. " << std::endl;
  }
}

#endif