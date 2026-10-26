// Copyright 2026 atframework
#include "string/ac_automation_v2.h"
#include "algorithm/bit.h"
#include "common/file_system.h"
#include "frame/test_macros.h"

#include <xxhash.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <fstream>
#include <memory>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {
using ac = atfw::util::string::ac_automation_v2;

// Independent version-2 format expectations keep corruption tests readable.
static constexpr uint64_t kXxh64Seed = 0;
static constexpr size_t kImageHeaderBytes = 128;
static constexpr size_t kImageFooterBytes = 8;
static constexpr size_t kDirectoryEntryBytes = 32;
static constexpr size_t kRequiredSectionCount = 8;
static constexpr size_t kDirectoryDataOffset = 8;
static constexpr size_t kDirectorySizeOffset = 16;
static constexpr size_t kDirectoryFlagsOffset = 4;
static constexpr size_t kDirectoryCountOffset = 24;
static constexpr size_t kHeaderMajorOffset = 12;
static constexpr size_t kHeaderMinorOffset = 14;
static constexpr size_t kHeaderSectionCountOffset = 28;
static constexpr size_t kHeaderNodeCountOffset = 48;
static constexpr size_t kHeaderKeywordCountOffset = 52;
static constexpr size_t kHeaderSymbolCountOffset = 56;
static constexpr size_t kHeaderFileSizeOffset = 32;
static constexpr size_t kHeaderMaxTokensOffset = 60;
static constexpr size_t kHeaderHashOffset = 80;
static constexpr size_t kHeaderChecksumOffset = 88;
static constexpr size_t kNodeStrideBytes = 16;
static constexpr size_t kIndexBytes = 4;
static constexpr size_t kNodeChildCountOffset = 4;
static constexpr size_t kNodeFailureOffset = 8;
static constexpr size_t kNodeOutputOffset = 12;
static constexpr size_t kKeywordLengthOffset = 4;
static constexpr size_t kKeywordSuffixOffset = 12;
static constexpr uint32_t kSectionNodes = 1;
static constexpr uint32_t kSectionLabels = 2;
static constexpr uint32_t kSectionSymbolOffsets = 3;
static constexpr uint32_t kSectionSkipBits = 5;
static constexpr uint32_t kSectionKeywords = 6;
static constexpr uint32_t kSectionFolds = 8;
static constexpr uint32_t kUnknownOptionalSectionType = 100;
static constexpr uint32_t kSectionOptionalFlag = 1;
static constexpr uint16_t kFutureMinorVersion = 1;
static constexpr uint16_t kUnsupportedMajorVersion = 3;
static constexpr size_t kExtraDirectoryEntryOffset = kImageHeaderBytes + kRequiredSectionCount * kDirectoryEntryBytes;
static constexpr size_t kOversizedTokenByteCount = SIZE_MAX;
static constexpr int kInvalidTokenStatusValue = 2;
static constexpr unsigned char kNonAsciiFirstByte = 0x80;
#if defined(ATFRAMEWORK_UTILS_ENABLE_EXCEPTION) && ATFRAMEWORK_UTILS_ENABLE_EXCEPTION
static constexpr char kCallbackExceptionMessage[] = "test callback";
#endif

static bool build(ac& value, std::initializer_list<const char*> words) {
  for (auto word : words) {
    auto code = value.insert_keyword(word);
    CASE_EXPECT_TRUE(code == ac::status::kOk);
    if (code != ac::status::kOk) {
      return false;
    }
  }
  auto code = value.build();
  CASE_EXPECT_TRUE(code == ac::status::kOk);
  return code == ac::status::kOk;
}

static ac::token_result whole(gsl::string_view input, void*) { return {ac::token_status::kValid, input.size()}; }

static ac::token_result bad_zero(gsl::string_view, void*) { return {ac::token_status::kValid, 0}; }

static ac::token_result bad_range(gsl::string_view input, void*) {
  return {ac::token_status::kValid, input.size() + 1};
}

static ac::token_result bad_max_count(gsl::string_view, void*) {
  return {ac::token_status::kValid, kOversizedTokenByteCount};
}

static ac::token_result bad_state(gsl::string_view, void*) {
  return {static_cast<ac::token_status>(kInvalidTokenStatusValue), 1};
}

static bool stop(const ac::match_t&, void*) { return false; }

static gsl::span<const unsigned char> bytes(const std::vector<unsigned char>& v) { return {v.data(), v.size()}; }

static bool dump_image(const ac& value, std::vector<unsigned char>& output) {
  auto code = value.dump(output);
  CASE_EXPECT_TRUE(code == ac::status::kOk);
  if (code != ac::status::kOk) {
    return false;
  }
  CASE_EXPECT_TRUE(output.size() >= kImageHeaderBytes + kImageFooterBytes);
  return output.size() >= kImageHeaderBytes + kImageFooterBytes;
}

static void resign(std::vector<unsigned char>& image) {
  atfw::util::bit::write_le_uint64(image.data() + kHeaderHashOffset, 0);
  atfw::util::bit::write_le_uint64(image.data() + kHeaderHashOffset,
                                   XXH64(image.data(), kImageHeaderBytes, kXxh64Seed));
  atfw::util::bit::write_le_uint64(image.data() + image.size() - kImageFooterBytes,
                                   XXH64(image.data(), image.size() - kImageFooterBytes, kXxh64Seed));
}

static size_t section_start(const std::vector<unsigned char>& image, uint32_t type) {
  return static_cast<size_t>(atfw::util::bit::read_le_uint64(
      image.data() + kImageHeaderBytes + (type - kSectionNodes) * kDirectoryEntryBytes + kDirectoryDataOffset));
}

static bool collect_node(const ac::node_view& node, void* data) {
  static_cast<std::vector<ac::node_view>*>(data)->push_back(node);
  return true;
}

static bool collect_hit(const ac::match_t& hit, void* data) {
  static_cast<std::vector<ac::match_t>*>(data)->push_back(hit);
  return true;
}

static bool expect_hits(const std::vector<ac::match_t>& expected, const std::vector<ac::match_t>& actual,
                        size_t input_bytes) {
  CASE_EXPECT_EQ(expected.size(), actual.size());
  if (expected.size() != actual.size()) {
    return false;
  }
  for (size_t i = 0; i < expected.size(); ++i) {
    CASE_EXPECT_EQ(expected[i].start, actual[i].start);
    CASE_EXPECT_EQ(expected[i].length, actual[i].length);
    CASE_EXPECT_EQ(expected[i].keyword_id, actual[i].keyword_id);
    CASE_EXPECT_TRUE(expected[i].keyword == actual[i].keyword);
    CASE_EXPECT_TRUE(actual[i].start <= input_bytes);
    if (actual[i].start <= input_bytes) {
      CASE_EXPECT_TRUE(actual[i].length <= input_bytes - actual[i].start);
    }
  }
  return true;
}

// Brute-force suffix comparison is independent of trie edges, failure links and the origin ring.
// This oracle accepts lowercase ASCII keywords; '-' is skipped only in normalized mode.
static std::vector<ac::match_t> ascii_oracle(const std::vector<std::string>& words, gsl::string_view input,
                                             bool overlap, bool normalize) {
  std::vector<ac::match_t> expected;
  std::string segment;
  std::vector<size_t> origins;
  for (size_t pos = 0; pos < input.size(); ++pos) {
    unsigned char c = static_cast<unsigned char>(input[pos]);
    if (c >= kNonAsciiFirstByte) {
      segment.clear();
      origins.clear();
      continue;
    }
    if (normalize && c == '-') {
      continue;
    }
    if (normalize && c >= 'A' && c <= 'Z') {
      c = static_cast<unsigned char>(c + ('a' - 'A'));
    }
    segment += static_cast<char>(c);
    origins.push_back(pos);
    std::vector<uint32_t> at_end;
    for (uint32_t id = 0; id < words.size(); ++id) {
      if (words[id].size() <= segment.size() &&
          segment.compare(segment.size() - words[id].size(), words[id].size(), words[id]) == 0) {
        at_end.push_back(id);
      }
    }
    std::sort(at_end.begin(), at_end.end(),
              [&](uint32_t left, uint32_t right) { return words[left].size() > words[right].size(); });
    for (uint32_t id : at_end) {
      size_t start = origins[segment.size() - words[id].size()];
      expected.push_back({start, pos + 1 - start, id, words[id]});
      if (!overlap) {
        segment.clear();
        origins.clear();
        break;
      }
    }
  }
  return expected;
}

static bool check_ascii_oracle(const std::vector<std::string>& words, gsl::string_view input, bool normalize) {
  ac value;
  if (normalize) {
    CASE_EXPECT_TRUE(value.set_nocase(true) == ac::status::kOk);
    CASE_EXPECT_TRUE(value.set_skip("-") == ac::status::kOk);
  }
  for (const auto& word : words) {
    auto code = value.insert_keyword(word);
    CASE_EXPECT_TRUE(code == ac::status::kOk);
    if (code != ac::status::kOk) {
      return false;
    }
  }
  auto code = value.build();
  CASE_EXPECT_TRUE(code == ac::status::kOk);
  if (code != ac::status::kOk) {
    return false;
  }
  std::vector<unsigned char> image;
  if (!dump_image(value, image)) {
    return false;
  }
  ac restored;
  ac::load_options load_options;
  load_options.verify_semantics = true;
  code = restored.load(bytes(image), load_options);
  CASE_EXPECT_TRUE(code == ac::status::kOk);
  if (code != ac::status::kOk) {
    return false;
  }
  ac::match_context context;
  for (bool overlap : {false, true}) {
    auto expected = ascii_oracle(words, input, overlap, normalize);
    for (const ac* model : {&value, &restored}) {
      ac::match_options options;
      options.allow_overlap = overlap;
      std::vector<ac::match_t> actual;
      auto report = model->match(input, actual, options, &context);
      CASE_EXPECT_TRUE(report.code == ac::status::kOk);
      CASE_EXPECT_TRUE(report.stopped == ac::stop_reason::kEndOfInput);
      CASE_EXPECT_EQ(input.size(), report.bytes_consumed);
      CASE_EXPECT_EQ(expected.size(), report.matches);
      if (!expect_hits(expected, actual, input.size())) {
        return false;
      }
      bool found = false;
      report = model->contains(input, found, options);
      CASE_EXPECT_TRUE(report.code == ac::status::kOk);
      CASE_EXPECT_TRUE(found == !expected.empty());
    }
  }
  return true;
}

struct ATFW_UTIL_SYMBOL_LOCAL limited_output_buffer : std::streambuf {
  size_t remaining;
  explicit limited_output_buffer(size_t size) : remaining(size) {}

  std::streamsize xsputn(const char*, std::streamsize count) override {
    size_t written = (std::min)(remaining, static_cast<size_t>(count));
    remaining -= written;
    return static_cast<std::streamsize>(written);
  }
};

struct ATFW_UTIL_SYMBOL_LOCAL nonseeking_input_buffer : std::streambuf {
  explicit nonseeking_input_buffer(std::string& input) {
    if (!input.empty()) {
      char* begin = &input[0];
      setg(begin, begin, begin + input.size());
    }
  }
};

#if defined(ATFRAMEWORK_UTILS_ENABLE_EXCEPTION) && ATFRAMEWORK_UTILS_ENABLE_EXCEPTION
static bool throw_on_hit(const ac::match_t&, void*) { throw std::runtime_error(kCallbackExceptionMessage); }

