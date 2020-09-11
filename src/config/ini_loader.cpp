#include <algorithm>
#include <cstddef>
#include <cstring>
#include <fstream>


#include <config/ini_loader.h>

namespace util {
    namespace config {
        // ================= 词法状态机 =================
        namespace analysis {
            // space
            struct spaces {
                static bool test_char(char c);
                bool        test(const char *begin, const char *end);
                const char *parse(const char *begin, const char *end);
            };

            // comment
            struct comment {
                bool        test(const char *begin, const char *end);
                const char *parse(const char *begin, const char *end);
            };

            // identify
            struct identify {
                const char *_begin_ptr;
                const char *_end_ptr;

                bool        test(const char *begin, const char *end);
                const char *parse(const char *begin, const char *end);
            };

            // key
            struct key {
                typedef std::list<std::pair<const char *, const char *> > list_type;
                list_type                                                 _keys;

                bool        test(const char *begin, const char *end);
                const char *parse(const char *begin, const char *end);
            };

            // section
            struct section {
                typedef std::list<std::pair<const char *, const char *> > list_type;
                list_type                                                 _keys;

                bool        test(const char *begin, const char *end);
                const char *parse(const char *begin, const char *end);
            };

            // string
            struct string {
                static char _convert_map[1 << (sizeof(char) * 8)];
                void        init_conver_map();

                std::string _value;

                bool        test(const char *begin, const char *end);
                const char *parse(const char *begin, const char *end, bool enable_convert = false);
            };

            // value
            struct value {
                std::string _value;

                bool        test(const char *begin, const char *end);
                const char *parse(const char *begin, const char *end);
            };

            // expression
            struct expression {
                key   _key;
                value _value;

                bool        test(const char *begin, const char *end);
                const char *parse(const char *begin, const char *end);
            };

            // sentence
            struct sentence {
                std::pair<bool, section>    _sect;
                std::pair<bool, expression> _exp;

                bool        test(const char *begin, const char *end);
                const char *parse(const char *begin, const char *end);
            };
        } // namespace analysis
        // ----------------- 词法状态机 -----------------

        namespace detail {
            typedef const char *const_cstring;
            template <typename T>
            T tolower(T c) {
                if (c >= 'A' && c <= 'Z') {
                    return c - 'A' + 'a';
                }

                return c;
            }
            template <typename T>
            T str2int(const_cstring str, const_cstring *left = NULL) {
                T out = 0;
                if (NULL == str || !(*str)) {
                    if (NULL != left) {
                        *left = str;
                    }

                    return out;
                }

                // negative
                bool is_negative = false;
                while (*str && *str == '-') {
                    is_negative = !is_negative;
                    ++str;
                }

                if (!(*str)) {
                    if (NULL != left) {
                        *left = str;
                    }

                    return out;
                }

                size_t i;
                if ('0' == str[0] && 'x' == str[1]) { // hex
                    for (i = 2; str[i]; ++i) {
                        char c = static_cast<char>(::tolower(str[i]));
                        if (c >= '0' && c <= '9') {
                            out <<= 4;
                            out += c - '0';
                        } else if (c >= 'a' && c <= 'f') {
                            out <<= 4;
                            out += c - 'a' + 10;
                        } else {
                            break;
                        }
                    }
                } else if ('\\' == str[0]) { // oct
                    for (i = 0; str[i] >= '0' && str[i] < '8'; ++i) {
                        out <<= 3;
                        out += str[i] - '0';
                    }
                } else { // dec
                    for (i = 0; str[i] >= '0' && str[i] <= '9'; ++i) {
                        out *= 10;
                        out += str[i] - '0';
                    }
                }

                if (NULL != left) {
                    *left = str + i;
                }

                if (is_negative) {
                    out = (~out) + 1;
                }

                return out;
            }
        } // namespace detail

