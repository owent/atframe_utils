// Copyright 2026 atframework

#include "test_resource_limit.h"  // NOLINT(build/include_subdir)

#include <config/compile_optimize.h>

#include <string/string_format.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

#include "cli/shell_font.h"

#include "test_event_listener.h"  // NOLINT(build/include_subdir)

#include <std/explicit_declare.h>

// Sanitizers inflate the address space (ASan/HWASan shadow memory, MSan/TSan bookkeeping, DFSan labels), so
// real memory consumption can not be measured accurately. Memory limits are ignored when any is active.
// Note: ATFW_UTIL_HAVE_LEAK_SANITIZER is intentionally not checked here: LeakSanitizer is integrated into
// AddressSanitizer on supported platforms and does not inflate the address space by itself.
#if (defined(ATFW_UTIL_HAVE_ADDRESS_SANITIZER) && ATFW_UTIL_HAVE_ADDRESS_SANITIZER) ||     \
    (defined(ATFW_UTIL_HAVE_HWADDRESS_SANITIZER) && ATFW_UTIL_HAVE_HWADDRESS_SANITIZER) || \
    (defined(ATFW_UTIL_HAVE_MEMORY_SANITIZER) && ATFW_UTIL_HAVE_MEMORY_SANITIZER) ||       \
    (defined(ATFW_UTIL_HAVE_THREAD_SANITIZER) && ATFW_UTIL_HAVE_THREAD_SANITIZER) ||       \
    (defined(ATFW_UTIL_HAVE_DATAFLOW_SANITIZER) && ATFW_UTIL_HAVE_DATAFLOW_SANITIZER)
#  define UTILS_TEST_RESOURCE_LIMIT_SANITIZER_ACTIVE 1
#else
#  define UTILS_TEST_RESOURCE_LIMIT_SANITIZER_ACTIVE 0
#endif

#if defined(_WIN32)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>

#  include <psapi.h>
#else
#  include <unistd.h>

#  include <sys/resource.h>
#  include <sys/types.h>
#endif

#if defined(__linux__) || defined(__ANDROID__)
#  include <fcntl.h>
#  include <cerrno>

#  include <sys/stat.h>
#  include <sys/sysinfo.h>
#endif

#if defined(__APPLE__)
#  include <mach/host_info.h>
#  include <mach/mach.h>
#  include <mach/vm_statistics.h>
#  include <sys/sysctl.h>
#endif

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace testing {

namespace {

static const int64_t kDefaultProgramTimeoutSeconds = 1800;                 // 30 minutes
static const int64_t kDefaultCaseTimeoutSeconds = 600;                     // 10 minutes
static const uint64_t kDefaultMaxMemoryBytes = 8ULL * 1024 * 1024 * 1024;  // 8GiB
static const int64_t kDefaultAvailablePhysicalMemoryPermyriad = 9000;      // 90%
static const int64_t kDefaultSingleCoreCpuPermyriad = 9000;                // 90%
static const unsigned int kWatchdogIntervalMilliseconds = 100;
static const unsigned int kResourceLimitExitCode = 137;  // 128 + 9 (SIGKILL), reports as "killed"

struct system_memory_info_t {
  uint64_t total_physical = 0;
  uint64_t available_physical = 0;
  uint64_t total_physical_and_virtual = 0;  // physical + swap
  bool supported = false;
};

// ============ platform: system memory information ============

#if defined(_WIN32)

static bool query_system_memory_info(system_memory_info_t &out) {
  MEMORYSTATUSEX status;
  memset(&status, 0, sizeof(status));
  status.dwLength = sizeof(status);
  if (!GlobalMemoryStatusEx(&status)) {
    return false;
  }
  out.total_physical = static_cast<uint64_t>(status.ullTotalPhys);
  out.available_physical = static_cast<uint64_t>(status.ullAvailPhys);
  // ullTotalPageFile is the commit limit: physical memory plus page files.
  out.total_physical_and_virtual = static_cast<uint64_t>(status.ullTotalPageFile);
  if (out.total_physical_and_virtual < out.total_physical) {
    out.total_physical_and_virtual = out.total_physical;
  }
  return true;
}

#elif defined(__linux__) || defined(__ANDROID__)

static bool query_system_memory_info(system_memory_info_t &out) {
  // /proc/meminfo provides MemAvailable, which is a better estimate than free memory.
  FILE *fp = fopen("/proc/meminfo", "r");
  if (nullptr != fp) {
    char line[256];
    while (nullptr != fgets(line, sizeof(line), fp)) {
      char key[64];
      unsigned long long value = 0;
      if (2 != sscanf(line, "%63[^:]: %llu", key, &value)) {
        continue;
      }
      // Values in /proc/meminfo are kibibytes.
      value *= 1024;
      if (0 == strcmp(key, "MemTotal")) {
        out.total_physical = value;
      } else if (0 == strcmp(key, "MemAvailable")) {
        out.available_physical = value;
      } else if (0 == strcmp(key, "SwapTotal")) {
        out.total_physical_and_virtual = value;
      }
    }
    fclose(fp);
  }

  if (0 == out.total_physical) {
    struct sysinfo info;
    if (0 != sysinfo(&info)) {
      return false;
    }
    out.total_physical = static_cast<uint64_t>(info.totalram) * info.mem_unit;
    if (0 == out.available_physical) {
      out.available_physical = static_cast<uint64_t>(info.freeram) * info.mem_unit;
    }
    out.total_physical_and_virtual = static_cast<uint64_t>(info.totalswap) * info.mem_unit;
  }
  if (0 == out.available_physical) {
    out.available_physical = out.total_physical;
  }
  out.total_physical_and_virtual += out.total_physical;
  return true;
}

#elif defined(__APPLE__)

static bool query_system_memory_info(system_memory_info_t &out) {
  size_t length = sizeof(out.total_physical);
  if (0 != sysctlbyname("hw.memsize", &out.total_physical, &length, nullptr, 0) || 0 == out.total_physical) {
    return false;
  }

  vm_statistics64_data_t vm_statistics;
  mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
  vm_size_t page_size = 0;
  if (KERN_SUCCESS == host_page_size(mach_host_self(), &page_size) &&
      KERN_SUCCESS == host_statistics64(mach_host_self(), HOST_VM_INFO64,
                                        reinterpret_cast<host_info64_t>(&vm_statistics), &count)) {
    // Free plus inactive pages approximate the memory available for new allocations.
    out.available_physical =
        (static_cast<uint64_t>(vm_statistics.free_count) + static_cast<uint64_t>(vm_statistics.inactive_count)) *
        static_cast<uint64_t>(page_size);
  }
  if (0 == out.available_physical) {
    out.available_physical = out.total_physical;
  }

  out.total_physical_and_virtual = out.total_physical;
  struct xsw_usage swap_usage;
  size_t swap_length = sizeof(swap_usage);
  if (0 == sysctlbyname("vm.swapusage", &swap_usage, &swap_length, nullptr, 0)) {
    out.total_physical_and_virtual += static_cast<uint64_t>(swap_usage.xsu_total);
  }
  return true;
}

#else

static bool query_system_memory_info(system_memory_info_t &out) {
#  if defined(_SC_PHYS_PAGES) && defined(_SC_AVPHYS_PAGES) && defined(_SC_PAGESIZE)
  long page_size = sysconf(_SC_PAGESIZE);
  long total_pages = sysconf(_SC_PHYS_PAGES);
  long available_pages = sysconf(_SC_AVPHYS_PAGES);
  if (page_size > 0 && total_pages > 0) {
    out.total_physical = static_cast<uint64_t>(total_pages) * static_cast<uint64_t>(page_size);
    if (available_pages > 0) {
      out.available_physical = static_cast<uint64_t>(available_pages) * static_cast<uint64_t>(page_size);
    } else {
      out.available_physical = out.total_physical;
    }
    // No portable way to query swap here, treat virtual as physical.
    out.total_physical_and_virtual = out.total_physical;
    return true;
  }
#  endif
  return false;
}

#endif

// ============ platform: current process memory usage (resident set size) ============

#if defined(_WIN32)

ATFW_EXPLICIT_UNUSED_ATTR static uint64_t query_current_memory_usage() {
  PROCESS_MEMORY_COUNTERS counters;
  memset(&counters, 0, sizeof(counters));
  // K32GetProcessMemoryInfo is exported by kernel32.dll, no extra library is required.
  if (!K32GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters))) {
    return 0;
  }
  return static_cast<uint64_t>(counters.WorkingSetSize);
}

