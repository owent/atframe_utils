// Copyright 2026 atframework
// Explicit opt-in external corpus benchmark; no corpus is bundled or downloaded.
#include "string/ac_automation.h"
#include "string/ac_automation_v2.h"
#include "string/utf8_char_t.h"

#include "algorithm/bit.h"

#include <xxhash.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace {
using ac = atfw::util::string::ac_automation_v2;
using clock_type = std::chrono::steady_clock;

static constexpr int kExitSuccess = 0;
static constexpr int kExitInputFailure = 1;
static constexpr int kExitBuildFailure = 2;
static constexpr int kExitValidationFailure = 3;
static constexpr int kInputArgumentCount = 2;
static constexpr int kInputArgumentIndex = 1;
static constexpr int kOutputDecimalPlaces = 3;
static constexpr size_t kMedianSampleCount = 7;
static constexpr size_t kMedianSampleIndex = kMedianSampleCount / 2;
static constexpr double kBytesPerMiB = 1024.0 * 1024.0;
static constexpr double kMillisecondsPerSecond = 1000.0;
static constexpr char kCsvDelimiter[] = ",";
static constexpr char kLineFeed = '\n';
static constexpr char kUsageText[] =
    "Usage: ac_automation_benchmark UTF8_WORD_LIST\n"
    "       ac_automation_benchmark --graph INPUT | --unicode ORACLE | --fuzz\n"
    "       ac_automation_benchmark --scan IMAGE INPUT\n";
static constexpr char kCarriageReturn = '\r';
static constexpr char kCsvHeader[] = "operation,median_ms,MiB_per_second\n";
static constexpr char kMetricInsert[] = "v2_insert";
static constexpr char kMetricBuild[] = "v2_build";
static constexpr char kNodesField[] = "# nodes=";
static constexpr char kKeywordsField[] = " keywords=";
static constexpr char kSymbolsField[] = " symbols=";
static constexpr char kPayloadBytesField[] = " payload_bytes=";
static constexpr char kUnknownProbeToken[] = "\xf4\x8f\xbf\xbf";
static constexpr char kMetricMatchHits[] = "v2_match_hits";
static constexpr char kMetricMatchUnknown[] = "v2_match_unknown";
static constexpr char kMetricContainsUnknown[] = "v2_contains_unknown";
static constexpr char kMetricDumpMemory[] = "v2_dump_memory";
static constexpr char kMetricLoadMemory[] = "v2_load_memory";
static constexpr char kMetricLoadStream[] = "v2_load_stream";
static constexpr char kLegacyInitializationProbe[] = "";
static constexpr char kMetricLegacyInsert[] = "v1_insert_and_init";
static constexpr char kMetricLegacyMatchHits[] = "v1_match_hits";
static constexpr char kMetricLegacyMatchUnknown[] = "v1_match_unknown";
static constexpr char kDumpBytesField[] = "# dump_bytes=";
static constexpr char kScratchCapacityField[] = " scratch_capacity=";
static constexpr char kChecksumField[] = " checksum=";

static double elapsed(clock_type::time_point begin) {
  return std::chrono::duration<double, std::milli>(clock_type::now() - begin).count();
}

static bool count_hit(const ac::match_t& hit, void* data) {
  *static_cast<uint64_t*>(data) += hit.start + hit.length + hit.keyword_id;
  return true;
}

struct ATFW_UTIL_SYMBOL_LOCAL expected_hit {
  uint32_t id;
  size_t bytes;
  bool found;
};

static bool check_exact_hit(const ac::match_t& hit, void* data) {
  auto& expected = *static_cast<expected_hit*>(data);
  if (hit.keyword_id == expected.id && hit.start == 0 && hit.length == expected.bytes) {
    expected.found = true;
  }
  return !expected.found;
}

static bool verify_words(const ac& value, const std::vector<std::string>& words, const std::vector<uint32_t>& ids) {
  ac::match_options options;
  options.allow_overlap = true;
  ac::match_context context;
  for (size_t i = 0; i < words.size(); ++i) {
    expected_hit expected{ids[i], words[i].size(), false};
    auto report = value.match(words[i], check_exact_hit, &expected, options, &context);
    if (report.code != ac::status::kOk || !expected.found) {
      return false;
    }
  }
  return true;
}

