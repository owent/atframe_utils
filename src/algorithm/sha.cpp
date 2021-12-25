#include <algorithm>

#include <common/string_oprs.h>

#include <algorithm/sha.h>

#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
#  define UTIL_HASH_IMPLEMENT_SHA_USING_OPENSSL 1
#elif defined(CRYPTO_USE_MBEDTLS)
#  define UTIL_HASH_IMPLEMENT_SHA_USING_MBEDTLS 1
#endif

LIBATFRAME_UTILS_NAMESPACE_BEGIN
namespace hash {

namespace detail {
#if defined(UTIL_HASH_IMPLEMENT_SHA_USING_OPENSSL) && UTIL_HASH_IMPLEMENT_SHA_USING_OPENSSL
struct sha_inner_data {
  unsigned char output[EVP_MAX_MD_SIZE];
  EVP_MD_CTX *ctx;
};

static inline sha_inner_data *into_inner_type(void *in) { return reinterpret_cast<sha_inner_data *>(in); }

static inline void free_inner_type(void *in, LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::type) {
  if (in == nullptr) {
    return;
  }

  if (into_inner_type(in)->ctx != nullptr) {
#  if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_MD_CTX_destroy(into_inner_type(in)->ctx);
#  else
    EVP_MD_CTX_free(into_inner_type(in)->ctx);
#  endif
    into_inner_type(in)->ctx = nullptr;
  }

  free(into_inner_type(in));
}

static inline sha_inner_data *malloc_inner_type(LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::type t) {
  sha_inner_data *ret = reinterpret_cast<sha_inner_data *>(malloc(sizeof(sha_inner_data)));
  if (ret == nullptr) {
    return nullptr;
  }
#  if OPENSSL_VERSION_NUMBER < 0x10100000L
  ret->ctx = EVP_MD_CTX_create();
#  else
  ret->ctx = EVP_MD_CTX_new();
#  endif
  if (ret->ctx == nullptr) {
    free_inner_type(ret, t);
    return nullptr;
  }

  const EVP_MD *md = nullptr;
  switch (t) {
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA1:
      md = EVP_get_digestbynid(NID_sha1);
      break;
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA224:
      md = EVP_get_digestbynid(NID_sha224);
      break;
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA256:
      md = EVP_get_digestbynid(NID_sha256);
      break;
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA384:
      md = EVP_get_digestbynid(NID_sha384);
      break;
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA512:
      md = EVP_get_digestbynid(NID_sha512);
      break;
    default:
      break;
  }

  if (md == nullptr) {
    free_inner_type(ret, t);
    return nullptr;
  }

  if (1 != EVP_DigestInit_ex(ret->ctx, md, nullptr)) {
    free_inner_type(ret, t);
    return nullptr;
  }

  return ret;
}

static inline unsigned char *get_output_buffer(void *in) {
  if (in == nullptr) {
    return nullptr;
  }

  return into_inner_type(in)->output;
}

#elif defined(UTIL_HASH_IMPLEMENT_SHA_USING_MBEDTLS) && UTIL_HASH_IMPLEMENT_SHA_USING_MBEDTLS
struct sha_inner_data {
  unsigned char output[64];
  union {
    mbedtls_sha1_context sha1_context;
    mbedtls_sha256_context sha224_context;
    mbedtls_sha256_context sha256_context;
    mbedtls_sha512_context sha384_context;
    mbedtls_sha512_context sha512_context;
  };
};

static inline sha_inner_data *into_inner_type(void *in) { return reinterpret_cast<sha_inner_data *>(in); }

static inline void free_inner_type(void *in, LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::type t) {
  if (in == nullptr) {
    return;
  }

  sha_inner_data *obj = into_inner_type(in);

  switch (t) {
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA1:
      mbedtls_sha1_free(&obj->sha1_context);
      break;
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA224:
      mbedtls_sha256_free(&obj->sha224_context);
      break;
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA256:
      mbedtls_sha256_free(&obj->sha256_context);
      break;
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA384:
      mbedtls_sha512_free(&obj->sha384_context);
      break;
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA512:
      mbedtls_sha512_free(&obj->sha384_context);
      break;
    default:
      break;
  }

  free(into_inner_type(in));
}

static inline sha_inner_data *malloc_inner_type(LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::type t) {
  sha_inner_data *ret = reinterpret_cast<sha_inner_data *>(malloc(sizeof(sha_inner_data)));
  if (ret == nullptr) {
    return nullptr;
  }

  bool is_success = false;
  switch (t) {
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA1:
      mbedtls_sha1_init(&ret->sha1_context);
      is_success = 0 == mbedtls_sha1_starts_ret(&ret->sha1_context);
      break;
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA224:
      mbedtls_sha256_init(&ret->sha224_context);
      is_success = 0 == mbedtls_sha256_starts_ret(&ret->sha224_context, 1);
      break;
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA256:
      mbedtls_sha256_init(&ret->sha256_context);
      is_success = 0 == mbedtls_sha256_starts_ret(&ret->sha256_context, 0);
      break;
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA384:
      mbedtls_sha512_init(&ret->sha384_context);
      is_success = 0 == mbedtls_sha512_starts_ret(&ret->sha384_context, 1);
      break;
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA512:
      mbedtls_sha512_init(&ret->sha512_context);
      is_success = 0 == mbedtls_sha512_starts_ret(&ret->sha384_context, 0);
      break;
    default:
      break;
  }

  if (false == is_success) {
    free_inner_type(ret, t);
    ret = nullptr;
  }

  return ret;
}

static inline unsigned char *get_output_buffer(void *in) {
  if (in == nullptr) {
    return nullptr;
  }

  return into_inner_type(in)->output;
}
#else
#  ifndef UL64
#    if defined(_MSC_VER) || defined(__WATCOMC__)
#      define UL64(x) x##ui64
#    else
#      define UL64(x) x##ULL
#    endif
#  endif
/*
 * 32-bit integer manipulation macros (big endian)
 */
#  ifndef GET_UINT32_BE
#    define GET_UINT32_BE(n, b, i)                                                                          \
      do {                                                                                                  \
        (n) = ((uint32_t)(b)[(i)] << 24) | ((uint32_t)(b)[(i) + 1] << 16) | ((uint32_t)(b)[(i) + 2] << 8) | \
              ((uint32_t)(b)[(i) + 3]);                                                                     \
      } while (0)
#  endif

#  ifndef PUT_UINT32_BE
#    define PUT_UINT32_BE(n, b, i)                 \
      do {                                         \
        (b)[(i)] = (unsigned char)((n) >> 24);     \
        (b)[(i) + 1] = (unsigned char)((n) >> 16); \
        (b)[(i) + 2] = (unsigned char)((n) >> 8);  \
        (b)[(i) + 3] = (unsigned char)((n));       \
      } while (0)
#  endif

/*
 * 64-bit integer manipulation macros (big endian)
 */
#  ifndef GET_UINT64_BE
#    define GET_UINT64_BE(n, b, i)                                                                               \
      {                                                                                                          \
        (n) = ((uint64_t)(b)[(i)] << 56) | ((uint64_t)(b)[(i) + 1] << 48) | ((uint64_t)(b)[(i) + 2] << 40) |     \
              ((uint64_t)(b)[(i) + 3] << 32) | ((uint64_t)(b)[(i) + 4] << 24) | ((uint64_t)(b)[(i) + 5] << 16) | \
              ((uint64_t)(b)[(i) + 6] << 8) | ((uint64_t)(b)[(i) + 7]);                                          \
      }
#  endif /* GET_UINT64_BE */

#  ifndef PUT_UINT64_BE
#    define PUT_UINT64_BE(n, b, i)                 \
      {                                            \
        (b)[(i)] = (unsigned char)((n) >> 56);     \
        (b)[(i) + 1] = (unsigned char)((n) >> 48); \
        (b)[(i) + 2] = (unsigned char)((n) >> 40); \
        (b)[(i) + 3] = (unsigned char)((n) >> 32); \
        (b)[(i) + 4] = (unsigned char)((n) >> 24); \
        (b)[(i) + 5] = (unsigned char)((n) >> 16); \
        (b)[(i) + 6] = (unsigned char)((n) >> 8);  \
        (b)[(i) + 7] = (unsigned char)((n));       \
      }
#  endif /* PUT_UINT64_BE */

struct sha1_context_t {
  uint32_t total[2];        /*!< The number of Bytes processed.  */
  uint32_t state[5];        /*!< The intermediate digest state.  */
  unsigned char buffer[64]; /*!< The data block being processed. */
};

struct sha256_context_t {
  uint32_t total[2];        /*!< The number of Bytes processed.  */
  uint32_t state[8];        /*!< The intermediate digest state.  */
  unsigned char buffer[64]; /*!< The data block being processed. */
  int is224;                /*!< Determines which function to use:
                                0: Use SHA-256, or 1: Use SHA-224. */
};

struct sha512_context_t {
  uint64_t total[2];         /*!< The number of Bytes processed. */
  uint64_t state[8];         /*!< The intermediate digest state. */
  unsigned char buffer[128]; /*!< The data block being processed. */
  int is384;                 /*!< Determines which function to use:
                                 0: Use SHA-512, or 1: Use SHA-384. */
};

struct sha_inner_data {
  unsigned char output[64];
  union {
    sha1_context_t sha1_context;
    sha256_context_t sha224_context;
    sha256_context_t sha256_context;
    sha512_context_t sha384_context;
    sha512_context_t sha512_context;
  };
};

static inline sha_inner_data *into_inner_type(void *in) { return reinterpret_cast<sha_inner_data *>(in); }

static inline void free_inner_type(void *in, LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::type) {
  if (in == nullptr) {
    return;
  }

  free(into_inner_type(in));
}

/*
 * SHA-1 context setup
 */
static void inner_sha1_start(sha1_context_t &ctx) {
  ctx.total[0] = 0;
  ctx.total[1] = 0;

  ctx.state[0] = 0x67452301;
  ctx.state[1] = 0xEFCDAB89;
  ctx.state[2] = 0x98BADCFE;
  ctx.state[3] = 0x10325476;
  ctx.state[4] = 0xC3D2E1F0;
}

static void inner_sha256_start(sha256_context_t &ctx, bool is224) {
  ctx.total[0] = 0;
  ctx.total[1] = 0;

  if (is224) {
    /* SHA-224 */
    ctx.state[0] = 0xC1059ED8;
    ctx.state[1] = 0x367CD507;
    ctx.state[2] = 0x3070DD17;
    ctx.state[3] = 0xF70E5939;
    ctx.state[4] = 0xFFC00B31;
    ctx.state[5] = 0x68581511;
    ctx.state[6] = 0x64F98FA7;
    ctx.state[7] = 0xBEFA4FA4;
    ctx.is224 = 1;
  } else {
    /* SHA-256 */
    ctx.state[0] = 0x6A09E667;
    ctx.state[1] = 0xBB67AE85;
    ctx.state[2] = 0x3C6EF372;
    ctx.state[3] = 0xA54FF53A;
    ctx.state[4] = 0x510E527F;
    ctx.state[5] = 0x9B05688C;
    ctx.state[6] = 0x1F83D9AB;
    ctx.state[7] = 0x5BE0CD19;
    ctx.is224 = 0;
  }
}

/*
 * SHA-512 context setup
 */
static void inner_sha512_starts_ret(sha512_context_t &ctx, bool is384) {
  ctx.total[0] = 0;
  ctx.total[1] = 0;

  if (is384) {
    /* SHA-384 */
    ctx.state[0] = UL64(0xCBBB9D5DC1059ED8);
    ctx.state[1] = UL64(0x629A292A367CD507);
    ctx.state[2] = UL64(0x9159015A3070DD17);
    ctx.state[3] = UL64(0x152FECD8F70E5939);
    ctx.state[4] = UL64(0x67332667FFC00B31);
    ctx.state[5] = UL64(0x8EB44A8768581511);
    ctx.state[6] = UL64(0xDB0C2E0D64F98FA7);
    ctx.state[7] = UL64(0x47B5481DBEFA4FA4);
    ctx.is384 = 1;
  } else {
    /* SHA-512 */
    ctx.state[0] = UL64(0x6A09E667F3BCC908);
    ctx.state[1] = UL64(0xBB67AE8584CAA73B);
    ctx.state[2] = UL64(0x3C6EF372FE94F82B);
    ctx.state[3] = UL64(0xA54FF53A5F1D36F1);
    ctx.state[4] = UL64(0x510E527FADE682D1);
    ctx.state[5] = UL64(0x9B05688C2B3E6C1F);
    ctx.state[6] = UL64(0x1F83D9ABFB41BD6B);
    ctx.state[7] = UL64(0x5BE0CD19137E2179);
    ctx.is384 = 0;
  }
}

static sha_inner_data *malloc_inner_type(LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::type t) {
  sha_inner_data *ret = reinterpret_cast<sha_inner_data *>(malloc(sizeof(sha_inner_data)));
  if (ret == nullptr) {
    return nullptr;
  }

  switch (t) {
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA1:
      memset(&ret->sha1_context, 0, sizeof(ret->sha1_context));
      inner_sha1_start(ret->sha1_context);
      break;
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA224:
      memset(&ret->sha224_context, 0, sizeof(ret->sha224_context));
      inner_sha256_start(ret->sha224_context, true);
      break;
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA256:
      memset(&ret->sha256_context, 0, sizeof(ret->sha256_context));
      inner_sha256_start(ret->sha256_context, false);
      break;
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA384:
      memset(&ret->sha384_context, 0, sizeof(ret->sha384_context));
      inner_sha512_starts_ret(ret->sha384_context, true);
      break;
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA512:
      memset(&ret->sha512_context, 0, sizeof(ret->sha512_context));
      inner_sha512_starts_ret(ret->sha384_context, false);
      break;
    default:
      break;
  }

  return ret;
}

static inline unsigned char *get_output_buffer(void *in) {
  if (in == nullptr) {
    return nullptr;
  }

  return into_inner_type(in)->output;
}

static void inner_internal_sha1_process(sha1_context_t &ctx, const unsigned char data[64]) {
  uint32_t temp, W[16], A, B, C, D, E;

  GET_UINT32_BE(W[0], data, 0);
  GET_UINT32_BE(W[1], data, 4);
  GET_UINT32_BE(W[2], data, 8);
  GET_UINT32_BE(W[3], data, 12);
  GET_UINT32_BE(W[4], data, 16);
  GET_UINT32_BE(W[5], data, 20);
  GET_UINT32_BE(W[6], data, 24);
  GET_UINT32_BE(W[7], data, 28);
  GET_UINT32_BE(W[8], data, 32);
  GET_UINT32_BE(W[9], data, 36);
  GET_UINT32_BE(W[10], data, 40);
  GET_UINT32_BE(W[11], data, 44);
  GET_UINT32_BE(W[12], data, 48);
  GET_UINT32_BE(W[13], data, 52);
  GET_UINT32_BE(W[14], data, 56);
  GET_UINT32_BE(W[15], data, 60);

#  define S(x, n) (((x) << (n)) | (((x)&0xFFFFFFFF) >> (32 - (n))))

#  define R(t) \
    (temp = W[((t)-3) & 0x0F] ^ W[((t)-8) & 0x0F] ^ W[((t)-14) & 0x0F] ^ W[(t)&0x0F], (W[(t)&0x0F] = S(temp, 1)))

#  define P(a, b, c, d, e, x)                        \
    do {                                             \
      (e) += S((a), 5) + F((b), (c), (d)) + K + (x); \
      (b) = S((b), 30);                              \
    } while (0)

  A = ctx.state[0];
  B = ctx.state[1];
  C = ctx.state[2];
  D = ctx.state[3];
  E = ctx.state[4];

#  define F(x, y, z) ((z) ^ ((x) & ((y) ^ (z))))
#  define K 0x5A827999

  P(A, B, C, D, E, W[0]);
  P(E, A, B, C, D, W[1]);
  P(D, E, A, B, C, W[2]);
  P(C, D, E, A, B, W[3]);
  P(B, C, D, E, A, W[4]);
  P(A, B, C, D, E, W[5]);
  P(E, A, B, C, D, W[6]);
  P(D, E, A, B, C, W[7]);
  P(C, D, E, A, B, W[8]);
  P(B, C, D, E, A, W[9]);
  P(A, B, C, D, E, W[10]);
  P(E, A, B, C, D, W[11]);
  P(D, E, A, B, C, W[12]);
  P(C, D, E, A, B, W[13]);
  P(B, C, D, E, A, W[14]);
  P(A, B, C, D, E, W[15]);
  P(E, A, B, C, D, R(16));
  P(D, E, A, B, C, R(17));
  P(C, D, E, A, B, R(18));
  P(B, C, D, E, A, R(19));

#  undef K
#  undef F

#  define F(x, y, z) ((x) ^ (y) ^ (z))
#  define K 0x6ED9EBA1

  P(A, B, C, D, E, R(20));
  P(E, A, B, C, D, R(21));
  P(D, E, A, B, C, R(22));
  P(C, D, E, A, B, R(23));
  P(B, C, D, E, A, R(24));
  P(A, B, C, D, E, R(25));
  P(E, A, B, C, D, R(26));
  P(D, E, A, B, C, R(27));
  P(C, D, E, A, B, R(28));
  P(B, C, D, E, A, R(29));
  P(A, B, C, D, E, R(30));
  P(E, A, B, C, D, R(31));
  P(D, E, A, B, C, R(32));
  P(C, D, E, A, B, R(33));
  P(B, C, D, E, A, R(34));
  P(A, B, C, D, E, R(35));
  P(E, A, B, C, D, R(36));
  P(D, E, A, B, C, R(37));
  P(C, D, E, A, B, R(38));
  P(B, C, D, E, A, R(39));

#  undef K
#  undef F

#  define F(x, y, z) (((x) & (y)) | ((z) & ((x) | (y))))
#  define K 0x8F1BBCDC

  P(A, B, C, D, E, R(40));
  P(E, A, B, C, D, R(41));
  P(D, E, A, B, C, R(42));
  P(C, D, E, A, B, R(43));
  P(B, C, D, E, A, R(44));
  P(A, B, C, D, E, R(45));
  P(E, A, B, C, D, R(46));
  P(D, E, A, B, C, R(47));
  P(C, D, E, A, B, R(48));
  P(B, C, D, E, A, R(49));
  P(A, B, C, D, E, R(50));
  P(E, A, B, C, D, R(51));
  P(D, E, A, B, C, R(52));
  P(C, D, E, A, B, R(53));
  P(B, C, D, E, A, R(54));
  P(A, B, C, D, E, R(55));
  P(E, A, B, C, D, R(56));
  P(D, E, A, B, C, R(57));
  P(C, D, E, A, B, R(58));
  P(B, C, D, E, A, R(59));

#  undef K
#  undef F

#  define F(x, y, z) ((x) ^ (y) ^ (z))
#  define K 0xCA62C1D6

  P(A, B, C, D, E, R(60));
  P(E, A, B, C, D, R(61));
  P(D, E, A, B, C, R(62));
  P(C, D, E, A, B, R(63));
  P(B, C, D, E, A, R(64));
  P(A, B, C, D, E, R(65));
  P(E, A, B, C, D, R(66));
  P(D, E, A, B, C, R(67));
  P(C, D, E, A, B, R(68));
  P(B, C, D, E, A, R(69));
  P(A, B, C, D, E, R(70));
  P(E, A, B, C, D, R(71));
  P(D, E, A, B, C, R(72));
  P(C, D, E, A, B, R(73));
  P(B, C, D, E, A, R(74));
  P(A, B, C, D, E, R(75));
  P(E, A, B, C, D, R(76));
  P(D, E, A, B, C, R(77));
  P(C, D, E, A, B, R(78));
  P(B, C, D, E, A, R(79));

#  undef K
#  undef F

  ctx.state[0] += A;
  ctx.state[1] += B;
  ctx.state[2] += C;
  ctx.state[3] += D;
  ctx.state[4] += E;

#  undef P
#  undef R
#  undef S
}

/*
 * SHA-1 process buffer
 */
static void inner_sha1_update(sha1_context_t &ctx, const unsigned char *input, size_t ilen) {
  size_t fill;
  uint32_t left;

  if (ilen == 0 || input == nullptr) {
    return;
  }

  left = ctx.total[0] & 0x3F;
  fill = 64 - left;

  ctx.total[0] += (uint32_t)ilen;
  ctx.total[0] &= 0xFFFFFFFF;

  if (ctx.total[0] < (uint32_t)ilen) ctx.total[1]++;

  if (left && ilen >= fill) {
    memcpy((void *)(ctx.buffer + left), input, fill);

    inner_internal_sha1_process(ctx, ctx.buffer);

    input += fill;
    ilen -= fill;
    left = 0;
  }

  while (ilen >= 64) {
    inner_internal_sha1_process(ctx, input);

    input += 64;
    ilen -= 64;
  }

  if (ilen > 0) {
    memcpy((void *)(ctx.buffer + left), input, ilen);
  }
}

/*
 * SHA-1 final digest
 */
static void inner_sha1_finish(sha1_context_t &ctx, unsigned char output[20]) {
  uint32_t used;
  uint32_t high, low;

  /*
   * Add padding: 0x80 then 0x00 until 8 bytes remain for the length
   */
  used = ctx.total[0] & 0x3F;

  ctx.buffer[used++] = 0x80;

  if (used <= 56) {
    /* Enough room for padding + length in current block */
    memset(ctx.buffer + used, 0, 56 - used);
  } else {
    /* We'll need an extra block */
    memset(ctx.buffer + used, 0, 64 - used);

    inner_internal_sha1_process(ctx, ctx.buffer);

    memset(ctx.buffer, 0, 56);
  }

  /*
   * Add message length
   */
  high = (ctx.total[0] >> 29) | (ctx.total[1] << 3);
  low = (ctx.total[0] << 3);

  PUT_UINT32_BE(high, ctx.buffer, 56);
  PUT_UINT32_BE(low, ctx.buffer, 60);

  inner_internal_sha1_process(ctx, ctx.buffer);

  /*
   * Output final state
   */
  PUT_UINT32_BE(ctx.state[0], output, 0);
  PUT_UINT32_BE(ctx.state[1], output, 4);
  PUT_UINT32_BE(ctx.state[2], output, 8);
  PUT_UINT32_BE(ctx.state[3], output, 12);
  PUT_UINT32_BE(ctx.state[4], output, 16);
}

static void inner_internal_sha256_process(sha256_context_t &ctx, const unsigned char data[64]) {
  uint32_t temp1, temp2, W[64];
  uint32_t A[8];
  unsigned int i;

  static constexpr const uint32_t K[] = {
      0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5, 0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
      0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3, 0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
      0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC, 0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
      0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7, 0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
      0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13, 0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
      0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3, 0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
      0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5, 0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
      0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208, 0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2,
  };

#  define SHR(x, n) (((x)&0xFFFFFFFF) >> (n))
#  define ROTR(x, n) (SHR(x, n) | ((x) << (32 - (n))))

#  define S0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ SHR(x, 3))
#  define S1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ SHR(x, 10))

#  define S2(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#  define S3(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))

#  define F0(x, y, z) (((x) & (y)) | ((z) & ((x) | (y))))
#  define F1(x, y, z) ((z) ^ ((x) & ((y) ^ (z))))

#  define R(t) (W[t] = S1(W[(t)-2]) + W[(t)-7] + S0(W[(t)-15]) + W[(t)-16])

#  define P(a, b, c, d, e, f, g, h, x, K)                  \
    do {                                                   \
      temp1 = (h) + S3(e) + F1((e), (f), (g)) + (K) + (x); \
      temp2 = S2(a) + F0((a), (b), (c));                   \
      (d) += temp1;                                        \
      (h) = temp1 + temp2;                                 \
    } while (0)

  for (i = 0; i < 8; i++) A[i] = ctx.state[i];
  for (i = 0; i < 16; i++) GET_UINT32_BE(W[i], data, 4 * i);

  for (i = 0; i < 16; i += 8) {
    P(A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], W[i + 0], K[i + 0]);
    P(A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], W[i + 1], K[i + 1]);
    P(A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], W[i + 2], K[i + 2]);
    P(A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], W[i + 3], K[i + 3]);
    P(A[4], A[5], A[6], A[7], A[0], A[1], A[2], A[3], W[i + 4], K[i + 4]);
    P(A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], W[i + 5], K[i + 5]);
    P(A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], W[i + 6], K[i + 6]);
    P(A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], W[i + 7], K[i + 7]);
  }

  for (i = 16; i < 64; i += 8) {
    P(A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], R(i + 0), K[i + 0]);
    P(A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], R(i + 1), K[i + 1]);
    P(A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], R(i + 2), K[i + 2]);
    P(A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], R(i + 3), K[i + 3]);
    P(A[4], A[5], A[6], A[7], A[0], A[1], A[2], A[3], R(i + 4), K[i + 4]);
    P(A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], R(i + 5), K[i + 5]);
    P(A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], R(i + 6), K[i + 6]);
    P(A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], R(i + 7), K[i + 7]);
  }

  for (i = 0; i < 8; i++) {
    ctx.state[i] += A[i];
  }

