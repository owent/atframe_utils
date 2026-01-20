// Copyright 2026 atframework
// Created by owent on 2026.01.20

#include "algorithm/compression.h"

#ifdef ATFW_UTIL_MACRO_COMPRESSION_ENABLED

#  include <algorithm>
#  include <cstring>
#  include <limits>

#  if defined(ATFW_UTIL_MACRO_COMPRESSION_ZSTD)
#    include <zstd.h>
#  endif

#  if defined(ATFW_UTIL_MACRO_COMPRESSION_LZ4)
#    include <lz4.h>
#    include <lz4hc.h>
#  endif

#  if defined(ATFW_UTIL_MACRO_COMPRESSION_SNAPPY)
#    include <snappy-c.h>
#  endif

#  if defined(ATFW_UTIL_MACRO_COMPRESSION_ZLIB)
#    include <zlib.h>
#  endif

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace compression {

namespace {

static bool _size_to_int(size_t input, int& output) noexcept {
  if (input > static_cast<size_t>((std::numeric_limits<int>::max)())) {
    return false;
  }
  output = static_cast<int>(input);
  return true;
}

#  if defined(ATFW_UTIL_MACRO_COMPRESSION_ZLIB)
static bool _size_to_ulongf(size_t input, uLongf& output) noexcept {
  if (input > static_cast<size_t>((std::numeric_limits<uLongf>::max)())) {
    return false;
  }
  output = static_cast<uLongf>(input);
  return true;
}
#  endif

static int _clamp_int(int value, int min_value, int max_value) noexcept {
  if (value < min_value) {
    return min_value;
  }
  if (value > max_value) {
    return max_value;
  }
  return value;
}

static mapped_level_t _map_level_lz4(level_t level) noexcept {
  mapped_level_t result{0, false};
  switch (level) {
    case level_t::kDefault:
      result.level = 0;
      result.use_high_compression = false;
      break;
    case level_t::kStorage:
      result.level = 1;
      result.use_high_compression = false;
      break;
    case level_t::kFast:
      result.level = 1;
      result.use_high_compression = false;
      break;
    case level_t::kLowCpu:
      result.level = 3;
      result.use_high_compression = false;
      break;
    case level_t::kBalanced:
      result.level = 6;
      result.use_high_compression = false;
      break;
    case level_t::kHighRatio:
      result.level = 9;
      result.use_high_compression = true;
      break;
    case level_t::kMaxRatio:
      result.level = 12;
      result.use_high_compression = true;
      break;
    default:
      result.level = 0;
      result.use_high_compression = false;
      break;
  }
  return result;
}

static int _map_level_zstd(level_t level) noexcept {
  switch (level) {
    case level_t::kDefault:
      return ZSTD_defaultCLevel();
    case level_t::kStorage:
      return -1;
    case level_t::kFast:
      return -1;
    case level_t::kLowCpu:
      return 1;
    case level_t::kBalanced:
      return ZSTD_defaultCLevel();
    case level_t::kHighRatio:
      return 6;
    case level_t::kMaxRatio:
      return 12;
    default:
      return ZSTD_defaultCLevel();
  }
}

static int _map_level_zlib(level_t level) noexcept {
  switch (level) {
    case level_t::kDefault:
#  if defined(ATFW_UTIL_MACRO_COMPRESSION_ZLIB)
      return Z_DEFAULT_COMPRESSION;
#  else
      return 6;
#  endif
    case level_t::kStorage:
      return 1;
    case level_t::kFast:
      return 1;
    case level_t::kLowCpu:
      return 3;
    case level_t::kBalanced:
      return 6;
    case level_t::kHighRatio:
      return 9;
    case level_t::kMaxRatio:
      return 9;
    default:
      return 6;
  }
}

static int _compress_internal(algorithm_t type, gsl::span<const unsigned char> input,
                              std::vector<unsigned char>& output, bool use_raw_level, int raw_level,
                              level_t level) noexcept {
  if (input.data() == nullptr && input.size() > 0) {
    return error_code_t::kInvalidParam;
  }

  output.clear();

  switch (type) {
    case algorithm_t::kZstd: {
#  if defined(ATFW_UTIL_MACRO_COMPRESSION_ZSTD)
      size_t bound = ZSTD_compressBound(input.size());
      if (bound == 0) {
        return error_code_t::kOperation;
      }
      output.resize(bound);
      int zstd_level = use_raw_level ? raw_level : _map_level_zstd(level);
      size_t ret = ZSTD_compress(output.data(), output.size(), input.data(), input.size(), zstd_level);
      if (ZSTD_isError(ret)) {
        output.clear();
        return error_code_t::kOperation;
      }
      output.resize(ret);
      return error_code_t::kOk;
#  else
      return error_code_t::kNotSupport;
#  endif
    }

    case algorithm_t::kLz4: {
#  if defined(ATFW_UTIL_MACRO_COMPRESSION_LZ4)
      int input_size = 0;
      if (!_size_to_int(input.size(), input_size)) {
        return error_code_t::kInvalidParam;
      }

      int bound = LZ4_compressBound(input_size);
      if (bound <= 0) {
        return error_code_t::kOperation;
      }
      output.resize(static_cast<size_t>(bound));

      int result_size = 0;
      if (use_raw_level) {
        if (raw_level <= 0) {
          result_size = LZ4_compress_default(reinterpret_cast<const char*>(input.data()),
                                             reinterpret_cast<char*>(output.data()), input_size, bound);
        } else if (raw_level < LZ4HC_CLEVEL_MIN) {
          int accel = _clamp_int(raw_level, 1, 12);
          result_size = LZ4_compress_fast(reinterpret_cast<const char*>(input.data()),
                                          reinterpret_cast<char*>(output.data()), input_size, bound, accel);
        } else {
          int hc_level = _clamp_int(raw_level, LZ4HC_CLEVEL_MIN, LZ4HC_CLEVEL_MAX);
          result_size = LZ4_compress_HC(reinterpret_cast<const char*>(input.data()),
                                        reinterpret_cast<char*>(output.data()), input_size, bound, hc_level);
        }
      } else {
        mapped_level_t mapped = _map_level_lz4(level);
        if (mapped.use_high_compression) {
          int hc_level = _clamp_int(mapped.level, LZ4HC_CLEVEL_MIN, LZ4HC_CLEVEL_MAX);
          result_size = LZ4_compress_HC(reinterpret_cast<const char*>(input.data()),
                                        reinterpret_cast<char*>(output.data()), input_size, bound, hc_level);
        } else if (mapped.level <= 0) {
          result_size = LZ4_compress_default(reinterpret_cast<const char*>(input.data()),
                                             reinterpret_cast<char*>(output.data()), input_size, bound);
        } else {
          int accel = _clamp_int(mapped.level, 1, 12);
          result_size = LZ4_compress_fast(reinterpret_cast<const char*>(input.data()),
                                          reinterpret_cast<char*>(output.data()), input_size, bound, accel);
        }
      }

      if (result_size <= 0) {
        output.clear();
        return error_code_t::kOperation;
      }
      output.resize(static_cast<size_t>(result_size));
      return error_code_t::kOk;
#  else
      return error_code_t::kNotSupport;
#  endif
    }

    case algorithm_t::kSnappy: {
#  if defined(ATFW_UTIL_MACRO_COMPRESSION_SNAPPY)
      if (use_raw_level) {
        return error_code_t::kNotSupport;
      }
      size_t max_len = snappy_max_compressed_length(input.size());
      output.resize(max_len);
      size_t output_len = max_len;
      snappy_status status = snappy_compress(reinterpret_cast<const char*>(input.data()), input.size(),
                                             reinterpret_cast<char*>(output.data()), &output_len);
      if (status != SNAPPY_OK) {
        output.clear();
        return error_code_t::kOperation;
      }
      output.resize(output_len);
      return error_code_t::kOk;
#  else
      return error_code_t::kNotSupport;
#  endif
    }

    case algorithm_t::kZlib: {
#  if defined(ATFW_UTIL_MACRO_COMPRESSION_ZLIB)
      uLong input_size = 0;
      if (input.size() > static_cast<size_t>((std::numeric_limits<uLong>::max)())) {
        return error_code_t::kInvalidParam;
      }
      input_size = static_cast<uLong>(input.size());
      uLongf dest_len = 0;
      if (!_size_to_ulongf(static_cast<size_t>(compressBound(input_size)), dest_len)) {
        return error_code_t::kInvalidParam;
      }
      output.resize(static_cast<size_t>(dest_len));
      int zlib_level = use_raw_level ? raw_level : _map_level_zlib(level);
      int zret = compress2(reinterpret_cast<Bytef*>(output.data()), &dest_len,
                           reinterpret_cast<const Bytef*>(input.data()), input_size, zlib_level);
      if (zret != Z_OK) {
        output.clear();
        return error_code_t::kOperation;
      }
      output.resize(static_cast<size_t>(dest_len));
      return error_code_t::kOk;
#  else
      return error_code_t::kNotSupport;
#  endif
    }

    case algorithm_t::kNone:
    default:
      return error_code_t::kInvalidParam;
  }
}

}  // namespace

ATFRAMEWORK_UTILS_API bool is_algorithm_supported(algorithm_t type) noexcept {
  switch (type) {
    case algorithm_t::kZstd:
#  if defined(ATFW_UTIL_MACRO_COMPRESSION_ZSTD)
      return true;
#  else
      return false;
#  endif
    case algorithm_t::kLz4:
#  if defined(ATFW_UTIL_MACRO_COMPRESSION_LZ4)
      return true;
#  else
      return false;
#  endif
    case algorithm_t::kSnappy:
#  if defined(ATFW_UTIL_MACRO_COMPRESSION_SNAPPY)
      return true;
#  else
      return false;
#  endif
    case algorithm_t::kZlib:
#  if defined(ATFW_UTIL_MACRO_COMPRESSION_ZLIB)
      return true;
#  else
      return false;
#  endif
    case algorithm_t::kNone:
    default:
      return false;
  }
}

ATFRAMEWORK_UTILS_API std::vector<algorithm_t> get_supported_algorithms() noexcept {
  std::vector<algorithm_t> result;
#  if defined(ATFW_UTIL_MACRO_COMPRESSION_ZSTD)
  result.push_back(algorithm_t::kZstd);
#  endif
#  if defined(ATFW_UTIL_MACRO_COMPRESSION_LZ4)
  result.push_back(algorithm_t::kLz4);
#  endif
#  if defined(ATFW_UTIL_MACRO_COMPRESSION_SNAPPY)
  result.push_back(algorithm_t::kSnappy);
#  endif
#  if defined(ATFW_UTIL_MACRO_COMPRESSION_ZLIB)
  result.push_back(algorithm_t::kZlib);
#  endif
  return result;
}

ATFRAMEWORK_UTILS_API const char* get_algorithm_name(algorithm_t type) noexcept {
  switch (type) {
    case algorithm_t::kZstd:
      return "zstd";
    case algorithm_t::kLz4:
      return "lz4";
    case algorithm_t::kSnappy:
      return "snappy";
    case algorithm_t::kZlib:
      return "zlib";
    case algorithm_t::kNone:
    default:
      return "none";
  }
}

ATFRAMEWORK_UTILS_API mapped_level_t map_compression_level(algorithm_t type, level_t level) noexcept {
  switch (type) {
    case algorithm_t::kZstd:
      return mapped_level_t{_map_level_zstd(level), false};
    case algorithm_t::kLz4:
      return _map_level_lz4(level);
    case algorithm_t::kSnappy:
      return mapped_level_t{0, false};
    case algorithm_t::kZlib:
      return mapped_level_t{_map_level_zlib(level), false};
    case algorithm_t::kNone:
    default:
      return mapped_level_t{0, false};
  }
}

ATFRAMEWORK_UTILS_API int compress(algorithm_t type, gsl::span<const unsigned char> input,
                                   std::vector<unsigned char>& output, level_t level) noexcept {
  return _compress_internal(type, input, output, false, 0, level);
}

ATFRAMEWORK_UTILS_API int compress_with_raw_level(algorithm_t type, gsl::span<const unsigned char> input,
                                                  std::vector<unsigned char>& output, int raw_level) noexcept {
  return _compress_internal(type, input, output, true, raw_level, level_t::kDefault);
}

ATFRAMEWORK_UTILS_API int decompress(algorithm_t type, gsl::span<const unsigned char> input, size_t original_size,
                                     std::vector<unsigned char>& output) noexcept {
  if (input.data() == nullptr && input.size() > 0) {
    return error_code_t::kInvalidParam;
  }

  output.clear();

  switch (type) {
    case algorithm_t::kZstd: {
#  if defined(ATFW_UTIL_MACRO_COMPRESSION_ZSTD)
      size_t expect_size = original_size;
      if (expect_size == 0) {
        unsigned long long frame_size = ZSTD_getFrameContentSize(input.data(), input.size());
        if (frame_size == ZSTD_CONTENTSIZE_ERROR || frame_size == ZSTD_CONTENTSIZE_UNKNOWN) {
          return error_code_t::kInvalidParam;
        }
        expect_size = static_cast<size_t>(frame_size);
      }
      output.resize(expect_size);
      size_t ret = ZSTD_decompress(output.data(), output.size(), input.data(), input.size());
      if (ZSTD_isError(ret)) {
        output.clear();
        return error_code_t::kOperation;
      }
      output.resize(ret);
      return error_code_t::kOk;
#  else
      return error_code_t::kNotSupport;
#  endif
    }

    case algorithm_t::kLz4: {
#  if defined(ATFW_UTIL_MACRO_COMPRESSION_LZ4)
      if (original_size == 0) {
        return error_code_t::kInvalidParam;
      }

      int input_size = 0;
      int output_size = 0;
      if (!_size_to_int(input.size(), input_size) || !_size_to_int(original_size, output_size)) {
        return error_code_t::kInvalidParam;
      }

      output.resize(original_size);
      int ret = LZ4_decompress_safe(reinterpret_cast<const char*>(input.data()), reinterpret_cast<char*>(output.data()),
                                    input_size, output_size);
      if (ret < 0) {
        output.clear();
        return error_code_t::kOperation;
      }
      if (ret != output_size) {
        output.clear();
        return error_code_t::kOperation;
      }
      return error_code_t::kOk;
#  else
      return error_code_t::kNotSupport;
#  endif
    }

    case algorithm_t::kSnappy: {
#  if defined(ATFW_UTIL_MACRO_COMPRESSION_SNAPPY)
      size_t expect_size = original_size;
      if (expect_size == 0) {
        if (snappy_uncompressed_length(reinterpret_cast<const char*>(input.data()), input.size(), &expect_size) !=
            SNAPPY_OK) {
          return error_code_t::kInvalidParam;
        }
      }
      output.resize(expect_size);
      snappy_status status = snappy_uncompress(reinterpret_cast<const char*>(input.data()), input.size(),
                                               reinterpret_cast<char*>(output.data()), &expect_size);
      if (status != SNAPPY_OK) {
        output.clear();
        return error_code_t::kOperation;
      }
      output.resize(expect_size);
      return error_code_t::kOk;
#  else
      return error_code_t::kNotSupport;
#  endif
    }

    case algorithm_t::kZlib: {
#  if defined(ATFW_UTIL_MACRO_COMPRESSION_ZLIB)
      if (original_size == 0) {
        return error_code_t::kInvalidParam;
      }
      uLongf dest_len = 0;
      if (!_size_to_ulongf(original_size, dest_len)) {
        return error_code_t::kInvalidParam;
      }
      output.resize(original_size);
      int zret = uncompress(reinterpret_cast<Bytef*>(output.data()), &dest_len,
                            reinterpret_cast<const Bytef*>(input.data()), static_cast<uLong>(input.size()));
      if (zret != Z_OK) {
        output.clear();
        return error_code_t::kOperation;
      }
      output.resize(static_cast<size_t>(dest_len));
      return error_code_t::kOk;
#  else
      return error_code_t::kNotSupport;
#  endif
    }

    case algorithm_t::kNone:
    default:
      return error_code_t::kInvalidParam;
  }
}

}  // namespace compression
ATFRAMEWORK_UTILS_NAMESPACE_END

#endif  // ATFW_UTIL_MACRO_COMPRESSION_ENABLED
