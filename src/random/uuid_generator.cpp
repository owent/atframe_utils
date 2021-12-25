#include <config/compile_optimize.h>
#include <config/compiler_features.h>

#include <lock/lock_holder.h>
#include <lock/spin_lock.h>
#include <random/random_generator.h>
#include <random/uuid_generator.h>

#if (defined(LIBATFRAME_UTILS_ENABLE_LIBUUID) && LIBATFRAME_UTILS_ENABLE_LIBUUID) ||         \
    (defined(LIBATFRAME_UTILS_ENABLE_UUID_WINRPC) && LIBATFRAME_UTILS_ENABLE_UUID_WINRPC) || \
    (defined(LIBATFRAME_UTILS_ENABLE_UUID_INTERNAL_IMPLEMENT) && LIBATFRAME_UTILS_ENABLE_UUID_INTERNAL_IMPLEMENT)

#  if defined(LIBATFRAME_UTILS_ENABLE_UUID_INTERNAL_IMPLEMENT) && LIBATFRAME_UTILS_ENABLE_UUID_INTERNAL_IMPLEMENT
#    include <stdio.h>
#    include <ctime>
#    ifdef HAVE_UNISTD_H
#      include <unistd.h>
#    endif
#    ifdef HAVE_STDLIB_H
#      include <stdlib.h>
#    endif
#    include <errno.h>
#    include <fcntl.h>
#    include <limits.h>
#    include <string.h>
#    include <sys/types.h>
#    ifdef HAVE_SYS_TIME_H
#      include <sys/time.h>
#    endif
#    include <sys/stat.h>
#    ifdef HAVE_SYS_FILE_H
#      include <sys/file.h>
#    endif
#    ifdef HAVE_SYS_IOCTL_H
#      include <sys/ioctl.h>
#    endif
#    ifdef HAVE_SYS_SOCKET_H
#      include <sys/socket.h>
#    endif
#    ifdef HAVE_SYS_UN_H
#      include <sys/un.h>
#    endif
#    ifdef HAVE_SYS_SOCKIO_H
#      include <sys/sockio.h>
#    endif
#    ifdef HAVE_NET_IF_H
#      include <net/if.h>
#    endif
#    ifdef HAVE_NETINET_IN_H
#      include <netinet/in.h>
#    endif
#    ifdef HAVE_NET_IF_DL_H
#      include <net/if_dl.h>
#    endif
#    if defined(__linux__) && defined(HAVE_SYS_SYSCALL_H)
#      include <sys/syscall.h>
#    endif

#    ifdef HAVE_NETINET_IN_H
#      include <fcntl.h>

#      ifdef O_CLOEXEC
#        define UL_CLOEXECSTR "e"
#      else
#        define UL_CLOEXECSTR ""
#      endif

#      ifndef O_CLOEXEC
#        define O_CLOEXEC 0
#      endif
#    endif

#    ifdef HAVE_TLS
#      define UTIL_CONFIG_UUID_INNER_THREAD_LOCAL static __thread
#    else
#      define UTIL_CONFIG_UUID_INNER_THREAD_LOCAL static
#    endif

#    ifdef _WIN32
#      ifndef WIN32_LEAN_AND_MEAN
#        define WIN32_LEAN_AND_MEAN
#      endif
#      include <Windows.h>
#    endif

#  endif

#  if defined(UTIL_CONFIG_COMPILER_CXX_STATIC_ASSERT) && UTIL_CONFIG_COMPILER_CXX_STATIC_ASSERT
#    include <type_traits>
#  endif

#  if defined(LIBATFRAME_UTILS_ENABLE_LIBUUID) && LIBATFRAME_UTILS_ENABLE_LIBUUID
#    if defined(UTIL_CONFIG_COMPILER_CXX_STATIC_ASSERT) && UTIL_CONFIG_COMPILER_CXX_STATIC_ASSERT
#      if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201402L)) ||                       \
          (defined(__cplusplus) && __cplusplus >= 201402L &&                        \
           !(!defined(__clang__) && defined(__GNUC__) && defined(__GNUC_MINOR__) && \
             __GNUC__ * 100 + __GNUC_MINOR__ <= 409))