#  undef P
#  undef R
#  undef F1
#  undef F0
#  undef S3
#  undef S2
#  undef S1
#  undef S0
#  undef ROTR
#  undef SHR
}

/*
 * SHA-256 process buffer
 */
static void inner_sha256_update(sha256_context_t &ctx, const unsigned char *input, size_t ilen) {
  size_t fill;
  uint32_t left;

  if (ilen == 0 || input == nullptr) return;

  left = ctx.total[0] & 0x3F;
  fill = 64 - left;

  ctx.total[0] += (uint32_t)ilen;
  ctx.total[0] &= 0xFFFFFFFF;

  if (ctx.total[0] < (uint32_t)ilen) ctx.total[1]++;

  if (left && ilen >= fill) {
    memcpy((void *)(ctx.buffer + left), input, fill);

    inner_internal_sha256_process(ctx, ctx.buffer);

    input += fill;
    ilen -= fill;
    left = 0;
  }

  while (ilen >= 64) {
    inner_internal_sha256_process(ctx, input);

    input += 64;
    ilen -= 64;
  }

  if (ilen > 0) memcpy((void *)(ctx.buffer + left), input, ilen);
}

/*
 * SHA-256 final digest
 */
static void inner_sha256_finish(sha256_context_t &ctx, unsigned char output[32]) {
  uint32_t used;
  uint32_t high, low;

  /*
   * Add padding: 0x80 then 0x00 until 8 bytes remain for the length
   */
  used = ctx.total[0] & 0x3F;

  ctx.buffer[used++] = 0x80;

  if (used <= 56) {
    /* Enough room for padding + length in current block */
    memset(ctx.buffer + used, 0, 56 - used);
  } else {
    /* We'll need an extra block */
    memset(ctx.buffer + used, 0, 64 - used);

    inner_internal_sha256_process(ctx, ctx.buffer);

    memset(ctx.buffer, 0, 56);
  }

  /*
   * Add message length
   */
  high = (ctx.total[0] >> 29) | (ctx.total[1] << 3);
  low = (ctx.total[0] << 3);

  PUT_UINT32_BE(high, ctx.buffer, 56);
  PUT_UINT32_BE(low, ctx.buffer, 60);

  inner_internal_sha256_process(ctx, ctx.buffer);

  /*
   * Output final state
   */
  PUT_UINT32_BE(ctx.state[0], output, 0);
  PUT_UINT32_BE(ctx.state[1], output, 4);
  PUT_UINT32_BE(ctx.state[2], output, 8);
  PUT_UINT32_BE(ctx.state[3], output, 12);
  PUT_UINT32_BE(ctx.state[4], output, 16);
  PUT_UINT32_BE(ctx.state[5], output, 20);
  PUT_UINT32_BE(ctx.state[6], output, 24);

  if (ctx.is224 == 0) PUT_UINT32_BE(ctx.state[7], output, 28);
}

