// Copyright 2026 atframework
#pragma once

#include <config/atframe_utils_build_feature.h>
#include <config/compile_optimize.h>
#include <gsl/select-gsl.h>

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <vector>

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN

namespace string {
namespace detail {
struct ac_v2_storage;
}

/**
 * @brief Compact Aho-Corasick automaton with shared immutable lookup storage.
 *
 * Tokenizers split input into opaque byte tokens without encoding conversion. Configure
 * the automaton before the first successful insertion, insert keywords, then call build().
 * Built copies share storage; modifying a collecting copy detaches its builder. Build,
 * load and reset replace only the current handle. Keyword/token views borrow their
 * snapshot and remain valid while at least one automaton owns that snapshot.
 *
 * Const queries may run concurrently with separate match_context instances. Mutations
 * of a queried handle and mutations/builds of shared collecting snapshots require external
 * synchronization. Callbacks must not mutate or destroy the handle being queried.
 * Input views must describe readable bytes of their advertised length; output spans
 * must describe writable match_t objects. All supplied storage must stay live for the
 * entire call. Callbacks must not invalidate buffers or mutate/move the active match_context;
 * nested scans require a different context. Tokenizer callbacks have the same lifetime
 * restrictions during insertion and must not mutate/destroy the handle being updated.
 * Allocation, stream and user callback exceptions propagate; status values describe
 * returned failures. In particular, vector rollback applies to returned scan errors.
 *
 * @par Frozen memory layout
 * The handle owns one shared_ptr; vectors, strings and the control block have
 * implementation-defined overhead. The logical payload uses the following separate
 * contiguous arrays. N includes root node 0, K is the keyword count, S is the symbol
 * count, A/W are the symbol/keyword byte-pool sizes, and Lmax is the longest keyword
 * in canonical, non-skipped tokens. All record fields below are uint32_t.
 * @code{.text}
 * Node[N]                 16 bytes: child_begin, child_count, fail, output_head
 * Label[N]                 4 bytes: incoming symbol ID; Label[0] = 0
 * SymbolOffsets[S + 1]     4 bytes: offsets[0] = 0, offsets[S] = A
 * SymbolBytes[A]           1 byte : concatenated opaque tokens, no terminators/padding
 * SkipBits[ceil(S / 8)]    1 byte : symbol ID i uses bit (i - 1)
 * Keyword[K]              16 bytes: byte_offset, byte_length, token_count, suffix_output
 * KeywordBytes[W]          1 byte : concatenated original keyword bytes
 * FoldTo[nocase ? S : 0]   4 bytes: symbol ID -> final canonical symbol ID
 * @endcode
 * Logical payload bytes are 20*N + 16*K + 4*(S+1) + A + W + ceil(S/8), plus
 * 4*S when nocase is enabled. Derived indices, allocator overhead and scratch are separate.
 * Symbols have IDs 1..S in unsigned-byte lexicographic order; 0 means unknown.
 * Token i occupies [SymbolOffsets[i - 1], SymbolOffsets[i]); offsets are strictly
 * increasing, so long tokens need no packed length field. Each byte pool is limited
 * to UINT32_MAX bytes. Counts/IDs use 32 bits, with UINT32_MAX reserved as invalid.
 * Keyword IDs start at 0 and preserve first-insertion order.
 *
 * Nodes use breadth-first numbering. A parent's sorted child labels occupy
 * [child_begin, child_begin + child_count); a label's array index is also its target
 * node ID. Child intervals partition nodes 1..N-1 in parent order. Leaves retain
 * the current child_begin, possibly N, with child_count=0. Root.fail=0; other failure
 * links target shallower nodes with smaller IDs. output_head/suffix_output use
 * UINT32_MAX for no keyword. A terminal's output head is its own keyword; other
 * nodes inherit the failure node's head. Suffix outputs strictly decrease in token
 * count. FoldTo is idempotent; trie labels and skip bits refer to canonical IDs.
 *
 * Lookup uses binary search over contiguous labels, with direct 0/1-child cases.
 * Symbol lookup derives 257 uint32_t first-byte bucket boundaries and 256 uint32_t
 * single-byte IDs (2052 bytes) per snapshot. These indices are not serialized.
 * Frozen lookup retains no map/hash builder. Match positions use a per-query ring
 * of at most min(Lmax, input byte size) size_t entries; contains() needs no ring.
 * The collecting builder may use standard map/hash containers and is released when
 * its last collecting owner releases it. XXH64 is used for interning/integrity only.
 *
 * @par Binary image layout, version 2.0
 * Integers are little endian and are read/written through algorithm/bit.h. Opaque
 * byte pools are never transcoded or byte-swapped. The 128-byte header is:
 * @code{.text}
 * Offset  Width  Field
 *   0       12   ASCII "ACAUTOMATION", no NUL
 *  12        2   major = 2
 *  14        2   minor = 0
 *  16        4   header_bytes = 128
 *  20        4   feature_flags: bit 0 = nocase; other bits zero
 *  24        4   endian_tag = 0x01020304
 *  28        4   section_count = 8 when writing
 *  32        8   file_bytes, including the footer
 *  40        8   directory_offset = 128
 *  48        4   N
 *  52        4   K
 *  56        4   S
 *  60        4   Lmax
 *  64        2   Node stride = 16
 *  66        2   Label width = 4
 *  68        2   Keyword stride = 16
 *  70        2   SymbolOffsets width = 4
 *  72        4   checksum_algorithm = 1 (XXH64, seed 0)
 *  76        4   reserved = 0
 *  80        8   header_hash64
 *  88        8   checksum_offset = file_bytes - 8
 *  96       32   reserved, all zero
 * @endcode
 * Each 32-byte directory entry is {type:u32, flags:u32, offset:u64,
 * byte_size:u64, count:u64}. Writers emit these eight required sections in order:
 * @code{.text}
 * Type  Section         Count                 Byte size
 *   1   Node            N                     16 * N
 *   2   Label           N                      4 * N
 *   3   SymbolOffsets   S + 1                  4 * (S + 1)
 *   4   SymbolBytes     A                      A
 *   5   SkipBits        ceil(S / 8)            ceil(S / 8)
 *   6   Keyword         K                     16 * K
 *   7   KeywordBytes    W                      W
 *   8   FoldTo          nocase ? S : 0         nocase ? 4 * S : 0
 * @endcode
 * Records have the field order shown in the memory-layout table and are explicitly
 * encoded rather than copied as C++ structs. Section offsets and the footer are
 * 8-byte aligned; all gaps/padding and unused skip bits are zero. Even S=0 retains
 * one zero SymbolOffsets entry. No pointers, tokenizer/encoding identifiers, Unicode
 * version, query options, derived indices or CPU settings enter the image.
 *
 * header_hash64 is XXH64 over the 128-byte header with bytes 80..87 zeroed.
 * The footer is one little-endian uint64_t: XXH64 over [0, checksum_offset), including
 * the filled header hash, directory, sections and padding. Both hashes use seed 0.
 * The footer permits buffered serialization and hashing in one pass without seeking.
 *
 * Load checks budgets, section ranges, hashes, token ordering, child partitions,
 * failure depths and output chains before publishing typed storage. Optional semantic
 * validation also recomputes longest failure suffixes; it cannot reconstruct the
 * original tokenizer or reinterpret original keyword bytes. Compatible minor versions
 * with unchanged known layouts are accepted. Readers allow 8..64 directory entries;
 * known sections require flags=0, while unknown sections require flags=1 (optional).
 * Unknown major versions, required sections and feature flags are rejected. Raw input
 * bytes are never reinterpreted as live C++ objects; load copies into owned arrays.
 */
class ATFW_UTIL_SYMBOL_VISIBLE ac_automation_v2 {
 public:
  enum class status {
    kOk,
    kAlreadyExists,
    kInvalidState,
    kInvalidArgument,
    kEmptyKeyword,
    kInvalidEncoding,
    kInvalidTokenizerResult,
    kCapacityExceeded,
    kInvalidFormat,
    kUnsupportedVersion,
    kChecksumMismatch,
    kIoError,
    kResourceLimit
  };
  enum class token_status { kValid, kInvalid };

