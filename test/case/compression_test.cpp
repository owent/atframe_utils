// Copyright 2026 atframework

#include <algorithm/compression.h>

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include "frame/test_macros.h"

#ifdef ATFW_UTIL_MACRO_COMPRESSION_ENABLED

namespace {

static std::vector<unsigned char> make_sample_data() {
  const std::string seed =
      "atframework compression test data. atframework compression test data. atframework compression test data.";
  std::vector<unsigned char> data;
  data.reserve(seed.size() * 64);
  for (size_t i = 0; i < 64; ++i) {
    data.insert(data.end(), seed.begin(), seed.end());
  }
  return data;
}

static void verify_roundtrip(atfw::util::compression::algorithm_t algorithm) {
  std::vector<unsigned char> input = make_sample_data();
  std::vector<unsigned char> compressed;
  std::vector<unsigned char> decompressed;

  int ret = atfw::util::compression::compress(algorithm, gsl::make_span(input), compressed,
                                              atfw::util::compression::level_t::kBalanced);
  CASE_EXPECT_EQ(atfw::util::compression::error_code_t::kOk, ret);

  ret = atfw::util::compression::decompress(algorithm, gsl::make_span(compressed), input.size(), decompressed);
  CASE_EXPECT_EQ(atfw::util::compression::error_code_t::kOk, ret);
  CASE_EXPECT_EQ(input.size(), decompressed.size());

  if (input.size() == decompressed.size()) {
    CASE_EXPECT_EQ(0, std::memcmp(input.data(), decompressed.data(), input.size()));
  }
}

}  // namespace

CASE_TEST(compression, supported_algorithms_roundtrip) {
#  if defined(ATFW_UTIL_MACRO_COMPRESSION_ZSTD)
  verify_roundtrip(atfw::util::compression::algorithm_t::kZstd);
#  endif
#  if defined(ATFW_UTIL_MACRO_COMPRESSION_LZ4)
  verify_roundtrip(atfw::util::compression::algorithm_t::kLz4);
#  endif
#  if defined(ATFW_UTIL_MACRO_COMPRESSION_SNAPPY)
  verify_roundtrip(atfw::util::compression::algorithm_t::kSnappy);
#  endif
#  if defined(ATFW_UTIL_MACRO_COMPRESSION_ZLIB)
  verify_roundtrip(atfw::util::compression::algorithm_t::kZlib);
#  endif
}

CASE_TEST(compression, raw_level_roundtrip) {
  std::vector<unsigned char> input = make_sample_data();
  std::vector<unsigned char> compressed;
  std::vector<unsigned char> decompressed;
  int ret = 0;

#  if defined(ATFW_UTIL_MACRO_COMPRESSION_ZSTD)
  ret = atfw::util::compression::compress_with_raw_level(atfw::util::compression::algorithm_t::kZstd,
                                                         gsl::make_span(input), compressed, 1);
  CASE_EXPECT_EQ(atfw::util::compression::error_code_t::kOk, ret);
  ret = atfw::util::compression::decompress(atfw::util::compression::algorithm_t::kZstd, gsl::make_span(compressed),
                                            input.size(), decompressed);
  CASE_EXPECT_EQ(atfw::util::compression::error_code_t::kOk, ret);
#  endif

#  if defined(ATFW_UTIL_MACRO_COMPRESSION_LZ4)
  compressed.clear();
  decompressed.clear();
  ret = atfw::util::compression::compress_with_raw_level(atfw::util::compression::algorithm_t::kLz4,
                                                         gsl::make_span(input), compressed, 4);
  CASE_EXPECT_EQ(atfw::util::compression::error_code_t::kOk, ret);
  ret = atfw::util::compression::decompress(atfw::util::compression::algorithm_t::kLz4, gsl::make_span(compressed),
                                            input.size(), decompressed);
  CASE_EXPECT_EQ(atfw::util::compression::error_code_t::kOk, ret);
#  endif

#  if defined(ATFW_UTIL_MACRO_COMPRESSION_ZLIB)
  compressed.clear();
  decompressed.clear();
  ret = atfw::util::compression::compress_with_raw_level(atfw::util::compression::algorithm_t::kZlib,
                                                         gsl::make_span(input), compressed, 3);
  CASE_EXPECT_EQ(atfw::util::compression::error_code_t::kOk, ret);
  ret = atfw::util::compression::decompress(atfw::util::compression::algorithm_t::kZlib, gsl::make_span(compressed),
                                            input.size(), decompressed);
  CASE_EXPECT_EQ(atfw::util::compression::error_code_t::kOk, ret);
#  endif
}