static void inner_internal_sha512_process(sha512_context_t &ctx, const unsigned char data[128]) {
  int i;
  uint64_t temp1, temp2, W[80];
  uint64_t A, B, C, D, E, F, G, H;

  /*
   * Round constants
   */
  static constexpr const uint64_t K[80] = {
      UL64(0x428A2F98D728AE22), UL64(0x7137449123EF65CD), UL64(0xB5C0FBCFEC4D3B2F), UL64(0xE9B5DBA58189DBBC),
      UL64(0x3956C25BF348B538), UL64(0x59F111F1B605D019), UL64(0x923F82A4AF194F9B), UL64(0xAB1C5ED5DA6D8118),
      UL64(0xD807AA98A3030242), UL64(0x12835B0145706FBE), UL64(0x243185BE4EE4B28C), UL64(0x550C7DC3D5FFB4E2),
      UL64(0x72BE5D74F27B896F), UL64(0x80DEB1FE3B1696B1), UL64(0x9BDC06A725C71235), UL64(0xC19BF174CF692694),
      UL64(0xE49B69C19EF14AD2), UL64(0xEFBE4786384F25E3), UL64(0x0FC19DC68B8CD5B5), UL64(0x240CA1CC77AC9C65),
      UL64(0x2DE92C6F592B0275), UL64(0x4A7484AA6EA6E483), UL64(0x5CB0A9DCBD41FBD4), UL64(0x76F988DA831153B5),
      UL64(0x983E5152EE66DFAB), UL64(0xA831C66D2DB43210), UL64(0xB00327C898FB213F), UL64(0xBF597FC7BEEF0EE4),
      UL64(0xC6E00BF33DA88FC2), UL64(0xD5A79147930AA725), UL64(0x06CA6351E003826F), UL64(0x142929670A0E6E70),
      UL64(0x27B70A8546D22FFC), UL64(0x2E1B21385C26C926), UL64(0x4D2C6DFC5AC42AED), UL64(0x53380D139D95B3DF),
      UL64(0x650A73548BAF63DE), UL64(0x766A0ABB3C77B2A8), UL64(0x81C2C92E47EDAEE6), UL64(0x92722C851482353B),
      UL64(0xA2BFE8A14CF10364), UL64(0xA81A664BBC423001), UL64(0xC24B8B70D0F89791), UL64(0xC76C51A30654BE30),
      UL64(0xD192E819D6EF5218), UL64(0xD69906245565A910), UL64(0xF40E35855771202A), UL64(0x106AA07032BBD1B8),
      UL64(0x19A4C116B8D2D0C8), UL64(0x1E376C085141AB53), UL64(0x2748774CDF8EEB99), UL64(0x34B0BCB5E19B48A8),
      UL64(0x391C0CB3C5C95A63), UL64(0x4ED8AA4AE3418ACB), UL64(0x5B9CCA4F7763E373), UL64(0x682E6FF3D6B2B8A3),
      UL64(0x748F82EE5DEFB2FC), UL64(0x78A5636F43172F60), UL64(0x84C87814A1F0AB72), UL64(0x8CC702081A6439EC),
      UL64(0x90BEFFFA23631E28), UL64(0xA4506CEBDE82BDE9), UL64(0xBEF9A3F7B2C67915), UL64(0xC67178F2E372532B),
      UL64(0xCA273ECEEA26619C), UL64(0xD186B8C721C0C207), UL64(0xEADA7DD6CDE0EB1E), UL64(0xF57D4F7FEE6ED178),
      UL64(0x06F067AA72176FBA), UL64(0x0A637DC5A2C898A6), UL64(0x113F9804BEF90DAE), UL64(0x1B710B35131C471B),
      UL64(0x28DB77F523047D84), UL64(0x32CAAB7B40C72493), UL64(0x3C9EBE0A15C9BEBC), UL64(0x431D67C49C100D4C),
      UL64(0x4CC5D4BECB3E42B6), UL64(0x597F299CFC657E2A), UL64(0x5FCB6FAB3AD6FAEC), UL64(0x6C44198C4A475817)};
#  define SHR(x, n) ((x) >> (n))
#  define ROTR(x, n) (SHR((x), (n)) | ((x) << (64 - (n))))

#  define S0(x) (ROTR(x, 1) ^ ROTR(x, 8) ^ SHR(x, 7))
#  define S1(x) (ROTR(x, 19) ^ ROTR(x, 61) ^ SHR(x, 6))

#  define S2(x) (ROTR(x, 28) ^ ROTR(x, 34) ^ ROTR(x, 39))
#  define S3(x) (ROTR(x, 14) ^ ROTR(x, 18) ^ ROTR(x, 41))

#  define F0(x, y, z) (((x) & (y)) | ((z) & ((x) | (y))))
#  define F1(x, y, z) ((z) ^ ((x) & ((y) ^ (z))))

#  define P(a, b, c, d, e, f, g, h, x, K)                  \
    do {                                                   \
      temp1 = (h) + S3(e) + F1((e), (f), (g)) + (K) + (x); \
      temp2 = S2(a) + F0((a), (b), (c));                   \
      (d) += temp1;                                        \
      (h) = temp1 + temp2;                                 \
    } while (0)

  for (i = 0; i < 16; i++) {
    GET_UINT64_BE(W[i], data, i << 3);
  }

  for (; i < 80; i++) {
    W[i] = S1(W[i - 2]) + W[i - 7] + S0(W[i - 15]) + W[i - 16];
  }

  A = ctx.state[0];
  B = ctx.state[1];
  C = ctx.state[2];
  D = ctx.state[3];
  E = ctx.state[4];
  F = ctx.state[5];
  G = ctx.state[6];
  H = ctx.state[7];
  i = 0;

  do {
    P(A, B, C, D, E, F, G, H, W[i], K[i]);
    i++;
    P(H, A, B, C, D, E, F, G, W[i], K[i]);
    i++;
    P(G, H, A, B, C, D, E, F, W[i], K[i]);
    i++;
    P(F, G, H, A, B, C, D, E, W[i], K[i]);
    i++;
    P(E, F, G, H, A, B, C, D, W[i], K[i]);
    i++;
    P(D, E, F, G, H, A, B, C, W[i], K[i]);
    i++;
    P(C, D, E, F, G, H, A, B, W[i], K[i]);
    i++;
    P(B, C, D, E, F, G, H, A, W[i], K[i]);
    i++;
  } while (i < 80);

  ctx.state[0] += A;
  ctx.state[1] += B;
  ctx.state[2] += C;
  ctx.state[3] += D;
  ctx.state[4] += E;
  ctx.state[5] += F;
  ctx.state[6] += G;
  ctx.state[7] += H;

#  undef P
#  undef F1
#  undef F0
#  undef S3
#  undef S2
#  undef S1
#  undef S0
#  undef ROTR
#  undef SHR
}