template <class Fn>
static double median(Fn fn) {
  std::vector<double> samples;
  fn();
  for (size_t i = 0; i < kMedianSampleCount; ++i) {
    auto begin = clock_type::now();
    fn();
    samples.push_back(elapsed(begin));
  }
  std::sort(samples.begin(), samples.end());
  return samples[kMedianSampleIndex];
}

static void print(const char* name, double milliseconds, size_t bytes = 0) {
  std::cout << name << kCsvDelimiter << milliseconds << kCsvDelimiter
            << (bytes > 0 ? bytes / (milliseconds * (kBytesPerMiB / kMillisecondsPerSecond)) : 0) << kLineFeed;
}

static constexpr int kModeArgument = 1;
static constexpr int kFileArgument = 2;
static constexpr int kModeOnlyArguments = 2;
static constexpr int kFileArguments = 3;
static constexpr char kGraphMode[] = "--graph";
static constexpr char kUnicodeMode[] = "--unicode";
static constexpr char kFuzzMode[] = "--fuzz";
static constexpr char kScanMode[] = "--scan";
static constexpr int kScanArguments = 4;
static constexpr int kScanInputArgument = 3;
static constexpr uint64_t kMaximumScanBytes = uint64_t{1} << 30;
static constexpr uint64_t kScanSampleBytes = uint64_t{32} << 20;
static constexpr size_t kMaximumScanRepeats = 256;
static constexpr char kMetricMatchFile[] = "v2_match_file";
static constexpr char kMetricContainsFile[] = "v2_contains_file";
static constexpr char kMatchesField[] = " matches=";
static constexpr char kInputBytesField[] = " input_bytes=";
static constexpr char kRepeatsField[] = " repeats=";
static constexpr char kContainsBytesField[] = " contains_bytes=";
static constexpr char kContainsFoundField[] = " contains_found=";
static constexpr char kContainsRepeatsField[] = " contains_repeats=";
static constexpr char kFailed[] = "AC v2 stress validation failed\n";
static constexpr char kSpace = ' ';
static constexpr uint32_t kInvalidId = UINT32_MAX;
static constexpr size_t kMaximumPatterns = 1000000;
static constexpr size_t kMaximumPatternBytes = 1000000;
static constexpr size_t kOracleHeaderBytes = 4;
static constexpr size_t kEncodingField = 0;
static constexpr size_t kInputLengthField = 1;
static constexpr size_t kValidField = 2;
static constexpr size_t kConsumedField = 3;
static constexpr unsigned char kUtf8Encoding = 0;
static constexpr unsigned char kUtf16LeEncoding = 1;
static constexpr unsigned char kUtf16BeEncoding = 2;
static constexpr unsigned char kValidMarker = 1;
static constexpr size_t kMaximumScalarBytes = 4;
static constexpr size_t kHeaderBytes = 128;
static constexpr size_t kHeaderHashOffset = 80;
static constexpr size_t kFooterBytes = 8;
static constexpr uint64_t kHashSeed = 0;
static constexpr uint32_t kRandomSeed = 0x6a09e667;
static constexpr size_t kMutationCount = 20000;
static constexpr size_t kMutationKinds = 3;
static constexpr size_t kByteMutation = 0;
static constexpr size_t kWordMutation = 1;
static constexpr unsigned char kMutationMask = 0x80;
static constexpr size_t kMaximumHits = 16;
static constexpr size_t kGuardStart = 0x1234;
static constexpr size_t kGuardLength = 0x2345;
static constexpr char kGuardKeyword[] = "guard";
static constexpr char kQuery[] = "AB-abcd-bcd-\xff-abcd";
static constexpr char kSkip[] = "-";
static constexpr const char* kKeywords[] = {"ab", "bc", "abcd", "bcd"};
static constexpr uint64_t kBoundaryValues[] = {0, 1, UINT32_MAX, uint64_t{UINT32_MAX} + 1, UINT64_MAX};
static constexpr char kMutationSummary[] = "mutations=";
static constexpr char kAcceptedSummary[] = " accepted=";
static constexpr char kRejectedSummary[] = " rejected=";
static constexpr char kUnicodeSummary[] = "unicode_records=";