CASE_TEST(compression, level_mapping) {
  atfw::util::compression::mapped_level_t map_zstd = atfw::util::compression::map_compression_level(
      atfw::util::compression::algorithm_t::kZstd, atfw::util::compression::level_t::kFast);
  CASE_EXPECT_EQ(-1, map_zstd.level);

  atfw::util::compression::mapped_level_t map_lz4 = atfw::util::compression::map_compression_level(
      atfw::util::compression::algorithm_t::kLz4, atfw::util::compression::level_t::kHighRatio);
  CASE_EXPECT_TRUE(map_lz4.use_high_compression);
  CASE_EXPECT_GE(map_lz4.level, 1);

  atfw::util::compression::mapped_level_t map_zlib = atfw::util::compression::map_compression_level(
      atfw::util::compression::algorithm_t::kZlib, atfw::util::compression::level_t::kBalanced);
  CASE_EXPECT_GE(map_zlib.level, 0);
}

CASE_TEST(compression, supported_algorithm_list) {
  std::vector<atfw::util::compression::algorithm_t> algos = atfw::util::compression::get_supported_algorithms();

  auto has_algo = [&algos](atfw::util::compression::algorithm_t algo) {
    return std::find(algos.begin(), algos.end(), algo) != algos.end();
  };

#  if defined(ATFW_UTIL_MACRO_COMPRESSION_ZSTD)
  CASE_EXPECT_TRUE(has_algo(atfw::util::compression::algorithm_t::kZstd));
#  else
  CASE_EXPECT_FALSE(has_algo(atfw::util::compression::algorithm_t::kZstd));
#  endif

#  if defined(ATFW_UTIL_MACRO_COMPRESSION_LZ4)
  CASE_EXPECT_TRUE(has_algo(atfw::util::compression::algorithm_t::kLz4));
#  else
  CASE_EXPECT_FALSE(has_algo(atfw::util::compression::algorithm_t::kLz4));
#  endif

#  if defined(ATFW_UTIL_MACRO_COMPRESSION_SNAPPY)
  CASE_EXPECT_TRUE(has_algo(atfw::util::compression::algorithm_t::kSnappy));
#  else
  CASE_EXPECT_FALSE(has_algo(atfw::util::compression::algorithm_t::kSnappy));
#  endif

#  if defined(ATFW_UTIL_MACRO_COMPRESSION_ZLIB)
  CASE_EXPECT_TRUE(has_algo(atfw::util::compression::algorithm_t::kZlib));
#  else
  CASE_EXPECT_FALSE(has_algo(atfw::util::compression::algorithm_t::kZlib));
#  endif
}

CASE_TEST(compression, unsupported_algorithm) {
  // algorithm_t::kNone should not be supported
  CASE_EXPECT_FALSE(atfw::util::compression::is_algorithm_supported(atfw::util::compression::algorithm_t::kNone));

  // Compress with unsupported algorithm
  std::vector<unsigned char> input = {1, 2, 3, 4, 5};
  std::vector<unsigned char> output;
  int ret =
      atfw::util::compression::compress(atfw::util::compression::algorithm_t::kNone, gsl::make_span(input), output);
  CASE_EXPECT_NE(atfw::util::compression::error_code_t::kOk, ret);

  // Decompress with unsupported algorithm
  ret = atfw::util::compression::decompress(atfw::util::compression::algorithm_t::kNone, gsl::make_span(input), 5,
                                            output);
  CASE_EXPECT_NE(atfw::util::compression::error_code_t::kOk, ret);
}

CASE_TEST(compression, get_algorithm_name) {
  CASE_EXPECT_TRUE(nullptr != atfw::util::compression::get_algorithm_name(atfw::util::compression::algorithm_t::kNone));

  // All supported algorithms should have names
  auto algos = atfw::util::compression::get_supported_algorithms();
  for (auto algo : algos) {
    const char *name = atfw::util::compression::get_algorithm_name(algo);
    CASE_EXPECT_TRUE(nullptr != name);
    CASE_EXPECT_TRUE(strlen(name) > 0);
    CASE_MSG_INFO() << "Algorithm: " << name << std::endl;
  }
}

