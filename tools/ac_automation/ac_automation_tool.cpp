// Copyright 2026 atframework
#include "cli/cmd_option.h"
#include "common/file_system.h"
#include "string/ac_automation_v2.h"
#include "unicode_ranges.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <streambuf>
#include <string>
#include <vector>

namespace {
using ac = atfw::util::string::ac_automation_v2;
using fs = atfw::util::file_system;
using cli = atfw::util::cli::cmd_option;
using params = atfw::util::cli::callback_param;

// Encoding, property and command-line bounds are shared by parsing and output.
static constexpr int kExitSuccess = 0;
static constexpr int kExitFailure = 1;
static constexpr int kCommandArgumentCount = 2;
static constexpr int kCommandArgumentIndex = 1;
static constexpr int kFirstOptionArgumentIndex = 2;
static constexpr size_t kValueOptionParameterCount = 1;
static constexpr uint64_t kDefaultMaxGraphNodes = 1000;
static constexpr int kDecimalRadix = 10;
static constexpr int kHexLetterValueBase = 10;
static constexpr int kInvalidHexDigit = -1;
static constexpr int kStreamSyncSuccess = 0;
static constexpr int kStreamSyncFailure = -1;
static constexpr size_t kHexDigitsPerByte = 2;
static constexpr size_t kHexBitsPerDigit = 4;
static constexpr unsigned char kHexDigitMask = 0x0f;
static constexpr char kHexDigits[] = "0123456789ABCDEF";
static constexpr size_t kUtf8TwoByteLength = 2;
static constexpr size_t kUtf8ThreeByteLength = 3;
static constexpr uint32_t kUtf8TwoBytePayloadMask = 0x1f;
static constexpr uint32_t kUtf8ThreeBytePayloadMask = 0x0f;
static constexpr uint32_t kUtf8FourBytePayloadMask = 0x07;
static constexpr size_t kUtf8ContinuationBits = 6;
static constexpr size_t kUtf8TwoContinuationBits = 2 * kUtf8ContinuationBits;
static constexpr size_t kUtf8ThreeContinuationBits = 3 * kUtf8ContinuationBits;
static constexpr uint32_t kUtf8ContinuationPayloadMask = 0x3f;
static constexpr uint32_t kUtf8SingleByteLimit = 0x80;
static constexpr uint32_t kUtf8TwoByteLimit = 0x800;
static constexpr uint32_t kUtf8ThreeByteLimit = 0x10000;
static constexpr uint32_t kUtf8TwoBytePrefix = 0xc0;
static constexpr uint32_t kUtf8ThreeBytePrefix = 0xe0;
static constexpr uint32_t kUtf8FourBytePrefix = 0xf0;
static constexpr uint32_t kUtf8ContinuationPrefix = 0x80;
static constexpr uint32_t kPropertyWhiteSpace = 1;
static constexpr uint32_t kPropertyPunctuation = 2;
static constexpr uint32_t kPropertySymbols = 4;
static constexpr uint32_t kPropertyInvisible = 8;

// ASCII boundaries and byte markers.
static constexpr char kAsciiDigitFirst = '0';
static constexpr char kAsciiDigitLast = '9';
static constexpr char kAsciiLowerFirst = 'a';
static constexpr char kAsciiLowerHexLast = 'f';
static constexpr char kAsciiLowerLast = 'z';
static constexpr char kAsciiUpperFirst = 'A';
static constexpr char kAsciiUpperHexLast = 'F';
static constexpr char kAsciiUpperLast = 'Z';
static constexpr char kHexPrefix[] = "0x";
static constexpr char kLabelSeparator = ' ';
static constexpr char kLineFeed = '\n';
static constexpr char kTemporarySuffix[] = ".tmp.";
static constexpr char kUtf8Bom[] = "\xef\xbb\xbf";
static constexpr size_t kUtf8BomBytes = sizeof(kUtf8Bom) - 1;

// Commands, option keys and parser events.
static constexpr char kCommandBuild[] = "build";
static constexpr char kCommandInfo[] = "info";
static constexpr char kCommandVerify[] = "verify";
static constexpr char kCommandVisualize[] = "visualize";
static constexpr char kOptionCaseFoldFile[] = "--case-fold-file";
static constexpr char kOptionFormat[] = "--format";
static constexpr char kOptionHelp[] = "--help";
static constexpr char kOptionIgnoreCase[] = "--ignore-case";
static constexpr char kOptionInput[] = "--input";
static constexpr char kOptionLabelFormat[] = "--label-format";
static constexpr char kOptionMaxDepth[] = "--max-depth";
static constexpr char kOptionMaxFileBytes[] = "--max-file-bytes";
static constexpr char kOptionMaxMemoryBytes[] = "--max-memory-bytes";
static constexpr char kOptionMaxNodes[] = "--max-nodes";
static constexpr char kOptionNoCase[] = "--nocase";
static constexpr char kOptionOutput[] = "--output";
static constexpr char kOptionRootId[] = "--root-id";
static constexpr char kOptionSemantic[] = "--semantic";
static constexpr char kOptionSkipFile[] = "--skip-file";
static constexpr char kOptionSkipInvisible[] = "--skip-invisible";
static constexpr char kOptionSkipPunctuation[] = "--skip-punctuation";
static constexpr char kOptionSkipSpace[] = "--skip-space";
static constexpr char kOptionSkipSymbols[] = "--skip-symbols";
static constexpr char kOptionUtf8Labels[] = "--utf8-labels";
static constexpr char kParserDefaultEvent[] = "@OnDefault";
static constexpr char kParserErrorEvent[] = "@OnError";

// Operation labels and diagnostics.
static constexpr char kDiagnosticPrefix[] = "ac_automation_tool: ";
static constexpr char kDiagnosticSeparator[] = ": ";
static constexpr char kErrorBuild[] = "build failed";
static constexpr char kErrorConfiguration[] = "invalid skip/fold configuration or unreadable configuration file";
static constexpr char kErrorCreateOutput[] = "cannot create temporary output";
static constexpr char kErrorFoldRequiresNoCase[] = "--case-fold-file requires --nocase";
static constexpr char kErrorFormat[] = "invalid format";
static constexpr char kErrorGraphBound[] = "invalid graph bound";
static constexpr char kErrorGraphLimit[] =
    "selected graph exceeds max-nodes; narrow root-id/max-depth or increase the limit";
static constexpr char kErrorGraphRange[] = "graph bound out of range";
static constexpr char kErrorGraphWrite[] = "graph write failed";
static constexpr char kErrorInvalidUtf8Line[] = "invalid UTF-8 on line ";
static constexpr char kErrorLabelFormat[] = "invalid or conflicting label format";
static constexpr char kErrorMalformedOption[] = "unknown, repeated, or malformed option";
static constexpr char kErrorMissingFormat[] = "--format is required";
static constexpr char kErrorMissingInput[] = "--input is required";
static constexpr char kErrorMissingOutput[] = "--output is required";
static constexpr char kErrorMissingRoot[] = "root-id does not exist";
static constexpr char kErrorOpenInput[] = "cannot open input";
static constexpr char kErrorPublishOutput[] = "cannot publish output; destination must not already exist";
static constexpr char kErrorResourceLimit[] = "invalid resource limit";
static constexpr char kErrorTrailingData[] = "trailing data after image";
static constexpr char kErrorUnknownCommand[] = "unknown command";
static constexpr char kOperationDump[] = "dump";
static constexpr char kOperationFold[] = "fold";
static constexpr char kOperationInsert[] = "insert";
static constexpr char kOperationLoad[] = "load";
static constexpr char kOperationNoCase[] = "nocase";
static constexpr char kOperationSkip[] = "skip";

// Report fields and verification messages.
static constexpr char kDuplicatesField[] = " duplicates=";
static constexpr char kIgnoredField[] = " ignored=";
static constexpr char kImageInfoPrefix[] = "version=2.0 nodes=";
static constexpr char kInsertedField[] = "inserted=";
static constexpr char kKeywordsField[] = " keywords=";
static constexpr char kMaxKeywordTokensField[] = " max_keyword_tokens=";
static constexpr char kNoCaseField[] = " nocase=";
static constexpr char kNodesField[] = " nodes=";
static constexpr char kPayloadBytesField[] = " payload_bytes=";
static constexpr char kSemanticVerificationSuffix[] = " (semantic)\n";
static constexpr char kStructuralVerificationSuffix[] = " (structural)\n";
static constexpr char kSymbolsField[] = " symbols=";
static constexpr char kVerifiedMessage[] = "verified";

// Graph formats and output syntax.
static constexpr char kDotEdge[] = " -> n";
static constexpr char kDotEpilogue[] = "}\n";
static constexpr char kDotFailureEdgeEnd[] = " [style=dashed,label=\"fail\"];\n";
static constexpr char kDotLabelBegin[] = " [label=\"";
static constexpr char kDotPreamble[] = "digraph AC {\n  rankdir=LR;\n";
static constexpr char kDotStatementEnd[] = "\"];\n";
static constexpr char kFormatDot[] = "dot";
static constexpr char kFormatMermaid[] = "mermaid";
static constexpr char kFormatPlantUml[] = "plantuml";
static constexpr char kIndentedNodePrefix[] = "  n";
static constexpr char kLabelFormatHex[] = "hex";
static constexpr char kLabelFormatUtf8[] = "utf8";
static constexpr char kMermaidEdgeLabelBegin[] = " -->|\"";
static constexpr char kMermaidEdgeLabelEnd[] = "\"| n";
static constexpr char kMermaidFailureEdge[] = " -. fail .-> n";
static constexpr char kMermaidLabelBegin[] = "[\"";
static constexpr char kMermaidLabelEnd[] = "\"]\n";
static constexpr char kMermaidPreamble[] = "flowchart LR\n";
static constexpr char kNodeCaptionPrefix[] = "node ";
static constexpr char kNodeIdPrefix[] = "n";
static constexpr char kOutputCaptionPrefix[] = " output ";
static constexpr char kPlantUmlEdge[] = " --> n";
static constexpr char kPlantUmlEpilogue[] = "@enduml\n";
static constexpr char kPlantUmlFailureEdge[] = " ..> n";
static constexpr char kPlantUmlFailureLabel[] = " : fail\n";
static constexpr char kPlantUmlLabelSeparator[] = " : ";
static constexpr char kPlantUmlPreamble[] = "@startuml\nleft to right direction\n";
static constexpr char kPlantUmlStateAlias[] = "\" as n";
static constexpr char kPlantUmlStateBegin[] = "state \"";

static constexpr char kUsageText[] =
    "Usage: ac_automation_tool <build|info|verify|visualize> --input FILE [options]\n"
    "  build: --output FILE [--nocase|--ignore-case] [--skip-space] [--skip-punctuation]\n"
    "         [--skip-symbols] [--skip-invisible] [--skip-file FILE] [--case-fold-file FILE]\n"
    "  verify: [--semantic]\n"
    "  visualize: --output FILE --format dot|mermaid|plantuml\n"
    "             [--max-nodes N (1000)] [--root-id N (0)] [--max-depth N] [--label-format hex|utf8]\n"
    "  info/verify/visualize: [--max-file-bytes N] [--max-memory-bytes N] (default 1 GiB)\n"
    "Output files must not exist. Options use separate values, not --name=value.\n";

struct ATFW_UTIL_SYMBOL_LOCAL arguments {
  std::map<std::string, std::string> values;
  std::set<std::string> flags;
  bool invalid = false;
};

static void parse_value(params p, arguments* a, const char* name) {
  if (p.get_params_number() != kValueOptionParameterCount || !a->values.emplace(name, p[0]->to_cpp_string()).second) {
    a->invalid = true;
  }
}

static void parse_flag(params p, arguments* a, const char* name) {
  if (p.get_params_number() != 0 || !a->flags.insert(name).second) {
    a->invalid = true;
  }
}

static void parse_error(params p, arguments* a) {
  if (p.get_params_number() > 0) {
    a->invalid = true;
  }
}

static bool number(const std::string& s, uint64_t& output) {
  if (s.empty()) {
    return false;
  }
  output = 0;
  for (char c : s) {
    if (c < kAsciiDigitFirst || c > kAsciiDigitLast ||
        output > (UINT64_MAX - static_cast<unsigned>(c - kAsciiDigitFirst)) / kDecimalRadix) {
      return false;
    }
    output = output * kDecimalRadix + static_cast<unsigned>(c - kAsciiDigitFirst);
  }
  return true;
}

static int fail(const std::string& message) {
  std::cerr << kDiagnosticPrefix << message << kLineFeed;
  return kExitFailure;
}

static bool check(ac::status s, const char* operation) {
  if (s == ac::status::kOk) {
    return true;
  }
  auto message = ac::status_message(s);
  std::cerr << operation << kDiagnosticSeparator << std::string(message.data(), message.size()) << kLineFeed;
  return false;
}

static uint32_t decode(gsl::string_view token) {
  const auto* p = reinterpret_cast<const unsigned char*>(token.data());
  uint32_t cp = token.size() == 1 ? p[0]
                                  : p[0] & (token.size() == kUtf8TwoByteLength     ? kUtf8TwoBytePayloadMask
                                            : token.size() == kUtf8ThreeByteLength ? kUtf8ThreeBytePayloadMask
                                                                                   : kUtf8FourBytePayloadMask);
  for (size_t i = 1; i < token.size(); ++i) {
    cp = (cp << kUtf8ContinuationBits) | (p[i] & kUtf8ContinuationPayloadMask);
  }
  return cp;
}

static std::string encode(uint32_t cp) {
  std::string result;
  if (cp < kUtf8SingleByteLimit) {
    result += static_cast<char>(cp);
  } else if (cp < kUtf8TwoByteLimit) {
    result += static_cast<char>(kUtf8TwoBytePrefix | (cp >> kUtf8ContinuationBits));
    result += static_cast<char>(kUtf8ContinuationPrefix | (cp & kUtf8ContinuationPayloadMask));
  } else if (cp < kUtf8ThreeByteLimit) {
    result += static_cast<char>(kUtf8ThreeBytePrefix | (cp >> kUtf8TwoContinuationBits));
    result +=
        static_cast<char>(kUtf8ContinuationPrefix | ((cp >> kUtf8ContinuationBits) & kUtf8ContinuationPayloadMask));
    result += static_cast<char>(kUtf8ContinuationPrefix | (cp & kUtf8ContinuationPayloadMask));
  } else {
    result += static_cast<char>(kUtf8FourBytePrefix | (cp >> kUtf8ThreeContinuationBits));
    result +=
        static_cast<char>(kUtf8ContinuationPrefix | ((cp >> kUtf8TwoContinuationBits) & kUtf8ContinuationPayloadMask));
    result +=
        static_cast<char>(kUtf8ContinuationPrefix | ((cp >> kUtf8ContinuationBits) & kUtf8ContinuationPayloadMask));
    result += static_cast<char>(kUtf8ContinuationPrefix | (cp & kUtf8ContinuationPayloadMask));
  }
  return result;
}

static uint32_t properties(uint32_t cp) {
  auto first = std::begin(kUnicodeSkipRanges);
  auto last = std::end(kUnicodeSkipRanges);
  auto it = std::lower_bound(first, last, cp, [](const unicode_range& range, uint32_t v) { return range.last < v; });
  return it != last && it->first <= cp ? it->mask : 0;
}

static bool trim_line(const std::string& line, gsl::string_view& trimmed) {
  size_t first = line.size();
  size_t end = 0;
  for (size_t pos = 0; pos < line.size();) {
    auto token = ac::next_utf8(gsl::string_view(line).substr(pos));
    if (token.state != ac::token_status::kValid) {
      return false;
    }
    if ((properties(decode(gsl::string_view(line).substr(pos, token.byte_count))) & kPropertyWhiteSpace) == 0) {
      first = (std::min)(first, pos);
      end = pos + token.byte_count;
    }
    pos += token.byte_count;
  }
  trimmed = end > 0 ? gsl::string_view(line).substr(first, end - first) : gsl::string_view{};
  return true;
}

static bool hex_decode(const std::string& input, std::string& result) {
  if (input.empty() || input.size() % kHexDigitsPerByte != 0) {
    return false;
  }
  result.clear();
  auto digit = [](char c) {
    return c >= kAsciiDigitFirst && c <= kAsciiDigitLast      ? c - kAsciiDigitFirst
           : c >= kAsciiLowerFirst && c <= kAsciiLowerHexLast ? c - kAsciiLowerFirst + kHexLetterValueBase
           : c >= kAsciiUpperFirst && c <= kAsciiUpperHexLast ? c - kAsciiUpperFirst + kHexLetterValueBase
                                                              : kInvalidHexDigit;
  };
  for (size_t i = 0; i < input.size(); i += kHexDigitsPerByte) {
    int a = digit(input[i]);
    int b = digit(input[i + 1]);
    if (a < 0 || b < 0) {
      return false;
    }
    result += static_cast<char>((a << kHexBitsPerDigit) | b);
  }
  return true;
}

static bool configure(ac& value, const arguments& args) {
  std::vector<std::string> owned_skips;
  uint32_t mask = ((args.flags.count(kOptionSkipSpace) > 0) ? kPropertyWhiteSpace : 0u) |
                  ((args.flags.count(kOptionSkipPunctuation) > 0) ? kPropertyPunctuation : 0u) |
                  ((args.flags.count(kOptionSkipSymbols) > 0) ? kPropertySymbols : 0u) |
                  ((args.flags.count(kOptionSkipInvisible) > 0) ? kPropertyInvisible : 0u);
  for (const auto& range : kUnicodeSkipRanges) {
    if ((range.mask & mask) != 0) {
      for (uint32_t cp = range.first; cp <= range.last; ++cp) {
        owned_skips.push_back(encode(cp));
      }
    }
  }
  auto skip_file = args.values.find(kOptionSkipFile);
  if (skip_file != args.values.end()) {
    std::ifstream file(skip_file->second, std::ios::binary);
    if (file.fail()) {
      return false;
    }
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (file.bad()) {
      return false;
    }
    if (content.size() >= kUtf8BomBytes && content.compare(0, kUtf8BomBytes, kUtf8Bom) == 0) {
      content.erase(0, kUtf8BomBytes);
    }
    for (size_t pos = 0; pos < content.size();) {
      auto token = ac::next_utf8(gsl::string_view(content).substr(pos));
      if (token.state != ac::token_status::kValid) {
        return false;
      }
      owned_skips.emplace_back(content.data() + pos, token.byte_count);
      pos += token.byte_count;
    }
  }
  std::vector<gsl::string_view> skips;
  for (const auto& token : owned_skips) {
    skips.emplace_back(token);
  }
  if (!check(value.set_skip(gsl::span<const gsl::string_view>(skips.data(), skips.size())), kOperationSkip)) {
    return false;
  }
  auto folds_file = args.values.find(kOptionCaseFoldFile);
  if (folds_file != args.values.end()) {
    std::ifstream file(folds_file->second, std::ios::binary);
    if (file.fail()) {
      return false;
    }
    std::vector<std::pair<std::string, std::string>> owned;
    std::string line;
    while (!std::getline(file, line).fail()) {
      std::istringstream words(line);
      std::string from;
      std::string to;
      std::string extra;
      if ((words >> from).fail()) {
        continue;
      }
      if ((words >> to).fail() || !(words >> extra).fail()) {
        return false;
      }
      std::string a;
      std::string b;
      if (!hex_decode(from, a) || !hex_decode(to, b)) {
        return false;
      }
      owned.emplace_back(std::move(a), std::move(b));
    }
    if (file.bad()) {
      return false;
    }
    std::vector<ac::case_fold_rule> rules;
    for (const auto& rule : owned) {
      rules.push_back({rule.first, rule.second});
    }
    if (!check(value.set_case_folding(gsl::span<const ac::case_fold_rule>(rules.data(), rules.size())),
               kOperationFold)) {
      return false;
    }
  }
  return check(value.set_nocase(args.flags.count(kOptionNoCase) != 0), kOperationNoCase);
}

class ATFW_UTIL_SYMBOL_LOCAL file_buffer : public std::streambuf {
 public:
  explicit file_buffer(FILE* file) : file_(file) {}

