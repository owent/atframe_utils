// Copyright 2026 atframework
#include "string/ac_automation_v2.h"

#include <algorithm/bit.h>
#include <xxhash.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <istream>
#include <limits>
#include <map>
#include <ostream>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN

namespace string {
namespace {
using ac = ac_automation_v2;
using status = ac::status;
static constexpr uint32_t kInvalidId = UINT32_MAX;
static constexpr uint32_t kRootNodeId = 0;
static constexpr size_t kRootNodeCount = 1;
static constexpr uint32_t kUnknownSymbolId = 0;
static constexpr uint32_t kFirstSymbolId = 1;
static constexpr size_t kBitsPerByte = 8;
static constexpr size_t kByteValueCount = 256;
static constexpr size_t kSymbolBucketCount = kByteValueCount + 1;
static constexpr size_t kSymbolIndexBytes = (kSymbolBucketCount + kByteValueCount) * sizeof(uint32_t);
static constexpr char kAsciiUpperFirst = 'A';
static constexpr char kAsciiUpperLast = 'Z';
static constexpr int kAsciiCaseOffset = 'a' - kAsciiUpperFirst;

// Tokenizer boundaries describe bytes only; none of these values enter frozen configuration.
static constexpr size_t kUtf8SingleByteLength = 1;
static constexpr size_t kUtf8TwoByteLength = 2;
static constexpr size_t kUtf8ThreeByteLength = 3;
static constexpr size_t kUtf8FourByteLength = 4;
static constexpr unsigned char kUtf8SingleByteLimit = 0x80;
static constexpr unsigned char kUtf8TwoByteFirst = 0xc2;
static constexpr unsigned char kUtf8TwoByteLast = 0xdf;
static constexpr unsigned char kUtf8ThreeByteFirst = 0xe0;
static constexpr unsigned char kUtf8ThreeByteLast = 0xef;
static constexpr unsigned char kUtf8FourByteFirst = 0xf0;
static constexpr unsigned char kUtf8FourByteLast = 0xf4;
static constexpr unsigned char kUtf8ContinuationMask = 0xc0;
static constexpr unsigned char kUtf8ContinuationTag = 0x80;
static constexpr unsigned char kUtf8ThreeByteLowerBound = 0xa0;
static constexpr unsigned char kUtf8SurrogateLead = 0xed;
static constexpr unsigned char kUtf8SurrogateLowerBound = 0xa0;
static constexpr unsigned char kUtf8FourByteLowerBound = 0x90;
static constexpr unsigned char kUtf8ScalarUpperBound = 0x90;
static constexpr size_t kUtf16CodeUnitBytes = 2;
static constexpr size_t kUtf16SurrogatePairBytes = 2 * kUtf16CodeUnitBytes;
static constexpr uint16_t kUtf16HighSurrogateFirst = 0xd800;
static constexpr uint16_t kUtf16HighSurrogateLast = 0xdbff;
static constexpr uint16_t kUtf16LowSurrogateFirst = 0xdc00;
static constexpr uint16_t kUtf16LowSurrogateLast = 0xdfff;

// Version 2.0 wire constants; keep the public layout tables in sync when changing the format.
static constexpr char kImageMagic[] = "ACAUTOMATION";
static constexpr size_t kImageMagicBytes = sizeof(kImageMagic) - 1;
static constexpr uint16_t kImageMajorVersion = 2;
static constexpr uint16_t kImageMinorVersion = 0;
static constexpr uint32_t kImageEndianTag = 0x01020304;
static constexpr uint32_t kFeatureNoCase = 1;
static constexpr uint32_t kSectionRequiredFlags = 0;
static constexpr uint32_t kSectionOptionalFlag = 1;
static constexpr uint32_t kChecksumAlgorithmXxh64 = 1;
static constexpr uint64_t kXxh64Seed = 0;
static constexpr uint64_t kDefaultLoadBudgetBytes = uint64_t{1} << 30;
static constexpr size_t kImageAlignmentBytes = 8;
static constexpr size_t kImageFooterBytes = sizeof(uint64_t);
static constexpr size_t kImageHeaderBytes = 128;
static constexpr size_t kDirectoryEntryBytes = 32;
static constexpr size_t kRequiredSectionCount = 8;
static constexpr size_t kMaxSectionCount = 64;
static constexpr size_t kIoBufferBytes = 65536;
static constexpr uint64_t kValidationFixedBytes = 2 * kIoBufferBytes;
static constexpr size_t kSerializedIndexBytes = sizeof(uint32_t);
static constexpr size_t kByteStrideBytes = sizeof(unsigned char);
static constexpr size_t kNodeStrideBytes = 16;
static constexpr size_t kKeywordStrideBytes = 16;

static constexpr size_t kHeaderMajorOffset = 12;
static constexpr size_t kHeaderMinorOffset = 14;
static constexpr size_t kHeaderSizeOffset = 16;
static constexpr size_t kHeaderFlagsOffset = 20;
static constexpr size_t kHeaderEndianOffset = 24;
static constexpr size_t kHeaderSectionCountOffset = 28;
static constexpr size_t kHeaderFileSizeOffset = 32;
static constexpr size_t kHeaderDirectoryOffset = 40;
static constexpr size_t kHeaderNodeCountOffset = 48;
static constexpr size_t kHeaderKeywordCountOffset = 52;
static constexpr size_t kHeaderSymbolCountOffset = 56;
static constexpr size_t kHeaderMaxTokensOffset = 60;
static constexpr size_t kHeaderNodeStrideOffset = 64;
static constexpr size_t kHeaderLabelWidthOffset = 66;
static constexpr size_t kHeaderKeywordStrideOffset = 68;
static constexpr size_t kHeaderSymbolWidthOffset = 70;
static constexpr size_t kHeaderChecksumAlgorithmOffset = 72;
static constexpr size_t kHeaderReservedWordOffset = 76;
static constexpr size_t kHeaderHashOffset = 80;
static constexpr size_t kHeaderChecksumOffset = 88;
static constexpr size_t kHeaderReservedTailOffset = 96;

static constexpr size_t kDirectoryFlagsOffset = 4;
static constexpr size_t kDirectoryDataOffset = 8;
static constexpr size_t kDirectorySizeOffset = 16;
static constexpr size_t kDirectoryCountOffset = 24;
static constexpr size_t kNodeChildCountOffset = 4;
static constexpr size_t kNodeFailureOffset = 8;
static constexpr size_t kNodeOutputOffset = 12;
static constexpr size_t kKeywordLengthOffset = 4;
static constexpr size_t kKeywordTokensOffset = 8;
static constexpr size_t kKeywordSuffixOffset = 12;

static constexpr uint32_t kSectionNodes = 1;
static constexpr uint32_t kSectionLabels = 2;
static constexpr uint32_t kSectionSymbolOffsets = 3;
static constexpr uint32_t kSectionSymbolBytes = 4;
static constexpr uint32_t kSectionSkipBits = 5;
static constexpr uint32_t kSectionKeywords = 6;
static constexpr uint32_t kSectionKeywordBytes = 7;
static constexpr uint32_t kSectionFolds = 8;
static constexpr size_t kSymbolBytesSectionIndex = kSectionSymbolBytes - 1;
static constexpr size_t kSkipBitsSectionIndex = kSectionSkipBits - 1;
static constexpr size_t kKeywordBytesSectionIndex = kSectionKeywordBytes - 1;
static constexpr size_t kFoldsSectionIndex = kSectionFolds - 1;

static constexpr char kStatusOk[] = "ok";
static constexpr char kStatusAlreadyExists[] = "already_exists";
static constexpr char kStatusInvalidState[] = "invalid_state";
static constexpr char kStatusInvalidArgument[] = "invalid_argument";
static constexpr char kStatusEmptyKeyword[] = "empty_keyword";
static constexpr char kStatusInvalidEncoding[] = "invalid_encoding";
static constexpr char kStatusInvalidTokenizerResult[] = "invalid_tokenizer_result";
static constexpr char kStatusCapacityExceeded[] = "capacity_exceeded";
static constexpr char kStatusInvalidFormat[] = "invalid_format";
static constexpr char kStatusUnsupportedVersion[] = "unsupported_version";
static constexpr char kStatusChecksumMismatch[] = "checksum_mismatch";
static constexpr char kStatusIoError[] = "io_error";
static constexpr char kStatusResourceLimit[] = "resource_limit";
static constexpr char kStatusUnknown[] = "unknown_status";

struct ATFW_UTIL_SYMBOL_LOCAL byte_less {
  bool operator()(gsl::string_view a, gsl::string_view b) const noexcept {
    size_t n = (std::min)(a.size(), b.size());
    int c = n > 0 ? std::memcmp(a.data(), b.data(), n) : 0;
    return c < 0 || (c == 0 && a.size() < b.size());
  }
};

struct ATFW_UTIL_SYMBOL_LOCAL string_hash {
  size_t operator()(const std::string& s) const noexcept {
    return static_cast<size_t>(XXH64(s.data(), s.size(), kXxh64Seed));
  }
};

static std::string own(gsl::string_view s) { return s.empty() ? std::string() : std::string(s.data(), s.size()); }

static gsl::string_view view(const std::string& s) { return gsl::string_view(s.data(), s.size()); }

// Native 16-byte row and serialized field offsets: begin@0, count@4, fail@8, output@12.
// Children occupy a sorted contiguous Label interval; each Label index is its target Node ID.
struct ATFW_UTIL_SYMBOL_LOCAL node {
  uint32_t begin = 0;
  uint32_t count = 0;
  uint32_t fail = 0;
  uint32_t output = kInvalidId;
};

// Native 16-byte row and serialized field offsets: offset@0, length@4, tokens@8, suffix@12.
// Bytes retain the first original spelling; suffix is a Keyword ID, not a Node ID.
struct ATFW_UTIL_SYMBOL_LOCAL keyword_record {
  uint32_t offset = 0;
  uint32_t length = 0;
  uint32_t tokens = 0;
  uint32_t suffix = kInvalidId;
};

static_assert(sizeof(node) == kNodeStrideBytes && sizeof(keyword_record) == kKeywordStrideBytes, "AC table layout");

struct ATFW_UTIL_SYMBOL_LOCAL build_node {
  std::map<uint32_t, uint32_t> children;
  uint32_t terminal = kInvalidId;
};

struct ATFW_UTIL_SYMBOL_LOCAL build_word {
  std::string bytes;
  uint32_t tokens;
};

struct ATFW_UTIL_SYMBOL_LOCAL builder {
  bool nocase = false;
  std::set<std::string, byte_less> skips;
  std::set<std::string, byte_less> effective_skips;
  std::map<std::string, std::string, byte_less> custom_folds;
  std::map<std::string, std::string, byte_less> folds;
  std::unordered_map<std::string, uint32_t, string_hash> intern;
  std::vector<std::string> symbols;
  std::vector<build_node> nodes{kRootNodeCount};
  std::vector<build_word> words;
  uint64_t word_bytes = 0;

