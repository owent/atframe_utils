// Copyright 2026 atframework
#include "cli/cmd_option.h"
#include "common/file_system.h"
#include "string/ac_automation_v2.h"
#include "unicode_ranges.h"

#if defined(_WIN32)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <Windows.h>
#endif

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <deque>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
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
static constexpr uint64_t kDefaultMaxGraphNodes = 1000000000;
static constexpr uint32_t kDefaultMaxGraphDepth = UINT32_MAX;
static constexpr uint64_t kDefaultLargeModelBudgetBytes = uint64_t{64} << 30;
static constexpr size_t kTextWriteChunkBytes = 16384;
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
static constexpr uint32_t kNoncharacterFirst = 0xfdd0;
static constexpr uint32_t kNoncharacterLast = 0xfdef;
static constexpr uint32_t kNoncharacterPlaneMask = 0xffff;
static constexpr uint32_t kNoncharacterPlaneFirst = 0xfffe;
static constexpr uint32_t kPrivateUseFirst = 0xe000;
static constexpr uint32_t kPrivateUseLast = 0xf8ff;
static constexpr uint32_t kJsonControlLimit = 0x20;
static constexpr uint32_t kAsciiDelete = 0x7f;
static constexpr uint32_t kUtf16HighSurrogateBase = 0xd800;
static constexpr uint32_t kUtf16LowSurrogateBase = 0xdc00;
static constexpr uint32_t kUtf16SurrogateMask = 0x3ff;
static constexpr size_t kUtf16SurrogateBits = 10;
static constexpr int kUnicodeEscapeFirstShift = 12;

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
static constexpr char kBackspace = '\b';
static constexpr char kFormFeed = '\f';
static constexpr char kCarriageReturn = '\r';
static constexpr char kTab = '\t';
static constexpr char kTemporarySuffix[] = ".tmp.";
static constexpr char kUtf8Bom[] = "\xef\xbb\xbf";
static constexpr size_t kUtf8BomBytes = sizeof(kUtf8Bom) - 1;

// Commands, option keys and parser events.
static constexpr char kCommandBuild[] = "build";
static constexpr char kCommandInfo[] = "info";
static constexpr char kCommandPrint[] = "print";
static constexpr char kCommandVerify[] = "verify";
static constexpr char kCommandVisualize[] = "visualize";
static constexpr char kOptionCaseFoldFile[] = "--case-fold-file";
static constexpr char kOptionReplacementFile[] = "--replacement-file";
static constexpr char kOptionBoundaryFile[] = "--boundary-file";
static constexpr char kOptionKeywordBoundaries[] = "--keyword-boundaries";
static constexpr char kBoundaryInclude[] = "include";
static constexpr char kBoundaryExclude[] = "exclude";
static constexpr char kBoundaryEdge[] = "edge";
static constexpr char kBoundaryNoEdge[] = "no-edge";
static constexpr char kBoundaryHexPrefix[] = "hex:";
static constexpr char kBoundaryTextPrefix[] = "text:";
static constexpr uint64_t kInvalidBoundaryRuleId = UINT32_MAX;
static constexpr uint32_t kNoBoundaryRuleId = 0;
static constexpr uint32_t kFirstBoundaryRuleId = 1;
static constexpr char kErrorBoundaryId[] = "invalid boundary rule ID on line ";
static constexpr char kErrorBoundaryDuplicate[] = "duplicate boundary rule ID: ";
static constexpr char kErrorBoundaryMixed[] = "cannot mix numbered and unnumbered boundary rules";
static constexpr char kErrorBoundaryReference[] = "invalid or unknown boundary reference on keyword line ";
static constexpr char kBoundaryTab = '\t';
static constexpr char kBoundaryRangeSeparator = '-';
static constexpr size_t kBoundaryRangeBytes = 3;
static constexpr size_t kBoundaryRangeLastIndex = 2;
static constexpr char kBoundaryLeftCaption[] = " left=";
static constexpr char kBoundaryRightCaption[] = " right=";
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
static constexpr char kOptionSection[] = "--section";
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
static constexpr char kErrorConfiguration[] =
    "invalid skip/fold/replacement/boundary configuration or unreadable configuration file";
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
static constexpr char kErrorPrintWrite[] = "print write failed";
static constexpr char kErrorResourceLimit[] = "invalid resource limit";
static constexpr char kErrorSection[] = "invalid print section";
static constexpr char kErrorTrailingData[] = "trailing data after image";
static constexpr char kErrorUnknownCommand[] = "unknown command";
static constexpr char kOperationDump[] = "dump";
static constexpr char kOperationFold[] = "fold";
static constexpr char kOperationReplacement[] = "replacement";
static constexpr char kOperationInsert[] = "insert";
static constexpr char kOperationLoad[] = "load";
static constexpr char kOperationNoCase[] = "nocase";
static constexpr char kOperationSkip[] = "skip";

// Report fields and verification messages.
static constexpr char kDuplicatesField[] = " duplicates=";
static constexpr char kIgnoredField[] = " ignored=";
static constexpr char kImageInfoPrefix[] = "version=";
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

// Inspection sections and lossless byte-display fields.
static constexpr char kFormatText[] = "text";
static constexpr char kFormatJson[] = "json";
static constexpr char kSectionAll[] = "all";
static constexpr char kSectionConfig[] = "config";
static constexpr char kSectionKeywords[] = "keywords";
static constexpr char kSectionSkips[] = "skips";
static constexpr char kSectionFolds[] = "folds";
static constexpr char kSectionReplacements[] = "replacements";
static constexpr char kSectionSymbols[] = "symbols";
static constexpr char kSectionBoundaries[] = "boundaries";
static constexpr char kFieldBoundaryRules[] = "boundary_rules";
static constexpr char kFieldBoundaryTokens[] = "boundary_tokens";
static constexpr char kFieldLeftBoundary[] = "left_boundary";
static constexpr char kFieldRightBoundary[] = "right_boundary";
static constexpr char kFieldMode[] = "mode";
static constexpr char kFieldInputEdge[] = "match_input_edge";
static constexpr char kFieldMembers[] = "members";
static constexpr char kBoundaryImageVersion[] = "2.2";
static constexpr const char* kPrintSections[] = {kSectionConfig,    kSectionKeywords,     kSectionSkips,
                                                 kSectionFolds,     kSectionReplacements, kSectionSymbols,
                                                 kSectionBoundaries};