static ac::token_result throw_on_token(gsl::string_view input, void*) {
  if (input[0] == '!') {
    throw std::runtime_error(kCallbackExceptionMessage);
  }
  return {ac::token_status::kValid, 1};
}
#endif

static ac::token_result late_invalid_token(gsl::string_view input, void* result) {
  return input[0] == '!' ? *static_cast<ac::token_result*>(result) : ac::token_result{ac::token_status::kValid, 1};
}
}  // namespace

CASE_TEST(ac_automation_v2, multiple_nonoverlap_and_suffix_outputs) {
  ac value;
  if (!build(value, {"she", "he", "e", "hers", "b", "abcd"})) {
    return;
  }
  std::vector<ac::match_t> hits;
  auto report = value.match("ushersabcdshe", hits);
  CASE_EXPECT_TRUE(report.code == ac::status::kOk);
  CASE_EXPECT_EQ(3, hits.size());
  if (hits.size() != 3) {
    return;
  }
  CASE_EXPECT_EQ(1, hits[0].start);
  CASE_EXPECT_EQ(3, hits[0].length);
  CASE_EXPECT_EQ(0, hits[0].keyword_id);
  CASE_EXPECT_EQ(7, hits[1].start);
  CASE_EXPECT_EQ(1, hits[1].length);
  CASE_EXPECT_EQ(10, hits[2].start);
  CASE_EXPECT_EQ(3, hits[2].length);
  hits.clear();
  ac::match_options options;
  options.allow_overlap = true;
  report = value.match("ushers", hits, options);
  CASE_EXPECT_EQ(4, hits.size());
  if (hits.size() != 4) {
    return;
  }
  CASE_EXPECT_EQ(0, hits[0].keyword_id);
  CASE_EXPECT_EQ(1, hits[1].keyword_id);
  CASE_EXPECT_EQ(2, hits[2].keyword_id);
  CASE_EXPECT_EQ(3, hits[3].keyword_id);
  CASE_EXPECT_EQ(2, hits[3].start);
  CASE_EXPECT_EQ(4, hits[3].length);
}

CASE_TEST(ac_automation_v2, skip_fold_and_original_byte_ranges) {
  ac value;
  gsl::string_view skips[] = {" ", "-", "\n", "\xe3\x80\x80"};
  CASE_EXPECT_TRUE(value.set_skip(gsl::span<const gsl::string_view>(skips, 4)) == ac::status::kOk);
  CASE_EXPECT_TRUE(value.set_nocase(true) == ac::status::kOk);
  uint32_t first = UINT32_MAX;
  uint32_t duplicate = UINT32_MAX;
  CASE_EXPECT_TRUE(value.insert_keyword("Ab", &first) == ac::status::kOk);
  CASE_EXPECT_TRUE(value.insert_keyword("a-b", &duplicate) == ac::status::kAlreadyExists);
  CASE_EXPECT_EQ(first, duplicate);
  CASE_EXPECT_TRUE(value.set_skip("/") == ac::status::kInvalidState);
  CASE_EXPECT_TRUE(value.insert_keyword("\xe6\xb5\x8b\xe8\xaf\x95") == ac::status::kOk);
  CASE_EXPECT_TRUE(value.build() == ac::status::kOk);
  std::vector<ac::match_t> hits;
  value.match(" A-\nB  \xe6\xb5\x8b\xe3\x80\x80\xe8\xaf\x95 ", hits);
  CASE_EXPECT_EQ(2, hits.size());
  if (hits.size() != 2) {
    return;
  }
  CASE_EXPECT_EQ(1, hits[0].start);
  CASE_EXPECT_EQ(4, hits[0].length);
  CASE_EXPECT_EQ(7, hits[1].start);
  CASE_EXPECT_EQ(9, hits[1].length);
  CASE_EXPECT_TRUE(hits[0].keyword == "Ab");
}

CASE_TEST(ac_automation_v2, invalid_input_recovery_and_strict_output) {
  ac value;
  if (!build(value, {"ab", "cd"})) {
    return;
  }
  std::string input = std::string("ab a") + '\xff' + "bcd";
  std::vector<ac::match_t> hits;
  auto report = value.match(input, hits);
  CASE_EXPECT_EQ(2, hits.size());
  CASE_EXPECT_EQ(1, report.invalid_units);
  CASE_EXPECT_EQ(4, report.first_invalid_offset);
  if (hits.size() != 2) {
    return;
  }
  CASE_EXPECT_EQ(6, hits[1].start);
  ac::match_options strict;
  strict.invalid_input = ac::invalid_input_policy::kReject;
  report = value.match(input, hits, strict);
  CASE_EXPECT_TRUE(report.code == ac::status::kInvalidEncoding);
  CASE_EXPECT_EQ(2, hits.size());
  CASE_EXPECT_EQ(0, report.matches);
  CASE_EXPECT_EQ(4, report.bytes_consumed);
  ac::match_t slots[4];
  report = value.match(input, gsl::span<ac::match_t>(slots, 4), strict);
  CASE_EXPECT_EQ(1, report.matches);
  CASE_EXPECT_TRUE(report.code == ac::status::kInvalidEncoding);
  hits.clear();
  report = value.match(std::string(1, '\xc2') + "abcd", hits);
  CASE_EXPECT_EQ(2, hits.size());
  CASE_EXPECT_EQ(1, report.invalid_units);
}

CASE_TEST(ac_automation_v2, early_stops_and_context_reuse) {
  ac value;
  if (!build(value, {"ab", "b"})) {
    return;
  }
  ac::match_options options;
  options.allow_overlap = true;
  options.max_matches = 1;
  std::vector<ac::match_t> hits;
  ac::match_context context;
  auto report = value.match("abab", hits, options, &context);
  CASE_EXPECT_EQ(1, report.matches);
  CASE_EXPECT_EQ(2, report.bytes_consumed);
  CASE_EXPECT_TRUE(report.stopped == ac::stop_reason::kMatchLimit);
  options.max_matches = 0;
  ac::match_t slot;
  report = value.match("abab", gsl::span<ac::match_t>(&slot, 1), options, &context);
  CASE_EXPECT_TRUE(report.stopped == ac::stop_reason::kOutputFull);
  CASE_EXPECT_EQ(2, report.bytes_consumed);
  report = value.match("abab", stop, nullptr, options, &context);
  CASE_EXPECT_TRUE(report.stopped == ac::stop_reason::kCallbackStopped);
  CASE_EXPECT_EQ(1, report.matches);
  bool found = false;
  report = value.contains("abab", found);
  CASE_EXPECT_TRUE(found);
  CASE_EXPECT_EQ(2, report.bytes_consumed);
  size_t capacity = context.capacity();
  hits.clear();
  value.match("z", hits, options, &context);
  CASE_EXPECT_EQ(capacity, context.capacity());
  CASE_EXPECT_TRUE(hits.empty());
}

CASE_TEST(ac_automation_v2, tokenizers_respect_exact_allocations) {
  static constexpr size_t kEncodingCount = 3;
  static constexpr size_t kScalarBytes = 4;
  static constexpr char kUtf8Scalar[] = "\xf4\x8f\xbf\xbf";
  static constexpr char kUtf16LeScalar[] = "\x00\xd8\x00\xdc";
  static constexpr char kUtf16BeScalar[] = "\xd8\x00\xdc\x00";
  static constexpr size_t kInvalidUnitBytes[kEncodingCount] = {1, 2, 2};
  const char* const samples[kEncodingCount] = {kUtf8Scalar, kUtf16LeScalar, kUtf16BeScalar};
  const ac::tokenize_fn splitters[kEncodingCount] = {ac::next_utf8, ac::next_utf16le, ac::next_utf16be};
  for (size_t encoding = 0; encoding < kEncodingCount; ++encoding) {
    auto empty = splitters[encoding](gsl::string_view(), nullptr);
    CASE_EXPECT_TRUE(empty.state == ac::token_status::kInvalid);
    CASE_EXPECT_EQ(0, empty.byte_count);
    for (size_t length = 1; length <= kScalarBytes; ++length) {
      // Exact allocations let ASAN detect a read past the logical view, including SSO-sized inputs.
      std::unique_ptr<char[]> input(new char[length]);
      std::memcpy(input.get(), samples[encoding], length);
      auto result = splitters[encoding](gsl::string_view(input.get(), length), nullptr);
      CASE_EXPECT_TRUE(result.state ==
                       (length == kScalarBytes ? ac::token_status::kValid : ac::token_status::kInvalid));
      CASE_EXPECT_EQ(length == kScalarBytes ? kScalarBytes : (std::min)(length, kInvalidUnitBytes[encoding]),
                     result.byte_count);
      CASE_EXPECT_TRUE(result.byte_count <= length);
    }
  }
}

CASE_TEST(ac_automation_v2, span_bounds_and_invalid_tokenizer_outputs) {
  static constexpr size_t kWritableSlots = 1;
  static constexpr size_t kGuardedSlots = kWritableSlots + 2;
  static constexpr size_t kCanaryStart = 0x1234;
  static constexpr size_t kCanaryLength = 0x2345;
  static constexpr uint32_t kCanaryKeywordId = 0xdeadbeef;
  static constexpr char kCanaryKeyword[] = "guard";
  static constexpr size_t kCanaryIndices[] = {0, kGuardedSlots - 1};
  ac value;
  if (!build(value, {"ab", "b"})) {
    return;
  }
  std::array<ac::match_t, kGuardedSlots> slots{};
  const ac::match_t canary{kCanaryStart, kCanaryLength, kCanaryKeywordId, kCanaryKeyword};
  slots.fill(canary);
  ac::match_options options;
  options.allow_overlap = true;
  auto report = value.match("abab", gsl::span<ac::match_t>(&slots[1], kWritableSlots), options);
  CASE_EXPECT_TRUE(report.code == ac::status::kOk);
  CASE_EXPECT_TRUE(report.stopped == ac::stop_reason::kOutputFull);
  CASE_EXPECT_EQ(kWritableSlots, report.matches);
  CASE_EXPECT_EQ(2, report.bytes_consumed);
  CASE_EXPECT_EQ(0, slots[1].start);
  CASE_EXPECT_EQ(2, slots[1].length);
  CASE_EXPECT_EQ(0, slots[1].keyword_id);
  CASE_EXPECT_TRUE(slots[1].keyword == "ab");
  const ac::tokenize_fn invalid[] = {bad_zero, bad_range, bad_max_count, bad_state};
  for (auto splitter : invalid) {
    options.tokens = ac::tokenizer(splitter);
    report = value.match("abab", gsl::span<ac::match_t>(&slots[1], kWritableSlots), options);
    CASE_EXPECT_TRUE(report.code == ac::status::kInvalidTokenizerResult);
    CASE_EXPECT_EQ(0, report.matches);
    CASE_EXPECT_EQ(0, report.bytes_consumed);
    CASE_EXPECT_EQ(0, slots[1].start);
    CASE_EXPECT_EQ(2, slots[1].length);
    CASE_EXPECT_EQ(0, slots[1].keyword_id);
    CASE_EXPECT_TRUE(slots[1].keyword == "ab");
  }
  options.tokens = ac::tokenizer();
  report = value.match("abab", gsl::span<ac::match_t>(), options);
  CASE_EXPECT_TRUE(report.code == ac::status::kOk);
  CASE_EXPECT_TRUE(report.stopped == ac::stop_reason::kOutputFull);
  CASE_EXPECT_EQ(0, report.matches);
  CASE_EXPECT_EQ(0, report.bytes_consumed);
  for (size_t index : kCanaryIndices) {
    CASE_EXPECT_EQ(kCanaryStart, slots[index].start);
    CASE_EXPECT_EQ(kCanaryLength, slots[index].length);
    CASE_EXPECT_EQ(kCanaryKeywordId, slots[index].keyword_id);
    CASE_EXPECT_TRUE(slots[index].keyword == kCanaryKeyword);
  }
}