  status configure() {
    folds.clear();
    if (nocase) {
      for (char c = kAsciiUpperFirst; c <= kAsciiUpperLast; ++c) {
        folds.emplace(std::string(1, c), std::string(1, static_cast<char>(c + kAsciiCaseOffset)));
      }
      for (const auto& rule : custom_folds) {
        auto r = folds.emplace(rule);
        if (!r.second && r.first->second != rule.second) {
          return status::kInvalidArgument;
        }
      }
      for (const auto& rule : folds) {
        auto target = folds.find(rule.second);
        if (target != folds.end() && target->first != target->second) {
          return status::kInvalidArgument;
        }
      }
    }
    effective_skips.clear();
    for (const auto& token : skips) {
      effective_skips.insert(canonical(token));
    }
    return status::kOk;
  }

  const std::string& canonical(const std::string& token) const {
    auto it = folds.find(token);
    return it == folds.end() ? token : it->second;
  }
};

struct ATFW_UTIL_SYMBOL_LOCAL utf8_next {
  ac::token_result operator()(gsl::string_view s) const { return ac::next_utf8(s); }
};

struct ATFW_UTIL_SYMBOL_LOCAL utf16le_next {
  ac::token_result operator()(gsl::string_view s) const { return ac::next_utf16le(s); }
};

struct ATFW_UTIL_SYMBOL_LOCAL utf16be_next {
  ac::token_result operator()(gsl::string_view s) const { return ac::next_utf16be(s); }
};

struct ATFW_UTIL_SYMBOL_LOCAL custom_next {
  const ac::tokenizer& tokens;

  ac::token_result operator()(gsl::string_view s) const { return tokens.next(s, tokens.user_data); }
};

static bool valid_tokenizer(const ac::tokenizer& t) {
  return t.encoding == ac::token_encoding::kUtf8 || t.encoding == ac::token_encoding::kUtf16Le ||
         t.encoding == ac::token_encoding::kUtf16Be || (t.encoding == ac::token_encoding::kCustom && t.next != nullptr);
}

static ac::token_result next_token(gsl::string_view s, const ac::tokenizer& t) {
  switch (t.encoding) {
    case ac::token_encoding::kUtf8:
      return ac::next_utf8(s);
    case ac::token_encoding::kUtf16Le:
      return ac::next_utf16le(s);
    case ac::token_encoding::kUtf16Be:
      return ac::next_utf16be(s);
    default:
      return t.next(s, t.user_data);
  }
}

static bool valid_result(ac::token_result r, size_t remaining) {
  return r.byte_count > 0 && r.byte_count <= remaining &&
         (r.state == ac::token_status::kValid || r.state == ac::token_status::kInvalid);
}

static ac::token_result utf16_next(gsl::string_view s, bool le) noexcept {
  if (s.size() < kUtf16CodeUnitBytes) {
    return {ac::token_status::kInvalid, s.size()};
  }
  auto p = reinterpret_cast<const unsigned char*>(s.data());
  uint16_t a = le ? bit::read_le_uint16(p) : static_cast<uint16_t>((p[0] << kBitsPerByte) | p[1]);
  if (a >= kUtf16LowSurrogateFirst && a <= kUtf16LowSurrogateLast) {
    return {ac::token_status::kInvalid, kUtf16CodeUnitBytes};
  }
  if (a < kUtf16HighSurrogateFirst || a > kUtf16HighSurrogateLast) {
    return {ac::token_status::kValid, kUtf16CodeUnitBytes};
  }
  if (s.size() < kUtf16SurrogatePairBytes) {
    return {ac::token_status::kInvalid, kUtf16CodeUnitBytes};
  }
  uint16_t b = le ? bit::read_le_uint16(p + kUtf16CodeUnitBytes)
                  : static_cast<uint16_t>((p[kUtf16CodeUnitBytes] << kBitsPerByte) | p[kUtf16CodeUnitBytes + 1]);
  return b >= kUtf16LowSurrogateFirst && b <= kUtf16LowSurrogateLast
             ? ac::token_result{ac::token_status::kValid, kUtf16SurrogatePairBytes}
             : ac::token_result{ac::token_status::kInvalid, kUtf16CodeUnitBytes};
}
}  // namespace

namespace detail {
struct ac_v2_storage {
  // Frozen payload is a set of separately allocated contiguous arrays, not an object image.
  // nodes/words are 16-byte rows; labels/offsets/folds hold u32 values. Symbol offsets have
  // S+1 entries; skips have ceil(S/8) bytes; folds have S entries only when nocase is active.
  // pending belongs only to collecting snapshots and is released before built publication.
  // The complete native and versioned disk layouts are documented on ac_automation_v2.
  // Query indexing relies on build()/read_image() establishing these invariants once:
  // nonempty root/offset tables; every label/fold/keyword ID is in its corresponding table;
  // child intervals fit Node/Label arrays; failure/output chains decrease in depth/length.
  // Public view/span arguments must themselves refer to live memory supplied by the caller.
  std::unique_ptr<builder> pending{new builder};
  bool nocase = false;
  uint32_t max_tokens = 0;
  std::vector<node> nodes;
  std::vector<uint32_t> labels;
  std::vector<uint32_t> offsets;
  std::vector<uint32_t> folds;
  std::string symbol_bytes;
  std::string keyword_bytes;
  std::vector<unsigned char> skips;
  std::vector<keyword_record> words;
  // Derived from the opaque byte alphabet, independent of the input tokenizer.
  std::array<uint32_t, kSymbolBucketCount> symbol_buckets{};
  std::array<uint32_t, kByteValueCount> single_byte_symbols{};

  bool has_basic_layout() const noexcept {
    if (nodes.empty() || labels.size() != nodes.size() || nodes.size() >= kInvalidId || words.size() >= kInvalidId ||
        offsets.empty() || offsets.size() > kInvalidId || symbol_bytes.size() > UINT32_MAX ||
        keyword_bytes.size() > UINT32_MAX) {
      return false;
    }
    uint64_t symbols = offsets.size() - 1;
    return skips.size() == (symbols + kBitsPerByte - 1) / kBitsPerByte && folds.size() == (nocase ? symbols : 0);
  }

  bool initialize_symbol_index() noexcept {
    symbol_buckets.fill(kUnknownSymbolId);
    single_byte_symbols.fill(kUnknownSymbolId);
    if (offsets.empty() || offsets.size() > kInvalidId) {
      return false;
    }
    uint32_t id = kFirstSymbolId;
    for (uint32_t byte = 0; byte < kByteValueCount; ++byte) {
      symbol_buckets[byte] = id;
      while (id < offsets.size()) {
        gsl::string_view token = symbol(id);
        if (token.empty()) {
          return false;
        }
        uint32_t first_byte = static_cast<unsigned char>(token[0]);
        if (first_byte < byte) {
          return false;
        }
        if (first_byte != byte) {
          break;
        }
        if (token.size() == 1) {
          single_byte_symbols[byte] = id;
        }
        ++id;
      }
    }
    symbol_buckets[kByteValueCount] = id;
    return id == offsets.size();
  }

  gsl::string_view symbol(uint32_t id) const noexcept {
    if (id == kUnknownSymbolId || id >= offsets.size()) {
      return {};
    }
    uint32_t begin = offsets[id - 1];
    uint32_t end = offsets[id];
    if (begin >= end || end > symbol_bytes.size()) {
      return {};
    }
    return gsl::string_view(symbol_bytes.data() + begin, end - begin);
  }

  gsl::string_view keyword(uint32_t id) const noexcept {
    if (id >= words.size()) {
      return {};
    }
    const keyword_record& record = words[id];
    size_t begin = record.offset;
    size_t length = record.length;
    if (length == 0 || begin > keyword_bytes.size() || length > keyword_bytes.size() - begin) {
      return {};
    }
    return gsl::string_view(keyword_bytes.data() + begin, length);
  }

  uint32_t lookup(gsl::string_view token) const noexcept {
    if (token.empty() || offsets.empty()) {
      return kUnknownSymbolId;
    }
    auto byte = static_cast<unsigned char>(token[0]);
    if (token.size() == 1) {
      uint32_t id = single_byte_symbols[byte];
      return id < offsets.size() ? id : kUnknownSymbolId;
    }
    uint32_t lo = symbol_buckets[byte];
    uint32_t hi = symbol_buckets[byte + 1];
    if (lo == kUnknownSymbolId || lo >= hi || hi > offsets.size()) {
      return kUnknownSymbolId;
    }
    // Each end-offset row's array index is its Symbol ID. Compare row references directly
    // so lower_bound needs neither a separate ID table nor an allocating token projection.
    const uint32_t* first = offsets.data() + lo;
    const uint32_t* last = offsets.data() + hi;
    bool valid = true;
    const uint32_t* found =
        std::lower_bound(first, last, token, [this, &valid](const uint32_t& end_offset, gsl::string_view needle) {
          uint32_t id = static_cast<uint32_t>(&end_offset - offsets.data());
          gsl::string_view current = symbol(id);
          if (current.empty()) {
            valid = false;
            return false;
          }
          return byte_less{}(current, needle);
        });
    if (!valid || found == last) {
      return kUnknownSymbolId;
    }
    uint32_t id = static_cast<uint32_t>(found - offsets.data());
    gsl::string_view current = symbol(id);
    return !current.empty() && current == token ? id : kUnknownSymbolId;
  }

  bool skipped(uint32_t id) const noexcept {
    if (id == kUnknownSymbolId || offsets.empty() || id >= offsets.size()) {
      return false;
    }
    size_t index = (id - kFirstSymbolId) / kBitsPerByte;
    return index < skips.size() && (skips[index] & (1u << ((id - kFirstSymbolId) % kBitsPerByte))) != 0;
  }

  uint32_t child(uint32_t parent, uint32_t label) const noexcept {
    if (nodes.empty() || labels.size() != nodes.size() || nodes.size() >= kInvalidId || parent >= nodes.size()) {
      return kRootNodeId;
    }
    const node& p = nodes[parent];
    if (p.count == 0) {
      return kRootNodeId;
    }
    if (p.begin > labels.size() || p.count > labels.size() - p.begin || p.begin > nodes.size() ||
        p.count > nodes.size() - p.begin) {
      return kRootNodeId;
    }
    if (p.count == 1) {
      return labels[p.begin] == label ? p.begin : kRootNodeId;
    }
    const uint32_t* first = labels.data() + p.begin;
    const uint32_t* last = first + p.count;
    const uint32_t* found = std::lower_bound(first, last, label);
    return found != last && *found == label ? static_cast<uint32_t>(found - labels.data()) : kRootNodeId;
  }