#elif defined(__linux__) || defined(__ANDROID__)

ATFW_EXPLICIT_UNUSED_ATTR static uint64_t query_current_memory_usage() {
  FILE *fp = fopen("/proc/self/statm", "r");
  if (nullptr == fp) {
    return 0;
  }
  unsigned long long total_pages = 0;
  unsigned long long resident_pages = 0;
  int parsed = fscanf(fp, "%llu %llu", &total_pages, &resident_pages);
  fclose(fp);
  if (2 != parsed) {
    return 0;
  }
  long page_size = sysconf(_SC_PAGESIZE);
  if (page_size <= 0) {
    return 0;
  }
  return static_cast<uint64_t>(resident_pages) * static_cast<uint64_t>(page_size);
}

#elif defined(__APPLE__)

ATFW_EXPLICIT_UNUSED_ATTR static uint64_t query_current_memory_usage() {
  task_basic_info_64 info;
  mach_msg_type_number_t count = TASK_BASIC_INFO_64_COUNT;
  if (KERN_SUCCESS != task_info(mach_task_self(), TASK_BASIC_INFO_64, reinterpret_cast<task_info_t>(&info), &count)) {
    return 0;
  }
  return static_cast<uint64_t>(info.resident_size);
}

#else

// Fallback: peak resident set size from getrusage. Peak values only grow, which is acceptable for a kill
// threshold: the process did consume that much memory at some point.
ATFW_EXPLICIT_UNUSED_ATTR static uint64_t query_current_memory_usage() {
  struct rusage usage;
  if (0 != getrusage(RUSAGE_SELF, &usage)) {
    return 0;
  }
  // ru_maxrss is kilobytes on Linux and the BSDs (bytes on macOS, which never reaches this branch).
  return static_cast<uint64_t>(usage.ru_maxrss) * 1024;
}

#endif

// ============ platform: OS level limit enforcement ============

#if defined(_WIN32)

// The job object enforces the memory limit (commit charge), so allocations fail once the memory cap is
// exceeded (system level OOM). On single-core machines it also carries the CPU rate hard cap.
struct windows_job_limit_context_t {
  HANDLE job_handle = NULL;
  bool assigned = false;
  bool assign_failed = false;
};

static windows_job_limit_context_t &get_windows_job_limit_context() {
  static windows_job_limit_context_t ret;
  return ret;
}

static bool ensure_windows_job_assigned() {
  windows_job_limit_context_t &context = get_windows_job_limit_context();
  if (context.assigned) {
    return true;
  }
  if (context.assign_failed) {
    return false;
  }
  if (NULL == context.job_handle) {
    context.job_handle = CreateJobObjectW(nullptr, nullptr);
    if (NULL == context.job_handle) {
      context.assign_failed = true;
      return false;
    }
  }
  if (!AssignProcessToJobObject(context.job_handle, GetCurrentProcess())) {
    // Fails when the process already belongs to a job that forbids nesting (pre-Windows 8 behaviour,
    // some CI agents). Keep the watchdog fallback and report the failure at the call site.
    context.assign_failed = true;
    return false;
  }
  context.assigned = true;
  return true;
}

ATFW_EXPLICIT_UNUSED_ATTR static bool apply_os_memory_limit(uint64_t limit_bytes) {
  if (!ensure_windows_job_assigned()) {
    return false;
  }
  JOBOBJECT_EXTENDED_LIMIT_INFORMATION limit_info;
  memset(&limit_info, 0, sizeof(limit_info));
  limit_info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_PROCESS_MEMORY;
  limit_info.ProcessMemoryLimit = static_cast<SIZE_T>(limit_bytes);
  return FALSE != SetInformationJobObject(get_windows_job_limit_context().job_handle, JobObjectExtendedLimitInformation,
                                          &limit_info, sizeof(limit_info));
}

#  if defined(JOB_OBJECT_CPU_RATE_CONTROL_ENABLE) && defined(JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP)
// Throttle with a job object CPU rate hard cap. CpuRate counts processor cycles per 10000 cycles across
// all logical processors, so it maps directly onto the permyriad ratio of the total CPU capacity.
// This limits the CPU usage rate only; CPU affinity is left untouched because it may conflict with
// scheduling policies of the code under test.
static bool apply_os_cpu_limit(int64_t permyriad, unsigned int) {
  // A non-positive or out-of-range value restores the unlimited state (a hard cap of 10000 means every
  // cycle is allowed, which is equivalent to no limit).
  if (permyriad <= 0 || permyriad > kTestResourceLimitPermyriadFull) {
    permyriad = kTestResourceLimitPermyriadFull;
  }
  if (!ensure_windows_job_assigned()) {
    return false;
  }
  JOBOBJECT_CPU_RATE_CONTROL_INFORMATION cpu_info;
  memset(&cpu_info, 0, sizeof(cpu_info));
  cpu_info.ControlFlags = JOB_OBJECT_CPU_RATE_CONTROL_ENABLE | JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP;
  cpu_info.CpuRate = static_cast<ULONG>(permyriad);
  return FALSE != SetInformationJobObject(get_windows_job_limit_context().job_handle,
                                          JobObjectCpuRateControlInformation, &cpu_info, sizeof(cpu_info));
}
#  else
static bool apply_os_cpu_limit(int64_t, unsigned int) { return false; }
#  endif

#else