CASE_TEST(ac_automation_v2, mixed_opaque_tokens_and_utf16_recovery) {
  ac value;
  const std::string utf16("A\0B\0", 4);
  ac::tokenizer le(ac::token_encoding::kUtf16Le);
  CASE_EXPECT_TRUE(value.insert_keyword("AB") == ac::status::kOk);
  CASE_EXPECT_TRUE(value.insert_keyword(utf16, nullptr, le) == ac::status::kOk);
  CASE_EXPECT_TRUE(value.insert_keyword("AB", nullptr, ac::tokenizer(whole)) == ac::status::kOk);
  std::string large(100, 'x');
  CASE_EXPECT_TRUE(value.insert_keyword(large, nullptr, ac::tokenizer(whole)) == ac::status::kOk);
  CASE_EXPECT_TRUE(value.build() == ac::status::kOk);
  std::vector<ac::match_t> hits;
  ac::match_options options;
  options.tokens = le;
  std::string input("\0\xd8", 2);
  input += utf16;
  input += 'x';
  auto report = value.match(input, hits, options);
  CASE_EXPECT_EQ(1, hits.size());
  CASE_EXPECT_EQ(2, report.invalid_units);
  if (hits.size() != 1) {
    return;
  }
  CASE_EXPECT_EQ(2, hits[0].start);
  CASE_EXPECT_EQ(4, hits[0].length);
  CASE_EXPECT_EQ(1, hits[0].keyword_id);
  options.tokens = ac::tokenizer(whole);
  hits.clear();
  value.match("AB", hits, options);
  CASE_EXPECT_EQ(1, hits.size());
  if (hits.size() == 1) {
    CASE_EXPECT_EQ(2, hits[0].keyword_id);
  }
  hits.clear();
  value.match(large, hits, options);
  CASE_EXPECT_EQ(1, hits.size());
}

CASE_TEST(ac_automation_v2, copy_on_write_and_view_lifetime) {
  ac first;
  CASE_EXPECT_TRUE(first.insert_keyword("one") == ac::status::kOk);
  ac second = first;
  CASE_EXPECT_TRUE(second.insert_keyword("two") == ac::status::kOk);
  CASE_EXPECT_TRUE(first.build() == ac::status::kOk);
  CASE_EXPECT_TRUE(second.build() == ac::status::kOk);
  bool found = true;
  first.contains("two", found);
  CASE_EXPECT_FALSE(found);
  second.contains("two", found);
  CASE_EXPECT_TRUE(found);
  ac third = second;
  auto keyword = third.keyword(1);
  CASE_EXPECT_TRUE(keyword.data() == second.keyword(1).data());
  second.reset();
  CASE_EXPECT_TRUE(keyword == "two");
  third.contains("two", found);
  CASE_EXPECT_TRUE(found);
}

CASE_TEST(ac_automation_v2, reject_bad_callbacks_and_transactional_insertion) {
  ac value;
  CASE_EXPECT_TRUE(value.insert_keyword("ab", nullptr, ac::tokenizer(bad_zero)) == ac::status::kInvalidTokenizerResult);
  CASE_EXPECT_TRUE(value.insert_keyword("ab", nullptr, ac::tokenizer(bad_max_count)) ==
                   ac::status::kInvalidTokenizerResult);
  CASE_EXPECT_TRUE(value.insert_keyword("ab", nullptr, ac::tokenizer(bad_state)) ==
                   ac::status::kInvalidTokenizerResult);
  CASE_EXPECT_TRUE(value.insert_keyword(std::string("ab") + '\xff') == ac::status::kInvalidEncoding);
  CASE_EXPECT_TRUE(value.set_nocase(true) == ac::status::kOk);
  if (!build(value, {"cd"})) {
    return;
  }
  ac::match_options options;
  options.tokens = ac::tokenizer(bad_range);
  std::vector<ac::match_t> hits;
  auto report = value.match("cd", hits, options);
  CASE_EXPECT_TRUE(report.code == ac::status::kInvalidTokenizerResult);
  CASE_EXPECT_TRUE(hits.empty());
  bool found = true;
  value.contains("AB", found);
  CASE_EXPECT_FALSE(found);
}

CASE_TEST(ac_automation_v2, concurrent_shared_readers_keep_their_snapshot) {
  static constexpr unsigned kConcurrentReaderCount = 8;
  static constexpr unsigned kReaderIterations = 200;
  ac value;
  value.set_skip("-");
  if (!build(value, {"she", "he", "hers"})) {
    return;
  }
  std::atomic<bool> start{false};
  std::atomic<unsigned> failures{0};
  std::vector<std::thread> readers;
  for (unsigned i = 0; i < kConcurrentReaderCount; ++i) {
    ac copy = value;
    readers.emplace_back([copy, &start, &failures] {
      while (!start.load(std::memory_order_acquire)) {
        std::this_thread::yield();
      }
      ac::match_context context;
      std::vector<ac::match_t> hits;
      for (unsigned repeat = 0; repeat < kReaderIterations; ++repeat) {
        hits.clear();
        auto report = copy.match("s-heh-ers", hits, ac::match_options(), &context);
        // Earliest-end emits "he" before the longer "hers" can finish.
        if (report.code != ac::status::kOk || hits.size() != 2 || hits[0].start != 0 || hits[0].length != 4 ||
            hits[1].start != 4 || hits[1].length != 3 || hits[0].keyword != "she" || hits[1].keyword != "he") {
          ++failures;
        }
      }
    });
  }
  value.reset();
  start.store(true, std::memory_order_release);
  for (auto& reader : readers) {
    reader.join();
  }
  CASE_EXPECT_EQ(0, failures.load());
  CASE_EXPECT_FALSE(value.is_built());
}

CASE_TEST(ac_automation_v2, fold_configuration_is_atomic) {
  ac value;
  ac::case_fold_rule chain[] = {{"xy", "yz"}, {"yz", "zz"}};
  CASE_EXPECT_TRUE(value.set_case_folding(gsl::span<const ac::case_fold_rule>(chain, 2)) ==
                   ac::status::kInvalidArgument);
  ac::case_fold_rule rule[] = {{"long-token", "x"}};
  CASE_EXPECT_TRUE(value.set_case_folding(gsl::span<const ac::case_fold_rule>(rule, 1)) == ac::status::kOk);
  CASE_EXPECT_TRUE(value.set_nocase(true) == ac::status::kOk);
  CASE_EXPECT_TRUE(value.insert_keyword("x") == ac::status::kOk);
  CASE_EXPECT_TRUE(value.build() == ac::status::kOk);
  ac::match_options options;
  options.tokens = ac::tokenizer(whole);
  std::vector<ac::match_t> hits;
  value.match("long-token", hits, options);
  CASE_EXPECT_EQ(1, hits.size());
  if (!hits.empty()) {
    CASE_EXPECT_EQ(10, hits[0].length);
  }
}

CASE_TEST(ac_automation_v2, binary_roundtrip_and_failed_load_keeps_snapshot) {
  ac value;
  value.set_nocase(true);
  value.set_skip("-");
  if (!build(value, {"he", "she", "hers"})) {
    return;
  }
  std::vector<unsigned char> dump;
  CASE_EXPECT_TRUE(value.dump(dump) == ac::status::kOk);
  CASE_EXPECT_TRUE(dump.size() > 128);
  if (dump.size() <= 128) {
    return;
  }
  CASE_EXPECT_TRUE(std::memcmp(dump.data(), "ACAUTOMATION", 12) == 0);
  ac loaded;
  ac::load_options options;
  options.verify_semantics = true;
  CASE_EXPECT_TRUE(loaded.load(bytes(dump), options) == ac::status::kOk);
  std::vector<unsigned char> again;
  loaded.dump(again);
  CASE_EXPECT_TRUE(dump == again);
  std::vector<ac::match_t> hits;
  loaded.match("S-HE", hits);
  CASE_EXPECT_EQ(1, hits.size());
  std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
  CASE_EXPECT_TRUE(value.dump(stream) == ac::status::kOk);
  ac streamed;
  CASE_EXPECT_TRUE(streamed.load(stream, options) == ac::status::kOk);
  auto old_view = loaded.keyword(1);
  const char* old_address = old_view.data();
  auto damaged = dump;
  damaged[damaged.size() - 1] ^= 1;
  CASE_EXPECT_TRUE(loaded.load(bytes(damaged)) == ac::status::kChecksumMismatch);
  CASE_EXPECT_TRUE(loaded.keyword(1).data() == old_address);
  CASE_EXPECT_TRUE(old_view == "she");
  options.max_memory_bytes = 1;
  CASE_EXPECT_TRUE(loaded.load(bytes(dump), options) == ac::status::kResourceLimit);
  for (size_t length : {size_t{0}, size_t{12}, size_t{127}, dump.size() - 1}) {
    CASE_EXPECT_TRUE(loaded.load(gsl::span<const unsigned char>(dump.data(), length)) != ac::status::kOk);
  }
}

CASE_TEST(ac_automation_v2, empty_automaton_and_bounded_scratch) {
  ac empty;
  CASE_EXPECT_TRUE(empty.build() == ac::status::kOk);
  std::vector<unsigned char> dump;
  empty.dump(dump);
  ac loaded;
  CASE_EXPECT_TRUE(loaded.load(bytes(dump)) == ac::status::kOk);
  std::vector<ac::match_t> hits;
  auto report = loaded.match("abc", hits);
  CASE_EXPECT_TRUE(report.code == ac::status::kOk);
  CASE_EXPECT_TRUE(hits.empty());
  ac value;
  std::string long_word(10000, 'x');
  value.insert_keyword(long_word);
  value.build();
  ac::match_context context;
  value.match("ab", hits, ac::match_options(), &context);
  CASE_EXPECT_TRUE(context.capacity() < 100);
  CASE_EXPECT_TRUE(hits.empty());
}

CASE_TEST(ac_automation_v2, alphabet_larger_than_uint16) {
  ac value;
  ac::tokenizer token(whole);
  for (uint32_t i = 0; i < 100007; ++i) {
    char bytes[4];
    atfw::util::bit::write_le_uint32(reinterpret_cast<unsigned char*>(bytes), i);
    auto result = value.insert_keyword(gsl::string_view(bytes, 4), nullptr, token);
    if (result != ac::status::kOk) {
      CASE_EXPECT_TRUE(result == ac::status::kOk);
      return;
    }
  }
  CASE_EXPECT_TRUE(value.build() == ac::status::kOk);
  CASE_EXPECT_EQ(100007, value.get_statistics().symbols);
  ac::match_options options;
  options.tokens = token;
  char query[4];
  atfw::util::bit::write_le_uint32(reinterpret_cast<unsigned char*>(query), 100006);
  std::vector<ac::match_t> hits;
  value.match(gsl::string_view(query, 4), hits, options);
  CASE_EXPECT_EQ(1, hits.size());
  if (hits.size() == 1) {
    CASE_EXPECT_EQ(100006, hits[0].keyword_id);
  }
  std::vector<unsigned char> dump;
  value.dump(dump);
  ac loaded;
  CASE_EXPECT_TRUE(loaded.load(bytes(dump)) == ac::status::kOk);
  hits.clear();
  loaded.match(gsl::string_view(query, 4), hits, options);
  CASE_EXPECT_EQ(1, hits.size());
}