  uint64_t payload() const noexcept {
    return nodes.size() * uint64_t{kNodeStrideBytes} + labels.size() * uint64_t{kSerializedIndexBytes} +
           offsets.size() * uint64_t{kSerializedIndexBytes} + folds.size() * uint64_t{kSerializedIndexBytes} +
           words.size() * uint64_t{kKeywordStrideBytes} + symbol_bytes.size() + keyword_bytes.size() + skips.size();
  }

  template <class Next>
  ac::match_report scan(gsl::string_view input, ac::match_callback emit, void* data, const ac::match_options& options,
                        ac::match_context* context, bool only_contains, size_t output_capacity, Next next) const {
    // Every emitted position refers to the original input, including skipped bytes within a hit.
    ac::match_report report;
    auto make_error = [&report](status code) {
      report.code = code;
      report.stopped = ac::stop_reason::kError;
      return report;
    };
    if (!only_contains && output_capacity == 0) {
      report.stopped = ac::stop_reason::kOutputFull;
      return report;
    }
    if (!has_basic_layout()) {
      return make_error(status::kInvalidFormat);
    }
    // One ring entry per retained token is sufficient; contains() needs no position ring.
    size_t cap = only_contains ? 0 : (std::min)(static_cast<size_t>(max_tokens), input.size());
    if (cap > 0 && context == nullptr) {
      return make_error(status::kInvalidArgument);
    }
    if (cap > 0) {
      context->origins_.resize(cap);
      if (context->origins_.size() < cap) {
        return make_error(status::kInvalidFormat);
      }
    }
    size_t cursor = 0;
    size_t valid = 0;
    size_t pos = 0;
    uint32_t state = kRootNodeId;
    while (pos < input.size()) {
      auto r = next(input.substr(pos));
      if (!valid_result(r, input.size() - pos)) {
        report.code = status::kInvalidTokenizerResult;
        report.stopped = ac::stop_reason::kError;
        break;
      }
      // Recovery breaks the current candidate, so matches cannot bridge invalid encoding units.
      if (r.state == ac::token_status::kInvalid) {
        if (report.invalid_units == 0) {
          report.first_invalid_offset = pos;
        }
        ++report.invalid_units;
        if (options.invalid_input == ac::invalid_input_policy::kReject) {
          report.code = status::kInvalidEncoding;
          report.stopped = ac::stop_reason::kError;
          break;
        }
        state = kRootNodeId;
        valid = cursor = 0;
        pos += r.byte_count;
        report.bytes_consumed = pos;
        continue;
      }
      uint32_t symbol_id = lookup(input.substr(pos, r.byte_count));
      if (nocase && symbol_id != kUnknownSymbolId) {
        if (symbol_id > folds.size()) {
          return make_error(status::kInvalidFormat);
        }
        symbol_id = folds[symbol_id - 1];
        if (symbol_id == kUnknownSymbolId || offsets.empty() || symbol_id >= offsets.size()) {
          return make_error(status::kInvalidFormat);
        }
      }
      size_t start = pos;
      pos += r.byte_count;
      report.bytes_consumed = pos;
      if (skipped(symbol_id)) {
        continue;
      }
      if (symbol_id == kUnknownSymbolId) {
        state = kRootNodeId;
        valid = cursor = 0;
        continue;
      }
      // Record only non-skipped tokens, while retaining their original byte offsets.
      if (cap > 0) {
        if (context == nullptr || context->origins_.size() < cap || cursor >= cap) {
          return make_error(status::kInvalidFormat);
        }
        context->origins_[cursor] = start;
        if (++cursor == cap) {
          cursor = 0;
        }
        if (valid < cap) {
          ++valid;
        }
      }
      if (state >= nodes.size()) {
        return make_error(status::kInvalidFormat);
      }
      // BFS failure IDs decrease strictly, bounding every fallback traversal.
      uint32_t target = child(state, symbol_id);
      while (target == kRootNodeId && state != kRootNodeId) {
        uint32_t failure = nodes[state].fail;
        if (failure >= nodes.size() || failure >= state) {
          return make_error(status::kInvalidFormat);
        }
        state = failure;
        target = child(state, symbol_id);
      }
      state = target;
      if (state >= nodes.size()) {
        return make_error(status::kInvalidFormat);
      }
      // The output head is the longest terminal suffix; overlap mode follows shorter suffixes.
      uint32_t output = nodes[state].output;
      while (output != kInvalidId) {
        if (output >= words.size()) {
          return make_error(status::kInvalidFormat);
        }
        if (only_contains) {
          report.matches = 1;
          report.stopped = ac::stop_reason::kMatchLimit;
          return report;
        }
        const auto& word = words[output];
        if (word.tokens == 0 || cap == 0 || context == nullptr || cursor >= cap || word.tokens > valid ||
            word.tokens > cap || context->origins_.size() < cap) {
          return make_error(status::kInvalidFormat);
        }
        size_t index = cursor >= word.tokens ? cursor - word.tokens : cap - (word.tokens - cursor);
        if (index >= cap) {
          return make_error(status::kInvalidFormat);
        }
        size_t begin = context->origins_[index];
        if (begin > pos) {
          return make_error(status::kInvalidFormat);
        }
        ac::match_t hit{begin, pos - begin, output, keyword(output)};
        ++report.matches;
        bool keep_going = emit(hit, data);
        if (options.max_matches > 0 && report.matches >= options.max_matches) {
          report.stopped = ac::stop_reason::kMatchLimit;
          return report;
        }
        if (report.matches >= output_capacity) {
          report.stopped = ac::stop_reason::kOutputFull;
          return report;
        }
        if (!keep_going) {
          report.stopped = ac::stop_reason::kCallbackStopped;
          return report;
        }
        if (!options.allow_overlap) {
          state = kRootNodeId;
          cursor = valid = 0;
          break;
        }
        output = word.suffix;
      }
    }
    return report;
  }