static gsl::span<const unsigned char> bytes(const std::vector<unsigned char>& image) {
  return {image.data(), image.size()};
}

static bool collect_node(const ac::node_view& node, void* output) {
  auto& nodes = *static_cast<std::vector<ac::node_view>*>(output);
  if (node.id != nodes.size()) {
    return false;
  }
  nodes.push_back(node);
  return true;
}

static bool graph(const char* filename) {
  // Library Checker inputs contain one lowercase-ASCII pattern per line with a bounded total size.
  std::ifstream input(filename, std::ios::binary);
  size_t count = 0;
  input >> count;
  if (input.fail() || count == 0 || count > kMaximumPatterns) {
    return false;
  }
  ac value;
  std::vector<std::string> words(count);
  std::vector<uint32_t> keyword_ids(count);
  size_t total_bytes = 0;
  for (size_t i = 0; i < count; ++i) {
    input.width(static_cast<std::streamsize>(kMaximumPatternBytes + 1));
    input >> words[i];
    if (input.fail() || words[i].empty() || words[i].size() > kMaximumPatternBytes - total_bytes) {
      return false;
    }
    total_bytes += words[i].size();
    auto code = value.insert_keyword(words[i], &keyword_ids[i]);
    if (code != ac::status::kOk && code != ac::status::kAlreadyExists) {
      return false;
    }
  }
  if (value.build() != ac::status::kOk) {
    return false;
  }
  // Every generated trie also exercises serialization and semantic failure-link validation.
  std::vector<unsigned char> image;
  if (value.dump(image) != ac::status::kOk) {
    return false;
  }
  ac restored;
  ac::load_options options;
  options.verify_semantics = true;
  if (restored.load(bytes(image), options) != ac::status::kOk) {
    return false;
  }
  std::vector<ac::node_view> nodes;
  std::vector<ac::node_view> again;
  if (!value.visit_nodes(collect_node, &nodes) || !restored.visit_nodes(collect_node, &again) || nodes.empty() ||
      nodes.size() != again.size()) {
    return false;
  }
  // Compare complete public node views before translating IDs to the judge's insertion order.
  std::vector<uint32_t> parents(nodes.size(), 0);
  for (size_t i = 0; i < nodes.size(); ++i) {
    const auto& node = nodes[i];
    const auto& other = again[i];
    if (node.id != i || node.failure >= nodes.size() || node.child_begin > nodes.size() ||
        node.child_count > nodes.size() - node.child_begin || node.id != other.id || node.failure != other.failure ||
        node.child_begin != other.child_begin || node.child_count != other.child_count ||
        node.output_head != other.output_head || node.token != other.token || (i > 0 && node.token.size() != 1)) {
      return false;
    }
    for (size_t child = node.child_begin; child < node.child_begin + node.child_count; ++child) {
      if (child <= i) {
        return false;
      }
      parents[child] = node.id;
    }
  }
  // Translate public BFS IDs into the judge's first-insertion prefix IDs.
  std::vector<uint32_t> judge_id(nodes.size(), kInvalidId);
  std::vector<uint32_t> by_judge_id(1, 0);
  std::vector<uint32_t> terminals;
  judge_id[0] = 0;
  for (const auto& word : words) {
    uint32_t state = 0;
    for (char symbol : word) {
      const auto& node = nodes[state];
      auto first = nodes.begin() + node.child_begin;
      auto last = first + node.child_count;
      auto found = std::lower_bound(first, last, symbol,
                                    [](const ac::node_view& item, char token) { return item.token[0] < token; });
      if (found == last || found->token[0] != symbol) {
        return false;
      }
      state = found->id;
      if (judge_id[state] == kInvalidId) {
        judge_id[state] = static_cast<uint32_t>(by_judge_id.size());
        by_judge_id.push_back(state);
      }
    }
    terminals.push_back(judge_id[state]);
  }
  if (by_judge_id.size() != nodes.size()) {
    return false;
  }
  // Probe exact keyword IDs/ranges so a shorter prefix cannot hide a missing terminal.
  const size_t probes[] = {0, count / 2, count - 1};
  ac::match_options match_options;
  match_options.allow_overlap = true;
  ac::match_context context;
  for (size_t probe : probes) {
    expected_hit wanted{keyword_ids[probe], words[probe].size(), false};
    auto report = restored.match(words[probe], check_exact_hit, &wanted, match_options, &context);
    if (report.code != ac::status::kOk || !wanted.found) {
      return false;
    }
  }
  std::cout << nodes.size() << kLineFeed;
  for (size_t i = 1; i < by_judge_id.size(); ++i) {
    uint32_t id = by_judge_id[i];
    std::cout << judge_id[parents[id]] << kSpace << judge_id[nodes[id].failure] << kLineFeed;
  }
  for (size_t i = 0; i < terminals.size(); ++i) {
    if (i > 0) {
      std::cout << kSpace;
    }
    std::cout << terminals[i];
  }
  std::cout << kLineFeed;
  return std::cout.good();
}