UTIL_CONFIG_STATIC_ASSERT(std::is_trivially_copyable<uuid_t>::value);
UTIL_CONFIG_STATIC_ASSERT(std::is_trivially_copyable<LIBATFRAME_UTILS_NAMESPACE_ID::random::uuid>::value);
#      elif (defined(__cplusplus) && __cplusplus >= 201103L) || ((defined(_MSVC_LANG) && _MSVC_LANG >= 201103L))
UTIL_CONFIG_STATIC_ASSERT(std::is_trivial<uuid_t>::value);
UTIL_CONFIG_STATIC_ASSERT(std::is_trivial<LIBATFRAME_UTILS_NAMESPACE_ID::random::uuid>::value);
#      else
UTIL_CONFIG_STATIC_ASSERT(std::is_pod<uuid_t>::value);
UTIL_CONFIG_STATIC_ASSERT(std::is_pod<LIBATFRAME_UTILS_NAMESPACE_ID::random::uuid>::value);
#      endif
UTIL_CONFIG_STATIC_ASSERT(sizeof(uuid_t) == sizeof(LIBATFRAME_UTILS_NAMESPACE_ID::random::uuid));
#    endif
#  endif

LIBATFRAME_UTILS_NAMESPACE_BEGIN
namespace random {

#  if defined(LIBATFRAME_UTILS_ENABLE_UUID_INTERNAL_IMPLEMENT) && LIBATFRAME_UTILS_ENABLE_UUID_INTERNAL_IMPLEMENT
namespace details {
// Codes below just like https://sourceforge.net/p/libuuid/code/ci/master/tree/gen_uuid.c
#    ifndef LOCK_EX
/* flock() replacement */
#      define LOCK_EX 1
#      define LOCK_SH 2
#      define LOCK_UN 3
#      define LOCK_NB 4

static int flock(int fd, int op) {
  int rc = 0;

#      if defined(F_SETLK) && defined(F_SETLKW)
  struct flock fl = {0};

  switch (op & (LOCK_EX | LOCK_SH | LOCK_UN)) {
    case LOCK_EX:
      fl.l_type = F_WRLCK;
      break;

    case LOCK_SH:
      fl.l_type = F_RDLCK;
      break;

    case LOCK_UN:
      fl.l_type = F_UNLCK;
      break;

    default:
      errno = EINVAL;
      return -1;
  }

  fl.l_whence = SEEK_SET;
  rc = fcntl(fd, op & LOCK_NB ? F_SETLK : F_SETLKW, &fl);

  if (rc && (errno == EAGAIN)) errno = EWOULDBLOCK;
#      endif /* defined(F_SETLK) && defined(F_SETLKW)  */

  return rc;
}
#    endif /* LOCK_EX */

#    ifdef _WIN32
static void gettimeofday(struct timeval *tv, void *) {
  FILETIME ftime;
  uint64_t n;

  GetSystemTimeAsFileTime(&ftime);
  n = (((uint64_t)ftime.dwHighDateTime << 32) + (uint64_t)ftime.dwLowDateTime);
  if (n) {
    n /= 10;
    n -= ((369 * 365 + 89) * (uint64_t)86400) * 1000000;
  }

  tv->tv_sec = n / 1000000;
  tv->tv_usec = n % 1000000;
}

static int getuid(void) { return 1; }
#    endif

/**
 * Generate a stream of random nbytes into buf.
 * Use /dev/urandom if possible, and if not,
 * use glibc pseudo-random functions.
 */
static void random_get_bytes(unsigned char *buf, size_t nbytes) {
  using uuid_generator_rand_engine = LIBATFRAME_UTILS_NAMESPACE_ID::random::mt19937_64;
  static LIBATFRAME_UTILS_NAMESPACE_ID::lock::spin_lock random_generator_lock;
  static uuid_generator_rand_engine random_generator;
  static bool uuid_generator_rand_engine_inited = false;

  LIBATFRAME_UTILS_NAMESPACE_ID::lock::lock_holder<LIBATFRAME_UTILS_NAMESPACE_ID::lock::spin_lock> lock_guard(
      random_generator_lock);

  if (unlikely(!uuid_generator_rand_engine_inited)) {
    struct timeval tv;
    gettimeofday(&tv, 0);
    uuid_generator_rand_engine::result_type seed =
        static_cast<uuid_generator_rand_engine::result_type>((
#    ifdef _MSC_VER
                                                                 _getpid()
#    else
                                                                 getpid()
#    endif
                                                                 << 16) ^
                                                             getuid() ^ tv.tv_sec ^ tv.tv_usec);

    random_generator.init_seed(seed);
    uuid_generator_rand_engine_inited = true;
    for (int i = 0; i < 256; ++i) {
      random_generator.random();
    }
  }

  while (nbytes >= sizeof(uuid_generator_rand_engine::result_type)) {
    uuid_generator_rand_engine::result_type r = random_generator.random();
    memcpy(buf, &r, sizeof(r));

    nbytes -= sizeof(r);
    buf += sizeof(r);
  }

  if (nbytes > 0) {
    uuid_generator_rand_engine::result_type r = random_generator.random();
    memcpy(buf, &r, nbytes);
  }

  return;
}

/**
 * Get the ethernet hardware address, if we can find it...
 *
 * XXX for a windows version, probably should use GetAdaptersInfo:
 * http://www.codeguru.com/cpp/i-n/network/networkinformation/article.php/c5451
 * commenting out get_node_id just to get gen_uuid to compile under windows
 * is not the right way to go!
 */
static int get_node_id(unsigned char *node_id) {
  // #ifdef HAVE_NET_IF_H // this is required
  int sd;
  struct ifreq ifr, *ifrp;
  struct ifconf ifc;
  char buf[1024];
  int n, i;
  unsigned char *a;
#    ifdef HAVE_NET_IF_DL_H
  struct sockaddr_dl *sdlp;
#    endif

/**
 * BSD 4.4 defines the size of an ifreq to be
 * max(sizeof(ifreq), sizeof(ifreq.ifr_name)+ifreq.ifr_addr.sa_len
 * However, under earlier systems, sa_len isn't present, so the size is
 * just sizeof(struct ifreq)
 */
#    ifdef HAVE_SA_LEN
#      define ifreq_size(i) max(sizeof(struct ifreq), sizeof((i).ifr_name) + (i).ifr_addr.sa_len)
#    else
#      define ifreq_size(i) sizeof(struct ifreq)
#    endif /* HAVE_SA_LEN */

  sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (sd < 0) {
    return -1;
  }
  memset(buf, 0, sizeof(buf));
  ifc.ifc_len = sizeof(buf);
  ifc.ifc_buf = buf;
  if (ioctl(sd, SIOCGIFCONF, (char *)&ifc) < 0) {
    close(sd);
    return -1;
  }
  n = ifc.ifc_len;
  for (i = 0; i < n; i += ifreq_size(*ifrp)) {
    ifrp = (struct ifreq *)((char *)ifc.ifc_buf + i);
    strncpy(ifr.ifr_name, ifrp->ifr_name, IFNAMSIZ);
#    ifdef SIOCGIFHWADDR
    if (ioctl(sd, SIOCGIFHWADDR, &ifr) < 0) continue;
    a = (unsigned char *)&ifr.ifr_hwaddr.sa_data;
#    else
#      ifdef SIOCGENADDR
    if (ioctl(sd, SIOCGENADDR, &ifr) < 0) continue;
    a = (unsigned char *)ifr.ifr_enaddr;
#      else
#        ifdef HAVE_NET_IF_DL_H
    sdlp = (struct sockaddr_dl *)&ifrp->ifr_addr;
    if ((sdlp->sdl_family != AF_LINK) || (sdlp->sdl_alen != 6)) continue;
    a = (unsigned char *)&sdlp->sdl_data[sdlp->sdl_nlen];
#        else
    /*
     * XXX we don't have a way of getting the hardware
     * address
     */
    close(sd);
    return 0;
#        endif /* HAVE_NET_IF_DL_H */
#      endif   /* SIOCGENADDR */
#    endif     /* SIOCGIFHWADDR */
    if (!a[0] && !a[1] && !a[2] && !a[3] && !a[4] && !a[5]) continue;
    if (node_id) {
      memcpy(node_id, a, 6);
      close(sd);
      return 1;
    }
  }
  close(sd);
  // #endif
  return 0;
}

/* Assume that the gettimeofday() has microsecond granularity */
#    define MAX_ADJUSTMENT 10

/**
 * Get clock from global sequence clock counter.
 *
 * Return -1 if the clock counter could not be opened/locked (in this case
 * pseudorandom value is returned in @ret_clock_seq), otherwise return 0.
 */
struct uuid_generator_clock_file_guard_t {
  int state_fd;
  FILE *state_f;
  uuid_generator_clock_file_guard_t() : state_fd(-2), state_f(nullptr) {}
  ~uuid_generator_clock_file_guard_t() {
    close_file();
    close_fd();
  }