#  if defined(__linux__) || defined(__ANDROID__)

// Write the whole content to a procfs/sysfs style file.
static bool write_system_file(const std::string &path, const std::string &content) {
  int fd = ::open(path.c_str(), O_WRONLY | O_CLOEXEC);
  if (fd < 0) {
    return false;
  }
  ssize_t written = ::write(fd, content.data(), content.size());
  ::close(fd);
  return written == static_cast<ssize_t>(content.size());
}

// Read the cgroup path of this process from /proc/self/cgroup. For the cgroup v2 unified hierarchy the
// line is "0::/path"; for v1 it is "hierarchy-id:controller-list:path" where the list must contain
// v1_controller.
static std::string read_cgroup_self_path(bool v2, const char *v1_controller) {
  FILE *fp = fopen("/proc/self/cgroup", "r");
  if (nullptr == fp) {
    return "";
  }
  char line[512];
  std::string ret;
  while (nullptr != fgets(line, sizeof(line), fp)) {
    if (v2) {
      if (0 == strncmp(line, "0::", 3)) {
        ret = line + 3;
        break;
      }
      continue;
    }
    char *first_colon = strchr(line, ':');
    if (nullptr == first_colon) {
      continue;
    }
    char *second_colon = strchr(first_colon + 1, ':');
    if (nullptr == second_colon) {
      continue;
    }
    // The controller list is comma separated, e.g. "cpu,cpuacct".
    bool match = false;
    const char *list_begin = first_colon + 1;
    const char *cursor = list_begin;
    while (cursor < second_colon) {
      const char *comma = static_cast<const char *>(memchr(cursor, ',', static_cast<size_t>(second_colon - cursor)));
      const char *end = (nullptr == comma) ? second_colon : comma;
      size_t length = static_cast<size_t>(end - cursor);
      if (strlen(v1_controller) == length && 0 == strncmp(cursor, v1_controller, length)) {
        match = true;
        break;
      }
      if (nullptr == comma) {
        break;
      }
      cursor = comma + 1;
    }
    if (match) {
      ret = second_colon + 1;
      break;
    }
  }
  fclose(fp);
  while (!ret.empty() && ('\n' == ret.back() || '\r' == ret.back())) {
    ret.pop_back();
  }
  return ret;
}

// Group names carry the pid so concurrently running test processes never share a group: a shared group
// would let two test processes overwrite each other's quota and migrate each other.
static const std::string &cgroup_leaf_name() {
  static std::string ret = atfw::util::string::format("/atfw_test_limit_{}", static_cast<int>(::getpid()));
  return ret;
}

static const std::string &cgroup_staging_name() {
  static std::string ret = atfw::util::string::format("/atfw_test_limit_staging_{}", static_cast<int>(::getpid()));
  return ret;
}

static bool cgroup_path_ends_with(const std::string &path, const std::string &suffix) {
  return path.size() >= suffix.size() && 0 == path.compare(path.size() - suffix.size(), std::string::npos, suffix);
}

// cgroup v2: run this process inside its own leaf group and enable the given controller on the parent.
// Returns the leaf directory, or an empty string when the hierarchy is missing or not writable.
static std::string ensure_cgroup_v2_leaf(const char *controller) {
  static const char *kRoot = "/sys/fs/cgroup";
  if (0 != access("/sys/fs/cgroup/cgroup.controllers", F_OK)) {
    return "";  // not a cgroup v2 unified hierarchy
  }
  std::string self_path = read_cgroup_self_path(true, nullptr);
  if (self_path.empty()) {
    return "";
  }
  const std::string &leaf_name = cgroup_leaf_name();
  if (cgroup_path_ends_with(self_path, leaf_name)) {
    // Already inside the leaf group: enable the controller on the parent as needed. The parent hosts
    // no process (this process lives in the leaf), so enabling more controllers stays allowed.
    std::string leaf_dir = std::string(kRoot) + self_path;
    std::string base = leaf_dir.substr(0, leaf_dir.size() - leaf_name.size());
    return write_system_file(base + "/cgroup.subtree_control", std::string("+") + controller) ? leaf_dir : "";
  }

  std::string pid_text = atfw::util::string::format("{}", static_cast<int>(getpid()));
  std::string base = std::string(kRoot) + self_path;
  std::string staging_dir = base + cgroup_staging_name();
  std::string leaf_dir = base + leaf_name;

  // The cgroup v2 "no internal processes" rule forbids enabling a domain controller in subtree_control
  // while the cgroup itself hosts processes. Move this process into a staging child first. This also
  // means the limit can not be applied when sibling processes share the current cgroup; the caller then
  // falls back to no OS level limit.
  if (0 != ::mkdir(staging_dir.c_str(), 0755) && EEXIST != errno) {
    return "";
  }
  if (!write_system_file(staging_dir + "/cgroup.procs", pid_text)) {
    ::rmdir(staging_dir.c_str());
    return "";
  }

  bool ok = write_system_file(base + "/cgroup.subtree_control", std::string("+") + controller);
  if (ok && 0 != ::mkdir(leaf_dir.c_str(), 0755) && EEXIST != errno) {
    ok = false;
  }
  if (ok) {
    ok = write_system_file(leaf_dir + "/cgroup.procs", pid_text);
  }
  if (!ok) {
    // Best effort rollback: move back into the original cgroup and drop the created groups.
    write_system_file(base + "/cgroup.procs", pid_text);
    ::rmdir(leaf_dir.c_str());
  }
  ::rmdir(staging_dir.c_str());
  return ok ? leaf_dir : "";
}

// cgroup v2: throttle via cpu.max ("<quota> <period>" of CPU time). The quota scales with the CPU count,
// so the permyriad ratio of the total logical CPU capacity maps to permyriad * cpu_count * period / 10000.
static bool apply_os_cpu_limit_cgroup_v2(int64_t permyriad, unsigned int cpu_count) {
  if (permyriad <= 0 || permyriad >= kTestResourceLimitPermyriadFull) {
    // Restore the unlimited quota; only meaningful when the process is already inside its leaf group.
    std::string self_path = read_cgroup_self_path(true, nullptr);
    if (self_path.empty() || !cgroup_path_ends_with(self_path, cgroup_leaf_name())) {
      return false;
    }
    return write_system_file(std::string("/sys/fs/cgroup") + self_path + "/cpu.max", "max 100000");
  }
  std::string leaf_dir = ensure_cgroup_v2_leaf("cpu");
  if (leaf_dir.empty()) {
    return false;
  }
  const uint64_t period = 100000;
  uint64_t quota = static_cast<uint64_t>(permyriad) * static_cast<uint64_t>(cpu_count) * period /
                   static_cast<uint64_t>(kTestResourceLimitPermyriadFull);
  if (quota < 1) {
    quota = 1;
  }
  return write_system_file(leaf_dir + "/cpu.max", atfw::util::string::format("{} {}", quota, period));
}

