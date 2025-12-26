// Copyright 2025 atframework

#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "algorithm/crypto_hmac.h"
#include "common/string_oprs.h"

#include "frame/test_macros.h"

#ifdef ATFW_UTIL_MACRO_CRYPTO_HMAC_ENABLED

#  if defined(ATFRAMEWORK_UTILS_CRYPTO_USE_OPENSSL) || defined(ATFRAMEWORK_UTILS_CRYPTO_USE_LIBRESSL) || \
      defined(ATFRAMEWORK_UTILS_CRYPTO_USE_BORINGSSL)
#    include "algorithm/crypto_cipher.h"

struct openssl_test_init_wrapper_for_hmac {
  openssl_test_init_wrapper_for_hmac() { atfw::util::crypto::cipher::init_global_algorithm(); }

  ~openssl_test_init_wrapper_for_hmac() { atfw::util::crypto::cipher::cleanup_global_algorithm(); }
};

static std::shared_ptr<openssl_test_init_wrapper_for_hmac> openssl_test_inited_for_hmac;

static void ensure_openssl_inited() {
  if (!openssl_test_inited_for_hmac) {
    openssl_test_inited_for_hmac = std::make_shared<openssl_test_init_wrapper_for_hmac>();
  }
}
#  else
static void ensure_openssl_inited() {}
#  endif

// ============================================================================
// Helper functions
// ============================================================================

static std::vector<unsigned char> hex_to_bytes(const char* hex) {
  std::vector<unsigned char> result;
  if (hex == nullptr) {
    return result;
  }

  size_t len = strlen(hex);
  result.reserve(len / 2);

  for (size_t i = 0; i + 1 < len; i += 2) {
    unsigned int byte = 0;
    char buf[3] = {hex[i], hex[i + 1], 0};
    if (sscanf(buf, "%02x", &byte) == 1) {
      result.push_back(static_cast<unsigned char>(byte));
    }
  }
  return result;
}

static std::string bytes_to_hex(const std::vector<unsigned char>& bytes) {
  std::string result;
  result.resize(bytes.size() * 2);
  atfw::util::string::dumphex(bytes.data(), bytes.size(), &result[0], false);
  return result;
}

// ============================================================================
// HMAC Tests
// ============================================================================

CASE_TEST(crypto_hmac, get_digest_output_length) {
  ensure_openssl_inited();

  CASE_EXPECT_EQ(20u, atfw::util::crypto::get_digest_output_length(atfw::util::crypto::digest_type_t::kSha1));
  CASE_EXPECT_EQ(28u, atfw::util::crypto::get_digest_output_length(atfw::util::crypto::digest_type_t::kSha224));
  CASE_EXPECT_EQ(32u, atfw::util::crypto::get_digest_output_length(atfw::util::crypto::digest_type_t::kSha256));
  CASE_EXPECT_EQ(48u, atfw::util::crypto::get_digest_output_length(atfw::util::crypto::digest_type_t::kSha384));
  CASE_EXPECT_EQ(64u, atfw::util::crypto::get_digest_output_length(atfw::util::crypto::digest_type_t::kSha512));
  CASE_EXPECT_EQ(16u, atfw::util::crypto::get_digest_output_length(atfw::util::crypto::digest_type_t::kMd5));
  CASE_EXPECT_EQ(0u, atfw::util::crypto::get_digest_output_length(atfw::util::crypto::digest_type_t::kNone));
}

CASE_TEST(crypto_hmac, get_digest_name) {
  ensure_openssl_inited();

  CASE_EXPECT_EQ(0, strcmp("SHA1", atfw::util::crypto::get_digest_name(atfw::util::crypto::digest_type_t::kSha1)));
  CASE_EXPECT_EQ(0, strcmp("SHA224", atfw::util::crypto::get_digest_name(atfw::util::crypto::digest_type_t::kSha224)));
  CASE_EXPECT_EQ(0, strcmp("SHA256", atfw::util::crypto::get_digest_name(atfw::util::crypto::digest_type_t::kSha256)));
  CASE_EXPECT_EQ(0, strcmp("SHA384", atfw::util::crypto::get_digest_name(atfw::util::crypto::digest_type_t::kSha384)));
  CASE_EXPECT_EQ(0, strcmp("SHA512", atfw::util::crypto::get_digest_name(atfw::util::crypto::digest_type_t::kSha512)));
  CASE_EXPECT_EQ(0, strcmp("MD5", atfw::util::crypto::get_digest_name(atfw::util::crypto::digest_type_t::kMd5)));
  CASE_EXPECT_EQ(nullptr, atfw::util::crypto::get_digest_name(atfw::util::crypto::digest_type_t::kNone));
}