/*
 * SHA-512 process buffer
 */
static void inner_sha512_update(sha512_context_t &ctx, const unsigned char *input, size_t ilen) {
  size_t fill;
  unsigned int left;

  if (ilen == 0 || input == nullptr) {
    return;
  }

  left = (unsigned int)(ctx.total[0] & 0x7F);
  fill = 128 - left;

  ctx.total[0] += (uint64_t)ilen;

  if (ctx.total[0] < (uint64_t)ilen) ctx.total[1]++;

  if (left && ilen >= fill) {
    memcpy((void *)(ctx.buffer + left), input, fill);

    inner_internal_sha512_process(ctx, ctx.buffer);

    input += fill;
    ilen -= fill;
    left = 0;
  }

  while (ilen >= 128) {
    inner_internal_sha512_process(ctx, input);

    input += 128;
    ilen -= 128;
  }

  if (ilen > 0) {
    memcpy((void *)(ctx.buffer + left), input, ilen);
  }
}

/*
 * SHA-512 final digest
 */
static void inner_sha512_finish(sha512_context_t &ctx, unsigned char output[64]) {
  unsigned used;
  uint64_t high, low;

  /*
   * Add padding: 0x80 then 0x00 until 16 bytes remain for the length
   */
  used = ctx.total[0] & 0x7F;

  ctx.buffer[used++] = 0x80;

  if (used <= 112) {
    /* Enough room for padding + length in current block */
    memset(ctx.buffer + used, 0, 112 - used);
  } else {
    /* We'll need an extra block */
    memset(ctx.buffer + used, 0, 128 - used);

    inner_internal_sha512_process(ctx, ctx.buffer);

    memset(ctx.buffer, 0, 112);
  }

  /*
   * Add message length
   */
  high = (ctx.total[0] >> 61) | (ctx.total[1] << 3);
  low = (ctx.total[0] << 3);

  PUT_UINT64_BE(high, ctx.buffer, 112);
  PUT_UINT64_BE(low, ctx.buffer, 120);

  inner_internal_sha512_process(ctx, ctx.buffer);

  /*
   * Output final state
   */
  PUT_UINT64_BE(ctx.state[0], output, 0);
  PUT_UINT64_BE(ctx.state[1], output, 8);
  PUT_UINT64_BE(ctx.state[2], output, 16);
  PUT_UINT64_BE(ctx.state[3], output, 24);
  PUT_UINT64_BE(ctx.state[4], output, 32);
  PUT_UINT64_BE(ctx.state[5], output, 40);

  if (ctx.is384 == 0) {
    PUT_UINT64_BE(ctx.state[6], output, 48);
    PUT_UINT64_BE(ctx.state[7], output, 56);
  }
}
#endif
}  // namespace detail