  ac::match_report dispatch(gsl::string_view input, ac::match_callback emit, void* data,
                            const ac::match_options& options, ac::match_context* context, bool only_contains = false,
                            size_t capacity = SIZE_MAX) const {
    ac::match_report r;
    if (pending != nullptr) {
      r.code = status::kInvalidState;
    } else if ((!only_contains && emit == nullptr) || !valid_tokenizer(options.tokens) ||
               (options.invalid_input != ac::invalid_input_policy::kSkipCurrent &&
                options.invalid_input != ac::invalid_input_policy::kReject)) {
      r.code = status::kInvalidArgument;
    }
    if (r.code != status::kOk) {
      r.stopped = ac::stop_reason::kError;
      return r;
    }
    switch (options.tokens.encoding) {
      case ac::token_encoding::kUtf8:
        return scan(input, emit, data, options, context, only_contains, capacity, utf8_next{});
      case ac::token_encoding::kUtf16Le:
        return scan(input, emit, data, options, context, only_contains, capacity, utf16le_next{});
      case ac::token_encoding::kUtf16Be:
        return scan(input, emit, data, options, context, only_contains, capacity, utf16be_next{});
      default:
        return scan(input, emit, data, options, context, only_contains, capacity, custom_next{options.tokens});
    }
  }
};
}  // namespace detail

ac::tokenizer::tokenizer() noexcept : encoding(token_encoding::kUtf8), next(nullptr), user_data(nullptr) {}

ac::tokenizer::tokenizer(token_encoding e) noexcept : encoding(e), next(nullptr), user_data(nullptr) {}

ac::tokenizer::tokenizer(tokenize_fn cb, void* data) noexcept
    : encoding(token_encoding::kCustom), next(cb), user_data(data) {}

ac::match_options::match_options() noexcept
    : allow_overlap(false), max_matches(0), invalid_input(invalid_input_policy::kSkipCurrent) {}

ac::match_report::match_report() noexcept
    : code(status::kOk),
      stopped(stop_reason::kEndOfInput),
      bytes_consumed(0),
      matches(0),
      invalid_units(0),
      first_invalid_offset(SIZE_MAX) {}

ac::load_options::load_options() noexcept
    : max_file_bytes(kDefaultLoadBudgetBytes), max_memory_bytes(kDefaultLoadBudgetBytes), verify_semantics(false) {}

ac::match_context::match_context() = default;
ac::match_context::~match_context() = default;
ac::match_context::match_context(match_context&&) noexcept = default;
ac::match_context& ac::match_context::operator=(match_context&&) noexcept = default;

void ac::match_context::reserve(size_t n) { origins_.reserve(n); }

size_t ac::match_context::capacity() const noexcept { return origins_.capacity(); }

ac::ac_automation_v2() : storage_(std::make_shared<detail::ac_v2_storage>()) {}

ac::~ac_automation_v2() = default;
ac::ac_automation_v2(const ac_automation_v2&) noexcept = default;
ac& ac::operator=(const ac_automation_v2&) noexcept = default;
ac::ac_automation_v2(ac_automation_v2&&) noexcept = default;
ac& ac::operator=(ac_automation_v2&&) noexcept = default;

void ac::reset() { storage_ = std::make_shared<detail::ac_v2_storage>(); }

bool ac::is_built() const noexcept { return storage_ != nullptr && storage_->pending == nullptr; }

namespace {
static std::shared_ptr<detail::ac_v2_storage> copy_builder(const std::shared_ptr<detail::ac_v2_storage>& s) {
  auto r = std::make_shared<detail::ac_v2_storage>();
  if (s != nullptr) {
    if (s->pending == nullptr) {
      return nullptr;
    }
    *r->pending = *s->pending;
  }
  return r;
}

static bool configurable(const std::shared_ptr<detail::ac_v2_storage>& s) {
  return s == nullptr || (s->pending != nullptr && s->pending->words.empty());
}
}  // namespace

status ac::set_nocase(bool value) {
  if (!configurable(storage_)) {
    return status::kInvalidState;
  }
  auto next = copy_builder(storage_);
  if (next == nullptr) {
    return status::kInvalidState;
  }
  next->pending->nocase = value;
  auto result = next->pending->configure();
  if (result == status::kOk) {
    storage_.swap(next);
  }
  return result;
}

status ac::set_skip(gsl::string_view token) { return set_skip(gsl::span<const gsl::string_view>(&token, 1)); }

status ac::set_skip(gsl::span<const gsl::string_view> tokens) {
  if (!configurable(storage_)) {
    return status::kInvalidState;
  }
  for (auto token : tokens) {
    if (token.empty() || token.size() > UINT32_MAX) {
      return status::kInvalidArgument;
    }
  }
  auto next = copy_builder(storage_);
  if (next == nullptr) {
    return status::kInvalidState;
  }
  for (auto token : tokens) {
    next->pending->skips.insert(own(token));
  }
  auto result = next->pending->configure();
  if (result == status::kOk) {
    storage_.swap(next);
  }
  return result;
}

status ac::unset_skip(gsl::string_view token) {
  if (!configurable(storage_)) {
    return status::kInvalidState;
  }
  if (token.empty()) {
    return status::kInvalidArgument;
  }
  auto next = copy_builder(storage_);
  if (next == nullptr) {
    return status::kInvalidState;
  }
  next->pending->skips.erase(own(token));
  auto result = next->pending->configure();
  if (result == status::kOk) {
    storage_.swap(next);
  }
  return result;
}

status ac::set_case_folding(gsl::span<const case_fold_rule> rules) {
  if (!configurable(storage_)) {
    return status::kInvalidState;
  }
  auto next = copy_builder(storage_);
  if (next == nullptr) {
    return status::kInvalidState;
  }
  next->pending->custom_folds.clear();
  for (const auto& rule : rules) {
    if (rule.source_token.empty() || rule.target_token.empty() || rule.source_token.size() > UINT32_MAX ||
        rule.target_token.size() > UINT32_MAX) {
      return status::kInvalidArgument;
    }
    auto r = next->pending->custom_folds.emplace(own(rule.source_token), own(rule.target_token));
    if (!r.second && view(r.first->second) != rule.target_token) {
      return status::kInvalidArgument;
    }
  }
  // Validate even while disabled, including collisions with the built-in ASCII rules.
  bool enabled = next->pending->nocase;
  next->pending->nocase = true;
  auto result = next->pending->configure();
  if (result != status::kOk) {
    return result;
  }
  next->pending->nocase = enabled;
  next->pending->configure();
  storage_.swap(next);
  return status::kOk;
}

status ac::insert_keyword(gsl::string_view text, uint32_t* keyword_id, const tokenizer& tokens) {
  // Tokenize and normalize completely before changing the shared builder.
  if (storage_ != nullptr && storage_->pending == nullptr) {
    return status::kInvalidState;
  }
  if (!valid_tokenizer(tokens)) {
    return status::kInvalidArgument;
  }
  if (text.size() > UINT32_MAX) {
    return status::kCapacityExceeded;
  }
  if (storage_ == nullptr) {
    reset();
  }
  const auto& original = *storage_->pending;
  std::vector<std::string> sequence;
  for (size_t pos = 0; pos < text.size();) {
    auto r = next_token(text.substr(pos), tokens);
    if (!valid_result(r, text.size() - pos)) {
      return status::kInvalidTokenizerResult;
    }
    if (r.state == token_status::kInvalid) {
      return status::kInvalidEncoding;
    }
    std::string token = own(text.substr(pos, r.byte_count));
    pos += r.byte_count;
    token = original.canonical(token);
    if (original.effective_skips.find(token) == original.effective_skips.end()) {
      sequence.emplace_back(std::move(token));
    }
  }
  if (sequence.empty()) {
    return status::kEmptyKeyword;
  }
  if (sequence.size() >= kInvalidId || original.words.size() >= kInvalidId ||
      static_cast<uint64_t>(original.nodes.size()) + sequence.size() >= kInvalidId ||
      original.word_bytes + text.size() > UINT32_MAX ||
      static_cast<uint64_t>(original.symbols.size()) + sequence.size() >= kInvalidId) {
    return status::kCapacityExceeded;
  }
  // A duplicate keeps its first-insertion ID and does not detach shared builder storage.
  uint32_t current = kRootNodeId;
  bool existing = true;
  for (const auto& token : sequence) {
    if (current >= original.nodes.size()) {
      return status::kInvalidFormat;
    }
    auto symbol = original.intern.find(token);
    if (symbol == original.intern.end()) {
      existing = false;
      break;
    }
    if (symbol->second >= original.symbols.size()) {
      return status::kInvalidFormat;
    }
    auto edge = original.nodes[current].children.find(symbol->second);
    if (edge == original.nodes[current].children.end()) {
      existing = false;
      break;
    }
    if (edge->second >= original.nodes.size()) {
      return status::kInvalidFormat;
    }
    current = edge->second;
  }
  if (current >= original.nodes.size()) {
    return status::kInvalidFormat;
  }
  if (existing && original.nodes[current].terminal != kInvalidId) {
    if (keyword_id != nullptr) {
      *keyword_id = original.nodes[current].terminal;
    }
    return status::kAlreadyExists;
  }
  // Detach only after validation and duplicate detection have succeeded.
  if (storage_.use_count() != 1) {
    auto copied = copy_builder(storage_);
    if (copied == nullptr) {
      return status::kInvalidFormat;
    }
    storage_.swap(copied);
  }
  if (storage_ == nullptr || storage_->pending == nullptr) {
    return status::kInvalidFormat;
  }
  auto& b = *storage_->pending;

  // Journal allocations so an allocation exception cannot leave a half-inserted path.
  struct transaction {
    builder& b;
    size_t nodes;
    size_t symbols;
    size_t words;
    std::vector<std::pair<uint32_t, uint32_t>> edges;
    bool committed = false;

    explicit transaction(builder& v) : b(v), nodes(v.nodes.size()), symbols(v.symbols.size()), words(v.words.size()) {}

    ~transaction() {
      if (committed) {
        return;
      }
      for (auto it = edges.rbegin(); it != edges.rend(); ++it) {
        if (it->first < b.nodes.size()) {
          b.nodes[it->first].children.erase(it->second);
        }
      }
      for (size_t i = symbols; i < b.symbols.size(); ++i) {
        b.intern.erase(b.symbols[i]);
      }
      b.nodes.resize(nodes);
      b.symbols.resize(symbols);
      b.words.resize(words);
    }
  } guard(b);

  // Reserve the rollback journal before any trie edge can be published.
  guard.edges.reserve(sequence.size());
  current = kRootNodeId;
  for (const auto& token : sequence) {
    if (current >= b.nodes.size()) {
      return status::kInvalidFormat;
    }
    auto symbol = b.intern.find(token);
    uint32_t id;
    if (symbol == b.intern.end()) {
      if (b.symbols.size() >= kInvalidId) {
        return status::kCapacityExceeded;
      }
      id = static_cast<uint32_t>(b.symbols.size());
      b.symbols.push_back(token);
      b.intern.emplace(token, id);
    } else {
      id = symbol->second;
      if (id >= b.symbols.size()) {
        return status::kInvalidFormat;
      }
    }
    auto edge = b.nodes[current].children.find(id);
    if (edge == b.nodes[current].children.end()) {
      if (b.nodes.size() >= kInvalidId) {
        return status::kCapacityExceeded;
      }
      uint32_t target = static_cast<uint32_t>(b.nodes.size());
      b.nodes.emplace_back();
      b.nodes[current].children.emplace(id, target);
      guard.edges.emplace_back(current, id);
      current = target;
    } else {
      if (edge->second >= b.nodes.size()) {
        return status::kInvalidFormat;
      }
      current = edge->second;
    }
  }
  if (current >= b.nodes.size()) {
    return status::kInvalidFormat;
  }
  if (b.words.size() >= kInvalidId) {
    return status::kCapacityExceeded;
  }
  // Publish the terminal and byte count only after all throwing allocations have succeeded.
  uint32_t id = static_cast<uint32_t>(b.words.size());
  b.words.push_back(build_word{own(text), static_cast<uint32_t>(sequence.size())});
  b.nodes[current].terminal = id;
  b.word_bytes += text.size();
  guard.committed = true;
  if (keyword_id != nullptr) {
    *keyword_id = id;
  }
  return status::kOk;
}

status ac::build() {
  // Freeze into a separate candidate; errors leave the collecting snapshot unchanged.
  if (is_built()) {
    return status::kOk;
  }
  if (storage_ == nullptr) {
    reset();
  }
  const builder& b = *storage_->pending;
  if (b.nodes.empty() || b.nodes.size() >= kInvalidId || b.words.size() >= kInvalidId ||
      b.symbols.size() >= kInvalidId || b.word_bytes > UINT32_MAX) {
    return status::kInvalidFormat;
  }
  auto result = std::make_shared<detail::ac_v2_storage>();
  result->pending.reset();
  result->nocase = b.nocase;
  // Sort opaque token bytes once and share one symbol table across all trie nodes.
  std::set<std::string, byte_less> symbols(b.symbols.begin(), b.symbols.end());
  symbols.insert(b.skips.begin(), b.skips.end());
  symbols.insert(b.effective_skips.begin(), b.effective_skips.end());
  for (const auto& rule : b.folds) {
    symbols.insert(rule.first);
    symbols.insert(rule.second);
  }
  if (symbols.size() >= kInvalidId) {
    return status::kCapacityExceeded;
  }
  uint64_t symbol_bytes = 0;
  for (const auto& s : symbols) {
    symbol_bytes += s.size();
    if (symbol_bytes > UINT32_MAX) {
      return status::kCapacityExceeded;
    }
  }
  result->symbol_bytes.reserve(static_cast<size_t>(symbol_bytes));
  result->offsets.reserve(symbols.size() + 1);
  result->offsets.push_back(0);
  for (const auto& s : symbols) {
    if (s.size() > UINT32_MAX - result->symbol_bytes.size()) {
      return status::kCapacityExceeded;
    }
    result->symbol_bytes.append(s);
    result->offsets.push_back(static_cast<uint32_t>(result->symbol_bytes.size()));
  }
  if (!result->initialize_symbol_index()) {
    return status::kInvalidFormat;
  }
  // Materialize normalized skip/fold configuration as dense ID-indexed tables.
  result->skips.resize(static_cast<size_t>((static_cast<uint64_t>(symbols.size()) + kBitsPerByte - 1) / kBitsPerByte),
                       0);
  for (const auto& s : b.effective_skips) {
    uint32_t symbol_id = result->lookup(view(s));
    if (symbol_id == kUnknownSymbolId) {
      return status::kInvalidFormat;
    }
    uint32_t id = symbol_id - kFirstSymbolId;
    size_t index = id / kBitsPerByte;
    if (index >= result->skips.size()) {
      return status::kInvalidFormat;
    }
    result->skips[index] |= static_cast<unsigned char>(1u << (id % kBitsPerByte));
  }
  if (b.nocase) {
    result->folds.reserve(symbols.size());
    for (const auto& s : symbols) {
      uint32_t target = result->lookup(view(b.canonical(s)));
      if (target == kUnknownSymbolId) {
        return status::kInvalidFormat;
      }
      result->folds.push_back(target);
    }
  }
  // Translate collecting symbol IDs without changing stable keyword IDs or original bytes.
  std::vector<uint32_t> translated;
  translated.reserve(b.symbols.size());
  for (const auto& s : b.symbols) {
    uint32_t target = result->lookup(view(s));
    if (target == kUnknownSymbolId) {
      return status::kInvalidFormat;
    }
    translated.push_back(target);
  }
  result->words.reserve(b.words.size());
  result->keyword_bytes.reserve(static_cast<size_t>(b.word_bytes));
  for (const auto& word : b.words) {
    if (word.bytes.empty() || word.tokens == 0 || word.tokens > word.bytes.size() ||
        word.bytes.size() > UINT32_MAX - result->keyword_bytes.size()) {
      return status::kInvalidFormat;
    }
    keyword_record r;
    r.offset = static_cast<uint32_t>(result->keyword_bytes.size());
    r.length = static_cast<uint32_t>(word.bytes.size());
    r.tokens = word.tokens;
    result->words.push_back(r);
    result->keyword_bytes += word.bytes;
    result->max_tokens = (std::max)(result->max_tokens, word.tokens);
  }
  if (result->keyword_bytes.size() != b.word_bytes) {
    return status::kInvalidFormat;
  }
  // BFS allocation makes every parent's sorted children one contiguous array interval.
  result->nodes.reserve(b.nodes.size());
  result->labels.reserve(b.nodes.size());
  std::vector<uint32_t> queue;
  queue.reserve(b.nodes.size());
  queue.push_back(kRootNodeId);
  result->nodes.emplace_back();
  result->labels.push_back(0);
  std::vector<std::pair<uint32_t, uint32_t>> children;
  for (size_t i = 0; i < queue.size(); ++i) {
    if (queue[i] >= b.nodes.size() || i >= result->nodes.size() || i >= result->labels.size()) {
      return status::kInvalidFormat;
    }
    const auto& old = b.nodes[queue[i]];
    if (old.terminal != kInvalidId && old.terminal >= b.words.size()) {
      return status::kInvalidFormat;
    }
    children.clear();
    for (const auto& edge : old.children) {
      if (edge.first >= translated.size() || edge.second >= b.nodes.size()) {
        return status::kInvalidFormat;
      }
      children.emplace_back(translated[edge.first], edge.second);
    }
    std::sort(children.begin(), children.end());
    if (result->nodes.size() >= kInvalidId || children.size() >= kInvalidId) {
      return status::kCapacityExceeded;
    }
    result->nodes[i].begin = static_cast<uint32_t>(result->nodes.size());
    result->nodes[i].count = static_cast<uint32_t>(children.size());
    result->nodes[i].output = old.terminal;
    for (const auto& edge : children) {
      if (result->nodes.size() >= kInvalidId || result->labels.size() >= kInvalidId) {
        return status::kCapacityExceeded;
      }
      queue.push_back(edge.second);
      result->nodes.emplace_back();
      result->labels.push_back(edge.first);
    }
  }
  if (result->labels.size() != result->nodes.size()) {
    return status::kInvalidFormat;
  }
  // Parents and failure ancestors are already initialized when each child is visited.
  for (uint32_t i = 0; i < result->nodes.size(); ++i) {
    const auto p = result->nodes[i];
    if (p.begin > result->nodes.size() || p.count > result->nodes.size() - p.begin || p.begin > result->labels.size() ||
        p.count > result->labels.size() - p.begin || (i > 0 && (p.fail >= i || p.fail >= result->nodes.size()))) {
      return status::kInvalidFormat;
    }
    for (uint32_t j = p.begin; j < p.begin + p.count; ++j) {
      if (j >= result->nodes.size() || j >= result->labels.size()) {
        return status::kInvalidFormat;
      }
      uint32_t fail = kRootNodeId;
      if (i > 0) {
        uint32_t ancestor = p.fail;
        fail = result->child(ancestor, result->labels[j]);
        while (fail == kRootNodeId && ancestor != kRootNodeId) {
          uint32_t next_ancestor = result->nodes[ancestor].fail;
          if (next_ancestor >= result->nodes.size() || next_ancestor >= ancestor) {
            return status::kInvalidFormat;
          }
          ancestor = next_ancestor;
          fail = result->child(ancestor, result->labels[j]);
        }
      }
      if (fail >= result->nodes.size() || j <= i) {
        return status::kInvalidFormat;
      }
      result->nodes[j].fail = fail;
      uint32_t output = result->nodes[j].output;
      if (output != kInvalidId && output >= result->words.size()) {
        return status::kInvalidFormat;
      }
      if (output != kInvalidId) {
        result->words[output].suffix = result->nodes[fail].output;
      } else {
        result->nodes[j].output = result->nodes[fail].output;
      }
    }
  }
  if (!result->has_basic_layout()) {
    return status::kInvalidFormat;
  }
  storage_.swap(result);
  return status::kOk;
}

namespace {
static bool append_hit(const ac::match_t& hit, void* data) {
  if (data == nullptr) {
    return false;
  }
  static_cast<std::vector<ac::match_t>*>(data)->push_back(hit);
  return true;
}

struct ATFW_UTIL_SYMBOL_LOCAL span_output {
  ac::match_t* next;
  size_t remaining;
  bool failed;
};

static bool store_hit(const ac::match_t& hit, void* data) {
  if (data == nullptr) {
    return false;
  }
  span_output& output = *static_cast<span_output*>(data);
  if (output.failed || output.next == nullptr || output.remaining == 0) {
    output.failed = true;
    return false;
  }
  *output.next++ = hit;
  --output.remaining;
  return true;
}

static ac::match_report invalid_state_report() {
  ac::match_report r;
  r.code = status::kInvalidState;
  r.stopped = ac::stop_reason::kError;
  return r;
}
}  // namespace

ac::match_report ac::match(gsl::string_view input, std::vector<match_t>& output, const match_options& options,
                           match_context* context) const {
  size_t before = output.size();
  auto r = match(input, append_hit, &output, options, context);
  if (r.code != status::kOk) {
    output.resize(before);
    r.matches = 0;
  }
  return r;
}

ac::match_report ac::match(gsl::string_view input, gsl::span<match_t> output, const match_options& options,
                           match_context* context) const {
  if (storage_ == nullptr) {
    return invalid_state_report();
  }
  match_context local;
  span_output sink{output.data(), output.size(), false};
  auto report =
      storage_->dispatch(input, store_hit, &sink, options, context != nullptr ? context : &local, false, output.size());
  if (sink.failed && report.code == status::kOk) {
    report.code = status::kInvalidArgument;
    report.stopped = ac::stop_reason::kError;
    if (report.matches > 0) {
      --report.matches;
    }
  }
  return report;
}

ac::match_report ac::match(gsl::string_view input, match_callback callback, void* data, const match_options& options,
                           match_context* context) const {
  if (storage_ == nullptr) {
    return invalid_state_report();
  }
  match_context local;
  return storage_->dispatch(input, callback, data, options, context != nullptr ? context : &local);
}

ac::match_report ac::contains(gsl::string_view input, bool& found, const match_options& options) const {
  found = false;
  if (storage_ == nullptr) {
    return invalid_state_report();
  }
  auto r = storage_->dispatch(input, nullptr, nullptr, options, nullptr, true);
  found = r.matches != 0;
  return r;
}

ac::statistics ac::get_statistics() const noexcept {
  if (!is_built() || !storage_->has_basic_layout()) {
    return {};
  }
  return {static_cast<uint32_t>(storage_->nodes.size()),
          static_cast<uint32_t>(storage_->words.size()),
          static_cast<uint32_t>(storage_->offsets.size() - 1),
          storage_->max_tokens,
          storage_->payload(),
          static_cast<uint32_t>(kSymbolIndexBytes),
          storage_->nocase};
}

gsl::string_view ac::keyword(uint32_t id) const noexcept {
  return is_built() ? storage_->keyword(id) : gsl::string_view{};
}

bool ac::visit_nodes(node_callback callback, void* data) const {
  if (!is_built() || !storage_->has_basic_layout() || callback == nullptr) {
    return false;
  }
  for (uint32_t i = 0; i < storage_->nodes.size(); ++i) {
    const auto& n = storage_->nodes[i];
    if (!callback(node_view{i, n.begin, n.count, n.fail, n.output, storage_->labels[i],
                            storage_->symbol(storage_->labels[i])},
                  data)) {
      return false;
    }
  }
  return true;
}

ac::token_result ac::next_utf8(gsl::string_view input, void*) noexcept {
  if (input.empty()) {
    return {token_status::kInvalid, 0};
  }
  auto p = reinterpret_cast<const unsigned char*>(input.data());
  unsigned char c = p[0];
  if (c < kUtf8SingleByteLimit) {
    return {token_status::kValid, kUtf8SingleByteLength};
  }
  size_t n = c >= kUtf8TwoByteFirst && c <= kUtf8TwoByteLast       ? kUtf8TwoByteLength
             : c >= kUtf8ThreeByteFirst && c <= kUtf8ThreeByteLast ? kUtf8ThreeByteLength
             : c >= kUtf8FourByteFirst && c <= kUtf8FourByteLast   ? kUtf8FourByteLength
                                                                   : 0;
  if (n == 0 || n > input.size()) {
    return {token_status::kInvalid, kUtf8SingleByteLength};
  }
  for (size_t i = 1; i < n; ++i) {
    if ((p[i] & kUtf8ContinuationMask) != kUtf8ContinuationTag) {
      return {token_status::kInvalid, kUtf8SingleByteLength};
    }
  }
  if ((c == kUtf8ThreeByteFirst && p[1] < kUtf8ThreeByteLowerBound) ||
      (c == kUtf8SurrogateLead && p[1] >= kUtf8SurrogateLowerBound) ||
      (c == kUtf8FourByteFirst && p[1] < kUtf8FourByteLowerBound) ||
      (c == kUtf8FourByteLast && p[1] >= kUtf8ScalarUpperBound)) {
    return {token_status::kInvalid, kUtf8SingleByteLength};
  }
  return {token_status::kValid, n};
}

ac::token_result ac::next_utf16le(gsl::string_view input, void*) noexcept { return utf16_next(input, true); }

ac::token_result ac::next_utf16be(gsl::string_view input, void*) noexcept { return utf16_next(input, false); }

gsl::string_view ac::status_message(status s) noexcept {
  switch (s) {
    case status::kOk:
      return kStatusOk;
    case status::kAlreadyExists:
      return kStatusAlreadyExists;
    case status::kInvalidState:
      return kStatusInvalidState;
    case status::kInvalidArgument:
      return kStatusInvalidArgument;
    case status::kEmptyKeyword:
      return kStatusEmptyKeyword;
    case status::kInvalidEncoding:
      return kStatusInvalidEncoding;
    case status::kInvalidTokenizerResult:
      return kStatusInvalidTokenizerResult;
    case status::kCapacityExceeded:
      return kStatusCapacityExceeded;
    case status::kInvalidFormat:
      return kStatusInvalidFormat;
    case status::kUnsupportedVersion:
      return kStatusUnsupportedVersion;
    case status::kChecksumMismatch:
      return kStatusChecksumMismatch;
    case status::kIoError:
      return kStatusIoError;
    case status::kResourceLimit:
      return kStatusResourceLimit;
    default:
      return kStatusUnknown;
  }
}

namespace {
struct ATFW_UTIL_SYMBOL_LOCAL hash_state {
  XXH64_state_t* value = XXH64_createState();