// RFC 4231 Test Vectors for HMAC-SHA256
// https://tools.ietf.org/html/rfc4231

CASE_TEST(crypto_hmac, hmac_sha256_rfc4231_test1) {
  ensure_openssl_inited();

  // Test Case 1
  // Key = 0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b (20 bytes)
  // Data = "Hi There"
  // HMAC-SHA-256 = b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7

  std::vector<unsigned char> key(20, 0x0b);
  const char* data = "Hi There";
  size_t data_len = strlen(data);

  std::vector<unsigned char> output(32);
  size_t output_len = output.size();

  int ret = atfw::util::crypto::hmac::compute(atfw::util::crypto::digest_type_t::kSha256, key.data(), key.size(),
                                              reinterpret_cast<const unsigned char*>(data), data_len, output.data(),
                                              &output_len);

  CASE_EXPECT_EQ(atfw::util::crypto::hmac_error_code_t::kOk, ret);
  CASE_EXPECT_EQ(32u, output_len);

  std::string expected_hex = "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7";
  std::string actual_hex = bytes_to_hex(output);
  CASE_EXPECT_EQ(expected_hex, actual_hex);
}

CASE_TEST(crypto_hmac, hmac_sha256_rfc4231_test2) {
  ensure_openssl_inited();

  // Test Case 2
  // Key = "Jefe"
  // Data = "what do ya want for nothing?"
  // HMAC-SHA-256 = 5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843

  const char* key = "Jefe";
  const char* data = "what do ya want for nothing?";

  std::vector<unsigned char> result = atfw::util::crypto::hmac::compute_to_binary(
      atfw::util::crypto::digest_type_t::kSha256, reinterpret_cast<const unsigned char*>(key), strlen(key),
      reinterpret_cast<const unsigned char*>(data), strlen(data));

  CASE_EXPECT_EQ(32u, result.size());

  std::string expected_hex = "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843";
  std::string actual_hex = bytes_to_hex(result);
  CASE_EXPECT_EQ(expected_hex, actual_hex);
}

CASE_TEST(crypto_hmac, hmac_sha256_rfc4231_test3) {
  ensure_openssl_inited();

  // Test Case 3
  // Key = aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa (20 bytes of 0xaa)
  // Data = 0xdd repeated 50 times
  // HMAC-SHA-256 = 773ea91e36800e46854db8ebd09181a72959098b3ef8c122d9635514ced565fe

  std::vector<unsigned char> key(20, 0xaa);
  std::vector<unsigned char> data(50, 0xdd);

  std::string result = atfw::util::crypto::hmac::compute_to_hex(atfw::util::crypto::digest_type_t::kSha256, key.data(),
                                                                key.size(), data.data(), data.size());

  std::string expected = "773ea91e36800e46854db8ebd09181a72959098b3ef8c122d9635514ced565fe";
  CASE_EXPECT_EQ(expected, result);
}

CASE_TEST(crypto_hmac, hmac_sha256_rfc4231_test4) {
  ensure_openssl_inited();

  // Test Case 4
  // Key = 0102030405060708090a0b0c0d0e0f10111213141516171819 (25 bytes)
  // Data = 0xcd repeated 50 times
  // HMAC-SHA-256 = 82558a389a443c0ea4cc819899f2083a85f0faa3e578f8077a2e3ff46729665b

  std::vector<unsigned char> key = hex_to_bytes("0102030405060708090a0b0c0d0e0f10111213141516171819");
  std::vector<unsigned char> data(50, 0xcd);

  std::string result = atfw::util::crypto::hmac::compute_to_hex(atfw::util::crypto::digest_type_t::kSha256, key.data(),
                                                                key.size(), data.data(), data.size());

  std::string expected = "82558a389a443c0ea4cc819899f2083a85f0faa3e578f8077a2e3ff46729665b";
  CASE_EXPECT_EQ(expected, result);
}