LIBATFRAME_UTILS_API sha::sha() : hash_type_(EN_ALGORITHM_UNINITED), private_raw_data_(nullptr) {}
LIBATFRAME_UTILS_API sha::~sha() { close(); }

LIBATFRAME_UTILS_API sha::sha(sha &&other) : hash_type_(EN_ALGORITHM_UNINITED), private_raw_data_(nullptr) {
  swap(other);
}

LIBATFRAME_UTILS_API sha &sha::operator=(sha &&other) {
  swap(other);
  return *this;
}

LIBATFRAME_UTILS_API bool sha::init(type t) {
  close();
  private_raw_data_ = detail::malloc_inner_type(t);
  if (private_raw_data_ == nullptr) {
    return false;
  }

  hash_type_ = t;
  return true;
}

LIBATFRAME_UTILS_API void sha::close() {
  if (hash_type_ == EN_ALGORITHM_UNINITED || private_raw_data_ == nullptr) {
    return;
  }

  detail::free_inner_type(private_raw_data_, hash_type_);

  hash_type_ = EN_ALGORITHM_UNINITED;
  private_raw_data_ = nullptr;
  return;
}

LIBATFRAME_UTILS_API void sha::swap(sha &other) {
  using std::swap;
  swap(hash_type_, other.hash_type_);
  swap(private_raw_data_, other.private_raw_data_);
}

