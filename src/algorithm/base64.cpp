#include <assert.h>
#include <inttypes.h>
#include <limits>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


#include "algorithm/base64.h"

#define BASE64_SIZE_T_MAX ((size_t)-1)   /* SIZE_T_MAX is not standard */
#define BASE64_INVALID_CHARACTER -0x002C /**< Invalid character in input. */


namespace util {

    namespace detail {
        typedef const unsigned char base_enc_map_t[64];
        typedef const unsigned char base_dec_map_t[128];
        static base_enc_map_t base64_enc_map_basic = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                                      'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                                      'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                                      'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

        static base_dec_map_t base64_dec_map_basic = {
            127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
            127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 62,
            127, 127, 127, 63,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  127, 127, 127, 127, 127, 127, 127, 0,
            1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,
            23,  24,  25,  127, 127, 127, 127, 127, 127, 26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,
            39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  127, 127, 127, 127, 127};

        static base_enc_map_t base64_enc_map_imap = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                                     'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                                     'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                                     'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', ','};

        static base_dec_map_t base64_dec_map_imap = {
            127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
            127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 62,
            63,  127, 127, 127, 52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  127, 127, 127, 127, 127, 127, 127, 0,
            1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,
            23,  24,  25,  127, 127, 127, 127, 127, 127, 26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,
            39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  127, 127, 127, 127, 127};

        static const unsigned char base64_enc_map_url[64] = {
            'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
            'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
            's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-', '_'};

        static const unsigned char base64_dec_map_url[128] = {
            127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
            127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
            127, 62,  127, 127, 52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  127, 127, 127, 127, 127, 127, 127, 0,
            1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,
            23,  24,  25,  127, 127, 127, 127, 63,  127, 26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,
            39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  127, 127, 127, 127, 127};


        static int base64_encode_inner(unsigned char *dst, size_t dlen, size_t *olen, const unsigned char *src, size_t slen,
                                       base_enc_map_t &base64_enc_map, unsigned char padding_char) {
            size_t i, n, nopadding;
            int C1, C2, C3;
            unsigned char *p;

            if (slen == 0) {
                *olen = 0;
                return (0);
            }

            n = (slen + 2) / 3;

            if (n > (BASE64_SIZE_T_MAX - 1) / 4) {
                *olen = BASE64_SIZE_T_MAX;
                return -1;
            }

            n *= 4;

            // no padding
            if (0 == padding_char) {
                nopadding = slen % 3;
                if (0 != nopadding) {
                    n -= 3 - nopadding;
                }
            }

            if ((dlen < n + 1) || (NULL == dst)) {
                *olen = n + 1;
                return -1;
            }

            n = (slen / 3) * 3;

            for (i = 0, p = dst; i < n; i += 3) {
                C1 = *src++;
                C2 = *src++;
                C3 = *src++;

                *p++ = base64_enc_map[(C1 >> 2) & 0x3F];
                *p++ = base64_enc_map[(((C1 & 3) << 4) + (C2 >> 4)) & 0x3F];
                *p++ = base64_enc_map[(((C2 & 15) << 2) + (C3 >> 6)) & 0x3F];
                *p++ = base64_enc_map[C3 & 0x3F];
            }

            if (i < slen) {
                C1 = *src++;
                C2 = ((i + 1) < slen) ? *src++ : 0;

                *p++ = base64_enc_map[(C1 >> 2) & 0x3F];
                *p++ = base64_enc_map[(((C1 & 3) << 4) + (C2 >> 4)) & 0x3F];

                if ((i + 1) < slen) {
                    *p++ = base64_enc_map[((C2 & 15) << 2) & 0x3F];
                } else if (padding_char) {
                    *p++ = padding_char;
                }

                if (padding_char) {
                    *p++ = padding_char;
                }
            }

            *olen = p - dst;
            *p = 0;

            return (0);
        }