 protected:
  std::streamsize xsputn(const char* data, std::streamsize size) override {
    return static_cast<std::streamsize>(std::fwrite(data, 1, static_cast<size_t>(size), file_));
  }

  int_type overflow(int_type c) override {
    if (traits_type::eq_int_type(c, traits_type::eof())) {
      return traits_type::not_eof(c);
    }
    return std::fputc(traits_type::to_char_type(c), file_) == EOF ? traits_type::eof() : c;
  }

  int sync() override { return std::fflush(file_) == 0 ? kStreamSyncSuccess : kStreamSyncFailure; }

 private:
  FILE* file_;
};

class ATFW_UTIL_SYMBOL_LOCAL publication {
 public:
  explicit publication(const std::string& destination) : destination_(destination) {
    temporary_ = destination + kTemporarySuffix;
    if (!fs::generate_tmp_file_name(temporary_)) {
      return;
    }
    file_ = fs::open_exclusive(temporary_);
  }

  ~publication() {
    if (file_ != nullptr) {
      std::fclose(file_);
    }
    if (created_) {
      fs::remove(temporary_.c_str());
    }
  }

  FILE* file() {
    if (file_ != nullptr) {
      created_ = true;
    }
    return file_;
  }

  bool commit() {
    if (file_ == nullptr) {
      return false;
    }
    int closed = std::fclose(file_);
    file_ = nullptr;
    return closed == 0 && fs::publish_noreplace(temporary_, destination_);
  }