  hash_state() {
    if (value != nullptr) {
      XXH64_reset(value, kXxh64Seed);
    }
  }

  ~hash_state() { XXH64_freeState(value); }

  hash_state(const hash_state&) = delete;
  hash_state& operator=(const hash_state&) = delete;
};

struct ATFW_UTIL_SYMBOL_LOCAL image_writer {
  std::ostream* stream;
  std::vector<unsigned char>* bytes;
  hash_state hash;
  uint64_t position = 0;

  image_writer(std::ostream* output, std::vector<unsigned char>* buffer) : stream(output), bytes(buffer) {}

  bool write(const unsigned char* data, size_t count, bool checksum = true) {
    if (hash.value == nullptr || (bytes == nullptr && stream == nullptr) || (count > 0 && data == nullptr) ||
        count > UINT64_MAX - position ||
        (bytes == nullptr &&
         static_cast<uint64_t>(count) > static_cast<uint64_t>((std::numeric_limits<std::streamsize>::max)()))) {
      return false;
    }
    if (count == 0) {
      return bytes != nullptr || stream->good();
    }
    if (bytes != nullptr && count > bytes->max_size() - bytes->size()) {
      return false;
    }
    if (checksum) {
      XXH64_update(hash.value, data, count);
    }
    if (bytes != nullptr) {
      bytes->insert(bytes->end(), data, data + count);
    } else {
      stream->write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(count));
    }
    position += count;
    return bytes != nullptr || stream->good();
  }