// cgroup v2 memory: memory.max is a hard limit; exceeding it triggers the kernel OOM killer for
// processes inside this leaf group only.
static bool apply_os_memory_limit_cgroup_v2(uint64_t limit_bytes) {
  if (0 == limit_bytes) {
    return false;
  }
  std::string leaf_dir = ensure_cgroup_v2_leaf("memory");
  if (leaf_dir.empty()) {
    return false;
  }
  return write_system_file(leaf_dir + "/memory.max", atfw::util::string::format("{}", limit_bytes));
}

// cgroup v1: throttle via cpu.cfs_quota_us / cpu.cfs_period_us. The v1 hierarchy has no
// no-internal-process rule, so a leaf cgroup can be populated directly.
static bool apply_os_cpu_limit_cgroup_v1(int64_t permyriad, unsigned int cpu_count) {
  std::string self_path = read_cgroup_self_path(false, "cpu");
  if (self_path.empty()) {
    return false;
  }
  static const char *kMountCandidates[] = {"/sys/fs/cgroup/cpu", "/sys/fs/cgroup/cpu,cpuacct"};
  std::string root;
  for (size_t i = 0; i < sizeof(kMountCandidates) / sizeof(kMountCandidates[0]); ++i) {
    if (0 == access((std::string(kMountCandidates[i]) + "/cpu.cfs_quota_us").c_str(), F_OK)) {
      root = kMountCandidates[i];
      break;
    }
  }
  if (root.empty()) {
    return false;
  }

  std::string leaf_dir = root + self_path + cgroup_leaf_name();
  if (permyriad <= 0 || permyriad >= kTestResourceLimitPermyriadFull) {
    // Restore the unlimited quota; only meaningful when the process is already inside its leaf.
    if (!cgroup_path_ends_with(self_path, cgroup_leaf_name())) {
      return false;
    }
    return write_system_file(leaf_dir + "/cpu.cfs_quota_us", "-1");
  }

  std::string base = root + self_path;
  // cfs_period_us is inherited from the parent; read it for an accurate quota.
  uint64_t period = 100000;
  {
    FILE *fp = fopen((base + "/cpu.cfs_period_us").c_str(), "r");
    if (nullptr != fp) {
      unsigned long long parsed = 0;
      if (1 == fscanf(fp, "%llu", &parsed) && parsed > 0) {
        period = static_cast<uint64_t>(parsed);
      }
      fclose(fp);
    }
  }
  uint64_t quota = static_cast<uint64_t>(permyriad) * static_cast<uint64_t>(cpu_count) * period /
                   static_cast<uint64_t>(kTestResourceLimitPermyriadFull);
  if (quota < 1) {
    quota = 1;
  }
  std::string quota_text = atfw::util::string::format("{}", quota);

  std::string pid_text = atfw::util::string::format("{}", static_cast<int>(getpid()));
  if (0 != ::mkdir(leaf_dir.c_str(), 0755) && EEXIST != errno) {
    return false;
  }
  if (!write_system_file(leaf_dir + "/cpu.cfs_quota_us", quota_text)) {
    ::rmdir(leaf_dir.c_str());
    return false;
  }
  return write_system_file(leaf_dir + "/cgroup.procs", pid_text);
}

// cgroup v1 memory: memory.limit_in_bytes is a hard limit; exceeding it triggers the kernel OOM
// killer inside this leaf group only.
static bool apply_os_memory_limit_cgroup_v1(uint64_t limit_bytes) {
  if (0 == limit_bytes) {
    return false;
  }
  std::string self_path = read_cgroup_self_path(false, "memory");
  if (self_path.empty()) {
    return false;
  }
  static const char *kMountCandidates[] = {"/sys/fs/cgroup/memory"};
  std::string root;
  for (size_t i = 0; i < sizeof(kMountCandidates) / sizeof(kMountCandidates[0]); ++i) {
    if (0 == access((std::string(kMountCandidates[i]) + "/memory.limit_in_bytes").c_str(), F_OK)) {
      root = kMountCandidates[i];
      break;
    }
  }
  if (root.empty()) {
    return false;
  }

  std::string pid_text = atfw::util::string::format("{}", static_cast<int>(getpid()));
  std::string leaf_dir = root + self_path + cgroup_leaf_name();
  if (0 != ::mkdir(leaf_dir.c_str(), 0755) && EEXIST != errno) {
    return false;
  }
  if (!write_system_file(leaf_dir + "/memory.limit_in_bytes", atfw::util::string::format("{}", limit_bytes))) {
    ::rmdir(leaf_dir.c_str());
    return false;
  }
  return write_system_file(leaf_dir + "/cgroup.procs", pid_text);
}

static bool apply_os_cpu_limit(int64_t permyriad, unsigned int cpu_count) {
  return apply_os_cpu_limit_cgroup_v2(permyriad, cpu_count) || apply_os_cpu_limit_cgroup_v1(permyriad, cpu_count);
}

ATFW_EXPLICIT_UNUSED_ATTR static bool apply_os_memory_limit(uint64_t limit_bytes) {
  // cgroup memory.max / memory.limit_in_bytes is a hard limit: exceeding it triggers the kernel OOM
  // killer (SIGKILL, exit code 137) for processes inside this leaf group only. When no writable cgroup
  // hierarchy exists, the watchdog enforces the resident set size limit and kills the process instead.
  return apply_os_memory_limit_cgroup_v2(limit_bytes) || apply_os_memory_limit_cgroup_v1(limit_bytes);
}

#  else

// macOS/iOS/FreeBSD and generic POSIX expose no in-process CPU usage rate limit (per-thread affinity
// hints do not throttle usage; FreeBSD rctl rules can only be installed externally). No CPU limit is
// applied there; CPU affinity is intentionally left untouched because it may conflict with scheduling
// policies of the code under test.
static bool apply_os_cpu_limit(int64_t, unsigned int) { return false; }

ATFW_EXPLICIT_UNUSED_ATTR static bool apply_os_memory_limit(uint64_t) {
  // No accurate system level mechanism exists here: RLIMIT_AS limits the virtual address space
  // (including file mappings, which do not consume physical memory) and RLIMIT_RSS is ignored by
  // current macOS/FreeBSD kernels. The watchdog enforces the resident set size and kills the process.
  return false;
}

#  endif

#endif

// ============ shared state ============

struct resource_limit_state_t {
  std::mutex lock;
  bool enabled = true;
  bool setup_done = false;
  bool watchdog_started = false;
  bool shutdown = false;

  test_resource_limit_options program_options;
  std::unordered_map<std::string, test_resource_limit_options> suite_options;
  std::unordered_map<std::string, std::unordered_map<std::string, test_resource_limit_options>> case_options;

  system_memory_info_t memory_info;
  uint64_t program_memory_limit_bytes = 0;  // resolved at setup
  unsigned int cpu_count = 1;
  int64_t cpu_limit_permyriad = 0;  // resolved at setup, 0 = disabled
  bool memory_os_enforced = false;
  uint64_t memory_cap_applied = 0;  // OS-level memory cap currently in effect (Windows job object)
  bool cpu_os_enforced = false;