static bool unicode(const char* filename) {
  std::ifstream input(filename, std::ios::binary);
  if (!input.is_open()) {
    return false;
  }
  size_t count = 0;
  for (;;) {
    std::array<unsigned char, kOracleHeaderBytes> header{};
    input.read(reinterpret_cast<char*>(header.data()), static_cast<std::streamsize>(header.size()));
    if (input.gcount() == 0 && input.eof()) {
      break;
    }
    if (input.gcount() != static_cast<std::streamsize>(header.size()) || header[kEncodingField] > kUtf16BeEncoding ||
        header[kInputLengthField] == 0 || header[kInputLengthField] > kMaximumScalarBytes ||
        header[kValidField] > kValidMarker || header[kConsumedField] > header[kInputLengthField]) {
      return false;
    }
    std::array<char, kMaximumScalarBytes> token{};
    input.read(token.data(), header[kInputLengthField]);
    if (input.gcount() != header[kInputLengthField]) {
      return false;
    }
    gsl::string_view text(token.data(), header[kInputLengthField]);
    ac::token_result result = header[kEncodingField] == kUtf8Encoding      ? ac::next_utf8(text)
                              : header[kEncodingField] == kUtf16LeEncoding ? ac::next_utf16le(text)
                                                                           : ac::next_utf16be(text);
    if ((result.state == ac::token_status::kValid) != (header[kValidField] == kValidMarker) ||
        result.byte_count != header[kConsumedField]) {
      std::cerr << kUnicodeSummary << count << kLineFeed;
      return false;
    }
    ++count;
  }
  std::cout << kUnicodeSummary << count << kLineFeed;
  return count > 0;
}

static bool is_guard(const ac::match_t& hit) {
  return hit.start == kGuardStart && hit.length == kGuardLength && hit.keyword_id == kInvalidId &&
         hit.keyword == kGuardKeyword;
}