        LIBATFRAME_UTILS_API ini_value::ini_value() {}
        LIBATFRAME_UTILS_API ini_value::~ini_value() {}
        LIBATFRAME_UTILS_API ini_value::ini_value(const ini_value &other) : std::enable_shared_from_this<ini_value>() { *this = other; }

        LIBATFRAME_UTILS_API ini_value &ini_value::operator=(const ini_value &other) {
            data_           = other.data_;
            children_nodes_ = other.children_nodes_;
            return (*this);
        }

        LIBATFRAME_UTILS_API void ini_value::add(const std::string &val) { data_.push_back(val); }

        LIBATFRAME_UTILS_API void ini_value::add(const char *begin, const char *end) { data_.push_back(std::string(begin, end)); }

        LIBATFRAME_UTILS_API bool ini_value::empty() const { return data_.empty() && children_nodes_.empty(); }

        LIBATFRAME_UTILS_API bool ini_value::has_data() const { return false == data_.empty(); }

        LIBATFRAME_UTILS_API size_t ini_value::size() const { return data_.size(); }

        LIBATFRAME_UTILS_API void ini_value::clear() {
            data_.clear();
            children_nodes_.clear();
        }

        LIBATFRAME_UTILS_API ini_value &ini_value::operator[](const std::string key) {
            ptr_t &ret = children_nodes_[key];
            if (!ret) {
                ret = std::make_shared<ini_value>();
            }
            return *ret;
        }

        LIBATFRAME_UTILS_API ini_value::node_type &ini_value::get_children() { return children_nodes_; }

        LIBATFRAME_UTILS_API const ini_value::node_type &ini_value::get_children() const { return children_nodes_; }

        LIBATFRAME_UTILS_API ini_value::ptr_t ini_value::get_child_by_path(const std::string &path) const {
            const ini_value *ret = this;

            analysis::key _keys;
            const char *  begin = path.c_str();
            const char *  end   = begin + path.size();

            analysis::spaces spliter;
            begin = spliter.parse(begin, end);

            _keys.parse(begin, end);
            analysis::section::list_type::iterator iter = _keys._keys.begin();
            for (; iter != _keys._keys.end(); ++iter) {
                if (iter->first >= iter->second) {
                    continue;
                }

                std::string key;
                key.assign(iter->first, iter->second);

                node_type::const_iterator child_iter = ret->children_nodes_.find(key);
                if (child_iter == ret->children_nodes_.end()) {
                    ret = NULL;
                    break;
                }
                ret = child_iter->second.get();
            }

            if (ret) {
                return const_cast<ini_value *>(ret)->shared_from_this();
            }

            return NULL;
        }

        LIBATFRAME_UTILS_API const std::string &ini_value::get_empty_string() {
            static std::string empty_data;
            return empty_data;
        }

        LIBATFRAME_UTILS_API const std::string &ini_value::as_cpp_string(size_t index) const {
            if (index < data_.size()) {
                return data_[index];
            }

            return get_empty_string();
        }

        LIBATFRAME_UTILS_API char ini_value::as_char(size_t index) const { return detail::str2int<char>(as_cpp_string(index).c_str()); }

        LIBATFRAME_UTILS_API short ini_value::as_short(size_t index) const { return detail::str2int<short>(as_cpp_string(index).c_str()); }

        LIBATFRAME_UTILS_API int ini_value::as_int(size_t index) const { return detail::str2int<int>(as_cpp_string(index).c_str()); }

        LIBATFRAME_UTILS_API long ini_value::as_long(size_t index) const { return detail::str2int<long>(as_cpp_string(index).c_str()); }

        LIBATFRAME_UTILS_API long long ini_value::as_longlong(size_t index) const {
            return detail::str2int<long long>(as_cpp_string(index).c_str());
        }

        LIBATFRAME_UTILS_API double ini_value::as_double(size_t index) const { return as<double>(index); }

        LIBATFRAME_UTILS_API float ini_value::as_float(size_t index) const { return as<float>(index); }

