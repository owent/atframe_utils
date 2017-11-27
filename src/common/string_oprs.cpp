#include <common/string_oprs.h>


namespace util {
    namespace string {
        int version_compare(const char *l, const char *r) {
            typedef int64_t version_int_t[16];
            version_int_t lver = {0};
            version_int_t rver = {0};
            size_t llen = 0;
            size_t rlen = 0;

            size_t *opr_len[] = {&llen, &rlen};
            version_int_t *opr_ints[] = {&lver, &rver};
            const char *opr_str[] = {l, r};

            for (int i = 0; i < 2; ++i) {
                size_t *len = opr_len[i];
                version_int_t *vers = opr_ints[i];
                const char *ver_str = opr_str[i];
                *len = 0;

                while (*len < 16 && ver_str && *ver_str) {
                    while (*ver_str && (*ver_str == ' ' || *ver_str == '\t' || *ver_str == '\r' || *ver_str == '\n')) {
                        ++ver_str;
                    }

                    if (*ver_str) {
                        (*vers)[*len] = to_int<int64_t>(ver_str);
                        ++(*len);
                    }

                    // skip digital
                    while (*ver_str && *ver_str != '.') {
                        ++ver_str;
                    }

                    // skip dot
                    if (*ver_str) {
                        ++ver_str;
                    }
                }
            }

            size_t max_len = llen > rlen ? llen : rlen;

            for (size_t i = 0; i < max_len; ++i) {
                if (lver[i] != rver[i]) {
                    return lver[i] < rver[i] ? -1 : 1;
                }
            }

            return 0;
        }
    } // namespace string
} // namespace util