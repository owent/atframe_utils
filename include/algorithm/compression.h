// Copyright 2026 atframework
// Created by owent on 2026.01.20

#ifndef UTIL_ALGORITHM_COMPRESSION_H
#define UTIL_ALGORITHM_COMPRESSION_H

#pragma once

#include <config/atframe_utils_build_feature.h>

#include <gsl/select-gsl.h>

#if defined(ATFRAMEWORK_UTILS_COMPRESSION_ZSTD) && ATFRAMEWORK_UTILS_COMPRESSION_ZSTD
#  define ATFW_UTIL_MACRO_COMPRESSION_ZSTD 1
#endif

#if defined(ATFRAMEWORK_UTILS_COMPRESSION_LZ4) && ATFRAMEWORK_UTILS_COMPRESSION_LZ4
#  define ATFW_UTIL_MACRO_COMPRESSION_LZ4 1
#endif

#if defined(ATFRAMEWORK_UTILS_COMPRESSION_SNAPPY) && ATFRAMEWORK_UTILS_COMPRESSION_SNAPPY
#  define ATFW_UTIL_MACRO_COMPRESSION_SNAPPY 1
#endif

#if defined(ATFRAMEWORK_UTILS_COMPRESSION_ZLIB) && ATFRAMEWORK_UTILS_COMPRESSION_ZLIB
#  define ATFW_UTIL_MACRO_COMPRESSION_ZLIB 1
#endif

#if defined(ATFW_UTIL_MACRO_COMPRESSION_ZSTD) || defined(ATFW_UTIL_MACRO_COMPRESSION_LZ4) || \
    defined(ATFW_UTIL_MACRO_COMPRESSION_SNAPPY) || defined(ATFW_UTIL_MACRO_COMPRESSION_ZLIB)
#  define ATFW_UTIL_MACRO_COMPRESSION_ENABLED 1
#endif

#ifdef ATFW_UTIL_MACRO_COMPRESSION_ENABLED

#  include <cstddef>
#  include <cstdint>
#  include <string>
#  include <vector>

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace compression {

/**
 * @brief Compression algorithm type enumeration
 */
enum class algorithm_t : uint32_t {
  kNone = 0,
  kZstd = 100,
  kLz4 = 200,
  kSnappy = 300,
  kZlib = 400,
};

/**
 * @brief Unified compression level mapping
 */
enum class level_t : int32_t {
  kDefault = 0,
  kStorage = 100,
  kFast = 200,
  kLowCpu = 300,
  kBalanced = 400,
  kHighRatio = 500,
  kMaxRatio = 600,
};

/**
 * @brief Compression error codes
 */
struct ATFRAMEWORK_UTILS_API error_code_t {
  enum type {
    kOk = 0,
    kInvalidParam = -1,
    kNotSupport = -2,
    kBufferTooSmall = -3,
    kOperation = -4,
    kDisabled = -5,
  };
};

/**
 * @brief Mapped compression level for algorithm-specific parameters
 * @note For LZ4: use_high_compression=false uses LZ4_compress_fast with acceleration=level (0=default).
 *       use_high_compression=true uses LZ4_compress_HC with level.
 */
struct ATFRAMEWORK_UTILS_API mapped_level_t {
  int level;
  bool use_high_compression;
};

/**
 * @brief Check if algorithm is supported in current build
 */
ATFRAMEWORK_UTILS_API bool is_algorithm_supported(algorithm_t type) noexcept;

/**
 * @brief Get all supported compression algorithms in current build
 */
ATFRAMEWORK_UTILS_API std::vector<algorithm_t> get_supported_algorithms() noexcept;

/**
 * @brief Get algorithm name string
 */
ATFRAMEWORK_UTILS_API const char* get_algorithm_name(algorithm_t type) noexcept;

/**
 * @brief Map unified compression level to algorithm-specific parameters
 */
ATFRAMEWORK_UTILS_API mapped_level_t map_compression_level(algorithm_t type, level_t level) noexcept;

/**
 * @brief Compress input data with unified level mapping
 * @param type Compression algorithm
 * @param input Input data
 * @param output Output buffer (will be resized)
 * @param level Unified compression level
 * @return 0 on success, or error code
 */
ATFRAMEWORK_UTILS_API int compress(algorithm_t type, gsl::span<const unsigned char> input,
                                   std::vector<unsigned char>& output, level_t level = level_t::kDefault) noexcept;

/**
 * @brief Compress input data with raw level
 * @param type Compression algorithm
 * @param input Input data
 * @param output Output buffer (will be resized)
 * @param raw_level Algorithm-specific raw level
 * @return 0 on success, or error code
 */
ATFRAMEWORK_UTILS_API int compress_with_raw_level(algorithm_t type, gsl::span<const unsigned char> input,
                                                  std::vector<unsigned char>& output, int raw_level) noexcept;

/**
 * @brief Decompress input data
 * @param type Compression algorithm
 * @param input Compressed data
 * @param original_size Original size (bytes); 0 means auto-detect when supported
 * @param output Output buffer (will be resized)
 * @return 0 on success, or error code
 */
ATFRAMEWORK_UTILS_API int decompress(algorithm_t type, gsl::span<const unsigned char> input, size_t original_size,
                                     std::vector<unsigned char>& output) noexcept;

}  // namespace compression
ATFRAMEWORK_UTILS_NAMESPACE_END

#endif  // ATFW_UTIL_MACRO_COMPRESSION_ENABLED

#endif  // UTIL_ALGORITHM_COMPRESSION_H
