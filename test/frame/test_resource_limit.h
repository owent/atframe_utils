// Copyright 2026 atframework

#pragma once

#include <config/atframe_utils_build_feature.h>

#include "test_framework_export.h"  // NOLINT(build/include_subdir)

#include <stddef.h>
#include <stdint.h>

#include <string>

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace testing {

/**
 * @brief Full scale of all permyriad (per-ten-thousand, 万分率) ratio fields: 10000 means 100%.
 */
static const int64_t kTestResourceLimitPermyriadFull = 10000;
/**
 * @brief Resource limit options for one scope (whole test program, one test suite or one test case).
 *
 * Every field keeps the framework default when left zero, so callers only set what they want to override:
 * - *_permyriad fields are integer ratios in per-ten-thousand (万分率): 10000 means 100%, 9000 means 90%.
 *   Values above kTestResourceLimitPermyriadFull are clamped to 100%.
 * - Set a field to a negative value to explicitly disable that limit for the scope.
 *
 * Effective limits resolve per scope with "explicit configuration wins": an explicitly configured
 * suite/case field overrides the program scope value, including when it relaxes the program default
 * (e.g. benchmark cases may raise the memory cap or timeout). Fields never configured inherit the
 * parent scope. The program scope timeout yields to an explicit suite/case time budget: a suite or
 * case with an explicitly configured (or explicitly disabled) timeout is not killed by the program
 * timeout while it still has time budget left. The OS-level memory cap (Windows job object) is raised
 * to the maximum explicitly configured memory limit so relaxed scopes are not throttled by it.
 */
struct test_resource_limit_options {
  // Absolute memory consumption cap in bytes. Compared against the resident set size of the test process.
  size_t memory_limit_bytes = 0;
  // Memory cap as a permyriad (10000 = 100%) of the total physical memory of the machine.
  int64_t memory_limit_physical_memory_permyriad = 0;
  // Memory cap as a permyriad (10000 = 100%) of the total physical memory plus swap (virtual memory) of
  // the machine.
  int64_t memory_limit_physical_and_virtual_permyriad = 0;
  // Max CPU usage as a permyriad (10000 = 100%) of the total logical CPU capacity of the machine. When
  // unset, the default policy keeps at least one logical core free on multi-core machines and caps usage
  // at 90% (9000) on single-core machines. CPU usage is throttled without changing CPU affinity (which may
  // conflict with policies of the code under test): Windows uses a job object CPU rate hard cap,
  // Linux/Android uses a cgroup cpu.max / cpu.cfs_quota_us limit. When no such mechanism is available, no
  // CPU limit is applied. An explicit suite/case value is applied while that scope runs and the program
  // scope value is restored afterwards.
  int64_t cpu_limit_permyriad = 0;
  // Wall clock timeout of this scope in seconds. For the whole program scope this is the total run time
  // (default: 1800), for a test suite it is the total time of the suite (default: no limit), for a test
  // case it is the per-case timeout (default: 600). The test process is killed when a timeout is exceeded.
  int64_t timeout_seconds = 0;
};

/**
 * @brief Enable or disable all test resource limits (default: enabled).
 * @note Memory limits are always ignored when the test binary is built with a sanitizer
 *       (ASan/MSan/TSan/HWASan/DFSan), because sanitizers inflate the address space and make real
 *       memory consumption impossible to measure accurately.
 * @note Call before the test run starts (or before setup_test_resource_limit) for a complete disable;
 *       OS-level limits already applied can not be fully revoked on every platform.
 */
ATFRAMEWORK_TEST_API void set_test_resource_limit_enabled(bool enabled);
ATFRAMEWORK_TEST_API bool get_test_resource_limit_enabled();

/**
 * @brief Set resource limits for the whole test program.
 * @note Unset memory fields fall back to the default cap:
 *       min(90% of the available physical memory at process start, 8GiB).
 */
ATFRAMEWORK_TEST_API void set_test_resource_limit(const test_resource_limit_options &options);

/**
 * @brief Get the configured limits of the whole test program, so callers can change only some fields
 *        and set them back. Fields never configured keep their zero (default) values.
 */
ATFRAMEWORK_TEST_API test_resource_limit_options get_test_resource_limit();

/**
 * @brief Set resource limits for one test suite (group). Explicitly configured fields override the
 *        program scope while the suite runs; the OS-level memory cap (job object / cgroup) is raised
 *        to cover the configured limit so the suite is not throttled by the program scope cap.
 */
ATFRAMEWORK_TEST_API void set_test_suite_resource_limit(const std::string &suite_name,
                                                        const test_resource_limit_options &options);

/**
 * @brief Get the configured limits of one test suite, so callers can change only some fields and set
 *        them back.
 * @return false when no limit has been configured for the suite (out is reset to defaults).
 */
ATFRAMEWORK_TEST_API bool get_test_suite_resource_limit(const std::string &suite_name,
                                                        test_resource_limit_options &out);

/**
 * @brief Set resource limits for one test case. Explicitly configured fields override the program and
 *        suite scopes while the case runs.
 */
ATFRAMEWORK_TEST_API void set_test_case_resource_limit(const std::string &suite_name, const std::string &case_name,
                                                       const test_resource_limit_options &options);

/**
 * @brief Get the configured limits of one test case, so callers can change only some fields and set
 *        them back.
 * @return false when no limit has been configured for the case (out is reset to defaults).
 */
ATFRAMEWORK_TEST_API bool get_test_case_resource_limit(const std::string &suite_name, const std::string &case_name,
                                                       test_resource_limit_options &out);

/**
 * @brief Get the configured limits of the currently running test suite: its own configured limits when
 *        present, otherwise the configured limits of the parent scope (the whole test program).
 * @return false when no test suite is currently running (called outside the run loop).
 */
ATFRAMEWORK_TEST_API bool get_current_test_suite_resource_limit(test_resource_limit_options &out);

/**
 * @brief Get the configured limits of the currently running test case: its own configured limits when
 *        present, otherwise the configured limits of the currently running suite, otherwise those of
 *        the whole test program.
 * @return false when no test case is currently running (called outside the run loop).
 */
ATFRAMEWORK_TEST_API bool get_current_test_case_resource_limit(test_resource_limit_options &out);

/**
 * @brief Set resource limits for the currently running test suite, without naming it explicitly.
 *        Timeout, memory and CPU changes take effect on the next watchdog tick (100ms) and are
 *        restored to the program scope values when the suite ends.
 * @return false when no test suite is currently running (called outside the run loop).
 */
ATFRAMEWORK_TEST_API bool set_current_test_suite_resource_limit(const test_resource_limit_options &options);

/**
 * @brief Set resource limits for the currently running test case, without naming it explicitly.
 *        Typically called at the top of a test case body. Timeout, memory and CPU changes take effect
 *        on the next watchdog tick (100ms) and are restored to the parent scope values when the case
 *        ends.
 * @return false when no test case is currently running (called outside the run loop).
 */
ATFRAMEWORK_TEST_API bool set_current_test_case_resource_limit(const test_resource_limit_options &options);

/**
 * @brief Apply the currently configured limits and start the watchdog. Idempotent.
 * @note Called automatically by run_tests()/run_event_on_start(), which the framework entry points invoke
 *       for the private runner as well as for the GoogleTest and Boost.Test integrations. Manual calls are
 *       only needed by custom test main functions that do not use the framework entry points.
 *
 * Enforcement overview:
 * - Memory: on Windows the per-process commit charge is limited with a job object, so allocations fail
 *   once the cap is exceeded (system level OOM). On Linux/Android the process is moved into its own
 *   cgroup (named with its pid, so concurrently running test processes never interfere) with
 *   memory.max / memory.limit_in_bytes set, so exceeding the cap triggers the kernel OOM killer for
 *   this process only. On platforms without a usable mechanism (macOS/iOS/FreeBSD: RLIMIT_AS counts
 *   the virtual address space including file mappings, RLIMIT_RSS is ignored by current kernels) a
 *   watchdog samples the resident set size and kills the test process when it exceeds the effective
 *   cap; the watchdog also stays active as a fallback everywhere else.
 * - CPU: throttled where the OS supports it without changing CPU affinity (see cpu_limit_permyriad).
 *   Platforms without a CPU usage limit mechanism apply no CPU limit.
 * - Timeouts: a watchdog kills the test process when the program, suite or case timeout is exceeded.
 */
ATFRAMEWORK_TEST_API void setup_test_resource_limit();

}  // namespace testing
ATFRAMEWORK_UTILS_NAMESPACE_END