        LIBATFRAME_UTILS_API const char *ini_value::as_string(size_t index) const { return as_cpp_string(index).c_str(); }

        // ============ unsigned ============
        LIBATFRAME_UTILS_API unsigned char ini_value::as_uchar(size_t index) const {
            return detail::str2int<unsigned char>(as_cpp_string(index).c_str());
        }

        LIBATFRAME_UTILS_API unsigned short ini_value::as_ushort(size_t index) const {
            return detail::str2int<unsigned short>(as_cpp_string(index).c_str());
        }

        LIBATFRAME_UTILS_API unsigned int ini_value::as_uint(size_t index) const {
            return detail::str2int<unsigned int>(as_cpp_string(index).c_str());
        }

        LIBATFRAME_UTILS_API unsigned long ini_value::as_ulong(size_t index) const {
            return detail::str2int<unsigned long>(as_cpp_string(index).c_str());
        }

        LIBATFRAME_UTILS_API unsigned long long ini_value::as_ulonglong(size_t index) const {
            return detail::str2int<unsigned long long>(as_cpp_string(index).c_str());
        }

        LIBATFRAME_UTILS_API int8_t ini_value::as_int8(size_t index) const { return detail::str2int<int8_t>(as_cpp_string(index).c_str()); }

        LIBATFRAME_UTILS_API uint8_t ini_value::as_uint8(size_t index) const {
            return detail::str2int<uint8_t>(as_cpp_string(index).c_str());
        }

        LIBATFRAME_UTILS_API int16_t ini_value::as_int16(size_t index) const {
            return detail::str2int<int16_t>(as_cpp_string(index).c_str());
        }

        LIBATFRAME_UTILS_API uint16_t ini_value::as_uint16(size_t index) const {
            return detail::str2int<uint16_t>(as_cpp_string(index).c_str());
        }

        LIBATFRAME_UTILS_API int32_t ini_value::as_int32(size_t index) const {
            return detail::str2int<int32_t>(as_cpp_string(index).c_str());
        }

        LIBATFRAME_UTILS_API uint32_t ini_value::as_uint32(size_t index) const {
            return detail::str2int<uint32_t>(as_cpp_string(index).c_str());
        }

        LIBATFRAME_UTILS_API int64_t ini_value::as_int64(size_t index) const {
            return detail::str2int<int64_t>(as_cpp_string(index).c_str());
        }

        LIBATFRAME_UTILS_API uint64_t ini_value::as_uint64(size_t index) const {
            return detail::str2int<uint64_t>(as_cpp_string(index).c_str());
        }