  std::chrono::steady_clock::time_point program_start;
  bool suite_running = false;
  std::string running_suite_name;
  std::chrono::steady_clock::time_point suite_start;
  bool case_running = false;
  std::string running_case_name;
  std::chrono::steady_clock::time_point case_start;
};

// Intentionally leaked: the watchdog thread may outlive static destruction at process exit.
static resource_limit_state_t &get_resource_limit_state() {
  static resource_limit_state_t *ret = new resource_limit_state_t();
  return *ret;
}

static unsigned int query_cpu_count() {
  unsigned int ret = std::thread::hardware_concurrency();
  return ret > 0 ? ret : 1;
}

static int64_t clamp_permyriad(int64_t permyriad) {
  if (permyriad > kTestResourceLimitPermyriadFull) {
    return kTestResourceLimitPermyriadFull;
  }
  return permyriad;
}

// True when the scope configures any memory field, including a negative value (explicitly disabled).
static bool has_explicit_memory_limit(const test_resource_limit_options &options) {
  return options.memory_limit_bytes != 0 || options.memory_limit_physical_memory_permyriad != 0 ||
         options.memory_limit_physical_and_virtual_permyriad != 0;
}

// Resolve the memory cap of one scope in bytes; 0 means no constraint from this scope.
ATFW_EXPLICIT_UNUSED_ATTR static uint64_t resolve_memory_limit_bytes(const test_resource_limit_options &options,
                                                                     const system_memory_info_t &memory_info,
                                                                     bool is_program_scope) {
  bool any_memory_field_set = options.memory_limit_bytes != 0 || options.memory_limit_physical_memory_permyriad != 0 ||
                              options.memory_limit_physical_and_virtual_permyriad != 0;

  uint64_t ret = 0;
  if (options.memory_limit_bytes > 0) {
    ret = static_cast<uint64_t>(options.memory_limit_bytes);
  }
  if (options.memory_limit_physical_memory_permyriad > 0 && memory_info.total_physical > 0) {
    uint64_t candidate = memory_info.total_physical *
                         static_cast<uint64_t>(clamp_permyriad(options.memory_limit_physical_memory_permyriad)) /
                         static_cast<uint64_t>(kTestResourceLimitPermyriadFull);
    if (0 == ret || candidate < ret) {
      ret = candidate;
    }
  }
  if (options.memory_limit_physical_and_virtual_permyriad > 0 && memory_info.total_physical_and_virtual > 0) {
    uint64_t candidate = memory_info.total_physical_and_virtual *
                         static_cast<uint64_t>(clamp_permyriad(options.memory_limit_physical_and_virtual_permyriad)) /
                         static_cast<uint64_t>(kTestResourceLimitPermyriadFull);
    if (0 == ret || candidate < ret) {
      ret = candidate;
    }
  }

  if (0 == ret && !any_memory_field_set && is_program_scope && memory_info.supported) {
    // Default cap: min(90% of the available physical memory at process start, 8GiB).
    ret = memory_info.available_physical * static_cast<uint64_t>(kDefaultAvailablePhysicalMemoryPermyriad) /
          static_cast<uint64_t>(kTestResourceLimitPermyriadFull);
    if (ret > kDefaultMaxMemoryBytes) {
      ret = kDefaultMaxMemoryBytes;
    }
  }
  return ret;
}

// Resolve the timeout of one scope in seconds; 0 means no timeout for this scope.
static int64_t resolve_timeout_seconds(int64_t configured, int64_t default_value) {
  if (configured > 0) {
    return configured;
  }
  if (configured < 0) {
    return 0;  // explicitly disabled
  }
  return default_value;
}

ATFW_EXPLICIT_UNUSED_ATTR static std::string format_bytes(uint64_t bytes) {
  if (bytes >= (static_cast<uint64_t>(1) << 30)) {
    return atfw::util::string::format("{:.2f} GiB", static_cast<double>(bytes) / 1073741824.0);
  }
  if (bytes >= (static_cast<uint64_t>(1) << 20)) {
    return atfw::util::string::format("{:.2f} MiB", static_cast<double>(bytes) / 1048576.0);
  }
  return atfw::util::string::format("{} bytes", bytes);
}

static void write_stderr_raw(const char *message, size_t length) {
#if defined(_WIN32)
  HANDLE error_handle = GetStdHandle(STD_ERROR_HANDLE);
  if (NULL != error_handle && INVALID_HANDLE_VALUE != error_handle) {
    DWORD written = 0;
    WriteFile(error_handle, message, static_cast<DWORD>(length), &written, nullptr);
  }
#else
  ssize_t unused = ::write(STDERR_FILENO, message, length);
  (void)unused;
#endif
}

// Kill the current test process. Called from the watchdog thread when a limit is exceeded; must not touch
// locks, stdio buffers or heap allocations that other threads may hold, so it takes a stack formatted
// message and writes it to stderr with raw syscalls.
static void resource_limit_kill_process(const char *message, size_t length) {
  write_stderr_raw(message, length);
  write_stderr_raw("\n", 1);
#if defined(_WIN32)
  TerminateProcess(GetCurrentProcess(), kResourceLimitExitCode);
  _exit(static_cast<int>(kResourceLimitExitCode));  // never reached, keeps control flow obvious
#else
  _exit(static_cast<int>(kResourceLimitExitCode));
#endif
}