static constexpr char kFieldVersion[] = "version";
static constexpr char kImageVersion[] = "2.0";
static constexpr char kReplacementImageVersion[] = "2.1";
static constexpr char kFieldNodes[] = "nodes";
static constexpr char kFieldMaxKeywordTokens[] = "max_keyword_tokens";
static constexpr char kFieldPayloadBytes[] = "payload_bytes";
static constexpr char kFieldSymbolIndexBytes[] = "symbol_index_bytes";
static constexpr char kFieldNoCase[] = "nocase";
static constexpr char kFieldSkipSymbols[] = "skip_symbols";
static constexpr char kFieldCaseFoldRules[] = "case_fold_rules";
static constexpr char kFieldReplacementRules[] = "replacement_rules";
static constexpr char kFieldId[] = "id";
static constexpr char kFieldFoldedId[] = "folded_id";
static constexpr char kFieldCanonicalId[] = "canonical_id";
static constexpr char kFieldSkipped[] = "skipped";
static constexpr char kFieldSourceId[] = "source_id";
static constexpr char kFieldTargetId[] = "target_id";
static constexpr char kFieldSource[] = "source";
static constexpr char kFieldTarget[] = "target";
static constexpr char kFieldByteLength[] = "byte_length";
static constexpr char kFieldText[] = "text";
static constexpr char kFieldHex[] = "hex";
static constexpr char kBooleanTrue[] = "true";
static constexpr char kBooleanFalse[] = "false";
static constexpr char kRecordSeparator[] = ", ";
static constexpr char kJsonFieldSeparator[] = ": ";
static constexpr char kJsonArrayBegin[] = "[\n";
static constexpr char kJsonItemSeparator[] = ",\n";
static constexpr char kJsonDocumentBegin[] = "{\n";
static constexpr char kJsonDocumentEnd[] = "\n}\n";
static constexpr char kTextSectionEnd[] = "]\n";
static constexpr char kQuote = '"';
static constexpr char kBackslash = '\\';
static constexpr char kRecordBegin = '{';
static constexpr char kRecordEnd = '}';
static constexpr char kArrayBegin = '[';
static constexpr char kArrayEnd = ']';
static constexpr char kTextFieldSeparator = '=';
static constexpr char kEscapeQuote[] = "\\\"";
static constexpr char kEscapeBackslash[] = "\\\\";
static constexpr char kEscapeBackspace[] = "\\b";
static constexpr char kEscapeFormFeed[] = "\\f";
static constexpr char kEscapeLineFeed[] = "\\n";
static constexpr char kEscapeCarriageReturn[] = "\\r";
static constexpr char kEscapeTab[] = "\\t";
static constexpr char kEscapeUnicode[] = "\\u";

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
static constexpr char kFormatD2[] = "d2";
static constexpr char kFormatNomnoml[] = "nomnoml";
static constexpr char kFormatGraphMl[] = "graphml";
static constexpr char kD2Preamble[] = "direction: right\n";
static constexpr char kD2LabelBegin[] = ": \"";
static constexpr char kD2LabelEnd[] = "\"\n";
static constexpr char kD2FailureEnd[] = "\" {style.stroke-dash: 5}\n";
static constexpr char kNomnomlPreamble[] = "#direction: right\n";
static constexpr char kNomnomlNodeBegin[] = "[<state id=n";
static constexpr char kNomnomlNodeCaption[] = ">";
static constexpr char kNomnomlNodeEnd[] = "]\n";
static constexpr char kNomnomlReferenceBegin[] = "[n";
static constexpr char kNomnomlReferenceEnd[] = "] ";
static constexpr char kNomnomlEdge[] = " -> [n";
static constexpr char kNomnomlFailureEdge[] = " --> [n";
static constexpr char kGraphMlPreamble[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<graphml xmlns=\"http://graphml.graphdrawing.org/xmlns\" xmlns:y=\"http://www.yworks.com/xml/graphml\">\n"
    "  <key id=\"label\" for=\"all\" attr.name=\"label\" attr.type=\"string\"/>\n"
    "  <key id=\"kind\" for=\"edge\" attr.name=\"kind\" attr.type=\"string\"/>\n"
    "  <key id=\"ng\" for=\"node\" yfiles.type=\"nodegraphics\"/>\n"
    "  <key id=\"eg\" for=\"edge\" yfiles.type=\"edgegraphics\"/>\n"
    "  <graph id=\"AC\" edgedefault=\"directed\">\n";
static constexpr char kGraphMlEpilogue[] = "  </graph>\n</graphml>\n";
static constexpr char kGraphMlNodeBegin[] = "    <node id=\"n";
static constexpr char kGraphMlLabelBegin[] = "\"><data key=\"label\">";
static constexpr char kGraphMlNodeGraphics[] = "</data><data key=\"ng\"><y:ShapeNode><y:NodeLabel>";
static constexpr char kGraphMlNodeEnd[] = "</y:NodeLabel><y:Shape type=\"rectangle\"/></y:ShapeNode></data></node>\n";
static constexpr char kGraphMlEdgeBegin[] = "    <edge id=\"";
static constexpr char kGraphMlTrieId[] = "t";
static constexpr char kGraphMlFailureId[] = "f";
static constexpr char kGraphMlSource[] = "\" source=\"n";
static constexpr char kGraphMlTarget[] = "\" target=\"n";
static constexpr char kGraphMlKindBegin[] = "</data><data key=\"kind\">";
static constexpr char kGraphMlEdgeGraphics[] =
    "</data><data key=\"eg\"><y:PolyLineEdge><y:LineStyle color=\"#000000\" width=\"1.0\" type=\"";
static constexpr char kGraphMlSolid[] = "line";
static constexpr char kGraphMlDashed[] = "dashed";
static constexpr char kGraphMlArrow[] = "\"/><y:Arrows source=\"none\" target=\"standard\"/><y:EdgeLabel>";
static constexpr char kGraphMlEdgeEnd[] =
    "</y:EdgeLabel><y:BendStyle smoothed=\"false\"/></y:PolyLineEdge></data></edge>\n";
static constexpr char kTrieEdgeKind[] = "trie";
static constexpr char kFailureEdgeText[] = "fail";
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
    "Usage: ac_automation_tool <build|info|print|verify|visualize> --input FILE [options]\n"
    "  build: --output FILE [--nocase|--ignore-case] [--skip-space] [--skip-punctuation]\n"
    "         [--skip-symbols] [--skip-invisible] [--skip-file FILE] [--case-fold-file FILE]\n"
    "         [--replacement-file FILE] [--boundary-file FILE] [--keyword-boundaries]\n"
    "  boundary rows: ID include|exclude edge|no-edge [a-z|text:TOKENS|hex:BYTES ...]\n"
    "                 IDs may be sparse/out of order; compiled IDs are dense in source-ID order.\n"
    "  verify: [--semantic]\n"
    "  print: [--section all|config|keywords|skips|folds|replacements|symbols|boundaries (all)]\n"
    "         [--format text|json (text)] [--output FILE (stdout)]\n"
    "  visualize: --output FILE --format dot|mermaid|plantuml|d2|nomnoml|graphml\n"
    "             [--max-nodes N (1000000000)] [--root-id N (0)] [--max-depth N (4294967295)]\n"
    "             [--label-format utf8|hex (utf8)] [--utf8-labels]\n"
    "  info/print/verify/visualize: [--max-file-bytes N] [--max-memory-bytes N]\n"
    "                       (defaults: print/visualize 64 GiB each, info/verify 1 GiB each)\n"
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

static uint32_t display_properties(uint32_t cp) {
  auto first = std::begin(kUnicodeDisplayRanges);
  auto last = std::end(kUnicodeDisplayRanges);
  auto it = std::lower_bound(first, last, cp, [](const unicode_range& range, uint32_t v) { return range.last < v; });
  return it != last && it->first <= cp ? it->mask : 0;
}

static bool trim_line(gsl::string_view line, gsl::string_view& trimmed) {
  size_t first = line.size();
  size_t end = 0;
  for (size_t pos = 0; pos < line.size();) {
    auto token = ac::next_utf8(gsl::string_view(line).substr(pos));
    if (token.state != ac::token_status::kValid) {
      return false;
    }
    if ((display_properties(decode(gsl::string_view(line).substr(pos, token.byte_count))) & kPropertyWhiteSpace) == 0) {
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
  if (!check(value.set_nocase(args.flags.count(kOptionNoCase) != 0), kOperationNoCase)) {
    return false;
  }
  for (const char* option : {kOptionCaseFoldFile, kOptionReplacementFile}) {
    auto rules_file = args.values.find(option);
    if (rules_file == args.values.end()) {
      continue;
    }
    std::ifstream file(rules_file->second, std::ios::binary);
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
    if (gsl::string_view(option) == kOptionCaseFoldFile) {
      std::vector<ac::case_fold_rule> rules;
      for (const auto& rule : owned) {
        rules.push_back({rule.first, rule.second});
      }
      if (!check(value.set_case_folding(gsl::span<const ac::case_fold_rule>(rules.data(), rules.size())),
                 kOperationFold)) {
        return false;
      }
    } else {
      std::vector<ac::replacement_rule> rules;
      for (const auto& rule : owned) {
        rules.push_back({rule.first, rule.second});
      }
      if (!check(value.set_replacements(gsl::span<const ac::replacement_rule>(rules.data(), rules.size())),
                 kOperationReplacement)) {
        return false;
      }
    }
  }
  return true;
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

#if defined(_WIN32)
static constexpr size_t kConsoleMaxUtf8TokenBytes = 4;
static constexpr size_t kConsoleMaxUtf16TokenUnits = 2;
static constexpr size_t kConsoleWideBufferUnits = kConsoleMaxUtf16TokenUnits * kTextWriteChunkBytes;

// Narrow CRT console writes can reinterpret UTF-8 through the active locale/code page,
// corrupting valid characters or reporting a short write. Use the Unicode console API
// only for a real console; redirected stdout and explicit files retain UTF-8 bytes.
class ATFW_UTIL_SYMBOL_LOCAL console_buffer : public std::streambuf {
 public:
  explicit console_buffer(HANDLE handle) : handle_(handle) {}

 protected:
  std::streamsize xsputn(const char* data, std::streamsize size) override {
    if (size <= 0) {
      return 0;
    }
    const size_t count = static_cast<size_t>(size);
    size_t copied = 0;
    while (copied < count) {
      if (used_ == pending_.size() && !flush(false)) {
        return 0;
      }
      size_t amount = (std::min)(count - copied, pending_.size() - used_);
      std::memcpy(pending_.data() + used_, data + copied, amount);
      used_ += amount;
      copied += amount;
    }
    return size;
  }

  int_type overflow(int_type value) override {
    if (traits_type::eq_int_type(value, traits_type::eof())) {
      return sync() == kStreamSyncSuccess ? traits_type::not_eof(value) : traits_type::eof();
    }
    char byte = traits_type::to_char_type(value);
    return xsputn(&byte, sizeof(byte)) == sizeof(byte) ? value : traits_type::eof();
  }

  int sync() override { return flush(true) ? kStreamSyncSuccess : kStreamSyncFailure; }

 private:
  bool flush(bool final) {
    std::array<wchar_t, kConsoleWideBufferUnits> wide;
    size_t consumed = 0;
    size_t units = 0;
    while (consumed < used_) {
      gsl::string_view remaining(pending_.data() + consumed, used_ - consumed);
      auto token = ac::next_utf8(remaining);
      if (token.state != ac::token_status::kValid) {
        // Quoted input bytes have already been escaped. Only a valid scalar split
        // at the buffer boundary can remain; retain at most three bytes until refill.
        if (!final && remaining.size() < kConsoleMaxUtf8TokenBytes) {
          break;
        }
        return false;
      }
      if (units > wide.size() - kConsoleMaxUtf16TokenUnits) {
        return false;
      }
      uint32_t cp = decode(remaining.substr(0, token.byte_count));
      if (cp == kLineFeed) {
        wide[units++] = kCarriageReturn;
        wide[units++] = kLineFeed;
      } else if (cp < kUtf8ThreeByteLimit) {
        wide[units++] = static_cast<wchar_t>(cp);
      } else {
        cp -= kUtf8ThreeByteLimit;
        wide[units++] = static_cast<wchar_t>(kUtf16HighSurrogateBase + (cp >> kUtf16SurrogateBits));
        wide[units++] = static_cast<wchar_t>(kUtf16LowSurrogateBase + (cp & kUtf16SurrogateMask));
      }
      consumed += token.byte_count;
    }
    size_t written = 0;
    while (written < units) {
      DWORD amount = 0;
      DWORD requested = static_cast<DWORD>(units - written);
      if (WriteConsoleW(handle_, wide.data() + written, requested, &amount, nullptr) == 0 || amount == 0 ||
          amount > requested) {
        return false;
      }
      written += amount;
    }
    if (consumed > 0) {
      used_ -= consumed;
      std::memmove(pending_.data(), pending_.data() + consumed, used_);
    }
    return !final || used_ == 0;
  }

  HANDLE handle_;
  std::array<char, kTextWriteChunkBytes> pending_;
  size_t used_ = 0;
};
#endif

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

struct ATFW_UTIL_SYMBOL_LOCAL boundary_definition {
  ac::boundary_mode mode = ac::boundary_mode::kExcludeSet;
  bool match_input_edge = true;
  std::vector<std::string> tokens;
};

static bool configure_boundaries(ac& value, const arguments& args, std::map<uint32_t, uint32_t>& boundary_ids) {
  auto configured = args.values.find(kOptionBoundaryFile);
  if (configured == args.values.end()) {
    return true;
  }
  std::ifstream input(configured->second, std::ios::binary);
  if (input.fail()) {
    return false;
  }
  // Source IDs are labels, not vector offsets. Sorting makes explicit files independent
  // of line order; dense IDs keep frozen storage compact even for UINT32_MAX-1 labels.
  std::map<uint32_t, boundary_definition> definitions;
  bool explicit_ids = false;
  std::string line;
  size_t lines = 0;
  while (!std::getline(input, line).fail()) {
    ++lines;
    if (lines == 1 && line.compare(0, kUtf8BomBytes, kUtf8Bom) == 0) {
      line.erase(0, kUtf8BomBytes);
    }
    std::istringstream fields(line);
    std::string first;
    if ((fields >> first).fail()) {
      continue;
    }
    if (definitions.size() >= kInvalidBoundaryRuleId - kFirstBoundaryRuleId) {
      return false;
    }
    bool numbered = first != kBoundaryInclude && first != kBoundaryExclude;
    if (!definitions.empty() && numbered != explicit_ids) {
      fail(kErrorBoundaryMixed);
      return false;
    }
    explicit_ids = numbered;
    uint64_t source_id = static_cast<uint64_t>(definitions.size()) + kFirstBoundaryRuleId;
    std::string mode = first;
    if (numbered) {
      if (!number(first, source_id) || source_id == kNoBoundaryRuleId || source_id >= kInvalidBoundaryRuleId) {
        fail(kErrorBoundaryId + std::to_string(lines));
        return false;
      }
      if ((fields >> mode).fail()) {
        return false;
      }
    }
    std::string edge;
    if ((fields >> edge).fail() || (mode != kBoundaryInclude && mode != kBoundaryExclude) ||
        (edge != kBoundaryEdge && edge != kBoundaryNoEdge)) {
      return false;
    }
    boundary_definition definition;
    definition.mode = mode == kBoundaryInclude ? ac::boundary_mode::kIncludeSet : ac::boundary_mode::kExcludeSet;
    definition.match_input_edge = edge == kBoundaryEdge;

    std::string specification;
    while (!(fields >> specification).fail()) {
      if (specification.compare(0, sizeof(kBoundaryHexPrefix) - 1, kBoundaryHexPrefix) == 0) {
        std::string decoded;
        if (!hex_decode(specification.substr(sizeof(kBoundaryHexPrefix) - 1), decoded) || decoded.empty()) {
          return false;
        }
        definition.tokens.push_back(std::move(decoded));
      } else if (specification.compare(0, sizeof(kBoundaryTextPrefix) - 1, kBoundaryTextPrefix) == 0) {
        gsl::string_view text(specification);
        text.remove_prefix(sizeof(kBoundaryTextPrefix) - 1);
        if (text.empty()) {
          return false;
        }
        for (size_t pos = 0; pos < text.size();) {
          auto token = ac::next_utf8(text.substr(pos));
          if (token.state != ac::token_status::kValid) {
            return false;
          }
          definition.tokens.emplace_back(text.data() + pos, token.byte_count);
          pos += token.byte_count;
        }
      } else if (specification.size() == kBoundaryRangeBytes && specification[1] == kBoundaryRangeSeparator &&
                 static_cast<unsigned char>(specification.front()) <=
                     static_cast<unsigned char>(specification.back()) &&
                 static_cast<unsigned char>(specification.back()) < kUtf8SingleByteLimit) {
        for (unsigned int c = static_cast<unsigned char>(specification.front());
             c <= static_cast<unsigned char>(specification[kBoundaryRangeLastIndex]); ++c) {
          definition.tokens.emplace_back(1, static_cast<char>(c));
        }
      } else {
        return false;
      }
    }

    if (!definitions.emplace(static_cast<uint32_t>(source_id), std::move(definition)).second) {
      fail(kErrorBoundaryDuplicate + std::to_string(source_id));
      return false;
    }
  }
  if (input.bad()) {
    return false;
  }
  for (const auto& entry : definitions) {
    const auto& definition = entry.second;
    std::vector<gsl::string_view> tokens;
    for (const auto& token : definition.tokens) {
      tokens.emplace_back(token);
    }
    ac::boundary_rule rule;
    rule.mode = definition.mode;
    rule.match_input_edge = definition.match_input_edge;
    rule.tokens = gsl::span<const gsl::string_view>(tokens.data(), tokens.size());
    uint32_t internal_id = kNoBoundaryRuleId;
    if (!check(value.add_boundary_rule(rule, &internal_id), kSectionBoundaries)) {
      return false;
    }
    boundary_ids.emplace(entry.first, internal_id);
  }
  return true;
}

static bool keyword_boundaries(gsl::string_view& text, ac::keyword_options& options,
                               const std::map<uint32_t, uint32_t>& boundary_ids) {
  size_t first = text.find(kBoundaryTab);
  if (first == gsl::string_view::npos) {
    return false;
  }
  size_t second = text.find(kBoundaryTab, first + 1);
  if (second == gsl::string_view::npos) {
    return false;
  }
  uint64_t left = 0;
  uint64_t right = 0;
  if (!number(std::string(text.data(), first), left) || left >= UINT32_MAX ||
      !number(std::string(text.data() + first + 1, second - first - 1), right) || right >= UINT32_MAX) {
    return false;
  }
  auto resolve = [&boundary_ids](uint64_t source_id, uint32_t& result) {
    if (source_id == kNoBoundaryRuleId) {
      result = kNoBoundaryRuleId;
      return true;
    }
    auto found = boundary_ids.find(static_cast<uint32_t>(source_id));
    if (found == boundary_ids.end()) {
      return false;
    }
    result = found->second;
    return true;
  };
  if (!resolve(left, options.left_boundary) || !resolve(right, options.right_boundary)) {
    return false;
  }
  auto body = text.substr(second + 1);
  return trim_line(body, text);
}

static int build_file(const arguments& args) {
  ac value;
  std::map<uint32_t, uint32_t> boundary_ids;
  if (!configure(value, args) || !configure_boundaries(value, args, boundary_ids)) {
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
    ac::keyword_options boundaries;
    if (args.flags.count(kOptionKeywordBoundaries) > 0) {
      gsl::string_view annotated = gsl::string_view(line).substr(static_cast<size_t>(trimmed.data() - line.data()));
      if (!keyword_boundaries(annotated, boundaries, boundary_ids)) {
        return fail(kErrorBoundaryReference + std::to_string(lines));
      }
      trimmed = annotated;
      if (trimmed.empty()) {
        ++ignored;
        continue;
      }
    }
    auto result = value.insert_keyword(trimmed, boundaries);
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

enum class ATFW_UTIL_SYMBOL_LOCAL graph_format { kDot, kMermaid, kPlantUml, kD2, kNomnoml, kGraphMl };

static bool parse_graph_format(gsl::string_view name, graph_format& output) {
  const std::pair<const char*, graph_format> formats[] = {
      {kFormatDot, graph_format::kDot},           {kFormatMermaid, graph_format::kMermaid},
      {kFormatPlantUml, graph_format::kPlantUml}, {kFormatD2, graph_format::kD2},
      {kFormatNomnoml, graph_format::kNomnoml},   {kFormatGraphMl, graph_format::kGraphMl}};
  for (const auto& item : formats) {
    if (name == item.first) {
      output = item.second;
      return true;
    }
  }
  return false;
}

struct ATFW_UTIL_SYMBOL_LOCAL graph_range {
  uint32_t begin;
  uint32_t end;
};

struct ATFW_UTIL_SYMBOL_LOCAL graph_selection {
  uint32_t root;
  uint32_t max_depth;
  size_t limit;
  size_t count = 0;
  bool overflow = false;
  bool complete = false;
  uint32_t level_begin;
  uint32_t level_end;
  uint32_t next_begin = 0;
  uint32_t next_end = 0;
  uint32_t depth = 0;
  std::vector<graph_range> ranges;

  graph_selection(uint32_t start, uint32_t depth_limit, size_t maximum)
      : root(start), max_depth(depth_limit), limit(maximum), level_begin(start), level_end(start + 1) {}

  bool contains(uint32_t id) const {
    auto next = std::upper_bound(ranges.begin(), ranges.end(), id,
                                 [](uint32_t value, const graph_range& range) { return value < range.begin; });
    return next != ranges.begin() && id < (--next)->end;
  }
};

static bool select_node(const ac::node_view& node, void* data) {
  auto& graph = *static_cast<graph_selection*>(data);
  if (node.id < graph.level_begin) {
    return true;
  }
  if (node.id >= graph.level_end) {
    return false;
  }
  if (graph.count == graph.limit) {
    graph.overflow = true;
    return false;
  }
  ++graph.count;
  // A subtree occupies one contiguous BFS interval at each depth. Adjacent levels can merge,
  // so complete graphs and single chains need only one range, without copies of their nodes.
  if (!graph.ranges.empty() && graph.ranges.back().end == node.id) {
    graph.ranges.back().end = node.id + 1;
  } else {
    graph.ranges.push_back({node.id, node.id + 1});
  }
  if (graph.depth < graph.max_depth && node.child_count > 0) {
    if (graph.next_begin == graph.next_end) {
      graph.next_begin = node.child_begin;
    }
    graph.next_end = node.child_begin + node.child_count;
  }
  if (node.id + 1 == graph.level_end) {
    if (graph.depth == graph.max_depth || graph.next_begin == graph.next_end) {
      graph.complete = true;
      return false;
    }
    if (graph.next_end - graph.next_begin > graph.limit - graph.count) {
      graph.overflow = true;
      return false;
    }
    graph.level_begin = graph.next_begin;
    graph.level_end = graph.next_end;
    graph.next_begin = 0;
    graph.next_end = 0;
    ++graph.depth;
  }
  return true;
}

static std::string label(gsl::string_view bytes, bool utf8) {
  std::string result;
  for (size_t pos = 0; pos < bytes.size();) {
    auto token = ac::next_utf8(bytes.substr(pos));
    unsigned char c = static_cast<unsigned char>(bytes[pos]);
    uint32_t cp = token.state == ac::token_status::kValid ? decode(bytes.substr(pos, token.byte_count)) : 0;
    if (utf8 && token.state == ac::token_status::kValid &&
        (display_properties(cp) & (kPropertyInvisible | kPropertyWhiteSpace)) == 0 &&
        (cp < kNoncharacterFirst || cp > kNoncharacterLast) &&
        (cp & kNoncharacterPlaneMask) < kNoncharacterPlaneFirst &&
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

static void write_text(std::ostream& out, gsl::string_view text) {
  for (size_t pos = 0; pos < text.size() && out.good();) {
    size_t length = (std::min)(text.size() - pos, kTextWriteChunkBytes);
    out.write(text.data() + pos, static_cast<std::streamsize>(length));
    pos += length;
  }
}

static void write_unicode_escape(std::ostream& out, uint32_t unit) {
  out << kEscapeUnicode;
  for (int shift = kUnicodeEscapeFirstShift; shift >= 0; shift -= static_cast<int>(kHexBitsPerDigit)) {
    out.put(kHexDigits[(unit >> shift) & kHexDigitMask]);
  }
}

static void write_quoted(std::ostream& out, gsl::string_view bytes) {
  out.put(kQuote);
  for (size_t pos = 0; pos < bytes.size() && out.good();) {
    auto remaining = bytes.substr(pos);
    auto token = ac::next_utf8(remaining);
    if (token.state != ac::token_status::kValid) {
      // JSON cannot represent arbitrary byte strings. The accompanying hex field is
      // authoritative; map a malformed display byte to its U+00xx escape without loss there.
      write_unicode_escape(out, static_cast<unsigned char>(remaining[0]));
      ++pos;
      continue;
    }
    auto current = remaining.substr(0, token.byte_count);
    uint32_t cp = decode(current);
    const char* escape = nullptr;
    switch (cp) {
      case kQuote:
        escape = kEscapeQuote;
        break;
      case kBackslash:
        escape = kEscapeBackslash;
        break;
      case kBackspace:
        escape = kEscapeBackspace;
        break;
      case kFormFeed:
        escape = kEscapeFormFeed;
        break;
      case kLineFeed:
        escape = kEscapeLineFeed;
        break;
      case kCarriageReturn:
        escape = kEscapeCarriageReturn;
        break;
      case kTab:
        escape = kEscapeTab;
        break;
      default:
        break;
    }
    if (escape != nullptr) {
      out << escape;
    } else if (cp < kJsonControlLimit || cp == kAsciiDelete || cp >= kUtf8ThreeByteLimit ||
               (cp >= kPrivateUseFirst && cp <= kPrivateUseLast) ||
               (display_properties(cp) & (kPropertyWhiteSpace | kPropertyInvisible)) != 0 ||
               (cp >= kNoncharacterFirst && cp <= kNoncharacterLast) ||
               (cp & kNoncharacterPlaneMask) >= kNoncharacterPlaneFirst) {
      if (cp < kUtf8ThreeByteLimit) {
        write_unicode_escape(out, cp);
      } else {
        // Classic Windows consoles may replace supplementary-plane characters even
        // through WriteConsoleW. ASCII surrogate escapes remain readable everywhere.
        cp -= kUtf8ThreeByteLimit;
        write_unicode_escape(out, kUtf16HighSurrogateBase + (cp >> kUtf16SurrogateBits));
        write_unicode_escape(out, kUtf16LowSurrogateBase + (cp & kUtf16SurrogateMask));
      }
    } else {
      write_text(out, current);
    }
    pos += token.byte_count;
  }
  out.put(kQuote);
}

struct ATFW_UTIL_SYMBOL_LOCAL print_record {
  std::ostream& output;
  bool json;
  bool first = true;

  print_record(std::ostream& out, bool as_json) : output(out), json(as_json) { output.put(kRecordBegin); }

  void key(gsl::string_view name) {
    if (!first) {
      output << kRecordSeparator;
    }
    first = false;
    if (json) {
      write_quoted(output, name);
      output << kJsonFieldSeparator;
    } else {
      write_text(output, name);
      output.put(kTextFieldSeparator);
    }
  }

  void number(gsl::string_view name, uint64_t value) {
    key(name);
    output << value;
  }

  void boolean(gsl::string_view name, bool value) {
    key(name);
    output << (value ? kBooleanTrue : kBooleanFalse);
  }

  void text(gsl::string_view name, gsl::string_view value) {
    key(name);
    write_quoted(output, value);
  }

  void bytes(gsl::string_view value) {
    number(kFieldByteLength, value.size());
    text(kFieldText, value);
    key(kFieldHex);
    output.put(kQuote);
    for (unsigned char byte : value) {
      if (!output.good()) {
        break;
      }
      output.put(kHexDigits[byte >> kHexBitsPerDigit]);
      output.put(kHexDigits[byte & kHexDigitMask]);
    }
    output.put(kQuote);
  }

  void finish() { output.put(kRecordEnd); }
};

struct ATFW_UTIL_SYMBOL_LOCAL configuration_counts {
  uint32_t skips = 0;
  uint32_t folds = 0;
  uint32_t replacements = 0;
};

static bool count_configuration(const ac::symbol_view& symbol, void* data) {
  auto& counts = *static_cast<configuration_counts*>(data);
  if (symbol.skipped) {
    ++counts.skips;
  }
  if (symbol.id != symbol.folded_id) {
    ++counts.folds;
  }
  if (symbol.id == symbol.folded_id && symbol.id != symbol.canonical_id) {
    ++counts.replacements;
  }
  return true;
}

static void write_configuration(std::ostream& out, const ac& value, bool json) {
  configuration_counts counts;
  value.visit_symbols(count_configuration, &counts);
  auto stats = value.get_statistics();
  print_record record(out, json);
  record.text(kFieldVersion, stats.boundary_rules > 0
                                 ? kBoundaryImageVersion
                                 : (stats.replacements ? kReplacementImageVersion : kImageVersion));
  record.number(kFieldNodes, stats.nodes);
  record.number(kSectionKeywords, stats.keywords);
  record.number(kSectionSymbols, stats.symbols);
  record.number(kFieldMaxKeywordTokens, stats.max_keyword_tokens);
  record.number(kFieldPayloadBytes, stats.payload_bytes);
  record.number(kFieldSymbolIndexBytes, stats.symbol_index_bytes);
  record.boolean(kFieldNoCase, stats.nocase);
  record.number(kFieldSkipSymbols, counts.skips);
  record.number(kFieldCaseFoldRules, counts.folds);
  record.boolean(kSectionReplacements, stats.replacements);
  record.number(kFieldReplacementRules, counts.replacements);
  record.number(kFieldBoundaryRules, stats.boundary_rules);
  record.number(kFieldBoundaryTokens, stats.boundary_tokens);
  record.finish();
}

struct ATFW_UTIL_SYMBOL_LOCAL symbol_printer {
  std::ostream& output;
  gsl::string_view section;
  bool json;
  bool first = true;

  symbol_printer(std::ostream& out, gsl::string_view selected, bool as_json)
      : output(out), section(selected), json(as_json) {}

  void next() {
    if (json && !first) {
      output << kJsonItemSeparator;
    }
    first = false;
  }
};

static bool print_symbol(const ac::symbol_view& symbol, void* data) {
  auto& printer = *static_cast<symbol_printer*>(data);
  if ((printer.section == kSectionSkips && !symbol.skipped) ||
      (printer.section == kSectionFolds && symbol.id == symbol.folded_id) ||
      (printer.section == kSectionReplacements &&
       (symbol.id != symbol.folded_id || symbol.id == symbol.canonical_id))) {
    return true;
  }
  printer.next();
  print_record record(printer.output, printer.json);
  if (printer.section == kSectionFolds || printer.section == kSectionReplacements) {
    bool replacement = printer.section == kSectionReplacements;
    record.number(kFieldSourceId, symbol.id);
    record.number(kFieldTargetId, replacement ? symbol.canonical_id : symbol.folded_id);
    record.key(kFieldSource);
    print_record source(printer.output, printer.json);
    source.bytes(symbol.token);
    source.finish();
    record.key(kFieldTarget);
    print_record target(printer.output, printer.json);
    target.bytes(replacement ? symbol.canonical_token : symbol.folded_token);
    target.finish();
  } else {
    record.number(kFieldId, symbol.id);
    if (printer.section == kSectionSymbols) {
      record.number(kFieldFoldedId, symbol.folded_id);
      record.number(kFieldCanonicalId, symbol.canonical_id);
      record.boolean(kFieldSkipped, symbol.skipped);
    }
    record.bytes(symbol.token);
  }
  record.finish();
  if (!printer.json) {
    printer.output.put(kLineFeed);
  }
  return printer.output.good();
}

struct ATFW_UTIL_SYMBOL_LOCAL boundary_printer {
  std::ostream& output;
  const ac& value;
  bool json;
  bool first = true;

  boundary_printer(std::ostream& out, const ac& model, bool as_json) : output(out), value(model), json(as_json) {}
};

static bool print_boundary(const ac::boundary_rule_view& rule, void* data) {
  auto& printer = *static_cast<boundary_printer*>(data);
  if (printer.json && !printer.first) {
    printer.output << kJsonItemSeparator;
  }
  printer.first = false;
  print_record record(printer.output, printer.json);
  record.number(kFieldId, rule.id);
  record.text(kFieldMode, rule.mode == ac::boundary_mode::kIncludeSet ? kBoundaryInclude : kBoundaryExclude);
  record.boolean(kFieldInputEdge, rule.match_input_edge);
  record.key(kFieldMembers);
  printer.output << kJsonArrayBegin;
  bool first = true;
  for (auto id : rule.symbols) {
    if (!first) {
      printer.output << kRecordSeparator;
    }
    first = false;
    print_record member(printer.output, printer.json);
    member.number(kFieldId, id);
    member.bytes(printer.value.symbol(id));
    member.finish();
  }
  printer.output.put(kArrayEnd);
  record.finish();
  if (!printer.json) {
    printer.output.put(kLineFeed);
  }
  return printer.output.good();
}

static bool print_model(std::ostream& out, const ac& value, gsl::string_view section, bool json) {
  if (json) {
    out << kJsonDocumentBegin;
  }
  bool first = true;
  for (const char* name : kPrintSections) {
    if (section != kSectionAll && section != name) {
      continue;
    }
    if (json) {
      if (!first) {
        out << kJsonItemSeparator;
      }
      write_quoted(out, name);
      out << kJsonFieldSeparator;
    } else {
      out.put(kArrayBegin);
      out << name << kTextSectionEnd;
    }
    first = false;
    if (gsl::string_view(name) == kSectionConfig) {
      write_configuration(out, value, json);
      if (!json) {
        out.put(kLineFeed);
      }
      continue;
    }
    if (json) {
      out << kJsonArrayBegin;
    }
    if (gsl::string_view(name) == kSectionKeywords) {
      const uint32_t count = value.get_statistics().keywords;
      for (uint32_t id = 0; id < count && out.good(); ++id) {
        if (json && id > 0) {
          out << kJsonItemSeparator;
        }
        print_record record(out, json);
        record.number(kFieldId, id);
        record.bytes(value.keyword(id));
        auto boundaries = value.get_keyword_options(id);
        record.number(kFieldLeftBoundary, boundaries.left_boundary);
        record.number(kFieldRightBoundary, boundaries.right_boundary);
        record.finish();
        if (!json) {
          out.put(kLineFeed);
        }
      }
    } else if (gsl::string_view(name) == kSectionBoundaries) {
      boundary_printer printer(out, value, json);
      if (!value.visit_boundary_rules(print_boundary, &printer)) {
        return false;
      }
    } else {
      symbol_printer printer(out, name, json);
      if (!value.visit_symbols(print_symbol, &printer)) {
        return false;
      }
    }
    if (json) {
      out.put(kLineFeed);
      out.put(kArrayEnd);
    }
    if (!out.good()) {
      return false;
    }
  }
  if (json) {
    out << kJsonDocumentEnd;
  }
  return out.good();
}

static int print_file(const ac& value, gsl::string_view section, bool json, const arguments& args) {
  auto destination = args.values.find(kOptionOutput);
  if (destination == args.values.end()) {
#if defined(_WIN32)
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (handle != nullptr && handle != INVALID_HANDLE_VALUE && GetConsoleMode(handle, &mode) != 0) {
      console_buffer buffer(handle);
      std::ostream output(&buffer);
      if (!print_model(output, value, section, json)) {
        return fail(kErrorPrintWrite);
      }
      output.flush();
      return output.good() ? kExitSuccess : fail(kErrorPrintWrite);
    }
#endif
    if (!print_model(std::cout, value, section, json)) {
      return fail(kErrorPrintWrite);
    }
    std::cout.flush();
    return std::cout.good() ? kExitSuccess : fail(kErrorPrintWrite);
  }
  publication file(destination->second);
  if (file.file() == nullptr) {
    return fail(kErrorCreateOutput);
  }
  file_buffer buffer(file.file());
  std::ostream output(&buffer);
  if (!print_model(output, value, section, json)) {
    return fail(kErrorPrintWrite);
  }
  output.flush();
  if (output.fail() || !file.commit()) {
    return fail(kErrorPublishOutput);
  }
  return kExitSuccess;
}

static void write_node_caption(std::ostream& out, const ac::node_view& node, const ac& value) {
  out << kNodeCaptionPrefix << node.id;
  if (node.output_head != UINT32_MAX) {
    out << kOutputCaptionPrefix << node.output_head;
    auto options = value.get_keyword_options(node.output_head);
    if (options.left_boundary > 0) {
      out << kBoundaryLeftCaption << options.left_boundary;
    }
    if (options.right_boundary > 0) {
      out << kBoundaryRightCaption << options.right_boundary;
    }
  }
}

static void write_graph_node(std::ostream& out, const ac::node_view& node, graph_format format, const ac& value) {
  if (format == graph_format::kDot) {
    out << kIndentedNodePrefix << node.id << kDotLabelBegin;
    write_node_caption(out, node, value);
    out << kDotStatementEnd;
  } else if (format == graph_format::kMermaid) {
    out << kIndentedNodePrefix << node.id << kMermaidLabelBegin;
    write_node_caption(out, node, value);
    out << kMermaidLabelEnd;
  } else if (format == graph_format::kPlantUml) {
    out << kPlantUmlStateBegin;
    write_node_caption(out, node, value);
    out << kPlantUmlStateAlias << node.id << kLineFeed;
  } else if (format == graph_format::kD2) {
    out << kNodeIdPrefix << node.id << kD2LabelBegin;
    write_node_caption(out, node, value);
    out << kD2LabelEnd;
  } else if (format == graph_format::kNomnoml) {
    out << kNomnomlNodeBegin << node.id << kNomnomlNodeCaption;
    write_node_caption(out, node, value);
    out << kNomnomlNodeEnd;
  } else {
    out << kGraphMlNodeBegin << node.id << kGraphMlLabelBegin;
    write_node_caption(out, node, value);
    out << kGraphMlNodeGraphics;
    write_node_caption(out, node, value);
    out << kGraphMlNodeEnd;
  }
}

static void write_graph_edge(std::ostream& out, graph_format format, uint32_t source, uint32_t target,
                             gsl::string_view text, bool failure) {
  if (format == graph_format::kDot) {
    out << kIndentedNodePrefix << source << kDotEdge << target;
    if (failure) {
      out << kDotFailureEdgeEnd;
    } else {
      out << kDotLabelBegin;
      write_text(out, text);
      out << kDotStatementEnd;
    }
  } else if (format == graph_format::kMermaid) {
    out << kIndentedNodePrefix << source;
    if (failure) {
      out << kMermaidFailureEdge << target << kLineFeed;
    } else {
      out << kMermaidEdgeLabelBegin;
      write_text(out, text);
      out << kMermaidEdgeLabelEnd << target << kLineFeed;
    }
  } else if (format == graph_format::kPlantUml) {
    out << kNodeIdPrefix << source << (failure ? kPlantUmlFailureEdge : kPlantUmlEdge) << target;
    if (failure) {
      out << kPlantUmlFailureLabel;
    } else {
      out << kPlantUmlLabelSeparator;
      write_text(out, text);
      out << kLineFeed;
    }
  } else if (format == graph_format::kD2) {
    out << kNodeIdPrefix << source << kDotEdge << target << kD2LabelBegin;
    write_text(out, text);
    out << (failure ? kD2FailureEnd : kD2LabelEnd);
  } else if (format == graph_format::kNomnoml) {
    out << kNomnomlReferenceBegin << source << kNomnomlReferenceEnd;
    write_text(out, text);
    out << (failure ? kNomnomlFailureEdge : kNomnomlEdge) << target << kNomnomlNodeEnd;
  } else {
    out << kGraphMlEdgeBegin << (failure ? kGraphMlFailureId : kGraphMlTrieId) << (failure ? source : target)
        << kGraphMlSource << source << kGraphMlTarget << target << kGraphMlLabelBegin;
    write_text(out, text);
    out << kGraphMlKindBegin << (failure ? kFailureEdgeText : kTrieEdgeKind) << kGraphMlEdgeGraphics
        << (failure ? kGraphMlDashed : kGraphMlSolid) << kGraphMlArrow;
    write_text(out, text);
    out << kGraphMlEdgeEnd;
  }
}

struct ATFW_UTIL_SYMBOL_LOCAL graph_parent {
  uint32_t begin;
  uint32_t end;
  uint32_t id;
};

struct ATFW_UTIL_SYMBOL_LOCAL graph_writer {
  std::ostream& output;
  const graph_selection& selection;
  const ac& value;
  graph_format format;
  bool utf8;
  size_t written = 0;
  size_t range = 0;
  // Only parents whose children have not been visited remain in the BFS frontier.
  // A long chain uses one entry; selected node views and per-node lookup trees are never retained.
  std::deque<graph_parent> parents;

  graph_writer(std::ostream& stream, const graph_selection& graph, const ac& model, graph_format syntax,
               bool utf8_labels)
      : output(stream), selection(graph), value(model), format(syntax), utf8(utf8_labels) {}
};

static bool write_selected_node(const ac::node_view& node, void* data) {
  auto& writer = *static_cast<graph_writer*>(data);
  const auto& ranges = writer.selection.ranges;
  while (writer.range < ranges.size() && node.id >= ranges[writer.range].end) {
    ++writer.range;
  }
  if (writer.range >= ranges.size()) {
    return false;
  }
  if (node.id < ranges[writer.range].begin) {
    return true;
  }
  while (!writer.parents.empty() && node.id >= writer.parents.front().end) {
    writer.parents.pop_front();
  }
  write_graph_node(writer.output, node, writer.format, writer.value);
  if (node.id != writer.selection.root) {
    if (writer.parents.empty() || node.id < writer.parents.front().begin) {
      return false;
    }
    auto token = label(node.token, writer.utf8);
    write_graph_edge(writer.output, writer.format, writer.parents.front().id, node.id, token, false);
    if (node.id + 1 == writer.parents.front().end) {
      writer.parents.pop_front();
    }
  }
  if (node.id > 0 && writer.selection.contains(node.failure)) {
    write_graph_edge(writer.output, writer.format, node.id, node.failure, kFailureEdgeText, true);
  }
  if (!writer.output.good()) {
    return false;
  }
  if (node.child_count > 0 && writer.selection.contains(node.child_begin)) {
    writer.parents.push_back({node.child_begin, node.child_begin + node.child_count, node.id});
  }
  ++writer.written;
  return writer.written < writer.selection.count;
}

static bool write_graph(std::ostream& out, const ac& value, const graph_selection& graph, graph_format format,
                        bool utf8) {
  if (format == graph_format::kDot) {
    out << kDotPreamble;
  } else if (format == graph_format::kMermaid) {
    out << kMermaidPreamble;
  } else if (format == graph_format::kPlantUml) {
    out << kPlantUmlPreamble;
  } else if (format == graph_format::kD2) {
    out << kD2Preamble;
  } else if (format == graph_format::kNomnoml) {
    out << kNomnomlPreamble;
  } else {
    out << kGraphMlPreamble;
  }
  graph_writer writer(out, graph, value, format, utf8);
  value.visit_nodes(write_selected_node, &writer);
  if (writer.written != graph.count || !out.good()) {
    return false;
  }
  if (format == graph_format::kDot) {
    out << kDotEpilogue;
  } else if (format == graph_format::kPlantUml) {
    out << kPlantUmlEpilogue;
  } else if (format == graph_format::kGraphMl) {
    out << kGraphMlEpilogue;
  }
  return out.good();
}

static void usage() { std::cout << kUsageText; }
}  // namespace

int main(int argc, char* argv[]) {
  // Validate the subcommand before registering its command-specific options.
  if (argc == kCommandArgumentCount && std::string(argv[kCommandArgumentIndex]) == kOptionHelp) {
    usage();
    return kExitSuccess;
  }
  if (argc < kCommandArgumentCount) {
    usage();
    return kExitFailure;
  }
  std::string command = argv[kCommandArgumentIndex];
  if (command != kCommandBuild && command != kCommandInfo && command != kCommandPrint && command != kCommandVerify &&
      command != kCommandVisualize) {
    return fail(kErrorUnknownCommand);
  }
  arguments args;
  auto parser = cli::create();
  parser->bind_cmd(kParserDefaultEvent, parse_error, &args);
  parser->bind_cmd(kParserErrorEvent, parse_error, &args);
  // The parser borrows these option names; all names have static storage duration.
  std::vector<const char*> values{kOptionInput};
  std::vector<const char*> flags{kOptionHelp};
  if (command == kCommandBuild) {
    values.insert(values.end(),
                  {kOptionOutput, kOptionSkipFile, kOptionCaseFoldFile, kOptionReplacementFile, kOptionBoundaryFile});
    flags.insert(flags.end(), {kOptionNoCase, kOptionSkipSpace, kOptionSkipPunctuation, kOptionSkipSymbols,
                               kOptionSkipInvisible, kOptionKeywordBoundaries});
  } else {
    values.insert(values.end(), {kOptionMaxFileBytes, kOptionMaxMemoryBytes});
    if (command == kCommandVerify) {
      flags.push_back(kOptionSemantic);
    }
    if (command == kCommandPrint) {
      values.insert(values.end(), {kOptionOutput, kOptionFormat, kOptionSection});
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
  if (command == kCommandPrint && args.values.count(kOptionOutput) > 0 && args.values.at(kOptionOutput).empty()) {
    return fail(kErrorMissingOutput);
  }
  if (command == kCommandBuild) {
    if ((args.values.count(kOptionCaseFoldFile) > 0) && (args.flags.count(kOptionNoCase) == 0)) {
      return fail(kErrorFoldRequiresNoCase);
    }
    return build_file(args);
  }
  // Enforce resource limits before loading any image sections or selecting graph nodes.
  ac::load_options options;
  options.verify_semantics = args.flags.count(kOptionSemantic) != 0;
  if (command == kCommandVisualize || command == kCommandPrint) {
    options.max_file_bytes = kDefaultLargeModelBudgetBytes;
    options.max_memory_bytes = kDefaultLargeModelBudgetBytes;
  }
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
  uint64_t depth = kDefaultMaxGraphDepth;
  graph_format format = graph_format::kDot;
  bool utf8_labels = true;
  gsl::string_view print_section = kSectionAll;
  bool print_json = false;
  if (command == kCommandPrint) {
    auto section = args.values.find(kOptionSection);
    if (section != args.values.end()) {
      print_section = section->second;
    }
    if (print_section != kSectionAll &&
        std::none_of(std::begin(kPrintSections), std::end(kPrintSections),
                     [print_section](const char* name) { return print_section == name; })) {
      return fail(kErrorSection);
    }
    auto selected_format = args.values.find(kOptionFormat);
    if (selected_format != args.values.end()) {
      if (selected_format->second != kFormatText && selected_format->second != kFormatJson) {
        return fail(kErrorFormat);
      }
      print_json = selected_format->second == kFormatJson;
    }
  }
  if (command == kCommandVisualize) {
    auto label_format = args.values.find(kOptionLabelFormat);
    if (label_format != args.values.end()) {
      if (args.flags.count(kOptionUtf8Labels) > 0 ||
          (label_format->second != kLabelFormatHex && label_format->second != kLabelFormatUtf8)) {
        return fail(kErrorLabelFormat);
      }
      utf8_labels = label_format->second == kLabelFormatUtf8;
    }
    if ((args.values.count(kOptionFormat) == 0)) {
      return fail(kErrorMissingFormat);
    }
    if (!parse_graph_format(args.values.at(kOptionFormat), format)) {
      return fail(kErrorFormat);
    }
    const std::pair<const char*, uint64_t*> graph_bounds[] = {
        {kOptionMaxNodes, &max_nodes}, {kOptionRootId, &root}, {kOptionMaxDepth, &depth}};
    for (const auto& item : graph_bounds) {
      auto it = args.values.find(item.first);
      if (it != args.values.end() && !number(it->second, *item.second)) {
        return fail(kErrorGraphBound);
      }
    }
    if (max_nodes == 0 || max_nodes > SIZE_MAX || root >= UINT32_MAX || depth > UINT32_MAX) {
      return fail(kErrorGraphRange);
    }
  }
  // A loaded snapshot is validated before it becomes available to inspection or visualization.
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
  if (command == kCommandPrint) {
    return print_file(value, print_section, print_json, args);
  }
  if (command == kCommandInfo) {
    std::cout << kImageInfoPrefix
              << (stats.boundary_rules > 0 ? kBoundaryImageVersion
                                           : (stats.replacements ? kReplacementImageVersion : kImageVersion))
              << kNodesField << stats.nodes << kKeywordsField << stats.keywords << kSymbolsField << stats.symbols
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
  value.visit_nodes(select_node, &graph);
  if (graph.overflow) {
    return fail(kErrorGraphLimit);
  }
  if (!graph.complete) {
    return fail(kErrorGraphWrite);
  }
  publication file(args.values.at(kOptionOutput));
  if (file.file() == nullptr) {
    return fail(kErrorCreateOutput);
  }
  file_buffer buffer(file.file());
  std::ostream output(&buffer);
  if (!write_graph(output, value, graph, format, utf8_labels)) {
    return fail(kErrorGraphWrite);
  }
  output.flush();
  if (output.fail() || !file.commit()) {
    return fail(kErrorPublishOutput);
  }
  return kExitSuccess;
}