  struct ATFW_UTIL_SYMBOL_VISIBLE token_result {
    token_status state;  ///< Valid token or safely recoverable invalid encoding unit.
    size_t byte_count;   ///< Must be in [1, remaining.size()] for a nonempty input.
  };

  /**
   * @brief Split the first token of a nonempty remaining byte view.
   * @return Token validity and consumed byte count; invalid units also require a safe,
   *         positive count. Zero/oversized counts and unknown states fail the scan.
   * @note The second argument is tokenizer::user_data. Return boundaries into the
   *       supplied bytes; do not transform bytes or retain the temporary view object.
   */
  using tokenize_fn = token_result (*)(gsl::string_view, void*);
  enum class token_encoding { kUtf8, kUtf16Le, kUtf16Be, kCustom };

  struct ATFW_UTIL_SYMBOL_VISIBLE tokenizer {
    token_encoding encoding;
    tokenize_fn next;
    void* user_data;

    /** @brief Select the built-in UTF-8 tokenizer with no callback/user data. */
    ATFRAMEWORK_UTILS_API tokenizer() noexcept;

    /**
     * @brief Select a built-in encoding without converting input bytes.
     * @param value UTF-8, UTF-16LE or UTF-16BE; kCustom requires a callback instead.
     * @note Invalid selections are rejected by insert_keyword()/match(), not here.
     */
    ATFRAMEWORK_UTILS_API explicit tokenizer(token_encoding value) noexcept;

