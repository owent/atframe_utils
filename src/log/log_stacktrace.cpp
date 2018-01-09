#include "config/atframe_utils_build_feature.h"

#include "common/string_oprs.h"

#include "log/log_stacktrace.h"

#ifndef LOG_STACKTRACE_MAX_STACKS
#define LOG_STACKTRACE_MAX_STACKS 100
#endif
#define LOG_STACKTRACE_MAX_STACKS_ARRAY_SIZE (LOG_STACKTRACE_MAX_STACKS + 1)

#if defined(LOG_STACKTRACE_USING_LIBUNWIND) && LOG_STACKTRACE_USING_LIBUNWIND
#include <libunwind.h>

#elif defined(LOG_STACKTRACE_USING_EXECINFO) && LOG_STACKTRACE_USING_EXECINFO
#include <string>

#include <execinfo.h>

#elif defined(LOG_STACKTRACE_USING_UNWIND) && LOG_STACKTRACE_USING_UNWIND
#include <unwind.h>

#elif defined(LOG_STACKTRACE_USING_DBGHELP) && LOG_STACKTRACE_USING_DBGHELP
#include <Windows.h>

#include <DbgHelp.h>

#ifdef _MSC_VER
#include <atlconv.h>
#pragma comment(lib, "dbghelp.lib")

#ifdef UNICODE
#define LOG_STACKTRACE_VC_A2W(x) A2W(x)
#define LOG_STACKTRACE_VC_W2A(x) W2A(x)
#else
#define LOG_STACKTRACE_VC_A2W(x) x
#define LOG_STACKTRACE_VC_W2A(x) x
#endif


struct SymInitializeHelper {
    SymInitializeHelper() {
        process = GetCurrentProcess();

        // SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
        SymInitialize(process, NULL, TRUE);
    }

    ~SymInitializeHelper() { SymCleanup(process); }

    static const SymInitializeHelper &Inst() {
        static SymInitializeHelper ret;
        return ret;
    }

    HANDLE process;
};

#endif

#elif defined(LOG_STACKTRACE_USING_DBGENG) && LOG_STACKTRACE_USING_DBGENG
#include <Windows.h>

#include <DbgEng.h>

#ifdef _MSC_VER
#include <atlconv.h>
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "dbgeng.lib")

#ifdef UNICODE
#define LOG_STACKTRACE_VC_A2W(x) A2W(x)
#define LOG_STACKTRACE_VC_W2A(x) W2A(x)
#else
#define LOG_STACKTRACE_VC_A2W(x) x
#define LOG_STACKTRACE_VC_W2A(x) x
#endif

#endif

#ifdef __CRT_UUID_DECL // for __MINGW__
__CRT_UUID_DECL(IDebugClient, 0x27fe5639, 0x8407, 0x4f47, 0x83, 0x64, 0xee, 0x11, 0x8f, 0xb0, 0x8a, 0xc8)
__CRT_UUID_DECL(IDebugControl, 0x5182e668, 0x105e, 0x416e, 0xad, 0x92, 0x24, 0xef, 0x80, 0x04, 0x24, 0xba)
__CRT_UUID_DECL(IDebugSymbols, 0x8c31e98c, 0x983a, 0x48a5, 0x90, 0x16, 0x6f, 0xe5, 0xd6, 0x67, 0xa9, 0x50)
#elif defined(DEFINE_GUID) && !defined(BOOST_MSVC)
DEFINE_GUID(IID_IDebugClient, 0x27fe5639, 0x8407, 0x4f47, 0x83, 0x64, 0xee, 0x11, 0x8f, 0xb0, 0x8a, 0xc8);
DEFINE_GUID(IID_IDebugControl, 0x5182e668, 0x105e, 0x416e, 0xad, 0x92, 0x24, 0xef, 0x80, 0x04, 0x24, 0xba);
DEFINE_GUID(IID_IDebugSymbols, 0x8c31e98c, 0x983a, 0x48a5, 0x90, 0x16, 0x6f, 0xe5, 0xd6, 0x67, 0xa9, 0x50);
#endif