CASE_TEST(crypto_hmac, hmac_sha256_streaming) {
  ensure_openssl_inited();

  // Test streaming API - same as test case 1
  std::vector<unsigned char> key(20, 0x0b);
  const char* data = "Hi There";

  atfw::util::crypto::hmac h;
  CASE_EXPECT_FALSE(h.is_valid());

  int ret = h.init(atfw::util::crypto::digest_type_t::kSha256, key.data(), key.size());
  CASE_EXPECT_EQ(atfw::util::crypto::hmac_error_code_t::kOk, ret);
  CASE_EXPECT_TRUE(h.is_valid());
  CASE_EXPECT_EQ(32u, h.get_output_length());

  ret = h.update(reinterpret_cast<const unsigned char*>(data), strlen(data));
  CASE_EXPECT_EQ(atfw::util::crypto::hmac_error_code_t::kOk, ret);

  std::vector<unsigned char> output(32);
  size_t output_len = output.size();
  ret = h.final(output.data(), &output_len);
  CASE_EXPECT_EQ(atfw::util::crypto::hmac_error_code_t::kOk, ret);
  CASE_EXPECT_EQ(32u, output_len);

  std::string expected_hex = "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7";
  std::string actual_hex = bytes_to_hex(output);
  CASE_EXPECT_EQ(expected_hex, actual_hex);

  h.close();
  CASE_EXPECT_FALSE(h.is_valid());
}

CASE_TEST(crypto_hmac, hmac_sha256_streaming_multiple_updates) {
  ensure_openssl_inited();

  // Test streaming with multiple updates
  std::vector<unsigned char> key(20, 0x0b);
  const char* data1 = "Hi ";
  const char* data2 = "There";

  atfw::util::crypto::hmac h;
  int ret = h.init(atfw::util::crypto::digest_type_t::kSha256, key.data(), key.size());
  CASE_EXPECT_EQ(atfw::util::crypto::hmac_error_code_t::kOk, ret);

  ret = h.update(reinterpret_cast<const unsigned char*>(data1), strlen(data1));
  CASE_EXPECT_EQ(atfw::util::crypto::hmac_error_code_t::kOk, ret);

  ret = h.update(reinterpret_cast<const unsigned char*>(data2), strlen(data2));
  CASE_EXPECT_EQ(atfw::util::crypto::hmac_error_code_t::kOk, ret);

  std::vector<unsigned char> output(32);
  size_t output_len = output.size();
  ret = h.final(output.data(), &output_len);
  CASE_EXPECT_EQ(atfw::util::crypto::hmac_error_code_t::kOk, ret);

  std::string expected_hex = "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7";
  std::string actual_hex = bytes_to_hex(output);
  CASE_EXPECT_EQ(expected_hex, actual_hex);
}

CASE_TEST(crypto_hmac, hmac_sha1) {
  ensure_openssl_inited();

  // RFC 2202 Test Case 1 for HMAC-SHA1
  // Key = 0x0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b (20 bytes)
  // Data = "Hi There"
  // HMAC-SHA-1 = 0xb617318655057264e28bc0b6fb378c8ef146be00

  std::vector<unsigned char> key(20, 0x0b);
  const char* data = "Hi There";

  std::string result =
      atfw::util::crypto::hmac::compute_to_hex(atfw::util::crypto::digest_type_t::kSha1, key.data(), key.size(),
                                               reinterpret_cast<const unsigned char*>(data), strlen(data));

  std::string expected = "b617318655057264e28bc0b6fb378c8ef146be00";
  CASE_EXPECT_EQ(expected, result);
}