  bool padding(uint64_t end) {
    static constexpr unsigned char kPaddingZeros[kImageAlignmentBytes] = {};
    return end >= position && end - position < kImageAlignmentBytes &&
           write(kPaddingZeros, static_cast<size_t>(end - position));
  }
};

struct ATFW_UTIL_SYMBOL_LOCAL image_reader {
  std::istream* stream;
  const unsigned char* bytes;
  size_t length;
  hash_state hash;
  uint64_t position = 0;

  image_reader(std::istream* input, const unsigned char* buffer, size_t size)
      : stream(input), bytes(buffer), length(size) {}

  bool read(unsigned char* data, size_t count, bool checksum = true) {
    if (hash.value == nullptr || (bytes == nullptr && stream == nullptr) || (count > 0 && data == nullptr) ||
        count > UINT64_MAX - position ||
        (bytes == nullptr &&
         static_cast<uint64_t>(count) > static_cast<uint64_t>((std::numeric_limits<std::streamsize>::max)()))) {
      return false;
    }
    if (bytes != nullptr && (position > length || count > length - static_cast<size_t>(position))) {
      return false;
    }
    if (count == 0) {
      return true;
    }
    if (bytes != nullptr) {
      std::memcpy(data, bytes + static_cast<size_t>(position), count);
    } else {
      stream->read(reinterpret_cast<char*>(data), static_cast<std::streamsize>(count));
      if (stream->gcount() != static_cast<std::streamsize>(count)) {
        return false;
      }
    }
    if (checksum) {
      XXH64_update(hash.value, data, count);
    }
    position += count;
    return true;
  }