static void resource_limit_watchdog_loop() {
  resource_limit_state_t &state = get_resource_limit_state();

  // CPU limits are re-applied when the running scope changes the effective value. setup already applied
  // (or attempted) the program scope value, so start from it: this avoids a redundant re-apply on the
  // first tick and, on Windows, avoids creating a job object when the program scope is disabled.
  int64_t last_applied_cpu_permyriad;
  {
    std::lock_guard<std::mutex> lock_guard(state.lock);
    last_applied_cpu_permyriad = state.cpu_limit_permyriad;
  }

  for (;;) {
    std::chrono::steady_clock::time_point program_start;
    bool case_running = false;
    bool suite_running = false;
    std::string suite_name;
    std::string case_name;
    std::chrono::steady_clock::time_point case_start;
    std::chrono::steady_clock::time_point suite_start;
    int64_t program_timeout = 0;
    int64_t suite_timeout = 0;
    int64_t case_timeout = 0;
    int64_t cpu_permyriad = 0;
    unsigned int cpu_count = 1;
    bool suite_explicit_timeout = false;
    bool case_explicit_timeout = false;
    ATFW_EXPLICIT_UNUSED_ATTR uint64_t memory_cap = 0;

    {
      std::lock_guard<std::mutex> lock_guard(state.lock);
      if (state.shutdown || !state.enabled) {
        break;
      }
      program_start = state.program_start;
      suite_running = state.suite_running;
      case_running = state.case_running;
      suite_name = state.running_suite_name;
      case_name = state.running_case_name;
      suite_start = state.suite_start;
      case_start = state.case_start;

      program_timeout = resolve_timeout_seconds(state.program_options.timeout_seconds, kDefaultProgramTimeoutSeconds);
      case_timeout = kDefaultCaseTimeoutSeconds;
      cpu_permyriad = state.cpu_limit_permyriad;
      cpu_count = state.cpu_count;
#if !UTILS_TEST_RESOURCE_LIMIT_SANITIZER_ACTIVE
      memory_cap = state.program_memory_limit_bytes;
#endif

      // Explicitly configured scope fields override (and may relax) the program scope configuration;
      // fields never configured inherit the parent scope.
      if (suite_running) {
        std::unordered_map<std::string, test_resource_limit_options>::const_iterator suite_iter =
            state.suite_options.find(suite_name);
        if (suite_iter != state.suite_options.end()) {
          const test_resource_limit_options &suite_opts = suite_iter->second;
          if (suite_opts.timeout_seconds != 0) {
            suite_explicit_timeout = true;
            suite_timeout = resolve_timeout_seconds(suite_opts.timeout_seconds, 0);
          }
          if (suite_opts.cpu_limit_permyriad != 0) {
            cpu_permyriad = suite_opts.cpu_limit_permyriad < 0 ? 0 : clamp_permyriad(suite_opts.cpu_limit_permyriad);
          }
#if !UTILS_TEST_RESOURCE_LIMIT_SANITIZER_ACTIVE
          if (has_explicit_memory_limit(suite_opts)) {
            memory_cap = resolve_memory_limit_bytes(suite_opts, state.memory_info, false);
          }
#endif
        }
      }
      if (case_running) {
        std::unordered_map<std::string, std::unordered_map<std::string, test_resource_limit_options>>::const_iterator
            suite_case_iter = state.case_options.find(suite_name);
        if (suite_case_iter != state.case_options.end()) {
          std::unordered_map<std::string, test_resource_limit_options>::const_iterator case_iter =
              suite_case_iter->second.find(case_name);
          if (case_iter != suite_case_iter->second.end()) {
            const test_resource_limit_options &case_opts = case_iter->second;
            if (case_opts.timeout_seconds != 0) {
              case_explicit_timeout = true;
              case_timeout = resolve_timeout_seconds(case_opts.timeout_seconds, case_timeout);
            }
            if (case_opts.cpu_limit_permyriad != 0) {
              cpu_permyriad = case_opts.cpu_limit_permyriad < 0 ? 0 : clamp_permyriad(case_opts.cpu_limit_permyriad);
            }
#if !UTILS_TEST_RESOURCE_LIMIT_SANITIZER_ACTIVE
            if (has_explicit_memory_limit(case_opts)) {
              memory_cap = resolve_memory_limit_bytes(case_opts, state.memory_info, false);
            }
#endif
          }
        }
      }
    }

    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    if (program_timeout > 0) {
      int64_t elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - program_start).count();
      if (elapsed >= program_timeout) {
        // The program timeout is the default budget; it yields to a running scope with an explicitly
        // configured time budget while that scope still has time left. An explicitly disabled scope
        // timeout means the scope has no time limit at all.
        bool scope_alive = false;
        if (suite_explicit_timeout) {
          scope_alive = suite_timeout <= 0 || now < suite_start + std::chrono::seconds(suite_timeout);
        }
        if (case_explicit_timeout) {
          scope_alive = case_timeout <= 0 || now < case_start + std::chrono::seconds(case_timeout);
        }
        if (!scope_alive) {
          char message[256];
          auto fmt_result = atfw::util::string::format_to_n(
              message, sizeof(message) - 1, "[ RESOURCE ] test program timeout: {}s >= {}s, killing test process",
              elapsed, program_timeout);
          *fmt_result.out = '\0';
          resource_limit_kill_process(message, static_cast<size_t>(fmt_result.out - message));
        }
      }
    }
    if (suite_running && suite_timeout > 0) {
      int64_t elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - suite_start).count();
      if (elapsed >= suite_timeout) {
        char message[512];
        auto fmt_result = atfw::util::string::format_to_n(
            message, sizeof(message) - 1, "[ RESOURCE ] test suite {} timeout: {}s >= {}s, killing test process",
            suite_name, elapsed, suite_timeout);
        *fmt_result.out = '\0';
        resource_limit_kill_process(message, static_cast<size_t>(fmt_result.out - message));
      }
    }
    if (case_running && case_timeout > 0) {
      int64_t elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - case_start).count();
      if (elapsed >= case_timeout) {
        char message[512];
        auto fmt_result = atfw::util::string::format_to_n(
            message, sizeof(message) - 1, "[ RESOURCE ] test case {}.{} timeout: {}s >= {}s, killing test process",
            suite_name, case_name, elapsed, case_timeout);
        *fmt_result.out = '\0';
        resource_limit_kill_process(message, static_cast<size_t>(fmt_result.out - message));
      }
    }
#if !UTILS_TEST_RESOURCE_LIMIT_SANITIZER_ACTIVE
    if (memory_cap > 0) {
      uint64_t current_memory = query_current_memory_usage();
      if (current_memory > memory_cap) {
        char message[512];
        // Keep the kill path free of heap allocations: format whole MiB values into the stack buffer.
        auto fmt_result = atfw::util::string::format_to_n(
            message, sizeof(message) - 1,
            "[ RESOURCE ] test process memory usage {} MiB exceeded limit {} MiB, killing test process",
            current_memory >> 20, memory_cap >> 20);
        *fmt_result.out = '\0';
        resource_limit_kill_process(message, static_cast<size_t>(fmt_result.out - message));
      }
    }
#endif

    if (cpu_permyriad != last_applied_cpu_permyriad) {
      // An explicit suite/case CPU limit applies while the scope runs; the program scope value is
      // restored afterwards. A non-positive value restores the unlimited state where supported. The
      // result is intentionally ignored: setup already reported when the platform has no mechanism.
      apply_os_cpu_limit(cpu_permyriad, cpu_count);
      last_applied_cpu_permyriad = cpu_permyriad;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(kWatchdogIntervalMilliseconds));
  }
}

class test_resource_limit_event_listener : public test_event_listener {
 public:
  void on_test_program_end(int) override {
    resource_limit_state_t &state = get_resource_limit_state();
    std::lock_guard<std::mutex> lock_guard(state.lock);
    state.shutdown = true;
    state.case_running = false;
    state.suite_running = false;
  }

  void on_test_suite_start(const test_event_suite_info &info) override {
    resource_limit_state_t &state = get_resource_limit_state();
    std::lock_guard<std::mutex> lock_guard(state.lock);
    state.suite_running = true;
    state.running_suite_name.assign(info.name_.data(), info.name_.size());
    state.suite_start = std::chrono::steady_clock::now();
  }