template <class T>
class log_stacktrace_com_holder {
private:
    T *holder_;

private:
    log_stacktrace_com_holder(const log_stacktrace_com_holder &) UTIL_CONFIG_DELETED_FUNCTION;
    log_stacktrace_com_holder &operator=(const log_stacktrace_com_holder &) UTIL_CONFIG_DELETED_FUNCTION;

public:
    log_stacktrace_com_holder() UTIL_CONFIG_NOEXCEPT : holder_(NULL) {}
    ~log_stacktrace_com_holder() UTIL_CONFIG_NOEXCEPT {
        if (holder_) {
            holder_->Release();
        }
    }

    T *operator->() const UTIL_CONFIG_NOEXCEPT { return holder_; }

    PVOID *to_pvoid_ptr() UTIL_CONFIG_NOEXCEPT { return reinterpret_cast<PVOID *>(&holder_); }

    bool is_inited() const UTIL_CONFIG_NOEXCEPT { return !!holder_; }
};

#endif

// for demangle
#ifdef __GLIBCXX__
#include <cxxabi.h>
#define USING_LIBSTDCXX_ABI 1
#elif defined(_LIBCPP_ABI_VERSION)
#include <cxxabi.h>
#define USING_LIBCXX_ABI 1
#endif

namespace util {
    namespace log {
        bool is_stacktrace_enabled() UTIL_CONFIG_NOEXCEPT {
#if defined(LOG_STACKTRACE_USING_LIBUNWIND) && LOG_STACKTRACE_USING_LIBUNWIND
            return true;
#elif defined(LOG_STACKTRACE_USING_EXECINFO) && LOG_STACKTRACE_USING_EXECINFO
            return true;
#elif defined(LOG_STACKTRACE_USING_UNWIND) && LOG_STACKTRACE_USING_UNWIND
            return true;
#elif defined(LOG_STACKTRACE_USING_DBGHELP) && LOG_STACKTRACE_USING_DBGHELP
            return true;
#elif defined(LOG_STACKTRACE_USING_DBGENG) && LOG_STACKTRACE_USING_DBGENG
            return true;
#else
            return false;
#endif
        }

#if defined(LOG_STACKTRACE_USING_LIBUNWIND) && LOG_STACKTRACE_USING_LIBUNWIND
        size_t stacktrace_write(char *buf, size_t bufsz) {
            if (NULL == buf || bufsz <= 0) {
                return 0;
            }

            unw_context_t unw_ctx;
            unw_cursor_t unw_cur;
            unw_proc_info_t unw_proc;
            unw_getcontext(&unw_ctx);
            unw_init_local(&unw_cur, &unw_ctx);

            char func_name_cache[4096];
            func_name_cache[sizeof(func_name_cache) - 1] = 0;
            unw_word_t unw_offset;
            int frame_id = 0;
            int skip_frames = 1;

            size_t ret = 0;
            do {
                if (skip_frames <= 0) {
                    unw_get_proc_info(&unw_cur, &unw_proc);
                    if (0 == unw_proc.start_ip) {
                        break;
                    }
                    unw_get_proc_name(&unw_cur, func_name_cache, sizeof(func_name_cache) - 1, &unw_offset);

                    const char *func_name = func_name_cache;
#if defined(USING_LIBSTDCXX_ABI) || defined(USING_LIBCXX_ABI)
                    int cxx_abi_status;
                    char *realfunc_name = abi::__cxa_demangle(func_name_cache, 0, 0, &cxx_abi_status);
                    if (NULL != realfunc_name) {
                        func_name = realfunc_name;
                    }
#endif

                    int res = UTIL_STRFUNC_SNPRINTF(buf, bufsz, "Frame #%02d: (%s+0x%llx) [0x%llx]\r\n", frame_id, func_name,
                                                    static_cast<unsigned long long>(unw_offset),
                                                    static_cast<unsigned long long>(unw_proc.start_ip));

                    if (res <= 0) {
                        break;
                    }

                    ret += static_cast<size_t>(res);
                    buf += res;
                    bufsz -= static_cast<size_t>(res);

#if defined(USING_LIBSTDCXX_ABI) || defined(USING_LIBCXX_ABI)
                    if (NULL != realfunc_name) {
                        free(realfunc_name);
                        realfunc_name = NULL;
                    }
#endif
                }

                if (unw_step(&unw_cur) <= 0) {
                    break;
                }

                if (skip_frames > 0) {
                    --skip_frames;
                } else {
                    ++frame_id;
                }
            } while (true);

            return ret;
        }

#elif defined(LOG_STACKTRACE_USING_EXECINFO) && LOG_STACKTRACE_USING_EXECINFO
        struct stacktrace_symbol_group_t {
            std::string module_name;
            std::string func_name;
            std::string func_offset;
            std::string func_address;
        };