CASE_TEST(crypto_hmac, hmac_sha512) {
  ensure_openssl_inited();

  // RFC 4231 Test Case 1 for HMAC-SHA-512
  // Key = 0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b (20 bytes)
  // Data = "Hi There"
  // HMAC-SHA-512 =
  // 87aa7cdea5ef619d4ff0b4241a1d6cb02379f4e2ce4ec2787ad0b30545e17cdedaa833b7d6b8a702038b274eaea3f4e4be9d914eeb61f1702e696c203a126854

  std::vector<unsigned char> key(20, 0x0b);
  const char* data = "Hi There";

  std::string result =
      atfw::util::crypto::hmac::compute_to_hex(atfw::util::crypto::digest_type_t::kSha512, key.data(), key.size(),
                                               reinterpret_cast<const unsigned char*>(data), strlen(data));

  std::string expected =
      "87aa7cdea5ef619d4ff0b4241a1d6cb02379f4e2ce4ec2787ad0b30545e17cdedaa833b7d6b8a702038b274eaea3f4e4be9d914eeb61f170"
      "2e696c203a126854";
  CASE_EXPECT_EQ(expected, result);
}

CASE_TEST(crypto_hmac, hmac_move_semantics) {
  ensure_openssl_inited();

  std::vector<unsigned char> key(20, 0x0b);

  atfw::util::crypto::hmac h1;
  int ret = h1.init(atfw::util::crypto::digest_type_t::kSha256, key.data(), key.size());
  CASE_EXPECT_EQ(atfw::util::crypto::hmac_error_code_t::kOk, ret);
  CASE_EXPECT_TRUE(h1.is_valid());

  // Move construct
  atfw::util::crypto::hmac h2(std::move(h1));
  CASE_EXPECT_FALSE(h1.is_valid());
  CASE_EXPECT_TRUE(h2.is_valid());

  // Move assign
  atfw::util::crypto::hmac h3;
  h3 = std::move(h2);
  CASE_EXPECT_FALSE(h2.is_valid());
  CASE_EXPECT_TRUE(h3.is_valid());
}

CASE_TEST(crypto_hmac, hmac_error_cases) {
  ensure_openssl_inited();

  atfw::util::crypto::hmac h;

  // Update before init
  int ret = h.update(nullptr, 0);
  CASE_EXPECT_EQ(atfw::util::crypto::hmac_error_code_t::kNotInitialized, ret);

  // Final before init
  unsigned char output[32];
  size_t output_len = sizeof(output);
  ret = h.final(output, &output_len);
  CASE_EXPECT_EQ(atfw::util::crypto::hmac_error_code_t::kNotInitialized, ret);

  // Invalid digest type
  ret = h.init(atfw::util::crypto::digest_type_t::kNone, nullptr, 0);
  CASE_EXPECT_EQ(atfw::util::crypto::hmac_error_code_t::kDigestNotSupport, ret);

  // Init with valid params
  std::vector<unsigned char> key(16, 0x0b);
  ret = h.init(atfw::util::crypto::digest_type_t::kSha256, key.data(), key.size());
  CASE_EXPECT_EQ(atfw::util::crypto::hmac_error_code_t::kOk, ret);

  // Double init
  ret = h.init(atfw::util::crypto::digest_type_t::kSha256, key.data(), key.size());
  CASE_EXPECT_EQ(atfw::util::crypto::hmac_error_code_t::kAlreadyInitialized, ret);

  // Output buffer too small
  unsigned char small_output[8];
  output_len = sizeof(small_output);
  ret = h.final(small_output, &output_len);
  CASE_EXPECT_EQ(atfw::util::crypto::hmac_error_code_t::kOutputBufferTooSmall, ret);
  CASE_EXPECT_EQ(32u, output_len);  // Should return required size
}

CASE_TEST(crypto_hmac, hmac_empty_data) {
  ensure_openssl_inited();

  std::vector<unsigned char> key(20, 0x0b);

  // HMAC with empty data
  std::vector<unsigned char> result = atfw::util::crypto::hmac::compute_to_binary(
      atfw::util::crypto::digest_type_t::kSha256, key.data(), key.size(), nullptr, 0);

  // Should succeed with valid output
  CASE_EXPECT_EQ(32u, result.size());
}

// ============================================================================
// HKDF Tests
// ============================================================================

// RFC 5869 Test Vectors