 private:
  std::string destination_;
  std::string temporary_;
  FILE* file_ = nullptr;
  bool created_ = false;
};

static int build_file(const arguments& args) {
  ac value;
  if (!configure(value, args)) {
    return fail(kErrorConfiguration);
  }
  std::ifstream input(args.values.at(kOptionInput), std::ios::binary);
  if (input.fail()) {
    return fail(kErrorOpenInput);
  }
  std::string line;
  size_t lines = 0;
  size_t inserted = 0;
  size_t duplicates = 0;
  size_t ignored = 0;
  while (!std::getline(input, line).fail()) {
    ++lines;
    if (lines == 1 && line.size() >= kUtf8BomBytes && line.compare(0, kUtf8BomBytes, kUtf8Bom) == 0) {
      line.erase(0, kUtf8BomBytes);
    }
    gsl::string_view trimmed;
    if (!trim_line(line, trimmed)) {
      return fail(kErrorInvalidUtf8Line + std::to_string(lines));
    }
    if (trimmed.empty()) {
      ++ignored;
      continue;
    }
    auto result = value.insert_keyword(trimmed);
    if (result == ac::status::kAlreadyExists) {
      ++duplicates;
    } else if (result == ac::status::kEmptyKeyword) {
      ++ignored;
    } else if (!check(result, kOperationInsert)) {
      return kExitFailure;
    } else {
      ++inserted;
    }
  }
  if (input.bad() || !check(value.build(), kCommandBuild)) {
    return fail(kErrorBuild);
  }
  publication file(args.values.at(kOptionOutput));
  if (file.file() == nullptr) {
    return fail(kErrorCreateOutput);
  }
  file_buffer buffer(file.file());
  std::ostream output(&buffer);
  if (!check(value.dump(output), kOperationDump)) {
    return kExitFailure;
  }
  output.flush();
  if (output.fail() || !file.commit()) {
    return fail(kErrorPublishOutput);
  }
  std::cout << kInsertedField << inserted << kDuplicatesField << duplicates << kIgnoredField << ignored << kNodesField
            << value.get_statistics().nodes << kPayloadBytesField << value.get_statistics().payload_bytes << kLineFeed;
  return kExitSuccess;
}

struct ATFW_UTIL_SYMBOL_LOCAL graph_selection {
  uint32_t root;
  uint32_t max_depth;
  size_t limit;
  bool overflow = false;
  std::map<uint32_t, uint32_t> pending;
  std::vector<ac::node_view> nodes;