CASE_TEST(ac_automation_v2, rehashed_malformed_images_are_rejected) {
  ac value;
  value.set_nocase(true);
  value.set_skip("-");
  if (!build(value, {"ab", "bc", "abcd"})) {
    return;
  }
  std::vector<unsigned char> good;
  value.dump(good);
  const size_t nodes = section_start(good, kSectionNodes);
  const size_t labels = section_start(good, kSectionLabels);
  const size_t offsets = section_start(good, kSectionSymbolOffsets);
  const size_t words = section_start(good, kSectionKeywords);
  const size_t folds = section_start(good, kSectionFolds);
  const std::pair<size_t, uint32_t> corruptions[] = {{nodes, UINT32_MAX},
                                                     {nodes + kNodeChildCountOffset, UINT32_MAX},
                                                     {nodes + kNodeStrideBytes + kNodeFailureOffset, 1},
                                                     {nodes + kNodeOutputOffset, 0},
                                                     {labels + kIndexBytes, UINT32_MAX},
                                                     {offsets + kIndexBytes, 0},
                                                     {offsets + 2 * kIndexBytes, UINT32_MAX},
                                                     {words, UINT32_MAX},
                                                     {words + kKeywordLengthOffset, UINT32_MAX},
                                                     {words + kKeywordSuffixOffset, 0},
                                                     {folds, 0},
                                                     {kHeaderMaxTokensOffset, 0}};
  ac loaded;
  CASE_EXPECT_TRUE(loaded.load(bytes(good)) == ac::status::kOk);
  const char* before = loaded.keyword(0).data();
  for (const auto& corruption : corruptions) {
    auto image = good;
    atfw::util::bit::write_le_uint32(image.data() + corruption.first, corruption.second);
    resign(image);
    auto code = loaded.load(bytes(image));
    CASE_EXPECT_TRUE(code == ac::status::kInvalidFormat);
    CASE_EXPECT_TRUE(before == loaded.keyword(0).data());
  }
  auto image = good;
  // A rehashed section extending beyond the file must fail before allocation or traversal.
  atfw::util::bit::write_le_uint64(image.data() + kImageHeaderBytes + kDirectorySizeOffset, UINT64_MAX);
  resign(image);
  CASE_EXPECT_TRUE(loaded.load(bytes(image)) == ac::status::kInvalidFormat);
}

CASE_TEST(ac_automation_v2, unicode_boundaries_and_bounded_views) {
  const std::string invalid[] = {"\xc0\x80", "\xe0\x80\x80", "\xed\xa0\x80", "\xf4\x90\x80\x80", "\xf5\x80\x80\x80",
                                 "\x80",     "\xff",         "\xc2",         "\xe1\x80",         "\xf1\x80\x80"};
  for (const auto& input : invalid) {
    auto token = ac::next_utf8(input);
    CASE_EXPECT_TRUE(token.state == ac::token_status::kInvalid);
    CASE_EXPECT_EQ(1, token.byte_count);
  }
  const std::string valid[] = {std::string(1, '\0'), "\xc2\x80",     "\xdf\xbf",         "\xe0\xa0\x80",
                               "\xed\x9f\xbf",       "\xef\xbf\xbf", "\xf0\x90\x80\x80", "\xf4\x8f\xbf\xbf"};
  for (const auto& input : valid) {
    auto token = ac::next_utf8(input);
    CASE_EXPECT_TRUE(token.state == ac::token_status::kValid);
    CASE_EXPECT_EQ(input.size(), token.byte_count);
    for (size_t n = 1; n < input.size(); ++n) {
      CASE_EXPECT_TRUE(ac::next_utf8(gsl::string_view(input.data(), n)).state == ac::token_status::kInvalid);
    }
  }
  const char be[] = {'\xd8', '\0', '\xdc', '\0'};
  CASE_EXPECT_TRUE(ac::next_utf16be(gsl::string_view(be, 4)).state == ac::token_status::kValid);
  CASE_EXPECT_EQ(4, ac::next_utf16be(gsl::string_view(be, 4)).byte_count);
  for (size_t n = 1; n < 4; ++n) {
    CASE_EXPECT_TRUE(ac::next_utf16be(gsl::string_view(be, n)).state == ac::token_status::kInvalid);
  }
  ac value;
  ac::tokenizer tokens(ac::token_encoding::kUtf16Be);
  CASE_EXPECT_TRUE(value.insert_keyword(gsl::string_view(be, 4), nullptr, tokens) == ac::status::kOk);
  value.build();
  ac::match_options options;
  options.tokens = tokens;
  std::vector<ac::match_t> hits;
  value.match(gsl::string_view(be, 4), hits, options);
  CASE_EXPECT_EQ(1, hits.size());
}

CASE_TEST(ac_automation_v2, compatible_optional_sections_and_unknown_major) {
  ac value;
  if (!build(value, {"abc"})) {
    return;
  }
  std::vector<unsigned char> image;
  value.dump(image);
  image.insert(image.begin() + kExtraDirectoryEntryOffset, kDirectoryEntryBytes, 0);
  auto* data = image.data();
  atfw::util::bit::write_le_uint16(data + kHeaderMinorOffset, kFutureMinorVersion);
  atfw::util::bit::write_le_uint32(data + kHeaderSectionCountOffset, kRequiredSectionCount + 1);
  atfw::util::bit::write_le_uint64(data + kHeaderFileSizeOffset, image.size());
  atfw::util::bit::write_le_uint64(data + kHeaderChecksumOffset, image.size() - kImageFooterBytes);
  for (size_t i = 0; i < kRequiredSectionCount; ++i) {
    unsigned char* offset = data + kImageHeaderBytes + i * kDirectoryEntryBytes + kDirectoryDataOffset;
    atfw::util::bit::write_le_uint64(offset, atfw::util::bit::read_le_uint64(offset) + kDirectoryEntryBytes);
  }
  atfw::util::bit::write_le_uint32(data + kExtraDirectoryEntryOffset, kUnknownOptionalSectionType);
  atfw::util::bit::write_le_uint32(data + kExtraDirectoryEntryOffset + kDirectoryFlagsOffset, kSectionOptionalFlag);
  atfw::util::bit::write_le_uint64(data + kExtraDirectoryEntryOffset + kDirectoryDataOffset,
                                   image.size() - kImageFooterBytes);
  resign(image);
  ac loaded;
  CASE_EXPECT_TRUE(loaded.load(bytes(image)) == ac::status::kOk);
  bool found = false;
  loaded.contains("abc", found);
  CASE_EXPECT_TRUE(found);
  auto nonempty = image;
  static constexpr unsigned char kOptionalPayload[] = {0xff, 0, 1, 2, 3, 4, 5, 6};
  size_t payload_offset = nonempty.size() - kImageFooterBytes;
  nonempty.insert(nonempty.end() - kImageFooterBytes, kOptionalPayload, kOptionalPayload + sizeof(kOptionalPayload));
  atfw::util::bit::write_le_uint64(nonempty.data() + kHeaderFileSizeOffset, nonempty.size());
  atfw::util::bit::write_le_uint64(nonempty.data() + kHeaderChecksumOffset, nonempty.size() - kImageFooterBytes);
  atfw::util::bit::write_le_uint64(nonempty.data() + kExtraDirectoryEntryOffset + kDirectoryDataOffset, payload_offset);
  atfw::util::bit::write_le_uint64(nonempty.data() + kExtraDirectoryEntryOffset + kDirectorySizeOffset,
                                   sizeof(kOptionalPayload));
  atfw::util::bit::write_le_uint64(nonempty.data() + kExtraDirectoryEntryOffset + kDirectoryCountOffset,
                                   UINT64_MAX);  // An unknown section's count is opaque, not an allocation request.
  resign(nonempty);
  CASE_EXPECT_TRUE(loaded.load(bytes(nonempty)) == ac::status::kOk);
  found = false;
  loaded.contains("abc", found);
  CASE_EXPECT_TRUE(found);
  atfw::util::bit::write_le_uint32(data + kExtraDirectoryEntryOffset + kDirectoryFlagsOffset, 0);
  resign(image);
  CASE_EXPECT_TRUE(loaded.load(bytes(image)) == ac::status::kUnsupportedVersion);
  atfw::util::bit::write_le_uint16(data + kHeaderMajorOffset, kUnsupportedMajorVersion);
  resign(image);
  CASE_EXPECT_TRUE(loaded.load(bytes(image)) == ac::status::kUnsupportedVersion);
}

CASE_TEST(ac_automation_v2, semantic_validation_checks_longest_failure_suffix) {
  ac value;
  if (!build(value, {"abcd", "bcx"})) {
    return;
  }
  std::vector<ac::node_view> nodes;
  value.visit_nodes(collect_node, &nodes);
  uint32_t id = 0;
  for (char c : std::string("abc")) {
    uint32_t next = UINT32_MAX;
    for (uint32_t i = nodes[id].child_begin; i < nodes[id].child_begin + nodes[id].child_count; ++i) {
      if (nodes[i].token == gsl::string_view(&c, 1)) {
        next = i;
      }
    }
    CASE_EXPECT_TRUE(next != UINT32_MAX);
    if (next == UINT32_MAX) {
      return;
    }
    id = next;
  }
  CASE_EXPECT_TRUE(nodes[id].failure != 0);
  std::vector<unsigned char> image;
  value.dump(image);
  atfw::util::bit::write_le_uint32(
      image.data() + section_start(image, kSectionNodes) + id * kNodeStrideBytes + kNodeFailureOffset, 0);
  resign(image);
  ac loaded;
  CASE_EXPECT_TRUE(loaded.load(bytes(image)) == ac::status::kOk);
  ac::load_options options;
  options.verify_semantics = true;
  CASE_EXPECT_TRUE(loaded.load(bytes(image), options) == ac::status::kInvalidFormat);
}

CASE_TEST(ac_automation_v2, exhaustive_small_alphabet_oracle) {
  const std::vector<std::string> words = {"a", "ab", "bab", "baa", "bb", "aba"};
  ac value;
  for (const auto& w : words) {
    value.insert_keyword(w);
  }
  value.build();
  for (unsigned mask = 0; mask < 512; ++mask) {
    std::string text;
    for (unsigned i = 0; i < 9; ++i) {
      text += (mask & (1u << i)) != 0 ? 'a' : 'b';
    }
    for (bool overlap : {false, true}) {
      std::vector<std::pair<size_t, uint32_t>> expected;
      size_t boundary = 0;
      for (size_t end = 1; end <= text.size(); ++end) {
        std::vector<uint32_t> at_end;
        for (uint32_t id = 0; id < words.size(); ++id) {
          if (words[id].size() <= end - boundary &&
              text.compare(end - words[id].size(), words[id].size(), words[id]) == 0) {
            at_end.push_back(id);
          }
        }
        std::sort(at_end.begin(), at_end.end(),
                  [&](uint32_t a, uint32_t b) { return words[a].size() > words[b].size(); });
        for (auto id : at_end) {
          expected.emplace_back(end - words[id].size(), id);
          if (!overlap) {
            boundary = end;
            break;
          }
        }
      }
      ac::match_options options;
      options.allow_overlap = overlap;
      std::vector<ac::match_t> hits;
      value.match(text, hits, options);
      CASE_EXPECT_EQ(expected.size(), hits.size());
      if (expected.size() != hits.size()) {
        return;
      }
      for (size_t i = 0; i < hits.size(); ++i) {
        CASE_EXPECT_EQ(expected[i].first, hits[i].start);
        CASE_EXPECT_EQ(expected[i].second, hits[i].keyword_id);
      }
    }
  }
}