  void on_test_suite_end(const test_event_suite_info &) override {
    resource_limit_state_t &state = get_resource_limit_state();
    std::lock_guard<std::mutex> lock_guard(state.lock);
    state.suite_running = false;
    state.case_running = false;
  }

  void on_test_case_start(const test_event_case_info &info) override {
    resource_limit_state_t &state = get_resource_limit_state();
    std::lock_guard<std::mutex> lock_guard(state.lock);
    state.case_running = true;
    state.running_suite_name.assign(info.suite_name_.data(), info.suite_name_.size());
    state.running_case_name.assign(info.case_name_.data(), info.case_name_.size());
    state.case_start = std::chrono::steady_clock::now();
  }

  void on_test_case_end(const test_event_case_info &) override {
    resource_limit_state_t &state = get_resource_limit_state();
    std::lock_guard<std::mutex> lock_guard(state.lock);
    state.case_running = false;
  }
};

static void print_resource_limit_summary(const resource_limit_state_t &state) {
  atfw::util::cli::shell_stream ss(std::cout);
  if (!state.enabled) {
    ss() << atfw::util::cli::shell_font_style::SHELL_FONT_COLOR_YELLOW
         << atfw::util::cli::shell_font_style::SHELL_FONT_SPEC_BOLD << "[ RESOURCE ] "
         << atfw::util::cli::shell_font_style::SHELL_FONT_SPEC_NULL << "resource limits disabled" << std::endl;
    return;
  }

  ss() << atfw::util::cli::shell_font_style::SHELL_FONT_COLOR_GREEN
       << atfw::util::cli::shell_font_style::SHELL_FONT_SPEC_BOLD << "[ RESOURCE ] "
       << atfw::util::cli::shell_font_style::SHELL_FONT_SPEC_NULL;

#if UTILS_TEST_RESOURCE_LIMIT_SANITIZER_ACTIVE
  ss() << "memory limit: ignored under sanitizer";
#else
  if (state.program_memory_limit_bytes > 0) {
    ss() << "memory limit: " << format_bytes(state.program_memory_limit_bytes)
         << (state.memory_os_enforced ? " (OS enforced)" : " (watchdog)");
  } else {
    ss() << "memory limit: none";
  }
#endif

  if (state.cpu_limit_permyriad > 0) {
    ss() << "; cpu limit: " << state.cpu_limit_permyriad / 100 << ".";
    int64_t fraction = state.cpu_limit_permyriad % 100;
    if (fraction < 10) {
      ss() << "0";
    }
    ss() << fraction << "% of " << state.cpu_count << " core(s)"
         << (state.cpu_os_enforced ? " (OS enforced)" : " (unsupported on this platform)");
  } else {
    ss() << "; cpu limit: none";
  }

  int64_t program_timeout =
      resolve_timeout_seconds(state.program_options.timeout_seconds, kDefaultProgramTimeoutSeconds);
  ss() << "; case timeout: " << kDefaultCaseTimeoutSeconds << "s"
       << "; program timeout: ";
  if (program_timeout > 0) {
    ss() << program_timeout << "s";
  } else {
    ss() << "none";
  }
  ss() << std::endl;
}

// The OS-level memory cap must cover every explicitly configured scope limit so a relaxed suite/case is
// not throttled by the program scope cap. The watchdog still enforces the per-scope resolved caps.
static uint64_t resolve_os_memory_cap(const resource_limit_state_t &state) {
  uint64_t ret = state.program_memory_limit_bytes;
  for (std::unordered_map<std::string, test_resource_limit_options>::const_iterator iter = state.suite_options.begin();
       iter != state.suite_options.end(); ++iter) {
    if (has_explicit_memory_limit(iter->second)) {
      uint64_t scoped = resolve_memory_limit_bytes(iter->second, state.memory_info, false);
      if (scoped > ret) {
        ret = scoped;
      }
    }
  }
  for (std::unordered_map<std::string, std::unordered_map<std::string, test_resource_limit_options>>::const_iterator
           suite_iter = state.case_options.begin();
       suite_iter != state.case_options.end(); ++suite_iter) {
    for (std::unordered_map<std::string, test_resource_limit_options>::const_iterator case_iter =
             suite_iter->second.begin();
         case_iter != suite_iter->second.end(); ++case_iter) {
      if (has_explicit_memory_limit(case_iter->second)) {
        uint64_t scoped = resolve_memory_limit_bytes(case_iter->second, state.memory_info, false);
        if (scoped > ret) {
          ret = scoped;
        }
      }
    }
  }
  return ret;
}

// Raise the OS-level memory cap when an explicitly configured scope limit exceeds the cap currently
// applied. The cap is only raised, never lowered, so already running relaxed scopes stay unthrottled.
static void raise_os_memory_cap_if_needed(resource_limit_state_t &state) {
#if !UTILS_TEST_RESOURCE_LIMIT_SANITIZER_ACTIVE
  if (!state.setup_done || !state.enabled) {
    return;
  }
  uint64_t os_memory_cap = resolve_os_memory_cap(state);
  if (os_memory_cap > state.memory_cap_applied && apply_os_memory_limit(os_memory_cap)) {
    state.memory_cap_applied = os_memory_cap;
    state.memory_os_enforced = true;
  }
#else
  (void)state;
#endif
}

// Apply OS level limits with the current program scope configuration. Requires state.lock held or being
// called before the watchdog starts.
static void apply_os_limits(resource_limit_state_t &state) {
#if UTILS_TEST_RESOURCE_LIMIT_SANITIZER_ACTIVE
  state.memory_os_enforced = false;
  state.memory_cap_applied = 0;
#else
  state.memory_cap_applied = resolve_os_memory_cap(state);
  if (state.memory_cap_applied > 0) {
    state.memory_os_enforced = apply_os_memory_limit(state.memory_cap_applied);
    if (!state.memory_os_enforced) {
      state.memory_cap_applied = 0;
    }
  }
#endif

  int64_t permyriad = state.program_options.cpu_limit_permyriad;
  if (permyriad < 0) {
    state.cpu_limit_permyriad = 0;  // explicitly disabled
  } else if (permyriad > 0) {
    state.cpu_limit_permyriad = clamp_permyriad(permyriad);
  } else {
    // Default policy: keep at least one core free on multi-core machines, 90% on single-core machines.
    if (state.cpu_count > 1) {
      state.cpu_limit_permyriad = static_cast<int64_t>(state.cpu_count - 1) * kTestResourceLimitPermyriadFull /
                                  static_cast<int64_t>(state.cpu_count);
    } else {
      state.cpu_limit_permyriad = kDefaultSingleCoreCpuPermyriad;
    }
  }
  if (state.cpu_limit_permyriad > 0) {
    state.cpu_os_enforced = apply_os_cpu_limit(state.cpu_limit_permyriad, state.cpu_count);
  } else {
    state.cpu_os_enforced = false;
  }
}

}  // namespace