        LIBATFRAME_UTILS_API duration_value ini_value::as_duration(size_t index) const {
            duration_value ret;
            clear_data(ret);

            if (index >= data_.size()) {
                return ret;
            }

            detail::const_cstring word_begin = NULL;
            time_t                tm_val     = detail::str2int<time_t>(as_cpp_string(index).c_str(), &word_begin);
            while (word_begin && *word_begin) {
                if (analysis::spaces::test_char(*word_begin)) {
                    ++word_begin;
                    continue;
                }
                break;
            }

            detail::const_cstring word_end = word_begin;
            while (word_end && ((*word_end >= 'A' && *word_end <= 'Z') || (*word_end >= 'a' && *word_end <= 'z'))) {
                ++word_end;
            }

            std::string unit;
            if (word_begin && word_end && word_end > word_begin) {
                unit.assign(word_begin, word_end);
                std::transform(unit.begin(), unit.end(), unit.begin(), detail::tolower<char>);
            }

            bool fallback = true;
            do {
                if (unit.empty() || unit == "s" || unit == "sec" || unit == "second" || unit == "seconds") {
                    break;
                }

                if (unit == "ms" || unit == "millisecond" || unit == "milliseconds") {
                    fallback = false;
                    ret.sec  = tm_val / 1000;
                    ret.nsec = (tm_val % 1000) * 1000000;
                    break;
                }

                if (unit == "us" || unit == "microsecond" || unit == "microseconds") {
                    fallback = false;
                    ret.sec  = tm_val / 1000000;
                    ret.nsec = (tm_val % 1000000) * 1000;
                    break;
                }

                if (unit == "ns" || unit == "nanosecond" || unit == "nanoseconds") {
                    fallback = false;
                    ret.sec  = tm_val / 1000000000;
                    ret.nsec = tm_val % 1000000000;
                    break;
                }

                if (unit == "m" || unit == "minute" || unit == "minutes") {
                    fallback = false;
                    ret.sec  = tm_val * 60;
                    break;
                }

                if (unit == "h" || unit == "hour" || unit == "hours") {
                    fallback = false;
                    ret.sec  = tm_val * 3600;
                    break;
                }

                if (unit == "d" || unit == "day" || unit == "days") {
                    fallback = false;
                    ret.sec  = tm_val * 3600 * 24;
                    break;
                }

                if (unit == "w" || unit == "week" || unit == "weeks") {
                    fallback = false;
                    ret.sec  = tm_val * 3600 * 24 * 7;
                    break;
                }

            } while (false);

            // fallback to second
            if (fallback) {
                ret.sec = tm_val;
            }

            return ret;
        }

        namespace analysis {
            // space
            bool spaces::test_char(char c) { return (c == ' ' || c == '\r' || c == '\n' || c == '\t'); }

            bool spaces::test(const char *begin, const char *end) { return begin < end && test_char(*begin); }

            const char *spaces::parse(const char *begin, const char *end) {
                while (begin < end && test_char(*begin)) {
                    ++begin;
                }

                return begin;
            }

            // comment
            bool comment::test(const char *begin, const char *end) { return begin < end && ((*begin) == '#' || (*begin) == ';'); }

            const char *comment::parse(const char *begin, const char *end) {
                if (false == test(begin, end)) {
                    return begin;
                }

                return end;
            }

            // identify
            bool identify::test(const char *begin, const char *end) { return begin < end; }

            const char *identify::parse(const char *begin, const char *end) {
                _begin_ptr = _end_ptr = begin;
                if (false == test(begin, end)) {
                    return begin;
                }

                while (begin < end && (*begin) != ':' && (*begin) != '.' && (*begin) != '=') {
                    _end_ptr = (++begin);
                }

                // trim right
                while (_end_ptr > _begin_ptr && spaces::test_char(*(_end_ptr - 1)))
                    --_end_ptr;

                return begin;
            }

            // key
            bool key::test(const char *begin, const char *end) { return begin < end; }

            const char *key::parse(const char *begin, const char *end) {
                while (begin < end) {
                    if (false == test(begin, end)) {
                        return begin;
                    }

                    identify idt;
                    begin = idt.parse(begin, end);
                    if (idt._begin_ptr >= idt._end_ptr) {
                        return begin;
                    }

                    // 提取key
                    _keys.push_back(std::make_pair(idt._begin_ptr, idt._end_ptr));

                    spaces spliter;
                    begin = spliter.parse(begin, end);

                    if (begin >= end || (*begin) != '.') {
                        return begin;
                    }

                    begin = spliter.parse(begin + 1, end);
                }

                return begin;
            }

            // section
            bool section::test(const char *begin, const char *end) { return begin < end && (*begin) == '['; }