    /**
     * @brief Select a caller-owned custom tokenizer.
     * @param callback Non-null callback obeying the tokenize_fn contract.
     * @param data Opaque pointer passed to callback; may be null.
     * @note Callback and data are borrowed for the synchronous insertion/scan only.
     *       The callback must not mutate its automaton or invalidate the input bytes.
     */
    ATFRAMEWORK_UTILS_API tokenizer(tokenize_fn callback, void* data = nullptr) noexcept;
  };
  enum class invalid_input_policy { kSkipCurrent, kReject };

  struct ATFW_UTIL_SYMBOL_VISIBLE match_options {
    tokenizer tokens;
    bool allow_overlap;
    size_t max_matches;  // Zero means unlimited occurrences.
    invalid_input_policy invalid_input;

    /** @brief Use UTF-8, nonoverlapping unlimited matches, and kSkipCurrent recovery. */
    ATFRAMEWORK_UTILS_API match_options() noexcept;
  };
  enum class stop_reason { kEndOfInput, kMatchLimit, kOutputFull, kCallbackStopped, kError };

  struct ATFW_UTIL_SYMBOL_VISIBLE match_report {
    status code;
    stop_reason stopped;
    size_t bytes_consumed;        ///< Input prefix consumed; excludes a rejected invalid unit.
    size_t matches;               ///< Delivered hits; vector rollback resets this to zero.
    size_t invalid_units;         ///< Number of tokenizer-reported invalid encoding units.
    size_t first_invalid_offset;  // SIZE_MAX when no invalid unit was seen.

    /** @brief Initialize a kOk/kEndOfInput report with zero counts and SIZE_MAX invalid offset. */
    ATFRAMEWORK_UTILS_API match_report() noexcept;
  };