CASE_TEST(ac_automation_v2, configuration_rejection_is_atomic_and_ids_remain_stable) {
  static constexpr uint32_t kUnchangedId = 0xfedcba98;
  ac value;
  CASE_EXPECT_TRUE(value.set_skip("-") == ac::status::kOk);
  const gsl::string_view invalid_skips[] = {"!", gsl::string_view()};
  CASE_EXPECT_TRUE(value.set_skip(gsl::span<const gsl::string_view>(invalid_skips, 2)) == ac::status::kInvalidArgument);
  CASE_EXPECT_TRUE(value.unset_skip(gsl::string_view()) == ac::status::kInvalidArgument);
  CASE_EXPECT_TRUE(value.unset_skip("absent") == ac::status::kOk);
  CASE_EXPECT_TRUE(value.set_skip("/") == ac::status::kOk);
  CASE_EXPECT_TRUE(value.unset_skip("/") == ac::status::kOk);
  uint32_t id = kUnchangedId;
  CASE_EXPECT_TRUE(value.insert_keyword(gsl::string_view(), &id) == ac::status::kEmptyKeyword);
  CASE_EXPECT_TRUE(value.insert_keyword("---", &id) == ac::status::kEmptyKeyword);
  CASE_EXPECT_EQ(kUnchangedId, id);
  CASE_EXPECT_TRUE(value.set_nocase(true) == ac::status::kOk);
  CASE_EXPECT_TRUE(value.insert_keyword("A-B", &id) == ac::status::kOk);
  CASE_EXPECT_EQ(0, id);
  CASE_EXPECT_TRUE(value.insert_keyword("ab", &id) == ac::status::kAlreadyExists);
  CASE_EXPECT_EQ(0, id);
  CASE_EXPECT_TRUE(value.insert_keyword("!", &id) == ac::status::kOk);
  CASE_EXPECT_EQ(1, id);  // The failed bulk skip request must not have installed '!'.
  CASE_EXPECT_TRUE(value.insert_keyword("/", &id) == ac::status::kOk);
  CASE_EXPECT_EQ(2, id);
  CASE_EXPECT_TRUE(value.unset_skip("-") == ac::status::kInvalidState);
  CASE_EXPECT_TRUE(value.set_nocase(false) == ac::status::kInvalidState);
  CASE_EXPECT_TRUE(value.set_case_folding(gsl::span<const ac::case_fold_rule>()) == ac::status::kInvalidState);
  CASE_EXPECT_TRUE(value.build() == ac::status::kOk);
  CASE_EXPECT_TRUE(value.build() == ac::status::kOk);
  id = kUnchangedId;
  CASE_EXPECT_TRUE(value.insert_keyword("new", &id) == ac::status::kInvalidState);
  CASE_EXPECT_EQ(kUnchangedId, id);
  CASE_EXPECT_TRUE(value.keyword(0) == "A-B");
  CASE_EXPECT_TRUE(value.keyword(UINT32_MAX).empty());
  std::vector<ac::match_t> hits;
  auto report = value.match("--a--b-!/-", hits);
  CASE_EXPECT_TRUE(report.code == ac::status::kOk);
  const std::vector<ac::match_t> expected = {{2, 4, 0, "A-B"}, {7, 1, 1, "!"}, {8, 1, 2, "/"}};
  expect_hits(expected, hits, 10);
}

CASE_TEST(ac_automation_v2, fold_conflicts_preserve_previous_rules_and_skip_after_folding) {
  ac value;
  const ac::case_fold_rule previous[] = {{"X", "!"}};
  CASE_EXPECT_TRUE(value.set_case_folding(gsl::span<const ac::case_fold_rule>(previous, 1)) ==
                   ac::status::kInvalidArgument);  // Conflicts with the built-in ASCII X -> x rule.
  const ac::case_fold_rule rules[] = {{"\xc3\x84", "\xc3\xa4"}, {"@", "-"}};
  CASE_EXPECT_TRUE(value.set_case_folding(gsl::span<const ac::case_fold_rule>(rules, 2)) == ac::status::kOk);
  const ac::case_fold_rule conflicting[] = {{"@", "-"}, {"@", "!"}};
  CASE_EXPECT_TRUE(value.set_case_folding(gsl::span<const ac::case_fold_rule>(conflicting, 2)) ==
                   ac::status::kInvalidArgument);
  const ac::case_fold_rule empty_target[] = {{"@", gsl::string_view()}};
  CASE_EXPECT_TRUE(value.set_case_folding(gsl::span<const ac::case_fold_rule>(empty_target, 1)) ==
                   ac::status::kInvalidArgument);
  CASE_EXPECT_TRUE(value.set_skip("-") == ac::status::kOk);
  CASE_EXPECT_TRUE(value.set_nocase(true) == ac::status::kOk);
  uint32_t id = UINT32_MAX;
  CASE_EXPECT_TRUE(value.insert_keyword("\xc3\x84-B", &id) == ac::status::kOk);
  CASE_EXPECT_EQ(0, id);
  CASE_EXPECT_TRUE(value.insert_keyword("\xc3\xa4@b", &id) == ac::status::kAlreadyExists);
  CASE_EXPECT_EQ(0, id);
  CASE_EXPECT_TRUE(value.build() == ac::status::kOk);
  std::vector<unsigned char> image;
  if (!dump_image(value, image)) {
    return;
  }
  ac restored;
  CASE_EXPECT_TRUE(restored.load(bytes(image)) == ac::status::kOk);
  std::vector<ac::match_t> hits;
  auto report = restored.match("@\xc3\xa4@@B@", hits);
  CASE_EXPECT_TRUE(report.code == ac::status::kOk);
  const std::vector<ac::match_t> expected = {{1, 5, 0, "\xc3\x84-B"}};
  expect_hits(expected, hits, 7);
}

CASE_TEST(ac_automation_v2, invalid_options_do_not_consume_input_or_modify_output) {
  static constexpr int kUnknownEnumValue = 127;
  ac value;
  if (!build(value, {"a"})) {
    return;
  }
  const ac::match_t sentinel{9, 8, 7, "kept"};
  std::vector<ac::match_t> hits(1, sentinel);
  ac::match_options invalid[] = {ac::match_options(), ac::match_options(), ac::match_options()};
  invalid[0].tokens = ac::tokenizer(ac::token_encoding::kCustom);
  invalid[1].tokens.encoding = static_cast<ac::token_encoding>(kUnknownEnumValue);
  invalid[2].invalid_input = static_cast<ac::invalid_input_policy>(kUnknownEnumValue);
  ac::match_context context;
  for (const auto& options : invalid) {
    auto report = value.match("a", hits, options, &context);
    CASE_EXPECT_TRUE(report.code == ac::status::kInvalidArgument);
    CASE_EXPECT_TRUE(report.stopped == ac::stop_reason::kError);
    CASE_EXPECT_EQ(0, report.bytes_consumed);
    CASE_EXPECT_EQ(0, report.matches);
    CASE_EXPECT_EQ(1, hits.size());
    if (hits.size() != 1) {
      return;
    }
    CASE_EXPECT_TRUE(hits[0].keyword == sentinel.keyword);
    bool found = true;
    report = value.contains("a", found, options);
    CASE_EXPECT_TRUE(report.code == ac::status::kInvalidArgument);
    CASE_EXPECT_FALSE(found);
  }
  auto report = value.match("a", static_cast<ac::match_callback>(nullptr), nullptr);
  CASE_EXPECT_TRUE(report.code == ac::status::kInvalidArgument);
  CASE_EXPECT_EQ(0, report.bytes_consumed);
  ac collecting;
  bool found = true;
  report = collecting.contains("a", found);
  CASE_EXPECT_TRUE(report.code == ac::status::kInvalidState);
  CASE_EXPECT_FALSE(found);
  std::vector<unsigned char> image = {1, 2, 3};
  CASE_EXPECT_TRUE(collecting.dump(image) == ac::status::kInvalidState);
  CASE_EXPECT_EQ(3, image.size());
  CASE_EXPECT_EQ(1, image.front());
}

CASE_TEST(ac_automation_v2, occurrence_limits_and_all_output_sinks_agree) {
  ac value;
  if (!build(value, {"a", "aa", "aaa"})) {
    return;
  }
  const std::vector<std::string> words = {"a", "aa", "aaa"};
  const std::string text = "aaaa";
  for (bool overlap : {false, true}) {
    const auto all = ascii_oracle(words, text, overlap, false);
    for (size_t maximum : {size_t{0}, size_t{1}, size_t{3}, all.size(), SIZE_MAX}) {
      ac::match_options options;
      options.allow_overlap = overlap;
      options.max_matches = maximum;
      size_t count = maximum == 0 ? all.size() : (std::min)(maximum, all.size());
      std::vector<ac::match_t> expected(all.begin(), all.begin() + count);
      std::vector<ac::match_t> vector_hits;
      auto vector_report = value.match(text, vector_hits, options);
      std::vector<ac::match_t> callback_hits;
      auto callback_report = value.match(text, collect_hit, &callback_hits, options);
      std::vector<ac::match_t> slots(all.size() + 2);
      const ac::match_t sentinel{99, 88, 77, "untouched"};
      slots.front() = sentinel;
      slots.back() = sentinel;
      auto span_report = value.match(text, gsl::span<ac::match_t>(slots.data() + 1, all.size()), options);
      CASE_EXPECT_TRUE(span_report.matches <= all.size());
      if (span_report.matches > all.size()) {
        return;
      }
      std::vector<ac::match_t> span_hits(slots.begin() + 1, slots.begin() + 1 + span_report.matches);
      expect_hits(expected, vector_hits, text.size());
      expect_hits(expected, callback_hits, text.size());
      expect_hits(expected, span_hits, text.size());
      CASE_EXPECT_TRUE(vector_report.code == ac::status::kOk);
      CASE_EXPECT_TRUE(callback_report.code == ac::status::kOk);
      CASE_EXPECT_TRUE(span_report.code == ac::status::kOk);
      CASE_EXPECT_EQ(count, vector_report.matches);
      CASE_EXPECT_EQ(count, callback_report.matches);
      CASE_EXPECT_EQ(count, span_report.matches);
      bool limited = maximum > 0 && maximum <= all.size();
      auto expected_stop = limited ? ac::stop_reason::kMatchLimit : ac::stop_reason::kEndOfInput;
      CASE_EXPECT_TRUE(vector_report.stopped == expected_stop);
      CASE_EXPECT_TRUE(callback_report.stopped == expected_stop);
      CASE_EXPECT_TRUE(span_report.stopped == (limited ? ac::stop_reason::kMatchLimit : ac::stop_reason::kOutputFull));
      size_t consumed = limited ? expected.back().start + expected.back().length : text.size();
      CASE_EXPECT_EQ(consumed, vector_report.bytes_consumed);
      CASE_EXPECT_EQ(consumed, callback_report.bytes_consumed);
      CASE_EXPECT_EQ(consumed, span_report.bytes_consumed);
      CASE_EXPECT_TRUE(slots.front().keyword == sentinel.keyword);
      CASE_EXPECT_EQ(sentinel.start, slots.front().start);
      CASE_EXPECT_TRUE(slots.back().keyword == sentinel.keyword);
      CASE_EXPECT_EQ(sentinel.length, slots.back().length);
    }
  }
}