  bool gap(uint64_t end, bool must_be_zero) {
    std::array<unsigned char, kIoBufferBytes> buffer{};
    while (position < end) {
      size_t n = static_cast<size_t>((std::min)(end - position, uint64_t{buffer.size()}));
      if (!read(buffer.data(), n)) {
        return false;
      }
      if (must_be_zero &&
          std::find_if(buffer.data(), buffer.data() + n, [](unsigned char c) { return c != 0; }) != buffer.data() + n) {
        return false;
      }
    }
    return position == end;
  }
};

// Wire directory row: type:u32@0, flags:u32@4, offset:u64@8, size:u64@16, count:u64@24.
// Serialize each field explicitly: native C++ alignment/padding is never part of the file.
struct ATFW_UTIL_SYMBOL_LOCAL section {
  uint32_t type;
  uint32_t flags;
  uint64_t offset;
  uint64_t size;
  uint64_t count;
};

static uint64_t align_image(uint64_t n) { return (n + kImageAlignmentBytes - 1) & ~uint64_t{kImageAlignmentBytes - 1}; }

static std::array<section, kRequiredSectionCount> sections_for(const detail::ac_v2_storage& s) {
  std::array<section, kRequiredSectionCount> result{
      {{kSectionNodes, kSectionRequiredFlags, 0, s.nodes.size() * uint64_t{kNodeStrideBytes}, s.nodes.size()},
       {kSectionLabels, kSectionRequiredFlags, 0, s.labels.size() * uint64_t{kSerializedIndexBytes}, s.labels.size()},
       {kSectionSymbolOffsets, kSectionRequiredFlags, 0, s.offsets.size() * uint64_t{kSerializedIndexBytes},
        s.offsets.size()},
       {kSectionSymbolBytes, kSectionRequiredFlags, 0, s.symbol_bytes.size(), s.symbol_bytes.size()},
       {kSectionSkipBits, kSectionRequiredFlags, 0, s.skips.size(), s.skips.size()},
       {kSectionKeywords, kSectionRequiredFlags, 0, s.words.size() * uint64_t{kKeywordStrideBytes}, s.words.size()},
       {kSectionKeywordBytes, kSectionRequiredFlags, 0, s.keyword_bytes.size(), s.keyword_bytes.size()},
       {kSectionFolds, kSectionRequiredFlags, 0, s.folds.size() * uint64_t{kSerializedIndexBytes}, s.folds.size()}}};
  uint64_t offset = kImageHeaderBytes + kRequiredSectionCount * kDirectoryEntryBytes;
  for (auto& part : result) {
    part.offset = align_image(offset);
    offset = part.offset + part.size;
  }
  return result;
}

static status write_image(const detail::ac_v2_storage& s, image_writer& output) {
  // Image: header[128], directory[8*32], eight aligned sections, aligned XXH64 footer[8].
  // Header and section offsets/strides are specified in the public class documentation.
  // Hash the filled header and serialized payload while writing; the footer is excluded.
  if (s.pending != nullptr) {
    return status::kInvalidState;
  }
  if (!s.has_basic_layout()) {
    return status::kInvalidFormat;
  }
  auto directory = sections_for(s);
  uint64_t checksum_offset = align_image(directory.back().offset + directory.back().size);
  std::array<unsigned char, kImageHeaderBytes> header{};
  std::memcpy(header.data(), kImageMagic, kImageMagicBytes);
  bit::write_le_uint16(header.data() + kHeaderMajorOffset, kImageMajorVersion);
  bit::write_le_uint16(header.data() + kHeaderMinorOffset, kImageMinorVersion);
  bit::write_le_uint32(header.data() + kHeaderSizeOffset, kImageHeaderBytes);
  bit::write_le_uint32(header.data() + kHeaderFlagsOffset, s.nocase ? kFeatureNoCase : 0);
  bit::write_le_uint32(header.data() + kHeaderEndianOffset, kImageEndianTag);
  bit::write_le_uint32(header.data() + kHeaderSectionCountOffset, kRequiredSectionCount);
  bit::write_le_uint64(header.data() + kHeaderFileSizeOffset, checksum_offset + kImageFooterBytes);
  bit::write_le_uint64(header.data() + kHeaderDirectoryOffset, kImageHeaderBytes);
  bit::write_le_uint32(header.data() + kHeaderNodeCountOffset, static_cast<uint32_t>(s.nodes.size()));
  bit::write_le_uint32(header.data() + kHeaderKeywordCountOffset, static_cast<uint32_t>(s.words.size()));
  bit::write_le_uint32(header.data() + kHeaderSymbolCountOffset, static_cast<uint32_t>(s.offsets.size() - 1));
  bit::write_le_uint32(header.data() + kHeaderMaxTokensOffset, s.max_tokens);
  bit::write_le_uint16(header.data() + kHeaderNodeStrideOffset, kNodeStrideBytes);
  bit::write_le_uint16(header.data() + kHeaderLabelWidthOffset, kSerializedIndexBytes);
  bit::write_le_uint16(header.data() + kHeaderKeywordStrideOffset, kKeywordStrideBytes);
  bit::write_le_uint16(header.data() + kHeaderSymbolWidthOffset, kSerializedIndexBytes);
  bit::write_le_uint32(header.data() + kHeaderChecksumAlgorithmOffset, kChecksumAlgorithmXxh64);
  bit::write_le_uint64(header.data() + kHeaderChecksumOffset, checksum_offset);
  bit::write_le_uint64(header.data() + kHeaderHashOffset, XXH64(header.data(), header.size(), kXxh64Seed));
  if (!output.write(header.data(), header.size())) {
    return status::kIoError;
  }
  for (const auto& part : directory) {
    unsigned char entry[kDirectoryEntryBytes];
    bit::write_le_uint32(entry, part.type);
    bit::write_le_uint32(entry + kDirectoryFlagsOffset, part.flags);
    bit::write_le_uint64(entry + kDirectoryDataOffset, part.offset);
    bit::write_le_uint64(entry + kDirectorySizeOffset, part.size);
    bit::write_le_uint64(entry + kDirectoryCountOffset, part.count);
    if (!output.write(entry, sizeof(entry))) {
      return status::kIoError;
    }
  }
  std::array<unsigned char, kIoBufferBytes> buffer{};
  for (const auto& part : directory) {
    if (!output.padding(part.offset)) {
      return status::kIoError;
    }
    if (part.type == kSectionSymbolBytes || part.type == kSectionSkipBits || part.type == kSectionKeywordBytes) {
      const unsigned char* raw =
          part.type == kSectionSymbolBytes    ? reinterpret_cast<const unsigned char*>(s.symbol_bytes.data())
          : part.type == kSectionKeywordBytes ? reinterpret_cast<const unsigned char*>(s.keyword_bytes.data())
                                              : s.skips.data();
      if (part.size > 0 && raw == nullptr) {
        return status::kInvalidFormat;
      }
      for (size_t pos = 0; pos < part.size;) {
        size_t n = (std::min)(static_cast<size_t>(part.size) - pos, buffer.size());
        if (n == 0 || !output.write(raw + pos, n)) {
          return status::kIoError;
        }
        pos += n;
      }
    } else {
      static_assert(kNodeStrideBytes > 0 && kNodeStrideBytes <= kIoBufferBytes, "Node fits serialization buffer");
      static_assert(kKeywordStrideBytes > 0 && kKeywordStrideBytes <= kIoBufferBytes,
                    "Keyword fits serialization buffer");
      static_assert(kSerializedIndexBytes > 0 && kSerializedIndexBytes <= kIoBufferBytes,
                    "Index fits serialization buffer");
      const size_t stride = part.type == kSectionNodes      ? kNodeStrideBytes
                            : part.type == kSectionKeywords ? kKeywordStrideBytes
                                                            : kSerializedIndexBytes;
      for (size_t first = 0; first < part.count;) {
        size_t count = (std::min)(static_cast<size_t>(part.count) - first, buffer.size() / stride);
        if (count == 0) {
          return status::kInvalidFormat;
        }
        for (size_t j = 0; j < count; ++j) {
          auto p = buffer.data() + j * stride;
          size_t i = first + j;
          if (part.type == kSectionNodes) {
            if (i >= s.nodes.size()) {
              return status::kInvalidFormat;
            }
            const auto& n = s.nodes[i];
            bit::write_le_uint32(p, n.begin);
            bit::write_le_uint32(p + kNodeChildCountOffset, n.count);
            bit::write_le_uint32(p + kNodeFailureOffset, n.fail);
            bit::write_le_uint32(p + kNodeOutputOffset, n.output);
          } else if (part.type == kSectionKeywords) {
            if (i >= s.words.size()) {
              return status::kInvalidFormat;
            }
            const auto& w = s.words[i];
            bit::write_le_uint32(p, w.offset);
            bit::write_le_uint32(p + kKeywordLengthOffset, w.length);
            bit::write_le_uint32(p + kKeywordTokensOffset, w.tokens);
            bit::write_le_uint32(p + kKeywordSuffixOffset, w.suffix);
          } else {
            if ((part.type != kSectionLabels && part.type != kSectionSymbolOffsets && part.type != kSectionFolds) ||
                (part.type == kSectionLabels && i >= s.labels.size()) ||
                (part.type == kSectionSymbolOffsets && i >= s.offsets.size()) ||
                (part.type == kSectionFolds && i >= s.folds.size())) {
              return status::kInvalidFormat;
            }
            bit::write_le_uint32(p, part.type == kSectionLabels          ? s.labels[i]
                                    : part.type == kSectionSymbolOffsets ? s.offsets[i]
                                                                         : s.folds[i]);
          }
        }
        if (!output.write(buffer.data(), count * stride)) {
          return status::kIoError;
        }
        first += count;
      }
    }
  }
  if (!output.padding(checksum_offset)) {
    return status::kIoError;
  }
  unsigned char digest[kImageFooterBytes];
  bit::write_le_uint64(digest, XXH64_digest(output.hash.value));
  return output.write(digest, sizeof(digest), false) ? status::kOk : status::kIoError;
}

static status validate_storage(const detail::ac_v2_storage& s, bool semantic) {
  // Structural checks precede any traversal that dereferences failure or output IDs.
  if (!s.has_basic_layout()) {
    return status::kInvalidFormat;
  }
  const size_t n = s.nodes.size();
  const size_t k = s.words.size();
  const size_t symbols = s.offsets.size() - 1;
  if (s.offsets.front() != 0 || s.offsets.back() != s.symbol_bytes.size() || s.labels[0] != 0 || s.nodes[0].fail != 0 ||
      s.nodes[0].output != kInvalidId) {
    return status::kInvalidFormat;
  }
  // Nonempty, strictly sorted byte slices make binary symbol lookup well-defined.
  for (size_t i = 1; i <= symbols; ++i) {
    if (s.offsets[i] <= s.offsets[i - 1] || s.offsets[i] > s.symbol_bytes.size()) {
      return status::kInvalidFormat;
    }
    if (i > 1 && !byte_less{}(s.symbol(static_cast<uint32_t>(i - 1)), s.symbol(static_cast<uint32_t>(i)))) {
      return status::kInvalidFormat;
    }
  }
  if (symbols % kBitsPerByte != 0 && (s.skips.back() >> (symbols % kBitsPerByte)) != 0) {
    return status::kInvalidFormat;
  }
  if (s.nocase) {
    for (size_t i = 0; i < symbols; ++i) {
      uint32_t target = s.folds[i];
      if (target == kUnknownSymbolId || target > symbols || s.folds[target - 1] != target) {
        return status::kInvalidFormat;
      }
      if (s.skipped(static_cast<uint32_t>(i + 1)) && target != i + 1) {
        return status::kInvalidFormat;
      }
    }
  }
  // Keyword ranges partition the original byte pool; suffix chains decrease in token length.
  uint64_t end = 0;
  uint32_t maximum = 0;
  for (const auto& word : s.words) {
    if (word.offset != end || word.length == 0 || word.tokens == 0 || word.tokens > word.length ||
        word.length > s.keyword_bytes.size() - (std::min)(static_cast<size_t>(word.offset), s.keyword_bytes.size())) {
      return status::kInvalidFormat;
    }
    end += word.length;
    maximum = (std::max)(maximum, word.tokens);
    if (word.suffix != kInvalidId && (word.suffix >= k || s.words[word.suffix].tokens >= word.tokens)) {
      return status::kInvalidFormat;
    }
  }
  if (end != s.keyword_bytes.size() || maximum != s.max_tokens) {
    return status::kInvalidFormat;
  }
  // BFS child intervals partition all nonroot nodes and determine each terminal's depth.
  std::vector<uint32_t> depth(n, 0);
  std::vector<unsigned char> terminal_seen(k, 0);
  uint32_t next_child = 1;
  for (uint32_t i = 0; i < n; ++i) {
    const auto& p = s.nodes[i];
    if (p.begin != next_child || p.count > n - next_child || (p.output != kInvalidId && p.output >= k)) {
      return status::kInvalidFormat;
    }
    next_child += p.count;
    if (i > 0 && (p.fail >= i || depth[p.fail] >= depth[i])) {
      return status::kInvalidFormat;
    }
    bool own_output = p.output != kInvalidId && s.words[p.output].tokens == depth[i];
    if (own_output) {
      if (terminal_seen[p.output]++ != 0 || s.words[p.output].suffix != s.nodes[p.fail].output) {
        return status::kInvalidFormat;
      }
    } else if (p.output != s.nodes[p.fail].output || (i > 0 && p.count == 0)) {
      return status::kInvalidFormat;
    }
    if (p.output != kInvalidId && s.words[p.output].tokens > depth[i]) {
      return status::kInvalidFormat;
    }
    uint32_t previous_label = 0;
    for (uint32_t j = p.begin; j < p.begin + p.count; ++j) {
      uint32_t label = s.labels[j];
      if (j <= i || label <= previous_label || label > symbols || s.skipped(label) ||
          (s.nocase && s.folds[label - 1] != label)) {
        return status::kInvalidFormat;
      }
      previous_label = label;
      depth[j] = depth[i] + 1;
      // Optional semantic validation recomputes the longest suffix, beyond safe indexing alone.
      if (semantic) {
        uint32_t fail = kRootNodeId;
        uint32_t ancestor = p.fail;
        if (i > 0) {
          fail = s.child(ancestor, label);
          while (fail == kRootNodeId && ancestor != kRootNodeId) {
            ancestor = s.nodes[ancestor].fail;
            fail = s.child(ancestor, label);
          }
        }
        if (s.nodes[j].fail != fail) {
          return status::kInvalidFormat;
        }
      }
    }
  }
  if (next_child != n || std::find(terminal_seen.begin(), terminal_seen.end(), 0) != terminal_seen.end()) {
    return status::kInvalidFormat;
  }
  return status::kOk;
}

static status read_image(image_reader& input, const ac::load_options& options,
                         std::shared_ptr<detail::ac_v2_storage>& result) {
  // Verify the header checksum before trusting its counts, offsets or resource claims.
  std::array<unsigned char, kImageHeaderBytes> header{};
  if (!input.read(header.data(), header.size())) {
    return status::kIoError;
  }
  if (std::memcmp(header.data(), kImageMagic, kImageMagicBytes) != 0) {
    return status::kInvalidFormat;
  }
  if (bit::read_le_uint16(header.data() + kHeaderMajorOffset) != kImageMajorVersion) {
    return status::kUnsupportedVersion;
  }
  uint64_t header_hash = bit::read_le_uint64(header.data() + kHeaderHashOffset);
  bit::write_le_uint64(header.data() + kHeaderHashOffset, 0);
  if (header_hash != XXH64(header.data(), header.size(), kXxh64Seed)) {
    return status::kChecksumMismatch;
  }
  uint32_t flags = bit::read_le_uint32(header.data() + kHeaderFlagsOffset);
  uint32_t count = bit::read_le_uint32(header.data() + kHeaderSectionCountOffset);
  uint64_t file_size = bit::read_le_uint64(header.data() + kHeaderFileSizeOffset);
  uint64_t checksum_offset = bit::read_le_uint64(header.data() + kHeaderChecksumOffset);
  uint32_t n = bit::read_le_uint32(header.data() + kHeaderNodeCountOffset);
  uint32_t k = bit::read_le_uint32(header.data() + kHeaderKeywordCountOffset);
  uint32_t symbols = bit::read_le_uint32(header.data() + kHeaderSymbolCountOffset);
  if (bit::read_le_uint32(header.data() + kHeaderSizeOffset) != kImageHeaderBytes || flags > kFeatureNoCase ||
      bit::read_le_uint32(header.data() + kHeaderEndianOffset) != kImageEndianTag || count < kRequiredSectionCount ||
      count > kMaxSectionCount || bit::read_le_uint64(header.data() + kHeaderDirectoryOffset) != kImageHeaderBytes ||
      file_size < kImageHeaderBytes + count * uint64_t{kDirectoryEntryBytes} + kImageFooterBytes ||
      checksum_offset != file_size - kImageFooterBytes || checksum_offset % kImageAlignmentBytes != 0 || n == 0 ||
      n == kInvalidId || k == kInvalidId || symbols == kInvalidId ||
      bit::read_le_uint16(header.data() + kHeaderNodeStrideOffset) != kNodeStrideBytes ||
      bit::read_le_uint16(header.data() + kHeaderLabelWidthOffset) != kSerializedIndexBytes ||
      bit::read_le_uint16(header.data() + kHeaderKeywordStrideOffset) != kKeywordStrideBytes ||
      bit::read_le_uint16(header.data() + kHeaderSymbolWidthOffset) != kSerializedIndexBytes ||
      bit::read_le_uint32(header.data() + kHeaderChecksumAlgorithmOffset) != kChecksumAlgorithmXxh64 ||
      bit::read_le_uint32(header.data() + kHeaderReservedWordOffset) != 0 ||
      std::find_if(
          header.begin() + kHeaderReservedTailOffset, header.end(), [](unsigned char c) { return c != 0; }) !=
          header.end()) {
    return status::kInvalidFormat;
  }
  if (file_size > options.max_file_bytes) {
    return status::kResourceLimit;
  }
  if (input.bytes != nullptr && file_size != input.length) {
    return status::kInvalidFormat;
  }
  // Validate the directory and total candidate/scratch budget before allocating payload tables.
  std::vector<section> directory;
  directory.reserve(count);
  std::array<section, kRequiredSectionCount> known{};
  std::array<bool, kRequiredSectionCount> seen{};
  uint64_t memory = n * uint64_t{sizeof(uint32_t)} + k +
                    kValidationFixedBytes;  // Validation arrays, I/O buffers and directory/objects.
  for (uint32_t i = 0; i < count; ++i) {
    unsigned char entry[kDirectoryEntryBytes];
    if (!input.read(entry, sizeof(entry))) {
      return status::kIoError;
    }
    section p{bit::read_le_uint32(entry), bit::read_le_uint32(entry + kDirectoryFlagsOffset),
              bit::read_le_uint64(entry + kDirectoryDataOffset), bit::read_le_uint64(entry + kDirectorySizeOffset),
              bit::read_le_uint64(entry + kDirectoryCountOffset)};
    if (p.flags > kSectionOptionalFlag || p.offset % kImageAlignmentBytes != 0 ||
        p.offset < kImageHeaderBytes + count * uint64_t{kDirectoryEntryBytes} || p.offset > checksum_offset ||
        p.size > checksum_offset - p.offset) {
      return status::kInvalidFormat;
    }
    if (p.type >= kSectionNodes && p.type <= kSectionFolds) {
      if (seen[p.type - kSectionNodes] || p.flags != 0) {
        return status::kInvalidFormat;
      }
      seen[p.type - kSectionNodes] = true;
      known[p.type - kSectionNodes] = p;
      if (p.size > SIZE_MAX || p.size > options.max_memory_bytes || memory > options.max_memory_bytes - p.size) {
        return status::kResourceLimit;
      }
      memory += p.size;
    } else if ((p.flags & kSectionOptionalFlag) == 0) {
      return status::kUnsupportedVersion;
    }
    directory.push_back(p);
  }
  if (std::find(seen.begin(), seen.end(), false) != seen.end()) {
    return status::kInvalidFormat;
  }
  // Each required section has one exact count/stride contract; optional sections stay opaque.
  const uint64_t expected_counts[kRequiredSectionCount] = {n,
                                                           n,
                                                           uint64_t{symbols} + 1,
                                                           known[kSymbolBytesSectionIndex].size,
                                                           (uint64_t{symbols} + kBitsPerByte - 1) / kBitsPerByte,
                                                           k,
                                                           known[kKeywordBytesSectionIndex].size,
                                                           flags != 0 ? symbols : 0};
  static constexpr uint64_t kSectionStrides[kRequiredSectionCount] = {
      kNodeStrideBytes, kSerializedIndexBytes, kSerializedIndexBytes, kByteStrideBytes,
      kByteStrideBytes, kKeywordStrideBytes,   kByteStrideBytes,      kSerializedIndexBytes};
  for (size_t i = 0; i < known.size(); ++i) {
    if (known[i].count != expected_counts[i] || known[i].size != expected_counts[i] * kSectionStrides[i]) {
      return status::kInvalidFormat;
    }
  }
  if (known[kSymbolBytesSectionIndex].size > UINT32_MAX || known[kKeywordBytesSectionIndex].size > UINT32_MAX ||
      memory > options.max_memory_bytes || memory > SIZE_MAX) {
    return status::kResourceLimit;
  }
  // Sort by physical position to reject overlapping sections and check gaps while streaming.
  std::sort(directory.begin(), directory.end(), [](const section& a, const section& b) {
    return a.offset < b.offset || (a.offset == b.offset && a.size < b.size);
  });
  uint64_t previous = input.position;
  for (const auto& p : directory) {
    if (p.offset < previous) {
      return status::kInvalidFormat;
    }
    previous = p.offset + p.size;
  }
  // Decode into a detached candidate so I/O, checksum and structural failures are atomic.
  auto candidate = std::make_shared<detail::ac_v2_storage>();
  candidate->pending.reset();
  candidate->nocase = flags != 0;
  candidate->max_tokens = bit::read_le_uint32(header.data() + kHeaderMaxTokensOffset);
  candidate->nodes.resize(n);
  candidate->labels.resize(n);
  candidate->offsets.resize(static_cast<size_t>(symbols) + 1);
  candidate->symbol_bytes.resize(static_cast<size_t>(known[kSymbolBytesSectionIndex].size));
  candidate->skips.resize(static_cast<size_t>(known[kSkipBitsSectionIndex].size));
  candidate->words.resize(k);
  candidate->keyword_bytes.resize(static_cast<size_t>(known[kKeywordBytesSectionIndex].size));
  candidate->folds.resize(static_cast<size_t>(expected_counts[kFoldsSectionIndex]));
  std::array<unsigned char, kIoBufferBytes> buffer{};
  for (const auto& part : directory) {
    if (!input.gap(part.offset, true)) {
      return status::kInvalidFormat;
    }
    if (part.type < kSectionNodes || part.type > kSectionFolds) {
      if (!input.gap(part.offset + part.size, false)) {
        return status::kIoError;
      }
      continue;
    }
    if (part.type == kSectionSymbolBytes || part.type == kSectionSkipBits || part.type == kSectionKeywordBytes) {
      unsigned char* raw =
          part.size == 0                      ? nullptr
          : part.type == kSectionSymbolBytes  ? reinterpret_cast<unsigned char*>(&candidate->symbol_bytes[0])
          : part.type == kSectionKeywordBytes ? reinterpret_cast<unsigned char*>(&candidate->keyword_bytes[0])
                                              : candidate->skips.data();
      if (part.size > 0 && raw == nullptr) {
        return status::kInvalidFormat;
      }
      for (size_t pos = 0; pos < part.size;) {
        size_t amount = (std::min)(static_cast<size_t>(part.size) - pos, buffer.size());
        if (amount == 0 || !input.read(raw + pos, amount)) {
          return status::kIoError;
        }
        pos += amount;
      }
    } else {
      size_t stride = static_cast<size_t>(kSectionStrides[part.type - kSectionNodes]);
      if (stride == 0 || stride > buffer.size()) {
        return status::kInvalidFormat;
      }
      for (size_t first = 0; first < part.count;) {
        size_t amount = (std::min)(static_cast<size_t>(part.count) - first, buffer.size() / stride);
        if (amount == 0 || !input.read(buffer.data(), amount * stride)) {
          return status::kIoError;
        }
        for (size_t j = 0; j < amount; ++j) {
          const auto p = buffer.data() + j * stride;
          size_t i = first + j;
          if (part.type == kSectionNodes) {
            if (i >= candidate->nodes.size()) {
              return status::kInvalidFormat;
            }
            auto& v = candidate->nodes[i];
            v.begin = bit::read_le_uint32(p);
            v.count = bit::read_le_uint32(p + kNodeChildCountOffset);
            v.fail = bit::read_le_uint32(p + kNodeFailureOffset);
            v.output = bit::read_le_uint32(p + kNodeOutputOffset);
          } else if (part.type == kSectionKeywords) {
            if (i >= candidate->words.size()) {
              return status::kInvalidFormat;
            }
            auto& v = candidate->words[i];
            v.offset = bit::read_le_uint32(p);
            v.length = bit::read_le_uint32(p + kKeywordLengthOffset);
            v.tokens = bit::read_le_uint32(p + kKeywordTokensOffset);
            v.suffix = bit::read_le_uint32(p + kKeywordSuffixOffset);
          } else if (part.type == kSectionLabels) {
            if (i >= candidate->labels.size()) {
              return status::kInvalidFormat;
            }
            candidate->labels[i] = bit::read_le_uint32(p);
          } else if (part.type == kSectionSymbolOffsets) {
            if (i >= candidate->offsets.size()) {
              return status::kInvalidFormat;
            }
            candidate->offsets[i] = bit::read_le_uint32(p);
          } else {
            if (part.type != kSectionFolds || i >= candidate->folds.size()) {
              return status::kInvalidFormat;
            }
            candidate->folds[i] = bit::read_le_uint32(p);
          }
        }
        first += amount;
      }
    }
  }
  // Verify the complete image before interpreting its graph or publishing borrowed views.
  if (!input.gap(checksum_offset, true)) {
    return status::kInvalidFormat;
  }
  unsigned char digest[kImageFooterBytes];
  if (!input.read(digest, sizeof(digest), false)) {
    return status::kIoError;
  }
  if (bit::read_le_uint64(digest) != XXH64_digest(input.hash.value)) {
    return status::kChecksumMismatch;
  }
  // Check all indices/child intervals before semantic traversal calls child().
  auto valid = validate_storage(*candidate, false);
  if (valid == status::kOk && options.verify_semantics) {
    valid = validate_storage(*candidate, true);
  }
  if (valid == status::kOk && !candidate->initialize_symbol_index()) {
    valid = status::kInvalidFormat;
  }
  if (valid == status::kOk) {
    result.swap(candidate);
  }
  return valid;
}
}  // namespace