  struct ATFW_UTIL_SYMBOL_VISIBLE match_t {
    size_t start;              ///< Offset into the original input, in bytes.
    size_t length;             ///< Byte range including skips between matched tokens.
    uint32_t keyword_id;       ///< Stable first-insertion ID in the current snapshot.
    gsl::string_view keyword;  ///< Borrowed original keyword bytes from the snapshot.
  };

  class ATFW_UTIL_SYMBOL_VISIBLE match_context {
   public:
    /** @brief Create reusable query scratch with no retained scan state. */
    ATFRAMEWORK_UTILS_API match_context();

    /** @brief Release this context's scratch allocation. */
    ATFRAMEWORK_UTILS_API ~match_context();

    /** @brief Transfer scratch ownership from another context. */
    ATFRAMEWORK_UTILS_API match_context(match_context&&) noexcept;

    /** @brief Replace scratch with another context's allocation and return *this. */
    ATFRAMEWORK_UTILS_API match_context& operator=(match_context&&) noexcept;

    /** @brief Copying mutable query scratch is disabled; use one context per concurrent scan. */
    match_context(const match_context&) = delete;

    /** @brief Copy assignment is disabled; contexts can be moved or independently reserved. */
    match_context& operator=(const match_context&) = delete;

    /**
     * @brief Reserve origin-ring capacity without changing the automaton or scan state.
     * @param token_count Minimum number of size_t origin entries to accommodate.
     * @note match() grows scratch automatically; explicit preparation is optional.
     */
    ATFRAMEWORK_UTILS_API void reserve(size_t token_count);

    /** @brief Return the retained origin-ring capacity, in token entries rather than bytes. */
    ATFRAMEWORK_UTILS_API size_t capacity() const noexcept;

   private:
    friend struct detail::ac_v2_storage;
    std::vector<size_t> origins_;
  };

  struct ATFW_UTIL_SYMBOL_VISIBLE case_fold_rule {
    gsl::string_view source_token;
    gsl::string_view target_token;
  };

  struct ATFW_UTIL_SYMBOL_VISIBLE load_options {
    uint64_t max_file_bytes;
    uint64_t max_memory_bytes;  // Candidate payload and validation scratch; excludes old snapshots/input.
    bool verify_semantics;

    /**
     * @brief Limit file and candidate memory to 1 GiB each; disable semantic recomputation.
     * @note Memory limits exclude existing snapshots, caller input, allocator overhead
     *       and process RSS. Structural and integrity validation always runs.
     */
    ATFRAMEWORK_UTILS_API load_options() noexcept;
  };

  struct ATFW_UTIL_SYMBOL_VISIBLE statistics {
    uint32_t nodes;
    uint32_t keywords;
    uint32_t symbols;
    uint32_t max_keyword_tokens;
    uint64_t payload_bytes;
    uint32_t symbol_index_bytes;
    bool nocase;
  };

  struct ATFW_UTIL_SYMBOL_VISIBLE node_view {
    uint32_t id;
    uint32_t child_begin;
    uint32_t child_count;
    uint32_t failure;
    uint32_t output_head;  // UINT32_MAX means no keyword.
    uint32_t symbol_id;    // Root is zero; other symbols start at one.
    gsl::string_view token;
  };

  /** @brief Borrowed row of the frozen symbol, folding and skip tables. */
  struct ATFW_UTIL_SYMBOL_VISIBLE symbol_view {
    uint32_t id;                    ///< Symbol ID in [1, statistics::symbols]; zero is never visited.
    uint32_t folded_id;             ///< Final canonical ID; equals id when case folding is disabled.
    gsl::string_view token;         ///< Original opaque token bytes, without encoding conversion.
    gsl::string_view folded_token;  ///< Bytes of folded_id; borrows the same snapshot as token.
    bool skipped;                   ///< Stored skip bit for id; matching tests the bit for folded_id.
  };

  /** @brief Receive a synchronous hit; return false to request successful early termination. */
  using match_callback = bool (*)(const match_t&, void*);

