#include <common/string_oprs.h>


namespace util {
    namespace string {
        const char *version_tok(const char *v, int64_t &out) {
            if (NULL == v) {
                out = 0;
                return v;
            }

            while (*v && (*v == ' ' || *v == '\t' || *v == '\r' || *v == '\n')) {
                ++v;
            }

            if (*v) {
                out = to_int<int64_t>(v);
            } else {
                out = 0;
            }

            // skip digital
            while (*v && *v != '.' && *v != '-') {
                ++v;
            }

            // skip dot or minus
            if (*v) {
                ++v;
            }

            return v;
        }

        int version_compare(const char *l, const char *r) {
            while ((l && *l) || (r && *r)) {
                int64_t lver = 0;
                int64_t rver = 0;
                l = version_tok(l, lver);
                r = version_tok(r, rver);
                if (lver != rver) {
                    return lver < rver ? -1 : 1;
                }
            }

            return 0;
        } // namespace string
    }     // namespace string
} // namespace util