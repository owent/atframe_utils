/**
 * @file jiffies_timer.h
 * @brief O(1)复杂度定时器，设计参考linux kernel(4.9.10)的jiffies timer
 * @see https://git.kernel.org/cgit/linux/kernel/git/stable/linux-stable.git/tree/kernel/time/timer.c
 * Licensed under the MIT licenses.
 *
 * @note 可以根据实际需要自定义每个tick的间隔,所有的算法都是以tick为单位的
 *       4.9.10版本的jiffies_timer分为8-9级定时器，每级精度相差2^3倍，所有定时器列表集中存放在一个2^6*[8-9]=512-576个定时器列表
 *       由于本定时器是软实现，并不调用硬件时钟，所以如果tick精度很高的话开销比较大。这种情况请直接使用kernel的timer，因为内核版本会利用硬件时钟
 * @version 1.0
 * @author owent
 * @date 2017-02-16
 * @history
 *      2017-02-17: 第一版实现，暂时不加锁
 *      2025-06-30: 默认使用非线程安全的智能指针，支持自定义 allocator (除回调函数外)
 */

#pragma once

/**
 * 精度参考
 * HZ 1000 steps, LVL_BITS=6,LVL_CLK_SHIFT=3,LVL_DEPTH=9
 * Level Offset  Granularity            Range
 *  0      0         1 ms                0 ms -         63 ms
 *  1     64         8 ms               64 ms -        511 ms
 *  2    128        64 ms              512 ms -       4095 ms (512ms - ~4s)
 *  3    192       512 ms             4096 ms -      32767 ms (~4s - ~32s)
 *  4    256      4096 ms (~4s)      32768 ms -     262143 ms (~32s - ~4m)
 *  5    320     32768 ms (~32s)    262144 ms -    2097151 ms (~4m - ~34m)
 *  6    384    262144 ms (~4m)    2097152 ms -   16777215 ms (~34m - ~4h)
 *  7    448   2097152 ms (~34m)  16777216 ms -  134217727 ms (~4h - ~1d)
 *  8    512  16777216 ms (~4h)  134217728 ms - 1073741822 ms (~1d - ~12d)
 *
 * HZ  250, LVL_BITS=6,LVL_CLK_SHIFT=3,LVL_DEPTH=9
 * Level Offset  Granularity            Range
 *  0	     0         4 ms                0 ms -        255 ms
 *  1	    64        32 ms              256 ms -       2047 ms (256ms - ~2s)
 *  2	   128       256 ms             2048 ms -      16383 ms (~2s - ~16s)
 *  3	   192      2048 ms (~2s)      16384 ms -     131071 ms (~16s - ~2m)
 *  4	   256     16384 ms (~16s)    131072 ms -    1048575 ms (~2m - ~17m)
 *  5	   320    131072 ms (~2m)    1048576 ms -    8388607 ms (~17m - ~2h)
 *  6	   384   1048576 ms (~17m)   8388608 ms -   67108863 ms (~2h - ~18h)
 *  7	   448   8388608 ms (~2h)   67108864 ms -  536870911 ms (~18h - ~6d)
 *  8    512  67108864 ms (~18h) 536870912 ms - 4294967288 ms (~6d - ~49d)
 *
 * HZ  100, LVL_BITS=6,LVL_CLK_SHIFT=3,LVL_DEPTH=8
 * Level Offset  Granularity            Range
 *  0	   0         10 ms               0 ms -        630 ms
 *  1	  64         80 ms             640 ms -       5110 ms (640ms - ~5s)
 *  2	 128        640 ms            5120 ms -      40950 ms (~5s - ~40s)
 *  3	 192       5120 ms (~5s)     40960 ms -     327670 ms (~40s - ~5m)
 *  4	 256      40960 ms (~40s)   327680 ms -    2621430 ms (~5m - ~43m)
 *  5	 320     327680 ms (~5m)   2621440 ms -   20971510 ms (~43m - ~5h)
 *  6	 384    2621440 ms (~43m) 20971520 ms -  167772150 ms (~5h - ~1d)
 *  7	 448   20971520 ms (~5h) 167772160 ms - 1342177270 ms (~1d - ~15d)
 *
 * HZ  10, LVL_BITS=8,LVL_CLK_SHIFT=3,LVL_DEPTH=8
 * Level Offset  Granularity              Range
 *  0	     0        100 ms                 0 ms -       25500 ms (0s - ~25s)
 *  1	   256        800 ms             25600 ms -      204700 ms (25s - ~204s)
 *  2	   512       6400 ms (~6s)      204800 ms -     1638300 ms (~204s - ~27m)
 *  3	   768      51200 ms (~51s)    1638400 ms -    13107100 ms (~27m - ~3h)
 *  4   1024     409600 ms (~409s)  13107200 ms -   104857500 ms (~3h - ~29h)
 *  5   1280    3276800 ms (~55m)  104857600 ms -   838860700 ms (~29h - ~9d)
 *  6   1536   26214400 ms (~7h)   838860800 ms -  6710886300 ms (~9d - ~77d)
 *  7   1792  209715200 ms (~58h) 6710886400 ms - 13421772700 ms (~19d - ~621d)
 */