        inline bool stacktrace_is_space_char(char c) UTIL_CONFIG_NOEXCEPT { return ' ' == c || '\r' == c || '\t' == c || '\n' == c; }

        static const char *stacktrace_skip_space(const char *name) UTIL_CONFIG_NOEXCEPT {
            if (NULL == name) {
                return name;
            }

            while (*name && stacktrace_is_space_char(*name)) {
                ++name;
            }

            return name;
        }

        inline bool stacktrace_is_number_char(char c) UTIL_CONFIG_NOEXCEPT { return c >= '0' && c <= '9'; }

        inline bool stacktrace_is_ident_char(char c) UTIL_CONFIG_NOEXCEPT {
            return '_' == c || '$' == c || stacktrace_is_number_char(c) || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                   (c & 0x80); // utf-8 or unicode
        }

        static const char *stacktrace_get_ident_end(const char *name) UTIL_CONFIG_NOEXCEPT {
            if (NULL == name) {
                return name;
            }

            while (*name && !stacktrace_is_space_char(*name) && '(' != *name && ')' != *name && '+' != *name) {
                ++name;
            }

            return name;
        }

        static bool stacktrace_pick_ident(const char *name, const char *&start, const char *&end, char &previous_c,
                                          bool &clear_sym) UTIL_CONFIG_NOEXCEPT {
            previous_c = 0;
            start = name;
            end = name;

            bool ret = false;
            if (NULL == name) {
                return false;
            }

            while (*name) {
                // gcc in linux will get a string like MODULE_NAME(FUNCTION_NAME+OFFSET) [HEX_ADDRESS]
                // if we meet a (, we should clear the function name cache
                if ('(' == *name) {
                    clear_sym = true;
                }

                name = stacktrace_skip_space(name);

                if (stacktrace_is_ident_char(*name)) {
                    start = name;
                    end = stacktrace_get_ident_end(name);
                    ret = true;
                    break;
                } else {
                    previous_c = *name;
                    ++name;
                }
            }

            return ret;
        }

        static void stacktrace_fix_number(std::string &num) UTIL_CONFIG_NOEXCEPT {
            size_t fixed_len = num.size();
            while (fixed_len > 0 && (num[fixed_len - 1] > '9' || num[fixed_len - 1] < '0')) {
                --fixed_len;
            }

            num.resize(fixed_len);
        }

        static void stacktrace_pick_symbol_info(const char *name, stacktrace_symbol_group_t &out) {
            out.module_name.clear();
            out.func_name.clear();
            out.func_offset.clear();
            out.func_address.clear();

            if (NULL == name || 0 == *name) {
                return;
            }

            name = stacktrace_skip_space(name);
            while (name) {
                const char *start;
                char previous_c;
                bool clear_sym = false;

                if (stacktrace_pick_ident(name, start, name, previous_c, clear_sym)) {
                    if (stacktrace_is_number_char(*start)) {
                        if (clear_sym) {
                            out.func_name.clear();
                        }

                        if ('+' == previous_c) {
                            out.func_offset = "+";
                            out.func_offset.insert(out.func_offset.end(), start, name);
                            stacktrace_fix_number(out.func_offset);
                        } else {
                            out.func_address.assign(start, name);
                            stacktrace_fix_number(out.func_address);
                        }
                    } else {
                        if (out.module_name.empty()) {
                            out.module_name.assign(start, name);
                        } else {
                            out.func_name.assign(start, name);
                        }
                    }
                } else {
                    if (clear_sym) {
                        out.func_name.clear();
                    }
                    break;
                }

                name = stacktrace_skip_space(name);
            }
        }