void set_test_resource_limit_enabled(bool enabled) {
  resource_limit_state_t &state = get_resource_limit_state();
  std::lock_guard<std::mutex> lock_guard(state.lock);
  state.enabled = enabled;
}

bool get_test_resource_limit_enabled() {
  resource_limit_state_t &state = get_resource_limit_state();
  std::lock_guard<std::mutex> lock_guard(state.lock);
  return state.enabled;
}

void set_test_resource_limit(const test_resource_limit_options &options) {
  resource_limit_state_t &state = get_resource_limit_state();
  std::lock_guard<std::mutex> lock_guard(state.lock);
  state.program_options = options;
  if (state.setup_done && state.enabled) {
#if !UTILS_TEST_RESOURCE_LIMIT_SANITIZER_ACTIVE
    state.program_memory_limit_bytes = resolve_memory_limit_bytes(options, state.memory_info, true);
#endif
    apply_os_limits(state);
  }
}

test_resource_limit_options get_test_resource_limit() {
  resource_limit_state_t &state = get_resource_limit_state();
  std::lock_guard<std::mutex> lock_guard(state.lock);
  return state.program_options;
}

void set_test_suite_resource_limit(const std::string &suite_name, const test_resource_limit_options &options) {
  resource_limit_state_t &state = get_resource_limit_state();
  std::lock_guard<std::mutex> lock_guard(state.lock);
  state.suite_options[suite_name] = options;
  raise_os_memory_cap_if_needed(state);
}

bool get_test_suite_resource_limit(const std::string &suite_name, test_resource_limit_options &out) {
  resource_limit_state_t &state = get_resource_limit_state();
  std::lock_guard<std::mutex> lock_guard(state.lock);
  std::unordered_map<std::string, test_resource_limit_options>::const_iterator iter =
      state.suite_options.find(suite_name);
  if (iter == state.suite_options.end()) {
    out = test_resource_limit_options();
    return false;
  }
  out = iter->second;
  return true;
}

void set_test_case_resource_limit(const std::string &suite_name, const std::string &case_name,
                                  const test_resource_limit_options &options) {
  resource_limit_state_t &state = get_resource_limit_state();
  std::lock_guard<std::mutex> lock_guard(state.lock);
  state.case_options[suite_name][case_name] = options;
  raise_os_memory_cap_if_needed(state);
}

bool get_test_case_resource_limit(const std::string &suite_name, const std::string &case_name,
                                  test_resource_limit_options &out) {
  resource_limit_state_t &state = get_resource_limit_state();
  std::lock_guard<std::mutex> lock_guard(state.lock);
  std::unordered_map<std::string, std::unordered_map<std::string, test_resource_limit_options>>::const_iterator
      suite_iter = state.case_options.find(suite_name);
  if (suite_iter == state.case_options.end()) {
    out = test_resource_limit_options();
    return false;
  }
  std::unordered_map<std::string, test_resource_limit_options>::const_iterator iter =
      suite_iter->second.find(case_name);
  if (iter == suite_iter->second.end()) {
    out = test_resource_limit_options();
    return false;
  }
  out = iter->second;
  return true;
}

bool set_current_test_suite_resource_limit(const test_resource_limit_options &options) {
  resource_limit_state_t &state = get_resource_limit_state();
  std::lock_guard<std::mutex> lock_guard(state.lock);
  if (!state.suite_running) {
    return false;
  }
  state.suite_options[state.running_suite_name] = options;
  raise_os_memory_cap_if_needed(state);
  return true;
}

bool set_current_test_case_resource_limit(const test_resource_limit_options &options) {
  resource_limit_state_t &state = get_resource_limit_state();
  std::lock_guard<std::mutex> lock_guard(state.lock);
  if (!state.case_running) {
    return false;
  }
  state.case_options[state.running_suite_name][state.running_case_name] = options;
  raise_os_memory_cap_if_needed(state);
  return true;
}

bool get_current_test_suite_resource_limit(test_resource_limit_options &out) {
  resource_limit_state_t &state = get_resource_limit_state();
  std::lock_guard<std::mutex> lock_guard(state.lock);
  if (!state.suite_running) {
    out = test_resource_limit_options();
    return false;
  }
  std::unordered_map<std::string, test_resource_limit_options>::const_iterator iter =
      state.suite_options.find(state.running_suite_name);
  if (iter != state.suite_options.end()) {
    out = iter->second;
  } else {
    // No configuration on the suite itself: fall back to the program scope.
    out = state.program_options;
  }
  return true;
}

bool get_current_test_case_resource_limit(test_resource_limit_options &out) {
  resource_limit_state_t &state = get_resource_limit_state();
  std::lock_guard<std::mutex> lock_guard(state.lock);
  if (!state.case_running) {
    out = test_resource_limit_options();
    return false;
  }
  std::unordered_map<std::string, std::unordered_map<std::string, test_resource_limit_options>>::const_iterator
      suite_case_iter = state.case_options.find(state.running_suite_name);
  if (suite_case_iter != state.case_options.end()) {
    std::unordered_map<std::string, test_resource_limit_options>::const_iterator case_iter =
        suite_case_iter->second.find(state.running_case_name);
    if (case_iter != suite_case_iter->second.end()) {
      out = case_iter->second;
      return true;
    }
  }
  // No configuration on the case itself: fall back to the running suite, then to the program scope.
  std::unordered_map<std::string, test_resource_limit_options>::const_iterator suite_iter =
      state.suite_options.find(state.running_suite_name);
  if (suite_iter != state.suite_options.end()) {
    out = suite_iter->second;
  } else {
    out = state.program_options;
  }
  return true;
}

void setup_test_resource_limit() {
  resource_limit_state_t &state = get_resource_limit_state();
  {
    std::lock_guard<std::mutex> lock_guard(state.lock);
    if (state.setup_done) {
      return;
    }
    state.setup_done = true;
    state.program_start = std::chrono::steady_clock::now();
    state.cpu_count = query_cpu_count();
    state.memory_info.supported = query_system_memory_info(state.memory_info);
    if (state.enabled) {
#if !UTILS_TEST_RESOURCE_LIMIT_SANITIZER_ACTIVE
      state.program_memory_limit_bytes = resolve_memory_limit_bytes(state.program_options, state.memory_info, true);
#endif
      apply_os_limits(state);
    }
    print_resource_limit_summary(state);
  }

  if (!state.enabled) {
    return;
  }

  // The listener drives per-suite and per-case tracking. append_test_event_listener takes ownership and
  // works with the private runner, GoogleTest and Boost.Test integrations.
  append_test_event_listener(new test_resource_limit_event_listener());

  {
    std::lock_guard<std::mutex> lock_guard(state.lock);
    if (state.watchdog_started) {
      return;
    }
    state.watchdog_started = true;
  }
  std::thread watchdog_thread(&resource_limit_watchdog_loop);
  watchdog_thread.detach();
}

}  // namespace testing
ATFRAMEWORK_UTILS_NAMESPACE_END