  /** @brief Receive a synchronous node view; return false to stop visit_nodes(). */
  using node_callback = bool (*)(const node_view&, void*);

  /** @brief Receive a synchronous symbol view; return false to stop visit_symbols(). */
  using symbol_callback = bool (*)(const symbol_view&, void*);

  /** @brief Create an empty collecting automaton with case-sensitive matching and no skips/folds. */
  ATFRAMEWORK_UTILS_API ac_automation_v2();

  /** @brief Release this handle; other owners keep the shared snapshot and its views alive. */
  ATFRAMEWORK_UTILS_API ~ac_automation_v2();

  /** @brief Share another handle's collecting or built storage without copying its tables. */
  ATFRAMEWORK_UTILS_API ac_automation_v2(const ac_automation_v2&) noexcept;

  /** @brief Share another handle's storage, release this handle's previous ownership, and return *this. */
  ATFRAMEWORK_UTILS_API ac_automation_v2& operator=(const ac_automation_v2&) noexcept;

  /** @brief Transfer storage ownership; the source may be reset or initialized again. */
  ATFRAMEWORK_UTILS_API ac_automation_v2(ac_automation_v2&&) noexcept;

  /** @brief Replace this handle with another handle's storage and return *this. */
  ATFRAMEWORK_UTILS_API ac_automation_v2& operator=(ac_automation_v2&&) noexcept;

  /**
   * @brief Replace this handle with an empty collecting automaton and default settings.
   * @note Shared copies keep their previous storage. Views into the previous storage
   *       require another owner to remain alive.
   */
  ATFRAMEWORK_UTILS_API void reset();

  /** @brief Return whether this handle has a frozen snapshot ready for queries and dump(). */
  ATFRAMEWORK_UTILS_API bool is_built() const noexcept;

  /**
   * @brief Enable or disable canonical token folding before the first successful insertion.
   * @param value Enable single-byte ASCII A-Z to a-z and explicit set_case_folding() rules.
   * @return kOk on success; kInvalidState after insertion/build. Returned failures preserve settings.
   * @note Folding compares opaque tokens; UTF-16 and other multibyte case rules must
   *       be supplied explicitly. No Unicode conversion or full Unicode folding occurs.
   */
  ATFRAMEWORK_UTILS_API status set_nocase(bool value);

  /**
   * @brief Add one opaque token to the skip set before the first successful insertion.
   * @param token Nonempty raw token bytes; copied without invoking a tokenizer.
   * @return kOk, kInvalidArgument for an empty/oversized token, or kInvalidState.
   * @note Folding precedes skip lookup. Duplicate entries are harmless.
   */
  ATFRAMEWORK_UTILS_API status set_skip(gsl::string_view token);

  /**
   * @brief Atomically add a set of opaque skip tokens; existing entries remain configured.
   * @param tokens Nonempty raw tokens; views are copied during this call. An empty span is allowed.
   * @return kOk, kInvalidArgument for an invalid token, or kInvalidState after insertion/build.
   * @note Returned failures leave the previous set intact; no tokenizer/encoding is stored.
   */
  ATFRAMEWORK_UTILS_API status set_skip(gsl::span<const gsl::string_view> tokens);

  /**
   * @brief Remove an exact configured skip token before the first successful insertion.
   * @param token Nonempty original bytes supplied to set_skip(), before canonical folding.
   * @return kOk even if absent, kInvalidArgument for an empty token, or kInvalidState.
   */
  ATFRAMEWORK_UTILS_API status unset_skip(gsl::string_view token);

  /**
   * @brief Atomically replace the explicit one-token-to-one-token case rules.
   * @param rules Nonempty source/target byte tokens; copied without tokenization. Empty clears custom rules.
   * @return kOk, kInvalidState after insertion/build, or kInvalidArgument for empty/oversized
   *         tokens, conflicting sources, ASCII-rule conflicts or noncanonical targets.
   * @note Each target may map only to itself. Rules are validated even while nocase
   *       is disabled and applied only when set_nocase(true) is active.
   */
  ATFRAMEWORK_UTILS_API status set_case_folding(gsl::span<const case_fold_rule> rules);