#include <config/compile_optimize.h>
#include <config/compiler_features.h>

#include <config/atframe_utils_build_feature.h>

#include <memory/rc_ptr.h>

#include <assert.h>
#include <stdint.h>
#include <bitset>
#include <cstddef>
#include <cstring>
#include <ctime>
#include <functional>
#include <list>
#include <memory>

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace time {
/**
 * @brief jiffies timer 定时器实现
 * @note 空间复杂度: O(LVL_DEPTH * 2^LVL_BITS * sizeof(std::list)) <br />
 *       每次tick的最低时间复杂度: O(LVL_DEPTH) <br />
 *       每层定时器误差倍数: 2^LVL_CLK_SHIFT <br />
 *       最大定时器范围: 2^(LVL_CLK_SHIFT * (LVL_DEPTH - 1) + LVL_BITS) * tick周期 <br />
 * @note 如果外部需要引用定时器对象，请使用 timer_t 代替函数签名中的 timer_type
 */
template <time_t LVL_BITS = 6, time_t LVL_CLK_SHIFT = 3, size_t LVL_DEPTH = 8,
          memory::compat_strong_ptr_mode PTR_MODE = memory::compat_strong_ptr_mode::kStrongRc>
class ATFRAMEWORK_UTILS_API_HEAD_ONLY jiffies_timer {
 public:
  UTIL_CONFIG_STATIC_ASSERT(LVL_CLK_SHIFT < LVL_BITS);

  enum lvl_clk_consts {
    LVL_CLK_DIV = 1 << LVL_CLK_SHIFT,
    LVL_CLK_MASK = LVL_CLK_DIV - 1,
  };
  static inline UTIL_CONFIG_CONSTEXPR time_t LVL_SHIFT(time_t n) { return n * LVL_CLK_SHIFT; }

  static inline UTIL_CONFIG_CONSTEXPR time_t LVL_GRAN(time_t n) { return static_cast<time_t>(1) << LVL_SHIFT(n); }

  enum lvl_consts {
    LVL_SIZE = 1 << LVL_BITS,  //
    LVL_MASK = LVL_SIZE - 1,
    WHEEL_SIZE = LVL_SIZE * LVL_DEPTH
  };
  static inline UTIL_CONFIG_CONSTEXPR size_t LVL_OFFS(size_t n) { return n * LVL_SIZE; }

  static inline UTIL_CONFIG_CONSTEXPR time_t LVL_START(size_t n) {
    return static_cast<time_t>((LVL_SIZE) << ((n - 1) * LVL_CLK_SHIFT));
  }

 private:
  struct timer_type;

 public:
  using timer_callback_fn_t = std::function<void(time_t tick_time, const timer_type &timer)>;
  using timer_ptr_t = typename memory::compat_strong_ptr_function_trait<PTR_MODE>::template shared_ptr<
      timer_type>;  // 外部请勿直接访问内部成员，只允许通过API访问
  using timer_wptr_t = typename memory::compat_strong_ptr_function_trait<PTR_MODE>::template weak_ptr<
      timer_type>;  // 外部请勿直接访问内部成员，只允许通过API访问

 private:
  struct timer_type {
    mutable uint32_t flags;                                // 定时器标记位
    uint32_t sequence;                                     // 定时器序号
    time_t timeout;                                        // 原始的超时时间
    void *private_data;                                    // 私有数据指针
    timer_callback_fn_t fn;                                // 回调函数
    jiffies_timer *owner;                                  // 所属的定时器管理器
    size_t owner_idx;                                      // 所属的定时器管理器所在时间轮下标
    std::list<timer_ptr_t> *owner_round;                   // 所属的时间轮
    typename std::list<timer_ptr_t>::iterator owner_iter;  // 所属的时间轮迭代器
  };  // 外部请勿直接访问内部成员，只允许通过API访问

 public:
  using timer_t = timer_type;  // 外部请勿直接访问内部成员，只允许通过API访问

  struct flag_t {
    enum type {
      EN_JTFT_INITED = 0,
      EN_JTFT_MAX,
    };
  };

  struct timer_flag_t {
    enum type {
      EN_JTTF_DISABLED = 0x0001,
      EN_JTTF_REMOVED = 0x0002,  // readonly flag
    };
  };

  struct error_type_t {
    enum type {
      EN_JTET_SUCCESS = 0,              // 成功
      EN_JTET_NOT_INITED = -101,        // 未初始化
      EN_JTET_ALREADY_INITED = -102,    // 已初始化
      EN_JTET_TIMEOUT_EXTENDED = -103,  // 超时时间超出上限
    };
  };

 public:
  jiffies_timer() : last_tick_(0), seq_alloc_(0), size_(0), private_data_(nullptr) {}

  /**
   * @brief 初始化定时器
   * @param init_tick 初始定时器tick数（绝对时间），定时器将从这个时间开始触发
   * @return 0或错误码
   */
  int init(time_t init_tick) noexcept {
    if (flags_.test(flag_t::EN_JTFT_INITED)) {
      return error_type_t::EN_JTET_ALREADY_INITED;
    }
    flags_.set(flag_t::EN_JTFT_INITED, true);

    last_tick_ = init_tick;
    seq_alloc_ = 0;
    size_ = 0;

    return error_type_t::EN_JTET_SUCCESS;
  }

 private:
  /**
   * @brief 添加定时器
   * @param delta 定时器间隔，相对时间（向下取整，即如果应该是3.8个后tick触发，这里应该取3）
   * @param fn 定时器回掉函数
   * @param priv_data
   * @param watcher 定时器的监视器指针，如果非空，这个weak_ptr会指向定时器对象，用于以后查询或修改数据
   * @note 定时器回调保证晚于指定时间间隔后的下一个误差范围内时间触发。
   *       这里有一个特殊的设计是认为当前tick的时间已被向下取整，即第3.8个tick的时间在定时器里记录的是3。
   *       所以会保证定时器的触发时间一定晚于指定的时间（即，3.8+2=5.8意味着第6个tick才会触发定时器）
   * @note 请尽量不要在外部直接保存定时器的智能指针(timer_ptr_t)，而仅仅使用监视器
   * @return 0或错误码
   */

  template <class AllocatorType>
  int internal_add_timer(const AllocatorType *alloc, time_t delta, timer_callback_fn_t &&fn, void *priv_data,
                         timer_wptr_t *watcher) {
    if (!flags_.test(flag_t::EN_JTFT_INITED)) {
      return error_type_t::EN_JTET_NOT_INITED;
    }

    if (delta > get_max_tick_distance()) {
      return error_type_t::EN_JTET_TIMEOUT_EXTENDED;
    }

    if (!fn) {
      return error_type_t::EN_JTET_SUCCESS;
    }

    // must greater than 0
    if (delta <= 0) {
      delta = 1;
    }

    timer_ptr_t timer_inst;
    if (nullptr == alloc) {
      timer_inst = memory::compat_strong_ptr_function_trait<PTR_MODE>::template make_shared<timer_type>();
    } else {
      timer_inst = memory::compat_strong_ptr_function_trait<PTR_MODE>::template allocate_shared<timer_type>(*alloc);
    }

    timer_inst->flags = 0;
    timer_inst->timeout = last_tick_ + delta;
    timer_inst->private_data = priv_data;
    timer_inst->owner_round = nullptr;
    timer_inst->owner = nullptr;
    timer_inst->owner_idx = static_cast<size_t>(-1);
    // timer_inst->owner_iter = ...
    while (0 == ++seq_alloc_) {
    }
    timer_inst->sequence = seq_alloc_;

    timer_inst->fn = std::move(fn);

    // assign to watcher
    if (watcher != nullptr) {
      *watcher = timer_inst;
    }

    insert_timer(std::move(timer_inst));
    return error_type_t::EN_JTET_SUCCESS;
  }

 public:
  /**
   * @brief 添加定时器
   * @param delta 定时器间隔，相对时间（向下取整，即如果应该是3.8个后tick触发，这里应该取3）
   * @param fn 定时器回掉函数
   * @param priv_data
   * @param watcher 定时器的监视器指针，如果非空，这个weak_ptr会指向定时器对象，用于以后查询或修改数据
   * @note 定时器回调保证晚于指定时间间隔后的下一个误差范围内时间触发。
   *       这里有一个特殊的设计是认为当前tick的时间已被向下取整，即第3.8个tick的时间在定时器里记录的是3。
   *       所以会保证定时器的触发时间一定晚于指定的时间（即，3.8+2=5.8意味着第6个tick才会触发定时器）
   * @note 请尽量不要在外部直接保存定时器的智能指针(timer_ptr_t)，而仅仅使用监视器
   * @return 0或错误码
   */
  template <class TCALLBACK,
            class = nostd::enable_if_t<!::std::is_same<nostd::remove_cvref_t<TCALLBACK>, timer_callback_fn_t>::value>>
  UTIL_FORCEINLINE int add_timer(time_t delta, TCALLBACK &&fn, void *priv_data, timer_wptr_t *watcher) {
    return internal_add_timer(static_cast<::std::allocator<timer_type> *>(nullptr), delta,
                              timer_callback_fn_t(std::forward<TCALLBACK>(fn)), priv_data, watcher);
  }

  /**
   * @brief 添加定时器
   * @param alloc 内存分配器
   * @param delta 定时器间隔，相对时间（向下取整，即如果应该是3.8个后tick触发，这里应该取3）
   * @param fn 定时器回掉函数
   * @param priv_data
   * @param watcher 定时器的监视器指针，如果非空，这个weak_ptr会指向定时器对象，用于以后查询或修改数据
   * @note 定时器回调保证晚于指定时间间隔后的下一个误差范围内时间触发。
   *       这里有一个特殊的设计是认为当前tick的时间已被向下取整，即第3.8个tick的时间在定时器里记录的是3。
   *       所以会保证定时器的触发时间一定晚于指定的时间（即，3.8+2=5.8意味着第6个tick才会触发定时器）
   * @note 请尽量不要在外部直接保存定时器的智能指针(timer_ptr_t)，而仅仅使用监视器
   * @return 0或错误码
   */
  template <class TALLOCATOR, class TCALLBACK,
            class = nostd::enable_if_t<!::std::is_same<nostd::remove_cvref_t<TCALLBACK>, timer_callback_fn_t>::value>>
  UTIL_FORCEINLINE int add_timer(const TALLOCATOR &alloc, time_t delta, TCALLBACK &&fn, void *priv_data,
                                 timer_wptr_t *watcher) {
    return internal_add_timer(&alloc, delta, timer_callback_fn_t(std::forward<TCALLBACK>(fn)), priv_data, watcher);
  }

  /**
   * @brief 添加定时器
   * @param delta 定时器间隔，相对时间（向下取整，即如果应该是3.8个后tick触发，这里应该取3）
   * @param fn 定时器回掉函数
   * @param priv_data
   * @param watcher 定时器的监视器指针，如果非空，这个weak_ptr会指向定时器对象，用于以后查询或修改数据
   * @note 定时器回调保证晚于指定时间间隔后的下一个误差范围内时间触发。
   *       这里有一个特殊的设计是认为当前tick的时间已被向下取整，即第3.8个tick的时间在定时器里记录的是3。
   *       所以会保证定时器的触发时间一定晚于指定的时间（即，3.8+2=5.8意味着第6个tick才会触发定时器）
   * @note 请尽量不要在外部直接保存定时器的智能指针(timer_ptr_t)，而仅仅使用监视器
   * @return 0或错误码
   */

  int add_timer(time_t delta, timer_callback_fn_t &&fn, void *priv_data, timer_wptr_t *watcher) {
    return internal_add_timer(static_cast<::std::allocator<timer_type> *>(nullptr), delta, std::move(fn), priv_data,
                              watcher);
  }

  /**
   * @brief 添加定时器
   * @param alloc 内存分配器
   * @param delta 定时器间隔，相对时间（向下取整，即如果应该是3.8个后tick触发，这里应该取3）
   * @param fn 定时器回掉函数
   * @param priv_data
   * @param watcher 定时器的监视器指针，如果非空，这个weak_ptr会指向定时器对象，用于以后查询或修改数据
   * @note 定时器回调保证晚于指定时间间隔后的下一个误差范围内时间触发。
   *       这里有一个特殊的设计是认为当前tick的时间已被向下取整，即第3.8个tick的时间在定时器里记录的是3。
   *       所以会保证定时器的触发时间一定晚于指定的时间（即，3.8+2=5.8意味着第6个tick才会触发定时器）
   * @note 请尽量不要在外部直接保存定时器的智能指针(timer_ptr_t)，而仅仅使用监视器
   * @return 0或错误码
   */
  template <class TALLOCATOR>
  int add_timer(const TALLOCATOR &alloc, time_t delta, timer_callback_fn_t &&fn, void *priv_data,
                timer_wptr_t *watcher) {
    return internal_add_timer(&alloc, delta, std::move(fn), priv_data, watcher);
  }

  /**
   * @brief 添加定时器
   * @param delta 定时器间隔，相对时间（向下取整，即如果应该是3.8个后tick触发，这里应该取3）
   * @param fn 定时器回掉函数
   * @param priv_data
   * @param watcher 定时器的监视器指针，如果非空，这个weak_ptr会指向定时器对象，用于以后查询或修改数据
   * @note 定时器回调保证晚于指定时间间隔后的下一个误差范围内时间触发。
   *       这里有一个特殊的设计是认为当前tick的时间已被向下取整，即第3.8个tick的时间在定时器里记录的是3。
   *       所以会保证定时器的触发时间一定晚于指定的时间（即，3.8+2=5.8意味着第6个tick才会触发定时器）
   * @note 请尽量不要在外部直接保存定时器的智能指针(timer_ptr_t)，而仅仅使用监视器
   * @return 0或错误码
   */
  UTIL_FORCEINLINE int add_timer(time_t delta, const timer_callback_fn_t &fn, void *priv_data) {
    return internal_add_timer(static_cast<::std::allocator<timer_type> *>(nullptr), delta, timer_callback_fn_t(fn),
                              priv_data, nullptr);
  }

  /**
   * @brief 添加定时器
   * @param alloc 内存分配器
   * @param delta 定时器间隔，相对时间（向下取整，即如果应该是3.8个后tick触发，这里应该取3）
   * @param fn 定时器回掉函数
   * @param priv_data
   * @param watcher 定时器的监视器指针，如果非空，这个weak_ptr会指向定时器对象，用于以后查询或修改数据
   * @note 定时器回调保证晚于指定时间间隔后的下一个误差范围内时间触发。
   *       这里有一个特殊的设计是认为当前tick的时间已被向下取整，即第3.8个tick的时间在定时器里记录的是3。
   *       所以会保证定时器的触发时间一定晚于指定的时间（即，3.8+2=5.8意味着第6个tick才会触发定时器）
   * @note 请尽量不要在外部直接保存定时器的智能指针(timer_ptr_t)，而仅仅使用监视器
   * @return 0或错误码
   */
  template <class TALLOCATOR>
  UTIL_FORCEINLINE int add_timer(const TALLOCATOR &alloc, time_t delta, const timer_callback_fn_t &fn,
                                 void *priv_data) {
    return internal_add_timer(&alloc, delta, timer_callback_fn_t(fn), priv_data, nullptr);
  }

  /**
   * @brief 添加定时器
   * @param delta 定时器间隔，相对时间（向下取整，即如果应该是3.8个后tick触发，这里应该取3）
   * @param fn 定时器回掉函数
   * @param priv_data
   * @param watcher 定时器的监视器指针，如果非空，这个weak_ptr会指向定时器对象，用于以后查询或修改数据
   * @note 定时器回调保证晚于指定时间间隔后的下一个误差范围内时间触发。
   *       这里有一个特殊的设计是认为当前tick的时间已被向下取整，即第3.8个tick的时间在定时器里记录的是3。
   *       所以会保证定时器的触发时间一定晚于指定的时间（即，3.8+2=5.8意味着第6个tick才会触发定时器）
   * @note 请尽量不要在外部直接保存定时器的智能指针(timer_ptr_t)，而仅仅使用监视器
   * @return 0或错误码
   */
  UTIL_FORCEINLINE int add_timer(time_t delta, timer_callback_fn_t &&fn, void *priv_data) {
    return internal_add_timer(static_cast<::std::allocator<timer_type> *>(nullptr), delta, std::move(fn), priv_data,
                              nullptr);
  }

  /**
   * @brief 添加定时器
   * @param alloc 内存分配器
   * @param delta 定时器间隔，相对时间（向下取整，即如果应该是3.8个后tick触发，这里应该取3）
   * @param fn 定时器回掉函数
   * @param priv_data
   * @param watcher 定时器的监视器指针，如果非空，这个weak_ptr会指向定时器对象，用于以后查询或修改数据
   * @note 定时器回调保证晚于指定时间间隔后的下一个误差范围内时间触发。
   *       这里有一个特殊的设计是认为当前tick的时间已被向下取整，即第3.8个tick的时间在定时器里记录的是3。
   *       所以会保证定时器的触发时间一定晚于指定的时间（即，3.8+2=5.8意味着第6个tick才会触发定时器）
   * @note 请尽量不要在外部直接保存定时器的智能指针(timer_ptr_t)，而仅仅使用监视器
   * @return 0或错误码
   */
  template <class TALLOCATOR>
  UTIL_FORCEINLINE int add_timer(const TALLOCATOR &alloc, time_t delta, timer_callback_fn_t &&fn, void *priv_data) {
    return internal_add_timer(&alloc, delta, std::move(fn), priv_data, nullptr);
  }

  /**
   * @brief 定时器滴答
   * @param expires 到期的定时器时间（绝对时间）
   * @return 错误码或触发的定时器数量
   */
  int tick(time_t expires) {
    std::list<timer_ptr_t> *timer_list[LVL_DEPTH];
    int ret = 0;

    if (!flags_.test(flag_t::EN_JTFT_INITED)) {
      return error_type_t::EN_JTET_NOT_INITED;
    }

    if (expires <= last_tick_) {
      return ret;
    }

    while (last_tick_ < expires) {
      ++last_tick_;

      size_t list_sz = collect_expired_timers(last_tick_, timer_list);
      while (list_sz > 0) {
        --list_sz;
        // 从高层级往低层级走，这样能保证定时器时序

        while (!timer_list[list_sz]->empty()) {
          // 在定时器回调函数中可能调用remove_timer来让当前迭代器失效
          // 所以这里必须保存定时器智能指针，然后直接滚动到下一个，因为当前迭代器可能失效
          timer_ptr_t timer_ptr = *timer_list[list_sz]->begin();
          if (timer_ptr) {
            if (timer_ptr->flags & timer_flag_t::EN_JTTF_REMOVED) {
              remove_timer(*timer_ptr);
              continue;
            }

            // 如果手动执行 insert_timer 把定时器从高阶移动到低阶，可能导致定时器未过期但被插入。
            // 此时直接跳过即可，否则会陷入插入定时器死循环。
            // 由于定时器总是按顺序插入，这里碰到第一个未过期定时器，那么后面的一定都未过期。
            if (timer_ptr->timeout > last_tick_) {
              // 定时器从高阶移动到低阶是允许的，手动执行 insert_timer 时会出现要插入的目标和当前相同的情况
              size_t idx = calc_wheel_index(timer_ptr->timeout, last_tick_);
              if (idx < WHEEL_SIZE && timer_list[list_sz] == &timer_base_[idx]) {
                break;
              }
            }

            remove_timer(*timer_ptr);

            if (timer_ptr->timeout > last_tick_) {
              insert_timer(std::move(timer_ptr));
              continue;
            }

            if (timer_ptr->fn && !(timer_ptr->flags & timer_flag_t::EN_JTTF_DISABLED)) {
              timer_ptr->fn(last_tick_, *timer_ptr);
              ++ret;
            }
          } else {
            timer_list[list_sz]->erase(timer_list[list_sz]->begin());
          }
        }
      }
    }

    return ret;
  }

  /**
   * @brief 获取最后一次定时器滴答时间（当前定时器时间）
   * @return 最后一次定时器滴答时间（当前定时器时间）
   */
  UTIL_FORCEINLINE time_t get_last_tick() const { return last_tick_; }

  /**
   * @brief 获取定时器数量
   * @return 定时器数量
   */
  UTIL_FORCEINLINE size_t size() const { return size_; }

  /**
   * @brief 获取绑定的私有数据
   * @return 绑定的私有数据
   */
  UTIL_FORCEINLINE void *get_private_data() const noexcept { return private_data_; }

  /**
   * @brief 绑定私有数据
   * @param priv_data 私有数据
   * @return 上一次绑定的私有数据
   */
  UTIL_FORCEINLINE void *set_private_data(void *priv_data) noexcept {
    void *old_value = private_data_;
    private_data_ = priv_data;
    return old_value;
  }

 public:
  /**
   * @brief 获取当前定时器类型的最大时间范围（tick）
   * @return 当前定时器类型的最大时间范围（tick）
   */
  UTIL_FORCEINLINE constexpr static time_t get_max_tick_distance() { return LVL_START(LVL_DEPTH) - 1; }

  static inline size_t calc_index(time_t expires, size_t lvl) noexcept {
    // 这里的expires 必然大于等于last_tick_，并且至少加一帧
    // 定时器精度升级时也不会插到当前帧，而是会插到上一级的匹配帧
    expires >>= LVL_SHIFT(static_cast<time_t>(lvl));
    return LVL_OFFS(lvl) + static_cast<size_t>(expires & LVL_MASK);
  }

  static size_t calc_wheel_index(time_t expires, time_t clk) noexcept {
    assert(expires > clk);
    time_t delta = expires - clk;
    size_t idx = WHEEL_SIZE;

    for (size_t lvl = 0; lvl < LVL_DEPTH; ++lvl) {
      if (delta < LVL_START(lvl + 1)) {
        idx = calc_index(expires, lvl);
        break;
      }
    }

    assert(idx < WHEEL_SIZE);
    return idx;
  }

 public:
  UTIL_FORCEINLINE static void *get_timer_private_data(const timer_type &timer) noexcept { return timer.private_data; }
  UTIL_FORCEINLINE static void *set_timer_private_data(timer_type &timer, void *priv_data) noexcept {
    void *old_value = timer.private_data;
    timer.private_data = priv_data;
    return old_value;
  }
  UTIL_FORCEINLINE static uint32_t get_timer_sequence(const timer_type &timer) noexcept { return timer.sequence; }
  UTIL_FORCEINLINE static size_t get_timer_wheel_index(const timer_type &timer) noexcept { return timer.owner_idx; }
  UTIL_FORCEINLINE static time_t get_timer_timeout(const timer_type &timer) noexcept { return timer.timeout; }
  UTIL_FORCEINLINE static bool check_timer_flags(const timer_type &timer, typename timer_flag_t::type f) noexcept {
    return !!(timer.flags & static_cast<uint32_t>(f));
  }
  UTIL_FORCEINLINE static void set_timer_flags(const timer_type &timer, typename timer_flag_t::type f) noexcept {
    timer.flags |= static_cast<uint32_t>(f);
  }
  UTIL_FORCEINLINE static void unset_timer_flags(const timer_type &timer, typename timer_flag_t::type f) noexcept {
    timer.flags &= ~static_cast<uint32_t>(f);
  }
  static inline void remove_timer(timer_type &timer) noexcept {
    if (nullptr != timer.owner_round) {
      if (timer.owner_iter != timer.owner_round->end()) {
        timer.owner_round->erase(timer.owner_iter);
      }

      timer.owner_iter = timer.owner_round->end();
      timer.owner_round = nullptr;
    }

    if (nullptr != timer.owner) {
      --timer.owner->size_;
      timer.owner = nullptr;
      timer.owner_idx = static_cast<size_t>(-1);
    }

    set_timer_flags(timer, timer_flag_t::EN_JTTF_REMOVED);
  }

  inline void insert_timer(timer_ptr_t timer_inst) noexcept {
    if (!timer_inst) {
      return;
    }

    if (nullptr != timer_inst->owner_round) {
      remove_timer(*timer_inst);
    }

    size_t idx = calc_wheel_index(timer_inst->timeout, last_tick_);
    assert(idx < WHEEL_SIZE);

    timer_inst->owner_iter = timer_base_[idx].insert(timer_base_[idx].end(), timer_inst);
    timer_inst->owner_round = &timer_base_[idx];
    timer_inst->owner = this;
    timer_inst->owner_idx = idx;
    unset_timer_flags(*timer_inst, timer_flag_t::EN_JTTF_REMOVED);

    ++size_;
  }

 private:
  size_t collect_expired_timers(time_t tick_time, std::list<timer_ptr_t> *timer_list[LVL_DEPTH]) noexcept {
    size_t ret = 0;
    bool active_level = true;
    for (size_t i = 0; i < LVL_DEPTH; ++i) {
      size_t idx = static_cast<size_t>(tick_time & LVL_MASK) + LVL_OFFS(i);

      if (active_level && !timer_base_[idx].empty()) {
        timer_list[ret++] = &timer_base_[idx];
      }

      active_level = 0 == (LVL_CLK_MASK & tick_time);
      tick_time >>= LVL_CLK_SHIFT;
    }

    return ret;
  }

 private:
  time_t last_tick_;
  std::bitset<flag_t::EN_JTFT_MAX> flags_;
  std::list<timer_ptr_t> timer_base_[WHEEL_SIZE];
  uint32_t seq_alloc_;
  size_t size_;
  void *private_data_;
};
}  // namespace time
ATFRAMEWORK_UTILS_NAMESPACE_END