CASE_TEST(ac_automation_v2, byte_tokens_cover_nuls_high_bytes_and_common_byte_prefixes) {
  static constexpr size_t kByteAlphabetSize = 256;
  static constexpr size_t kPrefixLengths = 64;
  ac value;
  std::vector<std::string> tokens;
  for (size_t i = 0; i < kByteAlphabetSize; ++i) {
    tokens.emplace_back(1, static_cast<char>(i));
  }
  for (size_t length = 1; length <= kPrefixLengths; ++length) {
    tokens.emplace_back(std::string(length, 'a') + std::string(1, '\0') + 'z');
  }
  ac::tokenizer tokenizer(whole);
  for (const auto& token : tokens) {
    CASE_EXPECT_TRUE(value.insert_keyword(token, nullptr, tokenizer) == ac::status::kOk);
  }
  CASE_EXPECT_TRUE(value.build() == ac::status::kOk);
  std::vector<unsigned char> image;
  if (!dump_image(value, image)) {
    return;
  }
  ac restored;
  CASE_EXPECT_TRUE(restored.load(bytes(image)) == ac::status::kOk);
  ac::match_options options;
  options.tokens = tokenizer;
  for (uint32_t id = 0; id < tokens.size(); ++id) {
    std::vector<ac::match_t> hits;
    auto report = restored.match(tokens[id], hits, options);
    CASE_EXPECT_TRUE(report.code == ac::status::kOk);
    const std::vector<ac::match_t> expected = {{0, tokens[id].size(), id, tokens[id]}};
    if (!expect_hits(expected, hits, tokens[id].size())) {
      return;
    }
  }
  std::vector<ac::match_t> hits;
  auto report = restored.match("aa", hits, options);  // A byte-prefix is not a complete opaque token.
  CASE_EXPECT_TRUE(report.code == ac::status::kOk);
  CASE_EXPECT_TRUE(hits.empty());
  report = restored.match(gsl::string_view(), hits, options);
  CASE_EXPECT_TRUE(report.code == ac::status::kOk);
  CASE_EXPECT_EQ(0, report.bytes_consumed);
}

CASE_TEST(ac_automation_v2, origin_ring_wraps_across_skips_unknowns_and_invalid_units) {
  const std::vector<std::string> words = {"abcab", "bcab", "cab", "ab", "bca"};
  std::string text;
  static constexpr size_t kRepetitions = 80;
  for (size_t i = 0; i < kRepetitions; ++i) {
    text += "--A-b-C-a-B-c-a-B--";
  }
  text += "abc!ab";
  text += '\xff';
  text += "cab--ABCAB";
  check_ascii_oracle(words, text, true);
}

CASE_TEST(ac_automation_v2, deterministic_dictionaries_match_independent_oracle) {
  static constexpr uint32_t kSeed = 0x6a09e667;
  static constexpr size_t kDictionaryCount = 96;
  static constexpr size_t kWordsPerDictionary = 12;
  static constexpr size_t kMaxWordLength = 9;
  static constexpr size_t kTextLength = 160;
  static constexpr char kInputAlphabet[] = "abcABC-!";
  std::mt19937 random(kSeed);
  for (size_t trial = 0; trial < kDictionaryCount; ++trial) {
    std::vector<std::string> words;
    while (words.size() < kWordsPerDictionary) {
      std::string word(1 + random() % kMaxWordLength, 'a');
      for (char& c : word) {
        c = static_cast<char>('a' + random() % 3);
      }
      if (std::find(words.begin(), words.end(), word) == words.end()) {
        words.push_back(word);
      }
    }
    std::string text;
    for (size_t i = 0; i < kTextLength; ++i) {
      text += kInputAlphabet[random() % (sizeof(kInputAlphabet) - 1)];
    }
    text += words.front();  // Every run contains a positive match as well as unrelated text.
    text += '\xff';
    text += words.back();
    if (!check_ascii_oracle(words, text, true)) {
      return;
    }
  }
}

CASE_TEST(ac_automation_v2, repetitive_periodic_and_fibonacci_pattern_families) {
  // Independently constructed families used by the Library Checker AC generators.
  // Source families and pinned upstream revision are recorded in case/ac_automation_v2/README.md.
  static constexpr size_t kSuffixDepth = 128;
  std::vector<std::string> repeated;
  for (size_t length = 1; length <= kSuffixDepth; ++length) {
    repeated.emplace_back(length, 'a');
  }
  if (!check_ascii_oracle(repeated, std::string(3 * kSuffixDepth, 'a'), false)) {
    return;
  }
  std::vector<std::string> periodic = {"abababa", "babab", "ababa", "ba", "bb", "ababababac"};
  if (!check_ascii_oracle(periodic, "ababababababababacabababababbabababa", false)) {
    return;
  }
  std::string first = "a";
  std::string second = "b";
  static constexpr size_t kFibonacciSteps = 12;
  for (size_t i = 0; i < kFibonacciSteps; ++i) {
    std::string next = second + first;
    first = second;
    second = std::move(next);
  }
  const std::vector<std::string> words = {"aba", "bab", "aab", "baa", "babaab", "abaaba", "bbbb"};
  check_ascii_oracle(words, second, false);
}

CASE_TEST(ac_automation_v2, callback_exceptions_leave_model_and_context_reusable) {
  ac value;
  if (!build(value, {"ab"})) {
    return;
  }
  ac::match_context context;
#if defined(ATFRAMEWORK_UTILS_ENABLE_EXCEPTION) && ATFRAMEWORK_UTILS_ENABLE_EXCEPTION
  bool threw = false;
  try {
    value.match("abab", throw_on_hit, nullptr, ac::match_options(), &context);
  } catch (const std::runtime_error&) {
    threw = true;
  }
  CASE_EXPECT_TRUE(threw);
  ac::match_options options;
  options.tokens = ac::tokenizer(throw_on_token);
  threw = false;
  try {
    std::vector<ac::match_t> discarded;
    value.match("ab!ab", discarded, options, &context);
  } catch (const std::runtime_error&) {
    threw = true;
  }
  CASE_EXPECT_TRUE(threw);
#endif
  ac::match_context moved(std::move(context));
  ac::match_context assigned;
  assigned = std::move(moved);
  std::vector<ac::match_t> hits;
  auto report = value.match("abab", hits, ac::match_options(), &assigned);
  CASE_EXPECT_TRUE(report.code == ac::status::kOk);
  const std::vector<ac::match_t> expected = {{0, 2, 0, "ab"}, {2, 2, 0, "ab"}};
  expect_hits(expected, hits, 4);
  hits.clear();
  report = value.match("ab", hits, ac::match_options(), &context);
  CASE_EXPECT_TRUE(report.code == ac::status::kOk);
  CASE_EXPECT_EQ(1, hits.size());
}

CASE_TEST(ac_automation_v2, exact_allocations_reject_every_truncated_image) {
  ac value;
  if (!build(value, {"ab", "bc", "abcd"})) {
    return;
  }
  std::vector<unsigned char> image;
  if (!dump_image(value, image)) {
    return;
  }
  ac loaded = value;
  const char* original = loaded.keyword(0).data();
  for (size_t length = 0; length < image.size(); ++length) {
    std::unique_ptr<unsigned char[]> exact(new unsigned char[length]);
    if (length > 0) {
      std::memcpy(exact.get(), image.data(), length);
    }
    auto code = loaded.load(gsl::span<const unsigned char>(exact.get(), length));
    CASE_EXPECT_TRUE(code != ac::status::kOk);
    CASE_EXPECT_TRUE(loaded.keyword(0).data() == original);
    std::string truncated(reinterpret_cast<const char*>(image.data()), length);
    std::istringstream stream(truncated, std::ios::binary);
    CASE_EXPECT_TRUE(loaded.load(stream) != ac::status::kOk);
    CASE_EXPECT_TRUE(loaded.keyword(0).data() == original);
  }
  bool found = false;
  auto report = loaded.contains("abcd", found);
  CASE_EXPECT_TRUE(report.code == ac::status::kOk);
  CASE_EXPECT_TRUE(found);
}

CASE_TEST(ac_automation_v2, unaligned_images_owned_storage_and_concatenated_streams) {
  ac first;
  ac second;
  if (!build(first, {"first"}) || !build(second, {"second"})) {
    return;
  }
  std::vector<unsigned char> image;
  if (!dump_image(second, image)) {
    return;
  }
  std::vector<unsigned char> unaligned(image.size() + 1);
  std::copy(image.begin(), image.end(), unaligned.begin() + 1);
  ac loaded = first;
  ac retained = loaded;
  auto old_view = retained.keyword(0);
  ac::load_options options;
  options.verify_semantics = true;
  CASE_EXPECT_TRUE(loaded.load(gsl::span<const unsigned char>(unaligned.data() + 1, image.size()), options) ==
                   ac::status::kOk);
  std::fill(unaligned.begin(), unaligned.end(), 0);
  CASE_EXPECT_TRUE(loaded.keyword(0) == "second");
  CASE_EXPECT_TRUE(old_view == "first");
  image.push_back(0);
  CASE_EXPECT_TRUE(loaded.load(bytes(image)) == ac::status::kInvalidFormat);
  CASE_EXPECT_TRUE(loaded.keyword(0) == "second");
  std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
  CASE_EXPECT_TRUE(first.dump(stream) == ac::status::kOk);
  CASE_EXPECT_TRUE(second.dump(stream) == ac::status::kOk);
  stream << '!';
  std::string serialized = stream.str();
  nonseeking_input_buffer buffer(serialized);
  std::istream input(&buffer);
  CASE_EXPECT_TRUE(loaded.load(input, options) == ac::status::kOk);
  CASE_EXPECT_TRUE(loaded.keyword(0) == "first");
  CASE_EXPECT_TRUE(loaded.load(input, options) == ac::status::kOk);
  CASE_EXPECT_TRUE(loaded.keyword(0) == "second");
  CASE_EXPECT_EQ('!', input.get());
}

CASE_TEST(ac_automation_v2, stream_write_failures_and_exceptions_are_reported) {
  ac value;
  if (!build(value, {"abcd"})) {
    return;
  }
  std::vector<unsigned char> image;
  if (!dump_image(value, image)) {
    return;
  }
  for (size_t length : {size_t{0}, kImageHeaderBytes - 1, kImageHeaderBytes, image.size() - 1, image.size()}) {
    limited_output_buffer buffer(length);
    std::ostream stream(&buffer);
    auto code = value.dump(stream);
    CASE_EXPECT_TRUE(code == (length == image.size() ? ac::status::kOk : ac::status::kIoError));
  }
#if defined(ATFRAMEWORK_UTILS_ENABLE_EXCEPTION) && ATFRAMEWORK_UTILS_ENABLE_EXCEPTION
  limited_output_buffer buffer(0);
  std::ostream failing(&buffer);
  failing.exceptions(std::ios::badbit | std::ios::failbit);
  bool threw = false;
  try {
    value.dump(failing);
  } catch (const std::ios_base::failure&) {
    threw = true;
  }
  CASE_EXPECT_TRUE(threw);
  std::istringstream short_input("AC", std::ios::binary);
  short_input.exceptions(std::ios::badbit | std::ios::failbit);
  const char* old_keyword = value.keyword(0).data();
  threw = false;
  try {
    value.load(short_input);
  } catch (const std::ios_base::failure&) {
    threw = true;
  }
  CASE_EXPECT_TRUE(threw);
  CASE_EXPECT_TRUE(value.keyword(0).data() == old_keyword);
#endif
}