  /**
   * @brief Tokenize, fold and remove skips, then insert one keyword into the collecting trie.
   * @param text Original bytes, including possible NULs; original spelling is copied for returned views.
   * @param keyword_id Optional output, written only on kOk/kAlreadyExists with the stable keyword ID.
   * @param tokens Input-boundary tokenizer; defaults to UTF-8 and is not retained by storage.
   * @return kOk when inserted; kAlreadyExists reuses the first canonical token sequence.
   *         Errors include kInvalidState, kInvalidArgument, kEmptyKeyword (including all-skipped
   *         input), kInvalidEncoding, kInvalidTokenizerResult, kCapacityExceeded or kInvalidFormat.
   * @note Invalid input is always rejected for insertion. Returned failures do not insert
   *       a partial keyword. The first successful insertion freezes all configuration.
   */
  ATFRAMEWORK_UTILS_API status insert_keyword(gsl::string_view text, uint32_t* keyword_id = nullptr,
                                              const tokenizer& tokens = tokenizer());

  /**
   * @brief Freeze the collecting trie into compact lookup tables for this handle.
   * @return kOk, including an empty/already-built automaton, kCapacityExceeded, or kInvalidFormat for
   *         inconsistent builder tables.
   * @note A candidate snapshot is published only on success. Copies of a collecting
   *       snapshot remain collecting; copies of a built snapshot remain unchanged.
   */
  ATFRAMEWORK_UTILS_API status build();

  /**
   * @brief Append matching byte ranges and borrowed keyword views to a vector.
   * @param input Bounded original bytes; no terminating NUL or encoding conversion is required.
   * @param output Existing hits are retained. A returned scan error rolls back this call's additions.
   * @param options Per-call tokenizer, overlap, occurrence limit and invalid-input policy.
   * @param context Optional exclusive scratch; null uses temporary scratch for this call.
   * @return Scan status, stopping reason, consumed bytes and counters. Not-built handles
   *         return kInvalidState; invalid options/tokenizer results return the corresponding error.
   * @note Default selection is earliest ending, then longest token count at the same end;
   *       it emits nonoverlapping hits. Overlap mode also emits the suffix-output chain.
   *       Limits count occurrences, not distinct keywords. Skips inside a hit remain in
   *       its byte range. kSkipCurrent clears the candidate on an invalid unit and resumes;
   *       kReject stops before that unit. Returned views require an owner of this snapshot.
   *       Input and output storage must remain valid for the whole scan, including callbacks.
   */
  ATFRAMEWORK_UTILS_API match_report match(gsl::string_view input, std::vector<match_t>& output,
                                           const match_options& options = match_options(),
                                           match_context* context = nullptr) const;

  /**
   * @brief Write hits into caller-owned slots using the same selection rules as vector match().
   * @param input Bounded original input bytes.
   * @param output Destination slots; report.matches is the number written. Filling the
   *               span stops with kOutputFull; an empty span consumes no input.
   * @param options Per-call tokenizer, overlap, occurrence limit and invalid-input policy.
   * @param context Optional exclusive reusable scratch; null uses temporary scratch.
   * @return Scan report, including already-written hits if a later encoding/tokenizer error occurs.
   * @note Written slots cannot be rolled back. Keyword views borrow the built snapshot.
   */
  ATFRAMEWORK_UTILS_API match_report match(gsl::string_view input, gsl::span<match_t> output,
                                           const match_options& options = match_options(),
                                           match_context* context = nullptr) const;