LIBATFRAME_UTILS_API bool sha::update(const unsigned char *in, size_t inlen) {
  detail::sha_inner_data *inner_obj = detail::into_inner_type(private_raw_data_);
  if (inner_obj == nullptr) {
    return false;
  }
#if defined(UTIL_HASH_IMPLEMENT_SHA_USING_OPENSSL) && UTIL_HASH_IMPLEMENT_SHA_USING_OPENSSL
  return 1 == EVP_DigestUpdate(inner_obj->ctx, reinterpret_cast<const void *>(in), inlen);
#elif defined(UTIL_HASH_IMPLEMENT_SHA_USING_MBEDTLS) && UTIL_HASH_IMPLEMENT_SHA_USING_MBEDTLS
  switch (hash_type_) {
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA1:
      return 0 == mbedtls_sha1_update_ret(&inner_obj->sha1_context, in, inlen);
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA224:
      return 0 == mbedtls_sha256_update_ret(&inner_obj->sha224_context, in, inlen);
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA256:
      return 0 == mbedtls_sha256_update_ret(&inner_obj->sha256_context, in, inlen);
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA384:
      return 0 == mbedtls_sha512_update_ret(&inner_obj->sha384_context, in, inlen);
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA512:
      return 0 == mbedtls_sha512_update_ret(&inner_obj->sha512_context, in, inlen);
    default:
      break;
  }
  return false;
#else
  switch (hash_type_) {
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA1:
      detail::inner_sha1_update(inner_obj->sha1_context, in, inlen);
      return true;
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA224:
      detail::inner_sha256_update(inner_obj->sha224_context, in, inlen);
      return true;
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA256:
      detail::inner_sha256_update(inner_obj->sha256_context, in, inlen);
      return true;
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA384:
      detail::inner_sha512_update(inner_obj->sha384_context, in, inlen);
      return true;
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA512:
      detail::inner_sha512_update(inner_obj->sha512_context, in, inlen);
      return true;
    default:
      break;
  }
  return false;
#endif
}