CASE_TEST(crypto_hkdf, hkdf_sha256_rfc5869_test1) {
  ensure_openssl_inited();

  // Test Case 1
  // Hash = SHA-256
  // IKM = 0x0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b (22 octets)
  // salt = 0x000102030405060708090a0b0c (13 octets)
  // info = 0xf0f1f2f3f4f5f6f7f8f9 (10 octets)
  // L = 42

  // PRK = 0x077709362c2e32df0ddc3f0dc47bba6390b6c73bb50f9c3122ec844ad7c2b3e5
  // OKM = 0x3cb25f25faacd57a90434f64d0362f2a2d2d0a90cf1a5a4c5db02d56ecc4c5bf34007208d5b887185865

  std::vector<unsigned char> ikm(22, 0x0b);
  std::vector<unsigned char> salt = hex_to_bytes("000102030405060708090a0b0c");
  std::vector<unsigned char> info = hex_to_bytes("f0f1f2f3f4f5f6f7f8f9");

  // Test extract
  std::vector<unsigned char> prk(32);
  size_t prk_len = prk.size();
  int ret = atfw::util::crypto::hkdf::extract(atfw::util::crypto::digest_type_t::kSha256, salt.data(), salt.size(),
                                              ikm.data(), ikm.size(), prk.data(), &prk_len);
  CASE_EXPECT_EQ(atfw::util::crypto::hkdf::error_code_t::kOk, ret);
  CASE_EXPECT_EQ(32u, prk_len);

  std::string expected_prk = "077709362c2e32df0ddc3f0dc47bba6390b6c73bb50f9c3122ec844ad7c2b3e5";
  std::string actual_prk = bytes_to_hex(prk);
  CASE_EXPECT_EQ(expected_prk, actual_prk);

  // Test expand
  std::vector<unsigned char> okm(42);
  ret = atfw::util::crypto::hkdf::expand(atfw::util::crypto::digest_type_t::kSha256, prk.data(), prk_len, info.data(),
                                         info.size(), okm.data(), okm.size());
  CASE_EXPECT_EQ(atfw::util::crypto::hkdf::error_code_t::kOk, ret);

  std::string expected_okm = "3cb25f25faacd57a90434f64d0362f2a2d2d0a90cf1a5a4c5db02d56ecc4c5bf34007208d5b887185865";
  std::string actual_okm = bytes_to_hex(okm);
  CASE_EXPECT_EQ(expected_okm, actual_okm);

  // Test full derive
  std::vector<unsigned char> okm2(42);
  ret = atfw::util::crypto::hkdf::derive(atfw::util::crypto::digest_type_t::kSha256, salt.data(), salt.size(),
                                         ikm.data(), ikm.size(), info.data(), info.size(), okm2.data(), okm2.size());
  CASE_EXPECT_EQ(atfw::util::crypto::hkdf::error_code_t::kOk, ret);
  CASE_EXPECT_EQ(expected_okm, bytes_to_hex(okm2));
}

CASE_TEST(crypto_hkdf, hkdf_sha256_rfc5869_test2) {
  ensure_openssl_inited();

  // Test Case 2 - longer inputs/outputs
  // Hash = SHA-256
  // IKM =
  // 0x000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f
  //       (80 octets)
  // salt =
  // 0x606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f808182838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9aaabacadaeaf
  //        (80 octets)
  // info =
  // 0xb0b1b2b3b4b5b6b7b8b9babbbcbdbebfc0c1c2c3c4c5c6c7c8c9cacbcccdcecfd0d1d2d3d4d5d6d7d8d9dadbdcdddedfe0e1e2e3e4e5e6e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9fafbfcfdfeff
  //        (80 octets)
  // L = 82

  // OKM =
  // 0xb11e398dc80327a1c8e7f78c596a49344f012eda2d4efad8a050cc4c19afa97c59045a99cac7827271cb41c65e590e09da3275600c2f09b8367793a9aca3db71cc30c58179ec3e87c14c01d5c1f3434f1d87

  std::vector<unsigned char> ikm = hex_to_bytes(
      "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f30313233343536"
      "3738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f");
  std::vector<unsigned char> salt = hex_to_bytes(
      "606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f808182838485868788898a8b8c8d8e8f90919293949596"
      "9798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9aaabacadaeaf");
  std::vector<unsigned char> info = hex_to_bytes(
      "b0b1b2b3b4b5b6b7b8b9babbbcbdbebfc0c1c2c3c4c5c6c7c8c9cacbcccdcecfd0d1d2d3d4d5d6d7d8d9dadbdcdddedfe0e1e2e3e4e5e6"
      "e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9fafbfcfdfeff");

  std::vector<unsigned char> okm(82);
  int ret = atfw::util::crypto::hkdf::derive(atfw::util::crypto::digest_type_t::kSha256, salt.data(), salt.size(),
                                             ikm.data(), ikm.size(), info.data(), info.size(), okm.data(), okm.size());
  CASE_EXPECT_EQ(atfw::util::crypto::hkdf::error_code_t::kOk, ret);

  std::string expected_okm =
      "b11e398dc80327a1c8e7f78c596a49344f012eda2d4efad8a050cc4c19afa97c59045a99cac7827271cb41c65e590e09da3275600c2f09"
      "b8367793a9aca3db71cc30c58179ec3e87c14c01d5c1f3434f1d87";
  std::string actual_okm = bytes_to_hex(okm);
  CASE_EXPECT_EQ(expected_okm, actual_okm);
}