        static inline int base64_encode_inner(std::string &dst, const unsigned char *src, size_t slen, base_enc_map_t &base64_enc_map,
                                              unsigned char padding_char) {
            size_t olen = 0;
            base64_encode_inner(NULL, 0, &olen, src, slen, base64_enc_map, padding_char);
            dst.resize(olen);

            int ret =
                base64_encode_inner(reinterpret_cast<unsigned char *>(&dst[0]), dst.size(), &olen, src, slen, base64_enc_map, padding_char);
            assert(0 != ret || dst.size() == olen + 1);
            // pop back last zero
            if (!dst.empty() && *dst.rbegin() == 0) {
                dst.resize(dst.size() - 1);
            }
            return ret;
        }

        static inline int base64_encode_inner(std::string &dst, const std::string &in, base_enc_map_t &base64_enc_map,
                                              unsigned char padding_char) {
            return base64_encode_inner(dst, reinterpret_cast<const unsigned char *>(in.c_str()), in.size(), base64_enc_map, padding_char);
        }

        static int base64_decode_inner(unsigned char *dst, size_t dlen, size_t *olen, const unsigned char *src, size_t slen,
                                       base_dec_map_t &base64_dec_map, unsigned char padding_char) {
            size_t i, n;
            uint32_t j, x;
            unsigned char *p;

            /* First pass: check for validity and get output length */
            for (i = n = j = 0; i < slen; i++) {
                /* Skip spaces before checking for EOL */
                x = 0;
                while (i < slen && (src[i] == ' ' || src[i] == '\t')) {
                    ++i;
                    ++x;
                }

                /* Spaces at end of buffer are OK */
                if (i == slen) break;

                if ((slen - i) >= 2 && src[i] == '\r' && src[i + 1] == '\n') continue;

                if (src[i] == '\n') continue;

                /* Space inside a line is an error */
                if (x != 0) return -2;

                if (src[i] == padding_char) {
                    if (++j > 2) {
                        return -2;
                    }
                } else {
                    if (src[i] > 127 || base64_dec_map[src[i]] == 127) return -2;
                }

                if (base64_dec_map[src[i]] < 64 && j != 0) return -2;

                n++;
            }

            if (n == 0) {
                *olen = 0;
                return (0);
            }

            // no padding, add j to padding length
            if (slen & 3) {
                j += 4 - (slen & 3);
                n += 4 - (slen & 3);
            }

            /* The following expression is to calculate the following formula without
             * risk of integer overflow in n:
             *     n = ( ( n * 6 ) + 7 ) >> 3;
             */
            n = (6 * (n >> 3)) + ((6 * (n & 0x7) + 7) >> 3);
            n -= j;

            if (dst == NULL || dlen < n) {
                *olen = n;
                return -1;
            }

            for (j = 3, n = x = 0, p = dst; i > 0; i--, src++) {
                if (*src == '\r' || *src == '\n' || *src == ' ' || *src == '\t') continue;

                j -= (*src == padding_char ? 1 : 0);
                x = (x << 6) | (base64_dec_map[*src] & 0x3F);

                if (++n == 4) {
                    n = 0;
                    if (j > 0) *p++ = (unsigned char)(x >> 16);
                    if (j > 1) *p++ = (unsigned char)(x >> 8);
                    if (j > 2) *p++ = (unsigned char)(x);
                }
            }

            // no padding, the tail code
            if (n == 2) {
                *p++ = (unsigned char)(x >> 4);
            } else if (n == 3) {
                *p++ = (unsigned char)(x >> 10);
                *p++ = (unsigned char)(x >> 2);
            }

            *olen = p - dst;

            return (0);
        }