CASE_TEST(ac_automation_v2, load_file_budget_checks_exact_boundary_before_allocation) {
  ac value;
  if (!build(value, {"ab"})) {
    return;
  }
  std::vector<unsigned char> image;
  if (!dump_image(value, image)) {
    return;
  }
  ac loaded = value;
  const char* old_keyword = loaded.keyword(0).data();
  for (uint64_t limit : {uint64_t{0}, uint64_t{image.size() - 1}, uint64_t{image.size()}, UINT64_MAX}) {
    ac::load_options options;
    options.max_file_bytes = limit;
    options.verify_semantics = true;
    auto code = loaded.load(bytes(image), options);
    CASE_EXPECT_TRUE(code == (limit < image.size() ? ac::status::kResourceLimit : ac::status::kOk));
    if (limit < image.size()) {
      CASE_EXPECT_TRUE(loaded.keyword(0).data() == old_keyword);
    }
    std::istringstream stream(std::string(reinterpret_cast<const char*>(image.data()), image.size()), std::ios::binary);
    code = loaded.load(stream, options);
    CASE_EXPECT_TRUE(code == (limit < image.size() ? ac::status::kResourceLimit : ac::status::kOk));
  }
  ac::load_options options;
  options.max_memory_bytes = 0;
  old_keyword = loaded.keyword(0).data();
  CASE_EXPECT_TRUE(loaded.load(bytes(image), options) == ac::status::kResourceLimit);
  CASE_EXPECT_TRUE(loaded.keyword(0).data() == old_keyword);
}

CASE_TEST(ac_automation_v2, rehashed_integer_limits_cannot_wrap_image_ranges_or_counts) {
  ac value;
  if (!build(value, {"ab", "bc", "bcd"})) {
    return;
  }
  std::vector<unsigned char> good;
  if (!dump_image(value, good)) {
    return;
  }
  ac loaded = value;
  const char* old_keyword = loaded.keyword(0).data();
  // Real small buffers carry forged metadata; no view advertises unallocated memory.
  const size_t fields64[] = {kImageHeaderBytes + kDirectoryDataOffset, kImageHeaderBytes + kDirectorySizeOffset,
                             kImageHeaderBytes + kDirectoryCountOffset, kHeaderChecksumOffset};
  for (size_t field : fields64) {
    for (uint64_t number : {UINT64_MAX, UINT64_MAX - 7, uint64_t{UINT32_MAX} + 1}) {
      auto image = good;
      atfw::util::bit::write_le_uint64(image.data() + field, number);
      resign(image);
      CASE_EXPECT_TRUE(loaded.load(bytes(image)) == ac::status::kInvalidFormat);
      CASE_EXPECT_TRUE(loaded.keyword(0).data() == old_keyword);
    }
  }
  const size_t fields32[] = {kHeaderSectionCountOffset, kHeaderNodeCountOffset, kHeaderKeywordCountOffset,
                             kHeaderSymbolCountOffset};
  for (size_t field : fields32) {
    auto image = good;
    atfw::util::bit::write_le_uint32(image.data() + field, UINT32_MAX);
    resign(image);
    CASE_EXPECT_TRUE(loaded.load(bytes(image)) == ac::status::kInvalidFormat);
    CASE_EXPECT_TRUE(loaded.keyword(0).data() == old_keyword);
  }
  for (size_t field : {kHeaderNodeCountOffset, kHeaderKeywordCountOffset, kHeaderSymbolCountOffset}) {
    auto image = good;
    atfw::util::bit::write_le_uint32(image.data() + field, UINT32_MAX - 1);
    resign(image);
    ac::load_options options;
    options.max_memory_bytes = UINT64_MAX;  // Reach 64-bit count/stride validation without allocating huge arrays.
    CASE_EXPECT_TRUE(loaded.load(bytes(image), options) == ac::status::kInvalidFormat);
    CASE_EXPECT_TRUE(loaded.keyword(0).data() == old_keyword);
  }
  auto image = good;
  static constexpr uint64_t kLargestAlignedFileBytes = UINT64_MAX - 7;
  atfw::util::bit::write_le_uint64(image.data() + kHeaderFileSizeOffset, kLargestAlignedFileBytes);
  atfw::util::bit::write_le_uint64(image.data() + kHeaderChecksumOffset, kLargestAlignedFileBytes - kImageFooterBytes);
  resign(image);
  CASE_EXPECT_TRUE(loaded.load(bytes(image)) == ac::status::kResourceLimit);
  ac::load_options options;
  options.max_file_bytes = UINT64_MAX;
  CASE_EXPECT_TRUE(loaded.load(bytes(image), options) == ac::status::kInvalidFormat);
  CASE_EXPECT_TRUE(loaded.keyword(0).data() == old_keyword);
}

CASE_TEST(ac_automation_v2, rehashed_overlapping_sections_bad_skip_bits_and_fold_cycles) {
  ac value;
  CASE_EXPECT_TRUE(value.set_nocase(true) == ac::status::kOk);
  CASE_EXPECT_TRUE(value.set_skip("-") == ac::status::kOk);
  if (!build(value, {"ab", "bc"})) {
    return;
  }
  std::vector<unsigned char> good;
  if (!dump_image(value, good)) {
    return;
  }
  ac loaded = value;
  const char* before = loaded.keyword(0).data();
  auto image = good;
  atfw::util::bit::write_le_uint64(image.data() + kImageHeaderBytes + kDirectoryEntryBytes + kDirectoryDataOffset,
                                   section_start(good, kSectionNodes));
  resign(image);
  CASE_EXPECT_TRUE(loaded.load(bytes(image)) == ac::status::kInvalidFormat);
  image = good;
  atfw::util::bit::write_le_uint32(image.data() + kImageHeaderBytes + kDirectoryEntryBytes, kSectionNodes);
  resign(image);
  CASE_EXPECT_TRUE(loaded.load(bytes(image)) == ac::status::kInvalidFormat);
  image = good;
  // The alphabet includes ASCII folding aliases, so locate the final partial skip byte from the directory.
  size_t skip_directory = kImageHeaderBytes + (kSectionSkipBits - kSectionNodes) * kDirectoryEntryBytes;
  size_t skip_bytes =
      static_cast<size_t>(atfw::util::bit::read_le_uint64(image.data() + skip_directory + kDirectorySizeOffset));
  CASE_EXPECT_TRUE(skip_bytes > 0);
  CASE_EXPECT_TRUE(value.get_statistics().symbols % 8 != 0);
  if (skip_bytes == 0 || value.get_statistics().symbols % 8 == 0) {
    return;
  }
  image[section_start(image, kSectionSkipBits) + skip_bytes - 1] |= 0x80;
  resign(image);
  CASE_EXPECT_TRUE(loaded.load(bytes(image)) == ac::status::kInvalidFormat);
  image = good;
  const size_t folds = section_start(image, kSectionFolds);
  atfw::util::bit::write_le_uint32(image.data() + folds, 2);
  atfw::util::bit::write_le_uint32(image.data() + folds + kIndexBytes, 1);
  resign(image);
  CASE_EXPECT_TRUE(loaded.load(bytes(image)) == ac::status::kInvalidFormat);
  CASE_EXPECT_TRUE(loaded.keyword(0).data() == before);
}

CASE_TEST(ac_automation_v2, all_two_byte_inputs_respect_utf8_and_utf16_boundaries) {
  static constexpr uint32_t kTwoByteCombinations = 0x10000;
  static constexpr size_t kInputBytes = 2;
  std::unique_ptr<char[]> input(new char[kInputBytes]);
  for (uint32_t number = 0; number < kTwoByteCombinations; ++number) {
    auto first = static_cast<unsigned char>(number >> 8);
    auto second = static_cast<unsigned char>(number);
    input[0] = static_cast<char>(first);
    input[1] = static_cast<char>(second);
    gsl::string_view view(input.get(), kInputBytes);
    auto utf8 = ac::next_utf8(view);
    bool ascii = first < 0x80;
    bool valid_pair = first >= 0xc2 && first <= 0xdf && second >= 0x80 && second <= 0xbf;
    CASE_EXPECT_TRUE(utf8.state == (ascii || valid_pair ? ac::token_status::kValid : ac::token_status::kInvalid));
    CASE_EXPECT_EQ(valid_pair ? 2 : 1, utf8.byte_count);
    auto be = ac::next_utf16be(view);
    auto le = ac::next_utf16le(view);
    bool be_surrogate = first >= 0xd8 && first <= 0xdf;
    bool le_surrogate = second >= 0xd8 && second <= 0xdf;
    CASE_EXPECT_TRUE(be.state == (be_surrogate ? ac::token_status::kInvalid : ac::token_status::kValid));
    CASE_EXPECT_TRUE(le.state == (le_surrogate ? ac::token_status::kInvalid : ac::token_status::kValid));
    CASE_EXPECT_EQ(kInputBytes, be.byte_count);
    CASE_EXPECT_EQ(kInputBytes, le.byte_count);
  }
}

CASE_TEST(ac_automation_v2, utf16_recovery_skips_and_surrogate_hits_keep_byte_offsets) {
  static constexpr size_t kEncodingCount = 2;
  static constexpr size_t kKeywordBytes = 8;
  static constexpr size_t kUnitBytes = 2;
  const std::string words[kEncodingCount] = {std::string("A\0\0\xd8\0\xdc"
                                                         "B\0",
                                                         kKeywordBytes),
                                             std::string("\0A\xd8\0\xdc\0\0B", kKeywordBytes)};
  const std::string skips[kEncodingCount] = {std::string(" \0", kUnitBytes), std::string("\0 ", kUnitBytes)};
  const std::string broken[kEncodingCount] = {std::string("\0\xdc"
                                                          "A\0\0\xd8"
                                                          "B\0",
                                                          kKeywordBytes),
                                              std::string("\xdc\0\0A\xd8\0\0B", kKeywordBytes)};
  const ac::token_encoding encodings[kEncodingCount] = {ac::token_encoding::kUtf16Le, ac::token_encoding::kUtf16Be};
  for (size_t encoding = 0; encoding < kEncodingCount; ++encoding) {
    ac value;
    CASE_EXPECT_TRUE(value.set_skip(skips[encoding]) == ac::status::kOk);
    ac::tokenizer tokenizer(encodings[encoding]);
    CASE_EXPECT_TRUE(value.insert_keyword(words[encoding], nullptr, tokenizer) == ac::status::kOk);
    CASE_EXPECT_TRUE(value.build() == ac::status::kOk);
    std::string text = broken[encoding] + words[encoding].substr(0, kUnitBytes) + skips[encoding] +
                       words[encoding].substr(kUnitBytes) + '!';
    ac::match_options options;
    options.tokens = tokenizer;
    std::vector<ac::match_t> hits;
    auto report = value.match(text, hits, options);
    CASE_EXPECT_TRUE(report.code == ac::status::kOk);
    CASE_EXPECT_EQ(3, report.invalid_units);
    CASE_EXPECT_EQ(0, report.first_invalid_offset);
    CASE_EXPECT_EQ(text.size(), report.bytes_consumed);
    const std::vector<ac::match_t> expected = {{8, 10, 0, words[encoding]}};
    expect_hits(expected, hits, text.size());
    options.invalid_input = ac::invalid_input_policy::kReject;
    report = value.match(text, hits, options);
    CASE_EXPECT_TRUE(report.code == ac::status::kInvalidEncoding);
    CASE_EXPECT_EQ(0, report.matches);
    CASE_EXPECT_EQ(0, report.bytes_consumed);
    expect_hits(expected, hits, text.size());
  }
}