LIBATFRAME_UTILS_API bool sha::final() {
  detail::sha_inner_data *inner_obj = detail::into_inner_type(private_raw_data_);
  if (inner_obj == nullptr) {
    return false;
  }
#if defined(UTIL_HASH_IMPLEMENT_SHA_USING_OPENSSL) && UTIL_HASH_IMPLEMENT_SHA_USING_OPENSSL
  unsigned int md_len = 0;
  return 1 == EVP_DigestFinal_ex(inner_obj->ctx, inner_obj->output, &md_len);
#elif defined(UTIL_HASH_IMPLEMENT_SHA_USING_MBEDTLS) && UTIL_HASH_IMPLEMENT_SHA_USING_MBEDTLS
  switch (hash_type_) {
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA1:
      return 0 == mbedtls_sha1_finish_ret(&inner_obj->sha1_context, inner_obj->output);
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA224:
      return 0 == mbedtls_sha256_finish_ret(&inner_obj->sha224_context, inner_obj->output);
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA256:
      return 0 == mbedtls_sha256_finish_ret(&inner_obj->sha256_context, inner_obj->output);
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA384:
      return 0 == mbedtls_sha512_finish_ret(&inner_obj->sha384_context, inner_obj->output);
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA512:
      return 0 == mbedtls_sha512_finish_ret(&inner_obj->sha512_context, inner_obj->output);
    default:
      break;
  }
  return false;
#else
  switch (hash_type_) {
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA1:
      detail::inner_sha1_finish(inner_obj->sha1_context, inner_obj->output);
      return true;
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA224:
      detail::inner_sha256_finish(inner_obj->sha224_context, inner_obj->output);
      return true;
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA256:
      detail::inner_sha256_finish(inner_obj->sha256_context, inner_obj->output);
      return true;
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA384:
      detail::inner_sha512_finish(inner_obj->sha384_context, inner_obj->output);
      return true;
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA512:
      detail::inner_sha512_finish(inner_obj->sha512_context, inner_obj->output);
      return true;
    default:
      break;
  }
  return false;
#endif
}