  void close_fd() {
    if (state_fd >= 0) {
      close(state_fd);
      state_fd = -1;
    }
  }

  void close_file() {
    if (state_f != nullptr) {
      fclose(state_f);
      state_f = nullptr;
    }
  }
};
UTIL_CONFIG_UUID_INNER_THREAD_LOCAL uuid_generator_clock_file_guard_t uuid_generator_state_file;

static int get_clock(uint32_t *clock_high, uint32_t *clock_low, uint16_t *ret_clock_seq) {
  UTIL_CONFIG_UUID_INNER_THREAD_LOCAL int adjustment = 0;
  UTIL_CONFIG_UUID_INNER_THREAD_LOCAL struct timeval last = {0, 0};
  UTIL_CONFIG_UUID_INNER_THREAD_LOCAL uint16_t clock_seq;
  struct timeval tv;
  uint64_t clock_reg;
  mode_t save_umask;
  int len;
  int ret = 0;

  if (uuid_generator_state_file.state_fd == -2) {
    save_umask = umask(0);
    uuid_generator_state_file.state_fd = open("/var/lib/libuuid/clock.txt", O_RDWR | O_CREAT | O_CLOEXEC, 0660);
    (void)umask(save_umask);
    if (uuid_generator_state_file.state_fd != -1) {
      uuid_generator_state_file.state_f = fdopen(uuid_generator_state_file.state_fd, "r+" UL_CLOEXECSTR);
      if (!uuid_generator_state_file.state_f) {
        uuid_generator_state_file.close_fd();
        ret = -1;
      }
    } else
      ret = -1;
  }
  if (uuid_generator_state_file.state_fd >= 0) {
    rewind(uuid_generator_state_file.state_f);
    while (flock(uuid_generator_state_file.state_fd, LOCK_EX) < 0) {
      if ((errno == EAGAIN) || (errno == EINTR)) continue;
      uuid_generator_state_file.close_file();
      uuid_generator_state_file.close_fd();
      ret = -1;
      break;
    }
  }
  if (uuid_generator_state_file.state_fd >= 0) {
    unsigned int cl;
    unsigned long tv1, tv2;
    int a;

    if (fscanf(uuid_generator_state_file.state_f, "clock: %04x tv: %lu %lu adj: %d\n", &cl, &tv1, &tv2, &a) == 4) {
      clock_seq = cl & 0x3FFF;
      last.tv_sec = tv1;
      last.tv_usec = tv2;
      adjustment = a;
    }
  }

  if ((last.tv_sec == 0) && (last.tv_usec == 0)) {
    random_get_bytes(reinterpret_cast<unsigned char *>(&clock_seq), sizeof(clock_seq));
    clock_seq &= 0x3FFF;
    gettimeofday(&last, 0);
    last.tv_sec--;
  }

try_again:
  gettimeofday(&tv, 0);
  if ((tv.tv_sec < last.tv_sec) || ((tv.tv_sec == last.tv_sec) && (tv.tv_usec < last.tv_usec))) {
    clock_seq = (clock_seq + 1) & 0x3FFF;
    adjustment = 0;
    last = tv;
  } else if ((tv.tv_sec == last.tv_sec) && (tv.tv_usec == last.tv_usec)) {
    if (adjustment >= MAX_ADJUSTMENT) goto try_again;
    adjustment++;
  } else {
    adjustment = 0;
    last = tv;
  }

  clock_reg = tv.tv_usec * 10 + adjustment;
  clock_reg += ((uint64_t)tv.tv_sec) * 10000000;
  clock_reg += (((uint64_t)0x01B21DD2) << 32) + 0x13814000;

  if (uuid_generator_state_file.state_fd >= 0) {
    rewind(uuid_generator_state_file.state_f);
    len = fprintf(uuid_generator_state_file.state_f, "clock: %04x tv: %016llu %08llu adj: %08d\n", clock_seq,
                  static_cast<unsigned long long>(last.tv_sec), static_cast<unsigned long long>(last.tv_usec),
                  adjustment);
    fflush(uuid_generator_state_file.state_f);
    if (ftruncate(uuid_generator_state_file.state_fd, len) < 0) {
      fprintf(uuid_generator_state_file.state_f, "                   \n");
      fflush(uuid_generator_state_file.state_f);
    }
    rewind(uuid_generator_state_file.state_f);
    flock(uuid_generator_state_file.state_fd, LOCK_UN);
  }

  *clock_high = clock_reg >> 32;
  *clock_low = clock_reg;
  *ret_clock_seq = clock_seq;
  return ret;
}

static int __uuid_generate_time(LIBATFRAME_UTILS_NAMESPACE_ID::random::uuid &out) {
  static unsigned char node_id[6];
  static int has_init = 0;
  uint32_t clock_mid;
  int ret;

  if (!has_init) {
    if (get_node_id(node_id) <= 0) {
      random_get_bytes(node_id, 6);
      /*
       * Set multicast bit, to prevent conflicts
       * with IEEE 802 addresses obtained from
       * network cards
       */
      node_id[0] |= 0x01;
    }
    has_init = 1;
  }
  ret = get_clock(&clock_mid, &out.time_low, &out.clock_seq);
  out.clock_seq |= 0x8000;
  out.time_mid = (uint16_t)clock_mid;
  out.time_hi_and_version = ((clock_mid >> 16) & 0x0FFF) | 0x1000;
  memcpy(out.node, node_id, 6);
  return ret;
}

static void __uuid_generate_random(LIBATFRAME_UTILS_NAMESPACE_ID::random::uuid &out) {
  random_get_bytes(reinterpret_cast<unsigned char *>(&out), sizeof(out));
  // Set version
  out.clock_seq = (out.clock_seq & 0x3FFF) | 0x8000;
  out.time_hi_and_version = (out.time_hi_and_version & 0x0FFF) | 0x4000;
}

/*
 * Check whether good random source (/dev/random or /dev/urandom)
 * is available.
 */
// static int have_random_source(void) {
//     struct stat s;
//     return (!stat("/dev/random", &s) || !stat("/dev/urandom", &s));
// }

/*
 * This is the generic front-end to uuid_generate_random and
 * uuid_generate_time.  It uses uuid_generate_random only if
 * /dev/urandom is available, since otherwise we won't have
 * high-quality randomness.
 */
// static void uuid_generate(LIBATFRAME_UTILS_NAMESPACE_ID::random::uuid& out) {
//     if (have_random_source()) {
//         __uuid_generate_random(out);
//     } else {
//         __uuid_generate_time(out);
//     }
// }
}  // namespace details
#  endif
LIBATFRAME_UTILS_API std::string uuid_generator::uuid_to_string(const uuid &id, bool remove_minus) {
  char str_buff[64] = {0};

#  if defined(LIBATFRAME_UTILS_ENABLE_LIBUUID) && LIBATFRAME_UTILS_ENABLE_LIBUUID
  uuid_t linux_uid;
  memcpy(linux_uid, &id, sizeof(uuid));
  uuid_unparse(linux_uid, str_buff);
  if (remove_minus) {
    size_t len = 0;
    for (size_t i = 0; i < sizeof(str_buff) && len < sizeof(str_buff) && str_buff[i]; ++i) {
      if ('-' != str_buff[i]) {
        str_buff[len++] = str_buff[i];
      }
    }
    if (len < sizeof(str_buff)) {
      str_buff[len] = 0;
    } else {
      str_buff[sizeof(str_buff) - 1] = 0;
    }
  }
#  else
  if (remove_minus) {
    UTIL_STRFUNC_SNPRINTF(str_buff, sizeof(str_buff), "%08x%04x%04x%04x%02x%02x%02x%02x%02x%02x", id.time_low,
                          id.time_mid, id.time_hi_and_version, id.clock_seq, id.node[0], id.node[1], id.node[2],
                          id.node[3], id.node[4], id.node[5]);
  } else {
    UTIL_STRFUNC_SNPRINTF(str_buff, sizeof(str_buff), "%08x-%04x-%04x-%04x-%02x%02x%02x%02x%02x%02x", id.time_low,
                          id.time_mid, id.time_hi_and_version, id.clock_seq, id.node[0], id.node[1], id.node[2],
                          id.node[3], id.node[4], id.node[5]);
  }
#  endif
  return std::string(str_buff);
}

LIBATFRAME_UTILS_API std::string uuid_generator::uuid_to_binary(const uuid &id) {
  std::string ret;
  ret.resize(sizeof(uuid));
  ret[0] = static_cast<char>((id.time_low >> 24) & 0xFF);
  ret[1] = static_cast<char>((id.time_low >> 16) & 0xFF);
  ret[2] = static_cast<char>((id.time_low >> 8) & 0xFF);
  ret[3] = static_cast<char>(id.time_low & 0xFF);
  ret[4] = static_cast<char>((id.time_mid >> 8) & 0xFF);
  ret[5] = static_cast<char>(id.time_mid & 0xFF);
  ret[6] = static_cast<char>((id.time_hi_and_version >> 8) & 0xFF);
  ret[7] = static_cast<char>(id.time_hi_and_version & 0xFF);
  ret[8] = static_cast<char>((id.clock_seq >> 8) & 0xFF);
  ret[9] = static_cast<char>(id.clock_seq & 0xFF);
  memcpy(&ret[10], id.node, sizeof(id.node));

  return ret;
}

LIBATFRAME_UTILS_API uuid uuid_generator::binary_to_uuid(const std::string &id_bin) {
  uuid ret;
  if (sizeof(uuid) > id_bin.size()) {
    memset(&ret, 0, sizeof(ret));
    return ret;
  }

  ret.time_low = (static_cast<uint32_t>(id_bin[0]) << 24) | static_cast<uint32_t>(id_bin[1] << 16) |
                 static_cast<uint32_t>(id_bin[2] << 8) | static_cast<uint32_t>(id_bin[3]);
  ret.time_mid = (static_cast<uint16_t>(id_bin[4]) << 8) | static_cast<uint16_t>(id_bin[5]);
  ret.time_hi_and_version = (static_cast<uint16_t>(id_bin[6]) << 8) | static_cast<uint16_t>(id_bin[7]);
  ret.clock_seq = (static_cast<uint16_t>(id_bin[8]) << 8) | static_cast<uint16_t>(id_bin[9]);
  memcpy(ret.node, &id_bin[10], sizeof(ret.node));
  return ret;
}

LIBATFRAME_UTILS_API uuid uuid_generator::generate() {
  uuid ret;

#  if defined(LIBATFRAME_UTILS_ENABLE_LIBUUID) && LIBATFRAME_UTILS_ENABLE_LIBUUID
  uuid_t linux_uid;
  uuid_generate(linux_uid);
  memcpy(&ret, linux_uid, sizeof(ret));
#  elif defined(LIBATFRAME_UTILS_ENABLE_UUID_WINRPC) && LIBATFRAME_UTILS_ENABLE_UUID_WINRPC
  UUID res;
  UuidCreate(&res);
  ret.time_low = static_cast<uint32_t>(res.Data1);
  ret.time_mid = res.Data2;
  ret.time_hi_and_version = res.Data3;
  ret.clock_seq = (static_cast<uint16_t>(res.Data4[0]) << 8) | static_cast<uint16_t>(res.Data4[1]);
  memcpy(ret.node, &res.Data4[2], sizeof(ret.node));
  static_assert(sizeof(res.Data4) >= sizeof(ret.clock_seq) + sizeof(ret.node), "uuid size mismatch");
#  else
  details::__uuid_generate_random(ret);
#  endif
  return ret;
}

LIBATFRAME_UTILS_API std::string uuid_generator::generate_string(bool remove_minus) {
  return uuid_to_string(generate(), remove_minus);
}

LIBATFRAME_UTILS_API uuid uuid_generator::generate_random() {
  uuid ret;

#  if defined(LIBATFRAME_UTILS_ENABLE_LIBUUID) && LIBATFRAME_UTILS_ENABLE_LIBUUID
  uuid_t linux_uid;
  uuid_generate_random(linux_uid);
  memcpy(&ret, linux_uid, sizeof(ret));
#  elif defined(LIBATFRAME_UTILS_ENABLE_UUID_WINRPC) && LIBATFRAME_UTILS_ENABLE_UUID_WINRPC
  UUID res;
  UuidCreate(&res);
  ret.time_low = static_cast<uint32_t>(res.Data1);
  ret.time_mid = res.Data2;
  ret.time_hi_and_version = res.Data3;
  ret.clock_seq = (static_cast<uint16_t>(res.Data4[0]) << 8) | static_cast<uint16_t>(res.Data4[1]);
  memcpy(ret.node, &res.Data4[2], sizeof(ret.node));
  static_assert(sizeof(res.Data4) >= sizeof(ret.clock_seq) + sizeof(ret.node), "uuid size mismatch");
#  else
  details::__uuid_generate_random(ret);
#  endif
  return ret;
}

LIBATFRAME_UTILS_API std::string uuid_generator::generate_string_random(bool remove_minus) {
  return uuid_to_string(generate_random(), remove_minus);
}

LIBATFRAME_UTILS_API uuid uuid_generator::generate_time() {
  uuid ret;

#  if defined(LIBATFRAME_UTILS_ENABLE_LIBUUID) && LIBATFRAME_UTILS_ENABLE_LIBUUID
  uuid_t linux_uid;
  uuid_generate_time(linux_uid);
  memcpy(&ret, linux_uid, sizeof(ret));
#  elif defined(LIBATFRAME_UTILS_ENABLE_UUID_WINRPC) && LIBATFRAME_UTILS_ENABLE_UUID_WINRPC
  UUID res;
  UuidCreateSequential(&res);
  ret.time_low = static_cast<uint32_t>(res.Data1);
  ret.time_mid = res.Data2;
  ret.time_hi_and_version = res.Data3;
  ret.clock_seq = (static_cast<uint16_t>(res.Data4[0]) << 8) | static_cast<uint16_t>(res.Data4[1]);
  memcpy(ret.node, &res.Data4[2], sizeof(ret.node));
  static_assert(sizeof(res.Data4) >= sizeof(ret.clock_seq) + sizeof(ret.node), "uuid size mismatch");
#  else
  details::__uuid_generate_time(ret);
#  endif
  return ret;
}

LIBATFRAME_UTILS_API std::string uuid_generator::generate_string_time(bool remove_minus) {
  return uuid_to_string(generate_time(), remove_minus);
}
}  // namespace random
LIBATFRAME_UTILS_NAMESPACE_END

#endif