CASE_TEST(ac_automation_v2, strict_errors_keep_callback_deliveries_and_reset_next_scan) {
  ac value;
  if (!build(value, {"ab", "b"})) {
    return;
  }
  const std::string text = std::string("ab") + '\xff' + "ab";
  ac::match_options options;
  options.invalid_input = ac::invalid_input_policy::kReject;
  options.allow_overlap = true;
  std::vector<ac::match_t> hits;
  ac::match_context context;
  auto report = value.match(text, collect_hit, &hits, options, &context);
  CASE_EXPECT_TRUE(report.code == ac::status::kInvalidEncoding);
  CASE_EXPECT_TRUE(report.stopped == ac::stop_reason::kError);
  CASE_EXPECT_EQ(2, report.matches);
  CASE_EXPECT_EQ(2, report.bytes_consumed);
  CASE_EXPECT_EQ(1, report.invalid_units);
  CASE_EXPECT_EQ(2, report.first_invalid_offset);
  const std::vector<ac::match_t> expected = {{0, 2, 0, "ab"}, {1, 1, 1, "b"}};
  expect_hits(expected, hits, text.size());
  bool found = false;
  report = value.contains(text, found, options);
  CASE_EXPECT_TRUE(report.code == ac::status::kOk);
  CASE_EXPECT_TRUE(found);  // contains() deliberately stops before the malformed suffix.
  CASE_EXPECT_EQ(0, report.invalid_units);
  CASE_EXPECT_EQ(SIZE_MAX, report.first_invalid_offset);
  hits.clear();
  report = value.match("ab", hits, options, &context);
  CASE_EXPECT_TRUE(report.code == ac::status::kOk);
  CASE_EXPECT_EQ(0, report.invalid_units);
  CASE_EXPECT_EQ(SIZE_MAX, report.first_invalid_offset);
  expect_hits(expected, hits, 2);
}

CASE_TEST(ac_automation_v2, rehashed_byte_mutations_preserve_safe_queryable_snapshots) {
  static constexpr unsigned char kMutationMask = 0x80;
  static constexpr char kQuery[] = "-AB-abcd-bcd-\xff-abcd";
  ac baseline;
  CASE_EXPECT_TRUE(baseline.set_nocase(true) == ac::status::kOk);
  CASE_EXPECT_TRUE(baseline.set_skip("-") == ac::status::kOk);
  if (!build(baseline, {"ab", "bc", "bcd", "abcd"})) {
    return;
  }
  std::vector<unsigned char> good;
  if (!dump_image(baseline, good)) {
    return;
  }
  ac loaded = baseline;
  ac::load_options options;
  options.verify_semantics = true;
  size_t accepted = 0;
  size_t rejected = 0;
  for (size_t offset = 0; offset < good.size(); ++offset) {
    if ((offset >= kHeaderHashOffset && offset < kHeaderHashOffset + kImageFooterBytes) ||
        offset >= good.size() - kImageFooterBytes) {
      continue;  // resign() replaces these bytes; mutating them would not exercise a changed image.
    }
    auto image = good;
    image[offset] ^= kMutationMask;
    resign(image);  // Exercise structural validation after passing both integrity checks.
    const char* before = loaded.keyword(0).data();
    auto code = loaded.load(bytes(image), options);
    if (code != ac::status::kOk) {
      ++rejected;
      CASE_EXPECT_TRUE(loaded.keyword(0).data() == before);
      continue;
    }
    ++accepted;
    ac::match_options query_options;
    query_options.allow_overlap = true;
    std::vector<ac::match_t> hits;
    auto report = loaded.match(kQuery, hits, query_options);
    CASE_EXPECT_TRUE(report.code == ac::status::kOk);
    CASE_EXPECT_EQ(sizeof(kQuery) - 1, report.bytes_consumed);
    for (const auto& hit : hits) {
      CASE_EXPECT_TRUE(hit.start < sizeof(kQuery) - 1);
      if (hit.start < sizeof(kQuery) - 1) {
        CASE_EXPECT_TRUE(hit.length > 0 && hit.length <= sizeof(kQuery) - 1 - hit.start);
      }
      CASE_EXPECT_TRUE(hit.keyword_id < loaded.get_statistics().keywords);
      CASE_EXPECT_TRUE(hit.keyword == loaded.keyword(hit.keyword_id));
    }
    std::vector<unsigned char> roundtrip;
    if (!dump_image(loaded, roundtrip)) {
      return;
    }
    ac restored;
    CASE_EXPECT_TRUE(restored.load(bytes(roundtrip), options) == ac::status::kOk);
  }
  CASE_EXPECT_TRUE(accepted > 0);
  CASE_EXPECT_TRUE(rejected > 0);
}

CASE_TEST(ac_automation_v2, library_checker_public_examples_preserve_duplicates_and_failure_links) {
  static constexpr char kSampleDirectory[] = "ac_automation_v2/library-checker/in";
  static constexpr const char* kSamples[] = {"example_00.in", "example_01.in", "example_02.in", "example_03.in"};
  std::string directory;
  atfw::util::file_system::dirname(__FILE__, 0, directory);
  directory += atfw::util::file_system::DIRECTORY_SEPARATOR;
  directory += kSampleDirectory;
  directory += atfw::util::file_system::DIRECTORY_SEPARATOR;
  for (const char* sample : kSamples) {
    std::ifstream input((directory + sample).c_str(), std::ios::in | std::ios::binary);
    CASE_EXPECT_TRUE(input.is_open());
    if (!input.is_open()) {
      return;
    }
    size_t count = 0;
    input >> count;
    CASE_EXPECT_TRUE(count > 0);
    if (input.fail() || count == 0) {
      CASE_EXPECT_FALSE(input.fail());
      return;
    }
    std::vector<std::string> unique_words;
    ac value;
    std::string text;
    for (size_t i = 0; i < count; ++i) {
      std::string word;
      input >> word;
      CASE_EXPECT_FALSE(input.fail());
      CASE_EXPECT_FALSE(word.empty());
      if (input.fail() || word.empty()) {
        return;
      }
      auto existing = std::find(unique_words.begin(), unique_words.end(), word);
      uint32_t id = UINT32_MAX;
      auto code = value.insert_keyword(word, &id);
      if (existing == unique_words.end()) {
        CASE_EXPECT_TRUE(code == ac::status::kOk);
        CASE_EXPECT_EQ(unique_words.size(), id);
        unique_words.push_back(word);
      } else {
        CASE_EXPECT_TRUE(code == ac::status::kAlreadyExists);
        CASE_EXPECT_EQ(static_cast<size_t>(existing - unique_words.begin()), id);
      }
      text += word;
    }
    CASE_EXPECT_TRUE(value.build() == ac::status::kOk);
    if (!check_ascii_oracle(unique_words, text + text, false)) {
      return;
    }
    std::vector<ac::node_view> nodes;
    CASE_EXPECT_TRUE(value.visit_nodes(collect_node, &nodes));
    std::vector<std::string> paths(nodes.size());
    for (const auto& node : nodes) {
      CASE_EXPECT_TRUE(node.id < nodes.size());
      CASE_EXPECT_TRUE(node.child_begin <= nodes.size());
      if (node.id >= nodes.size() || node.child_begin > nodes.size() ||
          node.child_count > nodes.size() - node.child_begin) {
        CASE_EXPECT_TRUE(false);
        return;
      }
      for (size_t child = node.child_begin; child < node.child_begin + node.child_count; ++child) {
        paths[child] = paths[node.id] + std::string(nodes[child].token.data(), nodes[child].token.size());
      }
    }
    // The judge defines failure as the longest proper suffix present in the prefix set.
    for (size_t id = 1; id < paths.size(); ++id) {
      size_t expected_failure = 0;
      for (size_t candidate = 1; candidate < paths.size(); ++candidate) {
        if (paths[candidate].size() > paths[expected_failure].size() && paths[candidate].size() < paths[id].size() &&
            paths[id].compare(paths[id].size() - paths[candidate].size(), paths[candidate].size(), paths[candidate]) ==
                0) {
          expected_failure = candidate;
        }
      }
      CASE_EXPECT_EQ(expected_failure, nodes[id].failure);
    }
  }
}

CASE_TEST(ac_automation_v2, late_tokenizer_failures_rollback_insertions_and_vector_additions) {
  static constexpr uint32_t kUnchangedId = 0xabcdef01;
  ac::token_result invalid[] = {{ac::token_status::kValid, 0},
                                {ac::token_status::kValid, SIZE_MAX},
                                {ac::token_status::kInvalid, 0},
                                {ac::token_status::kInvalid, SIZE_MAX},
                                {ac::token_status::kInvalid, 1}};
  ac value;
  if (!build(value, {"ab", "cd"})) {
    return;
  }
  for (auto& result : invalid) {
    bool encoding_error = result.state == ac::token_status::kInvalid && result.byte_count == 1;
    auto expected_code = encoding_error ? ac::status::kInvalidEncoding : ac::status::kInvalidTokenizerResult;
    ac::tokenizer tokenizer(late_invalid_token, &result);
    ac collecting;
    uint32_t id = kUnchangedId;
    CASE_EXPECT_TRUE(collecting.insert_keyword("ab!cd", &id, tokenizer) == expected_code);
    CASE_EXPECT_EQ(kUnchangedId, id);
    CASE_EXPECT_TRUE(collecting.set_nocase(true) == ac::status::kOk);
    CASE_EXPECT_TRUE(collecting.insert_keyword("cd", &id) == ac::status::kOk);
    CASE_EXPECT_EQ(0, id);
    CASE_EXPECT_TRUE(collecting.build() == ac::status::kOk);
    bool found = true;
    collecting.contains("ab", found);
    CASE_EXPECT_FALSE(found);
    ac::match_options options;
    options.tokens = tokenizer;
    options.invalid_input = ac::invalid_input_policy::kReject;
    ac::match_context context;
    const ac::match_t sentinel{9, 8, 7, "kept"};
    std::vector<ac::match_t> hits(1, sentinel);
    auto report = value.match("ab!cd", hits, options, &context);
    CASE_EXPECT_TRUE(report.code == expected_code);
    CASE_EXPECT_EQ(0, report.matches);
    CASE_EXPECT_EQ(2, report.bytes_consumed);
    CASE_EXPECT_EQ(1, hits.size());
    if (hits.size() != 1) {
      return;
    }
    CASE_EXPECT_TRUE(hits[0].keyword == sentinel.keyword);
    hits.clear();
    report = value.match("ab!cd", collect_hit, &hits, options, &context);
    CASE_EXPECT_TRUE(report.code == expected_code);
    CASE_EXPECT_EQ(1, report.matches);
    const std::vector<ac::match_t> expected = {{0, 2, 0, "ab"}};
    expect_hits(expected, hits, 5);
    hits.clear();
    report = value.match("cd", hits, options, &context);
    CASE_EXPECT_TRUE(report.code == ac::status::kOk);
    const std::vector<ac::match_t> next = {{0, 2, 1, "cd"}};
    expect_hits(next, hits, 2);
  }
}