            const char *section::parse(const char *begin, const char *end) {
                if (false == test(begin, end)) {
                    return begin;
                }

                ++begin;
                spaces spliter;
                bool   push_front = true;
                while (begin < end) {
                    // trim left
                    begin             = spliter.parse(begin, end);
                    const char *start = begin;
                    while (begin < end && (*begin) != ':' && (*begin) != '.' && (*begin) != ']') {
                        ++begin;
                    }

                    char stop_char = begin < end ? (*begin) : 0;


                    // trim right
                    while (begin > start && spaces::test_char(*(begin - 1))) {
                        --begin;
                    }

                    if (start < begin) {
                        // 提取key
                        if (push_front) {
                            _keys.push_front(std::make_pair(start, begin));
                        } else {
                            _keys.push_back(std::make_pair(start, begin));
                        }
                    }


                    begin = spliter.parse(begin, end);

                    if (begin >= end) {
                        break;
                    }

                    // 略过结尾的 ] 字符
                    if ((*begin) == ']') {
                        ++begin;
                        break;
                    }

                    if ('.' == stop_char) {
                        push_front = false;
                    } else if (':' == stop_char) {
                        push_front = true;
                    }

                    begin = spliter.parse(begin + 1, end);
                }

                return begin;
            }

            // string
            char string::_convert_map[1 << (sizeof(char) * 8)] = {0};

            void string::init_conver_map() {
                if (_convert_map[(int)'0']) {
                    return;
                }

                _convert_map[(int)'0']  = '\0';
                _convert_map[(int)'a']  = '\a';
                _convert_map[(int)'b']  = '\b';
                _convert_map[(int)'f']  = '\f';
                _convert_map[(int)'r']  = '\r';
                _convert_map[(int)'n']  = '\n';
                _convert_map[(int)'t']  = '\t';
                _convert_map[(int)'v']  = '\v';
                _convert_map[(int)'\\'] = '\\';
                _convert_map[(int)'\''] = '\'';
                _convert_map[(int)'\"'] = '\"';
            }

            bool string::test(const char *begin, const char *end) { return begin < end && ((*begin) == '\'' || (*begin) == '\"'); }

            const char *string::parse(const char *begin, const char *end, bool enable_convert) {
                if (false == test(begin, end)) {
                    return begin;
                }

                init_conver_map();
                char quot = *(begin++);

                // 禁止转义字符串
                if (false == enable_convert) {
                    const char *start = begin;
                    while (begin < end && (*begin) != quot) {
                        ++begin;
                    }
                    _value.assign(start, begin);

                    // 封闭字符串
                    if (begin < end) {
                        ++begin;
                    }
                } else { // 允许转义的逻辑复杂一些
                    while (begin < end && (*begin) != quot) {
                        // 转义字符
                        if ((*begin) == '\\' && begin + 1 < end) {
                            ++begin;
                            _value += _convert_map[(int)*(begin++)];
                            continue;
                        }

                        // 普通字符
                        _value += *(begin++);
                    }

                    // 封闭字符串
                    if (begin < end) {
                        ++begin;
                    }
                }

                return begin;
            }

            // value
            bool value::test(const char *begin, const char *end) { return begin < end; }

            const char *value::parse(const char *begin, const char *end) {
                if (false == test(begin, end)) {
                    return begin;
                }

                // trim left
                spaces spliter;
                begin = spliter.parse(begin, end);

                string  rule;
                comment com_s;
                while (begin < end) {

                    if (rule.test(begin, end)) {
                        rule._value.clear();
                        begin = rule.parse(begin, end, (*begin) == '\"');
                        _value += rule._value;
                        continue;
                    }

                    if (com_s.test(begin, end)) {
                        begin = com_s.parse(begin, end);
                        continue;
                    }

                    _value += *(begin++);
                }

                // trim right
                size_t len = _value.size();
                while (len > 0) {
                    if (false == spaces::test_char(_value[len - 1])) {
                        break;
                    }

                    --len;
                }

                _value = _value.substr(0, len);

                return begin;
            }

            // expression
            bool expression::test(const char *begin, const char *end) { return _key.test(begin, end); }

            const char *expression::parse(const char *begin, const char *end) {
                if (false == test(begin, end)) {
                    return begin;
                }

                spaces spliter;

                begin = _key.parse(begin, end);
                begin = spliter.parse(begin, end);

                if (begin >= end || (*begin) != '=') {
                    return begin;
                }

                begin = spliter.parse(begin + 1, end);

                return _value.parse(begin, end);
            }