CASE_TEST(compression, empty_input) {
  auto algos = atfw::util::compression::get_supported_algorithms();
  for (auto algo : algos) {
    std::vector<unsigned char> empty_input;
    std::vector<unsigned char> compressed;

    // Compress empty input - may succeed or fail depending on algorithm
    int ret = atfw::util::compression::compress(algo, gsl::make_span(empty_input), compressed);
    CASE_MSG_INFO() << "compress empty with " << atfw::util::compression::get_algorithm_name(algo) << ": ret=" << ret
                    << ", compressed_size=" << compressed.size() << std::endl;
  }
}

CASE_TEST(compression, all_compression_levels) {
  auto algos = atfw::util::compression::get_supported_algorithms();
  std::vector<unsigned char> input = make_sample_data();

  atfw::util::compression::level_t levels[] = {
      atfw::util::compression::level_t::kDefault,  atfw::util::compression::level_t::kStorage,
      atfw::util::compression::level_t::kFast,     atfw::util::compression::level_t::kLowCpu,
      atfw::util::compression::level_t::kBalanced, atfw::util::compression::level_t::kHighRatio,
      atfw::util::compression::level_t::kMaxRatio,
  };

  for (auto algo : algos) {
    for (auto level : levels) {
      std::vector<unsigned char> compressed;
      std::vector<unsigned char> decompressed;

      int ret = atfw::util::compression::compress(algo, gsl::make_span(input), compressed, level);
      CASE_EXPECT_EQ(atfw::util::compression::error_code_t::kOk, ret);

      if (atfw::util::compression::error_code_t::kOk == ret) {
        ret = atfw::util::compression::decompress(algo, gsl::make_span(compressed), input.size(), decompressed);
        CASE_EXPECT_EQ(atfw::util::compression::error_code_t::kOk, ret);
        CASE_EXPECT_EQ(input.size(), decompressed.size());
      }
    }
  }
}

CASE_TEST(compression, level_mapping_all_algorithms) {
  atfw::util::compression::algorithm_t algos[] = {
      atfw::util::compression::algorithm_t::kZstd,
      atfw::util::compression::algorithm_t::kLz4,
      atfw::util::compression::algorithm_t::kSnappy,
      atfw::util::compression::algorithm_t::kZlib,
  };

  atfw::util::compression::level_t levels[] = {
      atfw::util::compression::level_t::kDefault,  atfw::util::compression::level_t::kStorage,
      atfw::util::compression::level_t::kFast,     atfw::util::compression::level_t::kLowCpu,
      atfw::util::compression::level_t::kBalanced, atfw::util::compression::level_t::kHighRatio,
      atfw::util::compression::level_t::kMaxRatio,
  };

  for (auto algo : algos) {
    for (auto level : levels) {
      auto mapped = atfw::util::compression::map_compression_level(algo, level);
      // Just verify it doesn't crash and returns something
      (void)mapped;
    }
  }
}

CASE_TEST(compression, decompress_invalid_data) {
  auto algos = atfw::util::compression::get_supported_algorithms();

  for (auto algo : algos) {
    std::vector<unsigned char> garbage = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02, 0x03, 0x04};
    std::vector<unsigned char> output;

    // Decompressing garbage should fail or produce unexpected output
    int ret = atfw::util::compression::decompress(algo, gsl::make_span(garbage), 100, output);
    // We just verify it doesn't crash, error code depends on algorithm
    (void)ret;
  }
}

CASE_TEST(compression, small_input) {
  auto algos = atfw::util::compression::get_supported_algorithms();

  for (auto algo : algos) {
    // Single byte
    std::vector<unsigned char> input = {42};
    std::vector<unsigned char> compressed;
    std::vector<unsigned char> decompressed;

    int ret = atfw::util::compression::compress(algo, gsl::make_span(input), compressed);
    CASE_EXPECT_EQ(atfw::util::compression::error_code_t::kOk, ret);

    if (atfw::util::compression::error_code_t::kOk == ret) {
      ret = atfw::util::compression::decompress(algo, gsl::make_span(compressed), input.size(), decompressed);
      CASE_EXPECT_EQ(atfw::util::compression::error_code_t::kOk, ret);
      CASE_EXPECT_EQ(1, static_cast<int>(decompressed.size()));
      if (!decompressed.empty()) {
        CASE_EXPECT_EQ(42, static_cast<int>(decompressed[0]));
      }
    }
  }
}

#endif  // ATFW_UTIL_MACRO_COMPRESSION_ENABLED