        static inline int base64_decode_inner(std::string &dst, const unsigned char *src, size_t slen, base_dec_map_t &base64_dec_map,
                                              unsigned char padding_char) {
            size_t olen = 0;

            if (-2 == base64_decode_inner(NULL, 0, &olen, src, slen, base64_dec_map, padding_char)) {
                return -2;
            }

            dst.resize(olen);
            int ret =
                base64_decode_inner(reinterpret_cast<unsigned char *>(&dst[0]), dst.size(), &olen, src, slen, base64_dec_map, padding_char);
            assert(0 != ret || olen == dst.size());
            return ret;
        }

        static inline int base64_decode_inner(std::string &dst, const std::string &in, base_dec_map_t &base64_dec_map,
                                              unsigned char padding_char) {
            return base64_decode_inner(dst, reinterpret_cast<const unsigned char *>(in.c_str()), in.size(), base64_dec_map, padding_char);
        }

        static inline base_dec_map_t &base64_get_dec_map(base64_mode_t::type mode) {
            switch (mode) {
            case base64_mode_t::EN_BMT_STANDARD:
            case base64_mode_t::EN_BMT_UTF7:
                return base64_dec_map_basic;

            case base64_mode_t::EN_BMT_IMAP_MAILBOX_NAME:
                return base64_dec_map_imap;

            case base64_mode_t::EN_BMT_URL_FILENAME_SAFE:
                return base64_dec_map_url;

            default:
                return base64_dec_map_basic;
            }
        }

        static inline base_enc_map_t &base64_get_enc_map(base64_mode_t::type mode) {
            switch (mode) {
            case base64_mode_t::EN_BMT_STANDARD:
            case base64_mode_t::EN_BMT_UTF7:
                return base64_enc_map_basic;

            case base64_mode_t::EN_BMT_IMAP_MAILBOX_NAME:
                return base64_enc_map_imap;

            case base64_mode_t::EN_BMT_URL_FILENAME_SAFE:
                return base64_enc_map_url;

            default:
                return base64_enc_map_basic;
            }
        }

        static inline unsigned char base64_get_padding_char(base64_mode_t::type mode) {
            switch (mode) {
            case base64_mode_t::EN_BMT_STANDARD:
            case base64_mode_t::EN_BMT_URL_FILENAME_SAFE:
                return '=';

            case base64_mode_t::EN_BMT_UTF7:
            case base64_mode_t::EN_BMT_IMAP_MAILBOX_NAME:
                return 0;

            default:
                return '=';
            }
        }
    } // namespace detail

    int base64_encode(unsigned char *dst, size_t dlen, size_t *olen, const unsigned char *src, size_t slen, base64_mode_t::type mode) {
        return detail::base64_encode_inner(dst, dlen, olen, src, slen, detail::base64_get_enc_map(mode),
                                           detail::base64_get_padding_char(mode));
    }

    int base64_encode(std::string &dst, const unsigned char *src, size_t slen, base64_mode_t::type mode) {
        return detail::base64_encode_inner(dst, src, slen, detail::base64_get_enc_map(mode), detail::base64_get_padding_char(mode));
    }

    int base64_encode(std::string &dst, const std::string &in, base64_mode_t::type mode) {
        return detail::base64_encode_inner(dst, in, detail::base64_get_enc_map(mode), detail::base64_get_padding_char(mode));
    }

    int base64_decode(unsigned char *dst, size_t dlen, size_t *olen, const unsigned char *src, size_t slen, base64_mode_t::type mode) {
        return detail::base64_decode_inner(dst, dlen, olen, src, slen, detail::base64_get_dec_map(mode),
                                           detail::base64_get_padding_char(mode));
    }

    int base64_decode(std::string &dst, const unsigned char *src, size_t slen, base64_mode_t::type mode) {
        return detail::base64_decode_inner(dst, src, slen, detail::base64_get_dec_map(mode), detail::base64_get_padding_char(mode));
    }

    int base64_decode(std::string &dst, const std::string &in, base64_mode_t::type mode) {
        return detail::base64_decode_inner(dst, in, detail::base64_get_dec_map(mode), detail::base64_get_padding_char(mode));
    }
} // namespace util