static bool fuzz() {
  ac baseline;
  if (baseline.set_nocase(true) != ac::status::kOk || baseline.set_skip(kSkip) != ac::status::kOk) {
    return false;
  }
  for (const char* word : kKeywords) {
    if (baseline.insert_keyword(word) != ac::status::kOk) {
      return false;
    }
  }
  if (baseline.build() != ac::status::kOk) {
    return false;
  }
  std::vector<unsigned char> good;
  if (baseline.dump(good) != ac::status::kOk || good.size() < kHeaderBytes + kFooterBytes) {
    return false;
  }
  std::vector<size_t> positions;
  for (size_t i = 0; i < good.size() - kFooterBytes; ++i) {
    if (i < kHeaderHashOffset || i >= kHeaderHashOffset + sizeof(uint64_t)) {
      positions.push_back(i);
    }
  }
  if (positions.empty()) {
    return false;
  }
  std::mt19937 random(kRandomSeed);
  ac::load_options load_options;
  load_options.verify_semantics = true;
  ac::match_options match_options;
  match_options.allow_overlap = true;
  match_options.max_matches = kMaximumHits;
  size_t accepted = 0;
  size_t rejected = 0;
  for (size_t trial = 0; trial < kMutationCount; ++trial) {
    auto image = good;
    size_t position = positions[random() % positions.size()];
    size_t kind = random() % kMutationKinds;
    uint64_t value = kBoundaryValues[random() % (sizeof(kBoundaryValues) / sizeof(kBoundaryValues[0]))];
    if (kind == kByteMutation) {
      image[position] ^= kMutationMask;
    } else if (kind == kWordMutation && image.size() - position >= sizeof(uint32_t)) {
      atfw::util::bit::write_le_uint32(image.data() + position, static_cast<uint32_t>(value));
    } else if (image.size() - position >= sizeof(uint64_t)) {
      atfw::util::bit::write_le_uint64(image.data() + position, value);
    } else {
      image[position] ^= kMutationMask;
    }
    atfw::util::bit::write_le_uint64(image.data() + kHeaderHashOffset, 0);
    atfw::util::bit::write_le_uint64(image.data() + kHeaderHashOffset, XXH64(image.data(), kHeaderBytes, kHashSeed));
    atfw::util::bit::write_le_uint64(image.data() + image.size() - kFooterBytes,
                                     XXH64(image.data(), image.size() - kFooterBytes, kHashSeed));
    ac candidate = baseline;
    const char* before = candidate.keyword(0).data();
    auto code = candidate.load(bytes(image), load_options);
    if (code != ac::status::kOk) {
      ++rejected;
      if (candidate.keyword(0).data() != before) {
        return false;
      }
      continue;
    }
    ++accepted;
    std::array<ac::match_t, kMaximumHits + 2> output{};
    output.fill({kGuardStart, kGuardLength, kInvalidId, kGuardKeyword});
    auto report = candidate.match(kQuery, gsl::span<ac::match_t>(output.data() + 1, kMaximumHits), match_options);
    if (report.code != ac::status::kOk || report.matches > kMaximumHits || report.bytes_consumed > sizeof(kQuery) - 1 ||
        !is_guard(output.front()) || !is_guard(output.back())) {
      return false;
    }
    for (size_t i = 0; i < report.matches; ++i) {
      const auto& hit = output[i + 1];
      if (hit.start >= sizeof(kQuery) - 1 || hit.length == 0 || hit.length > sizeof(kQuery) - 1 - hit.start ||
          hit.keyword_id >= candidate.get_statistics().keywords || hit.keyword != candidate.keyword(hit.keyword_id)) {
        return false;
      }
    }
    std::vector<unsigned char> roundtrip;
    ac restored;
    if (candidate.dump(roundtrip) != ac::status::kOk ||
        restored.load(bytes(roundtrip), load_options) != ac::status::kOk) {
      return false;
    }
  }
  std::cout << kMutationSummary << kMutationCount << kAcceptedSummary << accepted << kRejectedSummary << rejected
            << kLineFeed;
  return accepted > 0 && rejected > 0;
}
// Measure already-built images with identical input and match options. File I/O and the
// first scratch allocation stay outside timing. Repetition targets 32 MiB per sample,
// capped at 256 scans; an input larger than the target is scanned once per sample.
static bool scan_file(const char* image_path, const char* input_path) {
  ac value;
  std::ifstream image(image_path, std::ios::binary);
  if (image.fail() || value.load(image) != ac::status::kOk || image.peek() != std::char_traits<char>::eof()) {
    return false;
  }
  std::ifstream input(input_path, std::ios::binary | std::ios::ate);
  auto length = input.tellg();
  if (length <= 0 || static_cast<uint64_t>(length) > kMaximumScanBytes) {
    return false;
  }
  std::string text(static_cast<size_t>(length), '\0');
  input.seekg(0);
  if (input.read(&text[0], static_cast<std::streamsize>(text.size())).fail()) {
    return false;
  }
  uint64_t checksum = 0;
  ac::match_context context;
  auto expected = value.match(text, count_hit, &checksum, ac::match_options{}, &context);
  if (expected.code != ac::status::kOk || expected.bytes_consumed != text.size()) {
    return false;
  }
  size_t repeats = static_cast<size_t>(
      (std::min)(static_cast<uint64_t>(kMaximumScanRepeats), (kScanSampleBytes + text.size() - 1) / text.size()));
  bool valid = true;
  double duration = median([&] {
    for (size_t i = 0; i < repeats; ++i) {
      auto report = value.match(text, count_hit, &checksum, ac::match_options{}, &context);
      if (report.code != ac::status::kOk || report.bytes_consumed != text.size() ||
          report.matches != expected.matches) {
        valid = false;
      }
    }
  });
  if (!valid) {
    return false;
  }
  bool expected_found = false;
  auto expected_contains = value.contains(text, expected_found);
  if (expected_contains.code != ac::status::kOk || expected_contains.bytes_consumed == 0) {
    return false;
  }
  size_t contains_repeats = static_cast<size_t>(
      (std::min)(static_cast<uint64_t>(kMaximumScanRepeats),
                 (kScanSampleBytes + expected_contains.bytes_consumed - 1) / expected_contains.bytes_consumed));
  double contains_duration = median([&] {
    for (size_t i = 0; i < contains_repeats; ++i) {
      bool found = false;
      auto report = value.contains(text, found);
      if (report.code != ac::status::kOk || report.bytes_consumed != expected_contains.bytes_consumed ||
          found != expected_found) {
        valid = false;
      }
      checksum += report.bytes_consumed;
    }
  });
  if (!valid) {
    return false;
  }
  auto stats = value.get_statistics();
  std::cout << std::fixed << std::setprecision(kOutputDecimalPlaces) << kCsvHeader;
  print(kMetricMatchFile, duration, text.size() * repeats);
  // contains() can stop before the end; report throughput only for bytes actually consumed.
  print(kMetricContainsFile, contains_duration, expected_contains.bytes_consumed * contains_repeats);
  std::cout << kNodesField << stats.nodes << kKeywordsField << stats.keywords << kSymbolsField << stats.symbols
            << kPayloadBytesField << stats.payload_bytes << kInputBytesField << text.size() << kRepeatsField << repeats
            << kMatchesField << expected.matches << kContainsBytesField << expected_contains.bytes_consumed
            << kContainsFoundField << expected_found << kContainsRepeatsField << contains_repeats
            << kScratchCapacityField << context.capacity() << kChecksumField << checksum << kLineFeed;
  return true;
}
}  // namespace