  /**
   * @brief Deliver hits synchronously with the same selection rules as vector match().
   * @param input Bounded original input bytes.
   * @param callback Non-null callback; returning false requests successful early termination.
   * @param user_data Opaque pointer passed unchanged to callback; may be null.
   * @param options Per-call tokenizer, overlap, occurrence limit and invalid-input policy.
   * @param context Optional exclusive reusable scratch; null uses temporary scratch.
   * @return Scan report counting delivered callbacks, including deliveries before a later error.
   * @note The match_t reference is valid only during callback. Copied keyword views still
   *       require a snapshot owner. Callback deliveries cannot be rolled back, and callback
   *       exceptions propagate. The callback must not mutate/destroy this handle.
   */
  ATFRAMEWORK_UTILS_API match_report match(gsl::string_view input, match_callback callback, void* user_data,
                                           const match_options& options = match_options(),
                                           match_context* context = nullptr) const;

  /**
   * @brief Test for a keyword and stop immediately after the first hit without a position ring.
   * @param input Bounded original input bytes.
   * @param found Set false initially, then true only when a hit is found.
   * @param options Tokenizer and invalid-input policy; overlap/count settings do not change this query.
   * @return Scan report with matches=1 on a hit; ordinary scan errors are reported as in match().
   * @note Bytes after the first hit are not inspected, including with kReject policy.
   */
  ATFRAMEWORK_UTILS_API match_report contains(gsl::string_view input, bool& found,
                                              const match_options& options = match_options()) const;

  /**
   * @brief Write the built snapshot in the binary format documented on this class.
   * @param output Binary output stream; seeking is not required.
   * @return kOk, kInvalidState for an unbuilt handle, kInvalidFormat for inconsistent tables, or
   *         kIoError for returned write failures.
   * @note A failed write may leave a partial image. Stream exceptions propagate.
   */
  ATFRAMEWORK_UTILS_API status dump(std::ostream& output) const;

  /**
   * @brief Serialize a built snapshot into an owned byte buffer.
   * @param output Replaced only on success; existing bytes remain on returned failure.
   * @return kOk, kInvalidState, kInvalidFormat, kResourceLimit for an unrepresentable buffer size, or kIoError.
   */
  ATFRAMEWORK_UTILS_API status dump(std::vector<unsigned char>& output) const;

  /**
   * @brief Read and validate one image, then replace only this handle's snapshot.
   * @param input Binary input stream; seeking is not required. Trailing stream data is left unread.
   * @param options File/candidate-memory budgets and optional semantic verification.
   * @return kOk or a format/version/checksum/resource/I/O error. Returned failures keep the old snapshot.
   * @note Input consumption is not rolled back on failure. Decoded tables own their bytes;
   *       tokenizers are neither invoked nor recovered from the image. Exceptions propagate.
   */
  ATFRAMEWORK_UTILS_API status load(std::istream& input, const load_options& options = load_options());

  /**
   * @brief Load an exact image-sized byte view into independently owned, validated storage.
   * @param input Complete binary image; trailing bytes are rejected. The view is borrowed only during this call.
   * @param options File/candidate-memory budgets and optional semantic verification.
   * @return kOk or a format/version/checksum/resource/I/O error; the old snapshot remains on failure.
   * @note No C++ object mapping or encoding conversion occurs. Existing copies continue
   *       owning their previous snapshot after this handle successfully loads a new one.
   */
  ATFRAMEWORK_UTILS_API status load(gsl::span<const unsigned char> input, const load_options& options = load_options());

  /**
   * @brief Load binary image bytes from a length-bounded string view, including embedded NULs.
   * @param input Exact binary image; copied into owned tables and never treated as text.
   * @param options File/candidate-memory budgets and optional semantic verification.
   * @return Same validation result and failure-atomic behavior as the span overload.
   */
  ATFRAMEWORK_UTILS_API status load(gsl::string_view input, const load_options& options = load_options());

  /**
   * @brief Return built-table counts and logical payload/index sizes; return all zeros if unbuilt.
   * @return Table payload and derived index bytes exclude allocator capacity, control blocks and query scratch.
   */
  ATFRAMEWORK_UTILS_API statistics get_statistics() const noexcept;