status ac::dump(std::ostream& output) const {
  if (!is_built()) {
    return status::kInvalidState;
  }
  image_writer writer{&output, nullptr};
  return write_image(*storage_, writer);
}

status ac::dump(std::vector<unsigned char>& output) const {
  if (!is_built()) {
    return status::kInvalidState;
  }
  if (!storage_->has_basic_layout()) {
    return status::kInvalidFormat;
  }
  std::vector<unsigned char> candidate;
  auto directory = sections_for(*storage_);
  uint64_t size = align_image(directory.back().offset + directory.back().size) + kImageFooterBytes;
  if (size > SIZE_MAX) {
    return status::kResourceLimit;
  }
  candidate.reserve(static_cast<size_t>(size));
  image_writer writer{nullptr, &candidate};
  auto result = write_image(*storage_, writer);
  if (result == status::kOk) {
    output.swap(candidate);
  }
  return result;
}

status ac::load(std::istream& input, const load_options& options) {
  image_reader reader{&input, nullptr, 0};
  return read_image(reader, options, storage_);
}

status ac::load(gsl::span<const unsigned char> input, const load_options& options) {
  if (input.size() < kImageHeaderBytes) {
    return status::kInvalidFormat;
  }
  image_reader reader{nullptr, input.data(), input.size()};
  return read_image(reader, options, storage_);
}

status ac::load(gsl::string_view input, const load_options& options) {
  return load(gsl::span<const unsigned char>(reinterpret_cast<const unsigned char*>(input.data()), input.size()),
              options);
}
}  // namespace string

ATFRAMEWORK_UTILS_NAMESPACE_END