LIBATFRAME_UTILS_API size_t sha::get_output_length() const { return get_output_length(hash_type_); }

LIBATFRAME_UTILS_API size_t sha::get_output_length(type bt) {
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
  const EVP_MD *md = nullptr;
  switch (bt) {
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA1:
      md = EVP_get_digestbynid(NID_sha1);
      break;
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA224:
      md = EVP_get_digestbynid(NID_sha224);
      break;
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA256:
      md = EVP_get_digestbynid(NID_sha256);
      break;
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA384:
      md = EVP_get_digestbynid(NID_sha384);
      break;
    case LIBATFRAME_UTILS_NAMESPACE_ID::hash::sha::EN_ALGORITHM_SHA512:
      md = EVP_get_digestbynid(NID_sha512);
      break;
    default:
      break;
  }

  if (md == nullptr) {
    return 0;
  }

  int ret = EVP_MD_size(md);
  if (ret <= 0) {
    return 0;
  }
  return static_cast<size_t>(ret);
#else
  switch (bt) {
    case EN_ALGORITHM_SHA1:
      return 20;
    case EN_ALGORITHM_SHA224:
      return 28;
    case EN_ALGORITHM_SHA256:
      return 32;
    case EN_ALGORITHM_SHA384:
      return 48;
    case EN_ALGORITHM_SHA512:
      return 64;
    default:
      return 0;
  }
#endif
}

LIBATFRAME_UTILS_API const unsigned char *sha::get_output() const {
  return detail::get_output_buffer(private_raw_data_);
}

LIBATFRAME_UTILS_API std::string sha::get_output_hex(bool is_uppercase) const {
  std::string ret;
  ret.resize(get_output_length() << 1, 0);
  LIBATFRAME_UTILS_NAMESPACE_ID::string::dumphex(get_output(), get_output_length(), &ret[0], is_uppercase);

  return ret;
}

LIBATFRAME_UTILS_API std::string sha::get_output_base64(LIBATFRAME_UTILS_NAMESPACE_ID::base64_mode_t::type bt) const {
  std::string ret;
  LIBATFRAME_UTILS_NAMESPACE_ID::base64_encode(ret, get_output(), get_output_length(), bt);
  return ret;
}

LIBATFRAME_UTILS_API std::string sha::hash_to_binary(type t, const void *in, size_t inlen) {
  sha obj;
  std::string ret;

  if (false == obj.init(t)) {
    return ret;
  }

  obj.update(reinterpret_cast<const unsigned char *>(in), inlen);
  obj.final();
  ret.assign(reinterpret_cast<const char *>(obj.get_output()), obj.get_output_length());

  return ret;
}

LIBATFRAME_UTILS_API std::string sha::hash_to_hex(type t, const void *in, size_t inlen, bool is_uppercase) {
  sha obj;

  if (false == obj.init(t)) {
    return std::string();
  }

  obj.update(reinterpret_cast<const unsigned char *>(in), inlen);
  obj.final();

  return obj.get_output_hex(is_uppercase);
}

LIBATFRAME_UTILS_API std::string sha::hash_to_base64(type t, const void *in, size_t inlen,
                                                     LIBATFRAME_UTILS_NAMESPACE_ID::base64_mode_t::type bt) {
  sha obj;

  if (false == obj.init(t)) {
    return std::string();
  }

  obj.update(reinterpret_cast<const unsigned char *>(in), inlen);
  obj.final();

  return obj.get_output_base64(bt);
}
}  // namespace hash
LIBATFRAME_UTILS_NAMESPACE_END