        size_t stacktrace_write(char *buf, size_t bufsz) {
            if (NULL == buf || bufsz <= 0) {
                return 0;
            }

            size_t ret = 0;
            void *array[LOG_STACKTRACE_MAX_STACKS_ARRAY_SIZE] = {NULL};
            size_t size;

            size = backtrace(array, LOG_STACKTRACE_MAX_STACKS_ARRAY_SIZE);
            char **func_name_cache = backtrace_symbols(array, size);
            size_t skip_frames = 1;

            for (size_t i = skip_frames; i < size; i++) {
                if (NULL == func_name_cache[i]) {
                    break;
                }

                stacktrace_symbol_group_t symbol;
                stacktrace_pick_symbol_info(func_name_cache[i], symbol);

#if defined(USING_LIBSTDCXX_ABI) || defined(USING_LIBCXX_ABI)
                if (!symbol.func_name.empty()) {
                    int cxx_abi_status;
                    char *realfunc_name = abi::__cxa_demangle(symbol.func_name.c_str(), 0, 0, &cxx_abi_status);
                    if (NULL != realfunc_name) {
                        symbol.func_name = realfunc_name;
                    }

                    if (NULL != realfunc_name) {
                        free(realfunc_name);
                    }
                }
#endif

                int res = UTIL_STRFUNC_SNPRINTF(buf, bufsz, "Frame #%02d: (%s%s) [%s]\r\n", static_cast<int>(i - skip_frames),
                                                symbol.func_name.c_str(), symbol.func_offset.c_str(), symbol.func_address.c_str());

                if (res <= 0) {
                    break;
                }

                ret += static_cast<size_t>(res);
                buf += res;
                bufsz -= static_cast<size_t>(res);
            }

            free(func_name_cache);

            return ret;
        }

#elif defined(LOG_STACKTRACE_USING_UNWIND) && LOG_STACKTRACE_USING_UNWIND
        struct stacktrace_unwind_state_t {
            size_t frames_to_skip;
            _Unwind_Word *current;
            _Unwind_Word *end;
        };

        static _Unwind_Reason_Code stacktrace_unwind_callback(::_Unwind_Context *context, void *arg) UTIL_CONFIG_NOEXCEPT {
            // Note: do not write `::_Unwind_GetIP` because it is a macro on some platforms.
            // Use `_Unwind_GetIP` instead!
            stacktrace_unwind_state_t *const state = reinterpret_cast<stacktrace_unwind_state_t *>(arg);
            if (state->frames_to_skip) {
                --state->frames_to_skip;
                return _Unwind_GetIP(context) ? ::_URC_NO_REASON : ::_URC_END_OF_STACK;
            }

            *state->current = _Unwind_GetIP(context);

            ++state->current;
            if (!*(state->current - 1) || state->current == state->end) {
                return ::_URC_END_OF_STACK;
            }

            return ::_URC_NO_REASON;
        }