  graph_selection(uint32_t start, uint32_t depth, size_t maximum) : root(start), max_depth(depth), limit(maximum) {}
};

static bool select_node(const ac::node_view& node, void* data) {
  auto& graph = *static_cast<graph_selection*>(data);
  auto it = graph.pending.find(node.id);
  if (it == graph.pending.end()) {
    return true;
  }
  uint32_t depth = it->second;
  graph.pending.erase(it);
  if (graph.nodes.size() == graph.limit) {
    graph.overflow = true;
    return false;
  }
  graph.nodes.push_back(node);
  if (depth < graph.max_depth) {
    if (node.child_count > graph.limit - graph.nodes.size() - graph.pending.size()) {
      graph.overflow = true;
      return false;
    }
    for (uint32_t i = node.child_begin; i < node.child_begin + node.child_count; ++i) {
      graph.pending.emplace(i, depth + 1);
    }
  }
  return true;
}

static std::string label(gsl::string_view bytes, bool utf8) {
  std::string result;
  for (size_t pos = 0; pos < bytes.size();) {
    auto token = ac::next_utf8(bytes.substr(pos));
    unsigned char c = static_cast<unsigned char>(bytes[pos]);
    if (utf8 && token.state == ac::token_status::kValid &&
        (properties(decode(bytes.substr(pos, token.byte_count))) & kPropertyInvisible) == 0 &&
        (token.byte_count > 1 || (c >= kAsciiLowerFirst && c <= kAsciiLowerLast) ||
         (c >= kAsciiUpperFirst && c <= kAsciiUpperLast) || (c >= kAsciiDigitFirst && c <= kAsciiDigitLast))) {
      result.append(bytes.data() + pos, token.byte_count);
      pos += token.byte_count;
    } else {
      if (!result.empty()) {
        result += kLabelSeparator;
      }
      result += kHexPrefix;
      result += kHexDigits[c >> kHexBitsPerDigit];
      result += kHexDigits[c & kHexDigitMask];
      ++pos;
    }
  }
  return result;
}

static bool write_graph(std::ostream& out, const graph_selection& graph, const std::string& format, bool utf8) {
  std::set<uint32_t> ids;
  std::vector<const ac::node_view*> parents;
  for (const auto& n : graph.nodes) {
    ids.insert(n.id);
    if (n.child_count > 0) {
      parents.push_back(&n);
    }
  }
  if (format == kFormatDot) {
    out << kDotPreamble;
  } else if (format == kFormatMermaid) {
    out << kMermaidPreamble;
  } else {
    out << kPlantUmlPreamble;
  }
  for (const auto& n : graph.nodes) {
    std::string text = kNodeCaptionPrefix + std::to_string(n.id);
    if (n.output_head != UINT32_MAX) {
      text += kOutputCaptionPrefix + std::to_string(n.output_head);
    }
    if (format == kFormatDot) {
      out << kIndentedNodePrefix << n.id << kDotLabelBegin << text << kDotStatementEnd;
    } else if (format == kFormatMermaid) {
      out << kIndentedNodePrefix << n.id << kMermaidLabelBegin << text << kMermaidLabelEnd;
    } else {
      out << kPlantUmlStateBegin << text << kPlantUmlStateAlias << n.id << kLineFeed;
    }
  }
  for (const auto& child : graph.nodes) {
    // BFS children are contiguous; only selected parents/targets are rendered.
    auto found = std::upper_bound(parents.begin(), parents.end(), child.id,
                                  [](uint32_t id, const ac::node_view* n) { return id < n->child_begin; });
    const ac::node_view* parent = found == parents.begin() ? nullptr : *--found;
    if (parent != nullptr && child.id - parent->child_begin < parent->child_count) {
      std::string token = label(child.token, utf8);
      if (format == kFormatDot) {
        out << kIndentedNodePrefix << parent->id << kDotEdge << child.id << kDotLabelBegin << token << kDotStatementEnd;
      } else if (format == kFormatMermaid) {
        out << kIndentedNodePrefix << parent->id << kMermaidEdgeLabelBegin << token << kMermaidEdgeLabelEnd << child.id
            << kLineFeed;
      } else {
        out << kNodeIdPrefix << parent->id << kPlantUmlEdge << child.id << kPlantUmlLabelSeparator << token
            << kLineFeed;
      }
    }
    if (child.id > 0 && ids.count(child.failure) > 0) {
      if (format == kFormatDot) {
        out << kIndentedNodePrefix << child.id << kDotEdge << child.failure << kDotFailureEdgeEnd;
      } else if (format == kFormatMermaid) {
        out << kIndentedNodePrefix << child.id << kMermaidFailureEdge << child.failure << kLineFeed;
      } else {
        out << kNodeIdPrefix << child.id << kPlantUmlFailureEdge << child.failure << kPlantUmlFailureLabel;
      }
    }
  }
  if (format == kFormatDot) {
    out << kDotEpilogue;
  } else if (format == kFormatPlantUml) {
    out << kPlantUmlEpilogue;
  }
  return out.good();
}

static void usage() { std::cout << kUsageText; }
}  // namespace