  /**
   * @brief View the original bytes of the first keyword stored for an ID.
   * @param id Stable keyword ID from insertion or a match.
   * @return Borrowed bytes, or an empty view for an invalid ID/unbuilt handle.
   * @note The view remains valid while an automaton owns this exact snapshot.
   */
  ATFRAMEWORK_UTILS_API gsl::string_view keyword(uint32_t id) const noexcept;

  /**
   * @brief Visit built nodes in breadth-first ID order, including root 0.
   * @param callback Non-null synchronous visitor; false stops iteration.
   * @param user_data Opaque pointer passed unchanged to callback; may be null.
   * @return True if every node was visited; false for an unbuilt handle, null callback or early stop.
   * @note The node_view reference is temporary; token views borrow the snapshot. The
   *       visitor must not mutate/destroy this handle. Callback exceptions propagate.
   */
  ATFRAMEWORK_UTILS_API bool visit_nodes(node_callback callback, void* user_data) const;

  /**
   * @brief Visit every frozen symbol in ID order, including configuration-only symbols.
   * @param callback Non-null synchronous visitor; false stops iteration.
   * @param user_data Opaque pointer passed unchanged to callback; may be null.
   * @return True if every symbol was visited (including an empty table); false for an
   *         unbuilt handle, null callback or early stop.
   * @note Iteration allocates no storage. Only effective canonical skip/fold rules are
   *       available; original configuration options and disabled custom folds are not retained.
   *       The symbol_view reference is temporary; token views borrow the snapshot and
   *       remain valid while an automaton owns it. The visitor must not mutate/destroy
   *       this handle. Callback exceptions propagate.
   */
  ATFRAMEWORK_UTILS_API bool visit_symbols(symbol_callback callback, void* user_data) const;

  /**
   * @brief Split one strictly valid UTF-8 scalar from a bounded byte view.
   * @param input Remaining input; may be empty or non-NUL-terminated.
   * @param unused Ignored callback data, accepted for tokenize_fn compatibility.
   * @return kValid with length 1..4, or kInvalid with length 1 for malformed/truncated
   *         nonempty input. Empty input returns kInvalid with length 0.
   * @note Overlong forms, surrogate values and values above U+10FFFF are rejected.
   */
  ATFRAMEWORK_UTILS_API static token_result next_utf8(gsl::string_view input, void* unused = nullptr) noexcept;

  /**
   * @brief Split one UTF-16LE scalar without alignment or terminator requirements.
   * @param input Remaining original little-endian UTF-16 bytes; may be empty.
   * @param unused Ignored callback data.
   * @return kValid with length 2 or 4; invalid surrogates consume 2 bytes, an odd final
   *         byte consumes 1, and empty input returns kInvalid with length 0.
   */
  ATFRAMEWORK_UTILS_API static token_result next_utf16le(gsl::string_view input, void* unused = nullptr) noexcept;

  /**
   * @brief Split one UTF-16BE scalar without converting the returned original byte token.
   * @param input Remaining original big-endian UTF-16 bytes; may be empty/unaligned.
   * @param unused Ignored callback data.
   * @return The same validity/recovery lengths as next_utf16le(), using big-endian code units.
   */
  ATFRAMEWORK_UTILS_API static token_result next_utf16be(gsl::string_view input, void* unused = nullptr) noexcept;

  /**
   * @brief Return a stable, non-owning diagnostic name for a status value.
   * @param value Status to describe.
   * @return Static-storage string view, or "unknown_status" for an unrecognized value.
   */
  ATFRAMEWORK_UTILS_API static gsl::string_view status_message(status value) noexcept;

 private:
  std::shared_ptr<detail::ac_v2_storage> storage_;
};
}  // namespace string

ATFRAMEWORK_UTILS_NAMESPACE_END