        size_t stacktrace_write(char *buf, size_t bufsz) {
            if (NULL == buf || bufsz <= 0) {
                return 0;
            }
            size_t ret = 0;

            _Unwind_Word stacks[LOG_STACKTRACE_MAX_STACKS_ARRAY_SIZE];
            stacktrace_unwind_state_t state;
            state.frames_to_skip = 0;
            state.current = stacks;
            state.end = stacks + LOG_STACKTRACE_MAX_STACKS_ARRAY_SIZE;

            ::_Unwind_Backtrace(&stacktrace_unwind_callback, &state);
            size_t frames_count = state.current - &stacks[0];
            size_t skip_frames = 1;
            for (size_t i = skip_frames; i < frames_count; ++i) {
                if (0 == stacks[i]) {
                    break;
                }

                int res = UTIL_STRFUNC_SNPRINTF(buf, bufsz, "Frame #%02d: () [0x%llx]\r\n", static_cast<int>(i - skip_frames),
                                                static_cast<unsigned long long>(stacks[i]));

                if (res <= 0) {
                    break;
                }

                ret += static_cast<size_t>(res);
                buf += res;
                bufsz -= static_cast<size_t>(res);
            }

            return ret;
        }

#elif (defined(LOG_STACKTRACE_USING_DBGHELP) && LOG_STACKTRACE_USING_DBGHELP) || \
    (defined(LOG_STACKTRACE_USING_DBGENG) && LOG_STACKTRACE_USING_DBGENG)
        size_t stacktrace_write(char *buf, size_t bufsz) {
            if (NULL == buf || bufsz <= 0) {
                return 0;
            }

            PVOID stacks[LOG_STACKTRACE_MAX_STACKS_ARRAY_SIZE];
            USHORT frames_count = CaptureStackBackTrace(0, LOG_STACKTRACE_MAX_STACKS_ARRAY_SIZE, stacks, NULL);

            size_t ret = 0;
            USHORT skip_frames = 1;

#if !defined(_MSC_VER)
            for (USHORT i = skip_frames; i < frames_count; ++i) {
                if (NULL == stacks[i]) {
                    break;
                }

                int res = UTIL_STRFUNC_SNPRINTF(buf, bufsz, "Frame #%02d: () [0x%llx]\r\n", static_cast<int>(i - skip_frames),
                                                reinterpret_cast<unsigned long long>(stacks[i]));

                if (res <= 0) {
                    break;
                }

                ret += static_cast<size_t>(res);
                buf += res;
                bufsz -= static_cast<size_t>(res);
            }

#elif (defined(LOG_STACKTRACE_USING_DBGHELP) && LOG_STACKTRACE_USING_DBGHELP)
            USES_CONVERSION;
            SYMBOL_INFO *symbol;
            DWORD64 displacement = 0;

            symbol = (SYMBOL_INFO *)malloc(sizeof(SYMBOL_INFO) + (MAX_SYM_NAME + 1) * sizeof(TCHAR));
            symbol->MaxNameLen = MAX_SYM_NAME;
            symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

            bool try_read_sym = true;
            for (USHORT i = skip_frames; i < frames_count; ++i) {
                if (NULL == stacks[i]) {
                    break;
                }

                int res;
                if (try_read_sym &&
                    SymFromAddr(SymInitializeHelper::Inst().process, reinterpret_cast<ULONG64>(stacks[i]), &displacement, symbol)) {
                    res = UTIL_STRFUNC_SNPRINTF(buf, bufsz, "Frame #%02d: (%s+0x%llx) [0x%llx]\r\n", static_cast<int>(i - skip_frames),
                                                LOG_STACKTRACE_VC_W2A(symbol->Name), static_cast<unsigned long long>(displacement),
                                                static_cast<unsigned long long>(symbol->Address));
                } else {
                    try_read_sym = false; // 失败一次基本上就是读不到符号信息了，后面也不需要再尝试了
                    res = UTIL_STRFUNC_SNPRINTF(buf, bufsz, "Frame #%02d: () [0x%llx]\r\n", static_cast<int>(i - skip_frames),
                                                reinterpret_cast<unsigned long long>(stacks[i]));
                }

                if (res <= 0) {
                    break;
                }

                ret += static_cast<size_t>(res);
                buf += res;
                bufsz -= static_cast<size_t>(res);
            }

            free(symbol);
#else
            USES_CONVERSION;

            log_stacktrace_com_holder<IDebugClient> dbg_cli;
            log_stacktrace_com_holder<IDebugControl> dbg_ctrl;
            log_stacktrace_com_holder<IDebugSymbols> dbg_sym;
            const char *error_msg = NULL;
            do {
                if (S_OK != ::DebugCreate(__uuidof(IDebugClient), dbg_cli.to_pvoid_ptr())) {
                    error_msg = "DebugCreate(IDebugClient) failed";
                    break;
                }

                if (S_OK != dbg_cli->QueryInterface(__uuidof(IDebugControl), dbg_ctrl.to_pvoid_ptr())) {
                    error_msg = "IDebugClient.QueryInterface(IDebugControl) failed";
                    break;
                }

                if (S_OK !=
                    dbg_cli->AttachProcess(0, ::GetCurrentProcessId(), DEBUG_ATTACH_NONINVASIVE | DEBUG_ATTACH_NONINVASIVE_NO_SUSPEND)) {
                    error_msg = "IDebugClient.AttachProcess(GetCurrentProcessId()) failed";
                    break;
                }

                if (S_OK != dbg_ctrl->WaitForEvent(DEBUG_WAIT_DEFAULT, INFINITE)) {
                    error_msg = "IDebugClient.WaitForEvent(DEBUG_WAIT_DEFAULT) failed";
                    break;
                }

                // No cheking: QueryInterface sets the output parameter to NULL in case of error.
                dbg_cli->QueryInterface(__uuidof(IDebugSymbols), dbg_sym.to_pvoid_ptr());

                bool try_read_sym = true;
                for (USHORT i = skip_frames; i < frames_count; ++i) {
                    if (NULL == stacks[i]) {
                        break;
                    }
                    const ULONG64 offset = reinterpret_cast<ULONG64>(stacks[i]);

                    int res = 0;
                    if (try_read_sym) {
                        if (!dbg_sym.is_inited()) {
                            error_msg = "IDebugClient.QueryInterface(IDebugSymbols) failed";
                            break;
                        }

                        // 先尝试用栈上的缓冲区
                        TCHAR func_name[256] = {0};
                        ULONG size = 0;
                        bool res_get_name = (S_OK == dbg_sym->GetNameByOffset(offset, func_name,
                                                                              (ULONG)(sizeof(func_name) / sizeof(func_name[0])), &size, 0));
                        if (!res_get_name && size != 0) { // 栈上的缓冲区不够再用堆内存
                            std::string result;
                            result.resize((size + 1) * sizeof(func_name[0]));
                            res_get_name = (S_OK == dbg_sym->GetNameByOffset(offset, (PSTR)&result[0],
                                                                             (ULONG)(result.size() / sizeof(func_name[0])), &size, 0));

                            if (res_get_name) {
                                res = UTIL_STRFUNC_SNPRINTF(buf, bufsz, "Frame #%02d: (%s) [0x%llx]\r\n", static_cast<int>(i - skip_frames),
                                                            LOG_STACKTRACE_VC_W2A(result.c_str()), static_cast<unsigned long long>(offset));
                            }
                        } else if (res_get_name) {
                            res = UTIL_STRFUNC_SNPRINTF(buf, bufsz, "Frame #%02d: (%s) [0x%llx]\r\n", static_cast<int>(i - skip_frames),
                                                        LOG_STACKTRACE_VC_W2A(func_name), static_cast<unsigned long long>(offset));
                        }

                        if (!res_get_name) {
                            try_read_sym = false;
                        }
                    }

                    // 读不到符号，就只写出地址
                    if (!try_read_sym) {
                        res = UTIL_STRFUNC_SNPRINTF(buf, bufsz, "Frame #%02d: () [0x%llx]\r\n", static_cast<int>(i - skip_frames),
                                                    static_cast<unsigned long long>(offset));
                    }

                    if (res <= 0) {
                        break;
                    }

                    ret += static_cast<size_t>(res);
                    buf += res;
                    bufsz -= static_cast<size_t>(res);
                }
            } while (false);

            // append error msg
            if (error_msg != NULL) {
                size_t error_msg_len = strlen(error_msg);
                if (NULL != buf && bufsz > error_msg_len) {
                    memcpy(buf, error_msg, error_msg_len + 1);
                    ret += error_msg_len;
                }
            }
#endif
            return ret;
        }
#else
        size_t stacktrace_write(char *buf, size_t bufsz) {
            const char *msg = "stacktrace disabled.";
            if (NULL == buf || bufsz <= strlen(msg)) {
                return 0;
            }

            size_t ret = strlen(msg);
            memcpy(buf, msg, ret + 1);
            return ret;
        }
#endif
    } // namespace log
} // namespace util