int main(int argc, char* argv[]) {
  // Validation modes share this executable with the original external-word-list benchmark.
  if (argc >= kModeOnlyArguments) {
    gsl::string_view mode(argv[kModeArgument]);
    if (mode == kScanMode) {
      if (argc != kScanArguments) {
        std::cerr << kUsageText;
        return kExitInputFailure;
      }
      bool valid = scan_file(argv[kFileArgument], argv[kScanInputArgument]);
      if (!valid) {
        std::cerr << kFailed;
      }
      return valid ? kExitSuccess : kExitValidationFailure;
    }
    if (mode == kGraphMode || mode == kUnicodeMode || mode == kFuzzMode) {
      bool valid = false;
      if (mode == kFuzzMode && argc == kModeOnlyArguments) {
        valid = fuzz();
      } else if (mode == kGraphMode && argc == kFileArguments) {
        valid = graph(argv[kFileArgument]);
      } else if (mode == kUnicodeMode && argc == kFileArguments) {
        valid = unicode(argv[kFileArgument]);
      } else {
        std::cerr << kUsageText;
        return kExitInputFailure;
      }
      if (!valid) {
        std::cerr << kFailed;
      }
      return valid ? kExitSuccess : kExitValidationFailure;
    }
  }
  if (argc != kInputArgumentCount) {
    std::cerr << kUsageText;
    return kExitInputFailure;
  }
  // This local CLI intentionally reads the operator-selected file with the operator's privileges.
  // Absolute paths and parent components are supported; no path is derived from file contents.
  std::ifstream source(argv[kInputArgumentIndex], std::ios::binary);
  if (source.fail()) {
    return kExitInputFailure;
  }
  std::vector<std::string> words;
  std::string line;
  while (!std::getline(source, line).fail()) {
    if (!line.empty() && line.back() == kCarriageReturn) {
      line.pop_back();
    }
    if (!line.empty()) {
      words.push_back(line);
    }
  }
  if (source.bad() || words.empty()) {
    return kExitInputFailure;
  }
  // Keep construction outside scan timings and verify every original keyword after build/load.
  std::cout << std::fixed << std::setprecision(kOutputDecimalPlaces) << kCsvHeader;
  ac value;
  std::vector<uint32_t> ids;
  ids.reserve(words.size());
  auto begin = clock_type::now();
  for (const auto& word : words) {
    uint32_t id = UINT32_MAX;
    auto code = value.insert_keyword(word, &id);
    if (code != ac::status::kOk && code != ac::status::kAlreadyExists) {
      return kExitBuildFailure;
    }
    ids.push_back(id);
  }
  print(kMetricInsert, elapsed(begin));
  begin = clock_type::now();
  if (value.build() != ac::status::kOk) {
    return kExitBuildFailure;
  }
  print(kMetricBuild, elapsed(begin));
  auto stats = value.get_statistics();
  std::cout << kNodesField << stats.nodes << kKeywordsField << stats.keywords << kSymbolsField << stats.symbols
            << kPayloadBytesField << stats.payload_bytes << kLineFeed;
  // The two probes separate keyword-heavy input from unknown-symbol lookup; neither models traffic.
  std::string hits;
  for (const auto& word : words) {
    hits += word;
    hits += kLineFeed;
  }
  std::string nohits;
  while (nohits.size() < hits.size()) {
    nohits += kUnknownProbeToken;
  }
  uint64_t checksum = 0;
  ac::match_context context;
  if (!verify_words(value, words, ids)) {
    return kExitValidationFailure;
  }
  print(kMetricMatchHits, median([&] { value.match(hits, count_hit, &checksum, ac::match_options(), &context); }),
        hits.size());
  print(kMetricMatchUnknown, median([&] { value.match(nohits, count_hit, &checksum, ac::match_options(), &context); }),
        nohits.size());
  bool found = true;
  print(kMetricContainsUnknown, median([&] {
          auto report = value.contains(nohits, found);
          checksum += report.bytes_consumed;
        }),
        nohits.size());
  if (found) {
    return kExitValidationFailure;
  }
  std::vector<unsigned char> image;
  value.dump(image);
  print(kMetricDumpMemory, median([&] {
          if (value.dump(image) != ac::status::kOk) {
            std::abort();
          }
        }),
        image.size());
  print(kMetricLoadMemory, median([&] {
          ac loaded;
          if (loaded.load(gsl::span<const unsigned char>(image.data(), image.size())) != ac::status::kOk) {
            std::abort();
          }
          checksum += loaded.get_statistics().nodes;
        }),
        image.size());
  std::ostringstream output(std::ios::binary);
  value.dump(output);
  std::string raw = output.str();
  print(kMetricLoadStream, median([&] {
          std::istringstream input(raw, std::ios::binary);
          ac loaded;
          if (loaded.load(input) != ac::status::kOk) {
            std::abort();
          }
          checksum += loaded.get_statistics().nodes;
        }),
        raw.size());
  ac loaded;
  if (loaded.load(gsl::span<const unsigned char>(image.data(), image.size())) != ac::status::kOk) {
    return kExitValidationFailure;
  }
  if (!verify_words(loaded, words, ids)) {
    return kExitValidationFailure;
  }
  // v1 initializes failure links lazily; trigger initialization before timing its scans.
  atfw::util::string::ac_automation<atfw::util::string::utf8_char_t> legacy;
  begin = clock_type::now();
  for (const auto& word : words) {
    legacy.insert_keyword(word);
  }
  legacy.match(kLegacyInitializationProbe);
  print(kMetricLegacyInsert, elapsed(begin));
  print(kMetricLegacyMatchHits, median([&] {
          auto matches = legacy.match(hits);
          checksum += matches.size();
        }),
        hits.size());
  print(kMetricLegacyMatchUnknown, median([&] {
          auto matches = legacy.match(nohits);
          checksum += matches.size();
        }),
        nohits.size());
  std::cout << kDumpBytesField << image.size() << kScratchCapacityField << context.capacity() << kChecksumField
            << checksum << kLineFeed;
  return kExitSuccess;
}