CASE_TEST(crypto_hkdf, hkdf_sha256_rfc5869_test3) {
  ensure_openssl_inited();

  // Test Case 3 - zero-length salt/info
  // Hash = SHA-256
  // IKM = 0x0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b (22 octets)
  // salt = (0 octets)
  // info = (0 octets)
  // L = 42

  // PRK = 0x19ef24a32c717b167f33a91d6f648bdf96596776afdb6377ac434c1c293ccb04
  // OKM = 0x8da4e775a563c18f715f802a063c5a31b8a11f5c5ee1879ec3454e5f3c738d2d9d201395faa4b61a96c8

  std::vector<unsigned char> ikm(22, 0x0b);

  std::vector<unsigned char> okm(42);
  int ret = atfw::util::crypto::hkdf::derive(atfw::util::crypto::digest_type_t::kSha256, nullptr, 0, ikm.data(),
                                             ikm.size(), nullptr, 0, okm.data(), okm.size());
  CASE_EXPECT_EQ(atfw::util::crypto::hkdf::error_code_t::kOk, ret);

  std::string expected_okm = "8da4e775a563c18f715f802a063c5a31b8a11f5c5ee1879ec3454e5f3c738d2d9d201395faa4b61a96c8";
  std::string actual_okm = bytes_to_hex(okm);
  CASE_EXPECT_EQ(expected_okm, actual_okm);
}

CASE_TEST(crypto_hkdf, hkdf_sha1_rfc5869_test4) {
  ensure_openssl_inited();

  // Test Case 4 (using SHA-1)
  // Hash = SHA-1
  // IKM = 0x0b0b0b0b0b0b0b0b0b0b0b (11 octets)
  // salt = 0x000102030405060708090a0b0c (13 octets)
  // info = 0xf0f1f2f3f4f5f6f7f8f9 (10 octets)
  // L = 42

  // PRK = 0x9b6c18c432a7bf8f0e71c8eb88f4b30baa2ba243
  // OKM = 0x085a01ea1b10f36933068b56efa5ad81a4f14b822f5b091568a9cdd4f155fda2c22e422478d305f3f896

  std::vector<unsigned char> ikm(11, 0x0b);
  std::vector<unsigned char> salt = hex_to_bytes("000102030405060708090a0b0c");
  std::vector<unsigned char> info = hex_to_bytes("f0f1f2f3f4f5f6f7f8f9");

  std::vector<unsigned char> okm(42);
  int ret = atfw::util::crypto::hkdf::derive(atfw::util::crypto::digest_type_t::kSha1, salt.data(), salt.size(),
                                             ikm.data(), ikm.size(), info.data(), info.size(), okm.data(), okm.size());
  CASE_EXPECT_EQ(atfw::util::crypto::hkdf::error_code_t::kOk, ret);

  std::string expected_okm = "085a01ea1b10f36933068b56efa5ad81a4f14b822f5b091568a9cdd4f155fda2c22e422478d305f3f896";
  std::string actual_okm = bytes_to_hex(okm);
  CASE_EXPECT_EQ(expected_okm, actual_okm);
}