            // sentence
            bool sentence::test(const char *begin, const char *end) { return begin < end; }

            const char *sentence::parse(const char *begin, const char *end) {
                _sect.first = false;
                _exp.first  = false;

                if (false == test(begin, end)) {
                    return begin;
                }

                spaces spliter;
                begin = spliter.parse(begin, end);

                // 空语句
                if (begin >= end) {
                    return begin;
                }

                // 纯注释语句
                comment com_s;
                if (com_s.test(begin, end)) {
                    return com_s.parse(begin, end);
                }

                // section语句
                _sect.first = _sect.second.test(begin, end);
                if (_sect.first) {
                    return _sect.second.parse(begin, end);
                }

                // expression语句
                _exp.first = _exp.second.test(begin, end);
                if (_exp.first) {
                    return _exp.second.parse(begin, end);
                }

                return begin;
            }
        } // namespace analysis

        LIBATFRAME_UTILS_API ini_loader::ini_loader() {
            root_node_        = std::make_shared<ini_value>();
            current_node_ptr_ = root_node_.get();
        }

        LIBATFRAME_UTILS_API ini_loader::~ini_loader() {}

        LIBATFRAME_UTILS_API ini_loader::ini_loader(const ini_loader &other) { *this = other; }

        LIBATFRAME_UTILS_API ini_loader &ini_loader::operator=(const ini_loader &other) {
            root_node_        = std::make_shared<ini_value>(*other.root_node_);
            current_node_ptr_ = root_node_.get();
            return *this;
        }

        LIBATFRAME_UTILS_API int ini_loader::load_stream(std::istream &in, bool is_append) {

            if (false == is_append) {
                clear();
            }

            std::string test_bom;
            test_bom.resize(3, 0);
            test_bom[0]                     = static_cast<char>(in.get());
            test_bom[1]                     = static_cast<char>(in.get());
            test_bom[2]                     = static_cast<char>(in.get());
            const unsigned char utf8_bom[3] = {0xef, 0xbb, 0xbf};

            if (0 == memcmp(test_bom.c_str(), utf8_bom, 3)) {
                test_bom.clear();
            }

            std::string line;
            while (std::getline(in, line)) {
                if (!test_bom.empty()) {
                    line = test_bom + line;
                    test_bom.clear();
                }
                analysis::sentence one_sentence;
                one_sentence.parse(line.c_str(), line.c_str() + line.size());

                // section 节点会改变当前配置区域
                if (one_sentence._sect.first) {
                    current_node_ptr_                           = &get_root_node();
                    analysis::section::list_type::iterator iter = one_sentence._sect.second._keys.begin();
                    for (; iter != one_sentence._sect.second._keys.end(); ++iter) {
                        if (iter->first >= iter->second) {
                            continue;
                        }

                        std::string key;
                        key.assign(iter->first, iter->second);
                        current_node_ptr_ = &get_node(key, current_node_ptr_);
                    }
                }

                // expression 节点为配置值
                if (one_sentence._exp.first) {
                    ini_value *                        opr_node = &get_section();
                    analysis::key::list_type::iterator iter     = one_sentence._exp.second._key._keys.begin();
                    for (; iter != one_sentence._exp.second._key._keys.end(); ++iter) {
                        if (iter->first >= iter->second) {
                            continue;
                        }

                        std::string key;
                        key.assign(iter->first, iter->second);
                        opr_node = &get_node(key, opr_node);
                    }

                    if (!one_sentence._exp.second._value._value.empty()) {
                        opr_node->add(one_sentence._exp.second._value._value);
                    }
                }
            }

            return EIEC_SUCCESS;
        }