int main(int argc, char* argv[]) {
  if (argc == kCommandArgumentCount && std::string(argv[kCommandArgumentIndex]) == kOptionHelp) {
    usage();
    return kExitSuccess;
  }
  if (argc < kCommandArgumentCount) {
    usage();
    return kExitFailure;
  }
  std::string command = argv[kCommandArgumentIndex];
  if (command != kCommandBuild && command != kCommandInfo && command != kCommandVerify &&
      command != kCommandVisualize) {
    return fail(kErrorUnknownCommand);
  }
  arguments args;
  auto parser = cli::create();
  parser->bind_cmd(kParserDefaultEvent, parse_error, &args);
  parser->bind_cmd(kParserErrorEvent, parse_error, &args);
  std::vector<const char*> values{kOptionInput};
  std::vector<const char*> flags{kOptionHelp};
  if (command == kCommandBuild) {
    values.insert(values.end(), {kOptionOutput, kOptionSkipFile, kOptionCaseFoldFile});
    flags.insert(flags.end(),
                 {kOptionNoCase, kOptionSkipSpace, kOptionSkipPunctuation, kOptionSkipSymbols, kOptionSkipInvisible});
  } else {
    values.insert(values.end(), {kOptionMaxFileBytes, kOptionMaxMemoryBytes});
    if (command == kCommandVerify) {
      flags.push_back(kOptionSemantic);
    }
    if (command == kCommandVisualize) {
      values.insert(values.end(), {kOptionOutput, kOptionFormat, kOptionMaxNodes, kOptionRootId, kOptionMaxDepth,
                                   kOptionLabelFormat});
      flags.push_back(kOptionUtf8Labels);
    }
  }
  for (auto name : values) {
    parser->bind_cmd(name, parse_value, &args, name);
  }
  for (auto name : flags) {
    parser->bind_cmd(name, parse_flag, &args, name);
  }
  if (command == kCommandBuild) {
    parser->bind_cmd(kOptionIgnoreCase, parse_flag, &args, kOptionNoCase);
  }
  parser->start(argc - kFirstOptionArgumentIndex, argv + kFirstOptionArgumentIndex);
  if (args.invalid) {
    return fail(kErrorMalformedOption);
  }
  if ((args.flags.count(kOptionHelp) > 0)) {
    usage();
    return kExitSuccess;
  }
  if ((args.values.count(kOptionInput) == 0) || args.values.at(kOptionInput).empty()) {
    return fail(kErrorMissingInput);
  }
  if ((command == kCommandBuild || command == kCommandVisualize) &&
      ((args.values.count(kOptionOutput) == 0) || args.values.at(kOptionOutput).empty())) {
    return fail(kErrorMissingOutput);
  }
  if (command == kCommandBuild) {
    if ((args.values.count(kOptionCaseFoldFile) > 0) && (args.flags.count(kOptionNoCase) == 0)) {
      return fail(kErrorFoldRequiresNoCase);
    }
    return build_file(args);
  }
  ac::load_options options;
  options.verify_semantics = args.flags.count(kOptionSemantic) != 0;
  for (const char* name : {kOptionMaxFileBytes, kOptionMaxMemoryBytes}) {
    auto it = args.values.find(name);
    if (it == args.values.end()) {
      continue;
    }
    uint64_t amount;
    if (!number(it->second, amount) || amount == 0) {
      return fail(kErrorResourceLimit);
    }
    if (std::string(name) == kOptionMaxFileBytes) {
      options.max_file_bytes = amount;
    } else {
      options.max_memory_bytes = amount;
    }
  }
  uint64_t max_nodes = kDefaultMaxGraphNodes;
  uint64_t root = 0;
  uint64_t depth = UINT32_MAX;
  std::string format;
  bool utf8_labels = args.flags.count(kOptionUtf8Labels) != 0;
  if (command == kCommandVisualize) {
    auto label_format = args.values.find(kOptionLabelFormat);
    if (label_format != args.values.end()) {
      if (utf8_labels || (label_format->second != kLabelFormatHex && label_format->second != kLabelFormatUtf8)) {
        return fail(kErrorLabelFormat);
      }
      utf8_labels = label_format->second == kLabelFormatUtf8;
    }
    if ((args.values.count(kOptionFormat) == 0)) {
      return fail(kErrorMissingFormat);
    }
    format = args.values.at(kOptionFormat);
    if (format != kFormatDot && format != kFormatMermaid && format != kFormatPlantUml) {
      return fail(kErrorFormat);
    }
    for (auto item : {std::make_pair(kOptionMaxNodes, &max_nodes), std::make_pair(kOptionRootId, &root),
                      std::make_pair(kOptionMaxDepth, &depth)}) {
      auto it = args.values.find(item.first);
      if (it != args.values.end() && !number(it->second, *item.second)) {
        return fail(kErrorGraphBound);
      }
    }
    if (max_nodes == 0 || max_nodes > SIZE_MAX || root >= UINT32_MAX || depth > UINT32_MAX) {
      return fail(kErrorGraphRange);
    }
  }
  std::ifstream input(args.values.at(kOptionInput), std::ios::binary);
  if (input.fail()) {
    return fail(kErrorOpenInput);
  }
  ac value;
  if (!check(value.load(input, options), kOperationLoad)) {
    return kExitFailure;
  }
  if (input.peek() != std::char_traits<char>::eof()) {
    return fail(kErrorTrailingData);
  }
  auto stats = value.get_statistics();
  if (command == kCommandInfo) {
    std::cout << kImageInfoPrefix << stats.nodes << kKeywordsField << stats.keywords << kSymbolsField << stats.symbols
              << kMaxKeywordTokensField << stats.max_keyword_tokens << kPayloadBytesField << stats.payload_bytes
              << kNoCaseField << stats.nocase << kLineFeed;
    return kExitSuccess;
  }
  if (command == kCommandVerify) {
    std::cout << kVerifiedMessage
              << (options.verify_semantics ? kSemanticVerificationSuffix : kStructuralVerificationSuffix);
    return kExitSuccess;
  }
  if (root >= stats.nodes) {
    return fail(kErrorMissingRoot);
  }
  graph_selection graph{static_cast<uint32_t>(root), static_cast<uint32_t>(depth), static_cast<size_t>(max_nodes)};
  graph.pending.emplace(graph.root, 0);
  value.visit_nodes(select_node, &graph);
  if (graph.overflow) {
    return fail(kErrorGraphLimit);
  }
  publication file(args.values.at(kOptionOutput));
  if (file.file() == nullptr) {
    return fail(kErrorCreateOutput);
  }
  file_buffer buffer(file.file());
  std::ostream output(&buffer);
  if (!write_graph(output, graph, format, utf8_labels)) {
    return fail(kErrorGraphWrite);
  }
  output.flush();
  if (output.fail() || !file.commit()) {
    return fail(kErrorPublishOutput);
  }
  return kExitSuccess;
}