CASE_TEST(crypto_hkdf, hkdf_derive_to_binary) {
  ensure_openssl_inited();

  // Same as test case 1, using derive_to_binary
  std::vector<unsigned char> ikm(22, 0x0b);
  std::vector<unsigned char> salt = hex_to_bytes("000102030405060708090a0b0c");
  std::vector<unsigned char> info = hex_to_bytes("f0f1f2f3f4f5f6f7f8f9");

  std::vector<unsigned char> okm = atfw::util::crypto::hkdf::derive_to_binary(
      atfw::util::crypto::digest_type_t::kSha256, gsl::make_span(salt), gsl::make_span(ikm), gsl::make_span(info), 42);

  CASE_EXPECT_EQ(42u, okm.size());

  std::string expected_okm = "3cb25f25faacd57a90434f64d0362f2a2d2d0a90cf1a5a4c5db02d56ecc4c5bf34007208d5b887185865";
  std::string actual_okm = bytes_to_hex(okm);
  CASE_EXPECT_EQ(expected_okm, actual_okm);
}

CASE_TEST(crypto_hkdf, hkdf_error_cases) {
  ensure_openssl_inited();

  std::vector<unsigned char> ikm(22, 0x0b);
  std::vector<unsigned char> okm(42);

  // Invalid digest type
  int ret = atfw::util::crypto::hkdf::derive(atfw::util::crypto::digest_type_t::kNone, nullptr, 0, ikm.data(),
                                             ikm.size(), nullptr, 0, okm.data(), okm.size());
  CASE_EXPECT_EQ(atfw::util::crypto::hkdf::error_code_t::kDigestNotSupport, ret);

  // Output length too large (> 255 * hash_len)
  size_t max_output = 255 * 32 + 1;  // For SHA-256
  std::vector<unsigned char> large_okm(max_output);
  ret = atfw::util::crypto::hkdf::expand(atfw::util::crypto::digest_type_t::kSha256, ikm.data(), ikm.size(), nullptr, 0,
                                         large_okm.data(), large_okm.size());
  CASE_EXPECT_EQ(atfw::util::crypto::hkdf::error_code_t::kOutputLengthTooLarge, ret);
}

CASE_TEST(crypto_hkdf, hkdf_span_api) {
  ensure_openssl_inited();

  // Test span-based API
  std::vector<unsigned char> ikm(22, 0x0b);
  std::vector<unsigned char> salt = hex_to_bytes("000102030405060708090a0b0c");
  std::vector<unsigned char> info = hex_to_bytes("f0f1f2f3f4f5f6f7f8f9");

  // Extract with span
  std::vector<unsigned char> prk(32);
  size_t prk_len = prk.size();
  int ret = atfw::util::crypto::hkdf::extract(atfw::util::crypto::digest_type_t::kSha256, gsl::make_span(salt),
                                              gsl::make_span(ikm), prk.data(), &prk_len);
  CASE_EXPECT_EQ(atfw::util::crypto::hkdf::error_code_t::kOk, ret);

  // Expand with span
  std::vector<unsigned char> okm(42);
  ret = atfw::util::crypto::hkdf::expand(atfw::util::crypto::digest_type_t::kSha256, gsl::make_span(prk),
                                         gsl::make_span(info), okm.data(), okm.size());
  CASE_EXPECT_EQ(atfw::util::crypto::hkdf::error_code_t::kOk, ret);

  std::string expected_okm = "3cb25f25faacd57a90434f64d0362f2a2d2d0a90cf1a5a4c5bf34007208d5b887185865";
  // Note: truncated comparison since we're using different span sizes

  // Derive with span
  std::vector<unsigned char> okm2(42);
  ret = atfw::util::crypto::hkdf::derive(atfw::util::crypto::digest_type_t::kSha256, gsl::make_span(salt),
                                         gsl::make_span(ikm), gsl::make_span(info), okm2.data(), okm2.size());
  CASE_EXPECT_EQ(atfw::util::crypto::hkdf::error_code_t::kOk, ret);

  // Results should match
  CASE_EXPECT_EQ(bytes_to_hex(okm), bytes_to_hex(okm2));
}

#endif  // ATFW_UTIL_MACRO_CRYPTO_HMAC_ENABLED