        LIBATFRAME_UTILS_API int ini_loader::load_file(const char *file_path, bool is_append) {
            if (NULL == file_path) {
                return EIEC_OPENFILE;
            }

            std::ifstream file_to_load;
            file_to_load.open(file_path, std::ios::in);
            if (false == file_to_load.is_open()) {
                return EIEC_OPENFILE;
            }

            if (false == is_append) {
                clear();
            }

            return load_stream(file_to_load, is_append);
        }

        LIBATFRAME_UTILS_API int ini_loader::load_file(const std::string &file_path, bool is_append) {
            return load_file(file_path.c_str(), is_append);
        }

        LIBATFRAME_UTILS_API void ini_loader::clear() {
            current_node_ptr_ = root_node_.get();
            root_node_->clear();
        }

        LIBATFRAME_UTILS_API void ini_loader::set_section(const std::string &path) {
            current_node_ptr_ = &get_node(path, &get_root_node());
        }

        LIBATFRAME_UTILS_API ini_value &ini_loader::get_section() { return *current_node_ptr_; }

        LIBATFRAME_UTILS_API const ini_value &ini_loader::get_section() const { return *current_node_ptr_; }

        LIBATFRAME_UTILS_API ini_value &ini_loader::get_root_node() { return *root_node_; }

        LIBATFRAME_UTILS_API const ini_value &ini_loader::get_root_node() const { return *root_node_; }

        LIBATFRAME_UTILS_API ini_value &ini_loader::get_node(const std::string &path, ini_value *father_ptr) {
            if (NULL == father_ptr) {
                father_ptr = root_node_.get();
            }

            analysis::key _keys;
            const char *  begin = path.c_str();
            const char *  end   = begin + path.size();

            analysis::spaces spliter;
            begin = spliter.parse(begin, end);

            _keys.parse(begin, end);
            analysis::section::list_type::iterator iter = _keys._keys.begin();
            for (; iter != _keys._keys.end(); ++iter) {
                if (iter->first >= iter->second) {
                    continue;
                }

                std::string key;
                key.assign(iter->first, iter->second);
                father_ptr = &get_child_node(key, father_ptr);
            }

            return *father_ptr;
        }

        LIBATFRAME_UTILS_API ini_value &ini_loader::get_child_node(const std::string &path, ini_value *father_ptr) {
            if (NULL == father_ptr) {
                father_ptr = root_node_.get();
            }

            return (*father_ptr)[path];
        }

        LIBATFRAME_UTILS_API void ini_loader::dump_to(const std::string &path, bool &val, bool is_force, size_t index) {
            ini_value &cur_node = get_node(path);

            if (false == cur_node.has_data() && false == is_force) {
                return;
            }

            // no data
            if (false == cur_node.has_data() && is_force) {
                val = false;
                return;
            }

            std::string trans = cur_node.as_cpp_string(index);
            std::transform(trans.begin(), trans.end(), trans.begin(), detail::tolower<char>);
            val = true;

            if ("0" == trans || "false" == trans || "no" == trans || "disable" == trans || "disabled" == trans || "" == trans) {
                val = false;
            }
        }

        LIBATFRAME_UTILS_API void ini_loader::dump_to(const std::string &path, std::string &val, bool is_force, size_t index) {
            ini_value &cur_node = get_node(path);
            if (cur_node.has_data() || is_force) {
                val = cur_node.as_cpp_string(index);
            }
        }

        LIBATFRAME_UTILS_API void ini_loader::dump_to(const std::string &path, char *begin, char *end, bool is_force, size_t index) {
            ini_value &cur_node = get_node(path);
            if (cur_node.has_data() || is_force) {
                const std::string &val = cur_node.as_cpp_string(index);
                memcpy(begin, val.c_str(), std::min<size_t>(end - begin, val.size()));
                if (static_cast<size_t>(end - begin) > val.size()) {
                    memset(begin + val.size(), 0, end - begin - val.size());
                }
            }
        }
    } // namespace config
} // namespace util
