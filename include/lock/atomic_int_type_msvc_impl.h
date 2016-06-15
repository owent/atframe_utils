﻿/**
 * @file atomic_int_type_msvc_impl.h
 * @brief 整数类型的原子操作-MSVC统一接口
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author owent
 * @date 2016-06-14
 *
 * @note 低版本 VC使用InterlockedExchange等实现原子操作
 * @see https://msdn.microsoft.com/en-us/library/windows/desktop/ms686360(v=vs.85).aspx
 *
 * @history
 *     2016-06-14
 */

#ifndef _UTIL_LOCK_ATOMIC_INT_TYPE_MSVC_IMPL_H_
#define _UTIL_LOCK_ATOMIC_INT_TYPE_MSVC_IMPL_H_

#pragma once

#include <WinBase.h>

namespace util {
    namespace lock {
        namespace detail {
            template<int INT_SIZE>
            struct atomic_msvc_oprs;

            template<>
            struct atomic_msvc_oprs<1> {
                typedef char opr_t;

                static opr_t exchange(volatile opr_t * target, opr_t value, ::util::lock::memory_order order) {
                    switch(order) {
                        case ::util::lock::memory_order_relaxed:
                            return InterlockedExchange8NoFence(target, value);
                        case ::util::lock::memory_order_acquire:
                            return InterlockedExchange8Acquire(target, value);
                        case ::util::lock::memory_order_release:
                            return InterlockedExchange8Release(target, value);
                        default:
                            return InterlockedExchange8(target, value);
                    }
                }

                static opr_t cas(volatile opr_t * target, opr_t value, opr_t expected, ::util::lock::memory_order order) {
                    switch(order) {
                        case ::util::lock::memory_order_relaxed:
                            return InterlockedCompareExchange8NoFence(target, value, expected);
                        case ::util::lock::memory_order_acquire:
                            return InterlockedCompareExchange8Acquire(target, value, expected);
                        case ::util::lock::memory_order_release:
                            return InterlockedCompareExchange8Release(target, value, expected);
                        default:
                            return InterlockedCompareExchange8(target, value, expected);
                    }
                }

                static opr_t inc(volatile opr_t * target, ::util::lock::memory_order order) {
                    switch(order) {
                        case ::util::lock::memory_order_relaxed:
                            return InterlockedIncrement8NoFence(target);
                        case ::util::lock::memory_order_acquire:
                            return InterlockedIncrement8Acquire(target);
                        case ::util::lock::memory_order_release:
                            return InterlockedIncrement8Release(target);
                        default:
                            return InterlockedIncrement8(target);
                    }
                }

                static opr_t dec(volatile opr_t * target, ::util::lock::memory_order order) {
                    switch(order) {
                        case ::util::lock::memory_order_relaxed:
                            return InterlockedDecrement8NoFence(target);
                        case ::util::lock::memory_order_acquire:
                            return InterlockedDecrement8Acquire(target);
                        case ::util::lock::memory_order_release:
                            return InterlockedDecrement8Release(target);
                        default:
                            return InterlockedDecrement8(target);
                    }
                }

                static opr_t add(volatile opr_t * target, opr_t value, ::util::lock::memory_order order) {
                    switch(order) {
                        case ::util::lock::memory_order_relaxed:
                            return InterlockedAdd8NoFence(target, value);
                        case ::util::lock::memory_order_acquire:
                            return InterlockedAdd8Acquire(target, value);
                        case ::util::lock::memory_order_release:
                            return InterlockedAdd8Release(target, value);
                        default:
                            return InterlockedAdd8(target, value);
                    }
                }

                static opr_t sub(volatile opr_t * target, opr_t value, ::util::lock::memory_order order) {
                    switch(order) {
                        case ::util::lock::memory_order_relaxed:
                            return InterlockedAdd8NoFence(target, ~(value - 1);
                        case ::util::lock::memory_order_acquire:
                            return InterlockedAdd8Acquire(target, ~(value - 1));
                        case ::util::lock::memory_order_release:
                            return InterlockedAdd8Release(target, ~(value - 1));
                        default:
                            return InterlockedAdd8(target, ~(value - 1));
                    }
                }

                static opr_t and(volatile opr_t * target, opr_t value, ::util::lock::memory_order order) {
                    switch(order) {
                        case ::util::lock::memory_order_relaxed:
                            return InterlockedAnd8NoFence(target, value);
                        case ::util::lock::memory_order_acquire:
                            return InterlockedAnd8Acquire(target, value);
                        case ::util::lock::memory_order_release:
                            return InterlockedAnd8Release(target, value);
                        default:
                            return InterlockedAnd8(target, value);
                    }
                }

                static opr_t or(volatile opr_t * target, opr_t value, ::util::lock::memory_order order) {
                    switch(order) {
                        case ::util::lock::memory_order_relaxed:
                            return InterlockedOr8NoFence(target, value);
                        case ::util::lock::memory_order_acquire:
                            return InterlockedOr8Acquire(target, value);
                        case ::util::lock::memory_order_release:
                            return InterlockedOr8Release(target, value);
                        default:
                            return InterlockedOr8(target, value);
                    }
                }

                static opr_t xor(volatile opr_t * target, opr_t value, ::util::lock::memory_order order) {
                    switch(order) {
                        case ::util::lock::memory_order_relaxed:
                            return InterlockedXor8NoFence(target, value);
                        case ::util::lock::memory_order_acquire:
                            return InterlockedXor8Acquire(target, value);
                        case ::util::lock::memory_order_release:
                            return InterlockedXor8Release(target, value);
                        default:
                            return InterlockedXor8(target, value);
                    }
                }
            };

            template<>
            struct atomic_msvc_oprs<2> {
                typedef SHORT opr_t;

                static opr_t exchange(volatile opr_t * target, opr_t value, ::util::lock::memory_order order) {
                    switch(order) {
                        case ::util::lock::memory_order_relaxed:
                            return InterlockedExchange16NoFence(target, value);
                        case ::util::lock::memory_order_acquire:
                            return InterlockedExchange16Acquire(target, value);
                        case ::util::lock::memory_order_release:
                            return InterlockedExchange16Release(target, value);
                        default:
                            return InterlockedExchange16(target, value);
                    }
                }

                static opr_t cas(volatile opr_t * target, opr_t value, opr_t expected, ::util::lock::memory_order order) {
                    switch(order) {
                        case ::util::lock::memory_order_relaxed:
                            return InterlockedCompareExchange16NoFence(target, value, expected);
                        case ::util::lock::memory_order_acquire:
                            return InterlockedCompareExchange16Acquire(target, value, expected);
                        case ::util::lock::memory_order_release:
                            return InterlockedCompareExchange16Release(target, value, expected);
                        default:
                            return InterlockedCompareExchange16(target, value, expected);
                    }
                }

                static opr_t inc(volatile opr_t * target, ::util::lock::memory_order order) {
                    switch(order) {
                        case ::util::lock::memory_order_relaxed:
                            return InterlockedIncrement16NoFence(target);
                        case ::util::lock::memory_order_acquire:
                            return InterlockedIncrement16Acquire(target);
                        case ::util::lock::memory_order_release:
                            return InterlockedIncrement16Release(target);
                        default:
                            return InterlockedIncrement16(target);
                    }
                }

                static opr_t dec(volatile opr_t * target, ::util::lock::memory_order order) {
                    switch(order) {
                        case ::util::lock::memory_order_relaxed:
                            return InterlockedDecrement16NoFence(target);
                        case ::util::lock::memory_order_acquire:
                            return InterlockedDecrement16Acquire(target);
                        case ::util::lock::memory_order_release:
                            return InterlockedDecrement16Release(target);
                        default:
                            return InterlockedDecrement16(target);
                    }
                }

                static opr_t add(volatile opr_t * target, opr_t value, ::util::lock::memory_order order) {
                    switch(order) {
                        case ::util::lock::memory_order_relaxed:
                            return InterlockedAdd16NoFence(target, value);
                        case ::util::lock::memory_order_acquire:
                            return InterlockedAdd16Acquire(target, value);
                        case ::util::lock::memory_order_release:
                            return InterlockedAdd16Release(target, value);
                        default:
                            return InterlockedAdd16(target, value);
                    }
                }

                static opr_t sub(volatile opr_t * target, opr_t value, ::util::lock::memory_order order) {
                    switch(order) {
                        case ::util::lock::memory_order_relaxed:
                            return InterlockedAdd16NoFence(target, ~(value - 1);
                        case ::util::lock::memory_order_acquire:
                            return InterlockedAdd16Acquire(target, ~(value - 1));
                        case ::util::lock::memory_order_release:
                            return InterlockedAdd16Release(target, ~(value - 1));
                        default:
                            return InterlockedAdd16(target, ~(value - 1));
                    }
                }

                static opr_t and(volatile opr_t * target, opr_t value, ::util::lock::memory_order order) {
                    switch(order) {
                        case ::util::lock::memory_order_relaxed:
                            return InterlockedAnd16NoFence(target, value);
                        case ::util::lock::memory_order_acquire:
                            return InterlockedAnd16Acquire(target, value);
                        case ::util::lock::memory_order_release:
                            return InterlockedAnd16Release(target, value);
                        default:
                            return InterlockedAnd16(target, value);
                    }
                }

                static opr_t or(volatile opr_t * target, opr_t value, ::util::lock::memory_order order) {
                    switch(order) {
                        case ::util::lock::memory_order_relaxed:
                            return InterlockedOr16NoFence(target, value);
                        case ::util::lock::memory_order_acquire:
                            return InterlockedOr16Acquire(target, value);
                        case ::util::lock::memory_order_release:
                            return InterlockedOr16Release(target, value);
                        default:
                            return InterlockedOr16(target, value);
                    }
                }

                static opr_t xor(volatile opr_t * target, opr_t value, ::util::lock::memory_order order) {
                    switch(order) {
                        case ::util::lock::memory_order_relaxed:
                            return InterlockedXor16NoFence(target, value);
                        case ::util::lock::memory_order_acquire:
                            return InterlockedXor16Acquire(target, value);
                        case ::util::lock::memory_order_release:
                            return InterlockedXor16Release(target, value);
                        default:
                            return InterlockedXor16(target, value);
                    }
                }
            };

            template<>
            struct atomic_msvc_oprs<8> {
                typedef LONGLONG opr_t;

                static opr_t exchange(volatile opr_t * target, opr_t value, ::util::lock::memory_order order) {
                    switch(order) {
                        case ::util::lock::memory_order_relaxed:
                            return InterlockedExchangeNoFence64(target, value);
                        case ::util::lock::memory_order_acquire:
                            return InterlockedExchangeAcquire64(target, value);
                        case ::util::lock::memory_order_release:
                            return InterlockedExchangeRelease64(target, value);
                        default:
                            return InterlockedExchange64(target, value);
                    }
                }

                static opr_t cas(volatile opr_t * target, opr_t value, opr_t expected, ::util::lock::memory_order order) {
                    switch(order) {
                        case ::util::lock::memory_order_relaxed:
                            return InterlockedCompareExchangeNoFence64(target, value, expected);
                        case ::util::lock::memory_order_acquire:
                            return InterlockedCompareExchangeAcquire64(target, value, expected);
                        case ::util::lock::memory_order_release:
                            return InterlockedCompareExchangeRelease64(target, value, expected);
                        default:
                            return InterlockedCompareExchange64(target, value, expected);
                    }
                }

                static opr_t inc(volatile opr_t * target, ::util::lock::memory_order order) {
                    switch(order) {
                        case ::util::lock::memory_order_relaxed:
                            return InterlockedIncrementNoFence64(target);
                        case ::util::lock::memory_order_acquire:
                            return InterlockedIncrementAcquire64(target);
                        case ::util::lock::memory_order_release:
                            return InterlockedIncrementRelease64(target);
                        default:
                            return InterlockedIncrement64(target);
                    }
                }

                static opr_t dec(volatile opr_t * target, ::util::lock::memory_order order) {
                    switch(order) {
                        case ::util::lock::memory_order_relaxed:
                            return InterlockedDecrementNoFence64(target);
                        case ::util::lock::memory_order_acquire:
                            return InterlockedDecrementAcquire64(target);
                        case ::util::lock::memory_order_release:
                            return InterlockedDecrementRelease64(target);
                        default:
                            return InterlockedDecrement64(target);
                    }
                }

                static opr_t add(volatile opr_t * target, opr_t value, ::util::lock::memory_order order) {
                    switch(order) {
                        case ::util::lock::memory_order_relaxed:
                            return InterlockedAddNoFence64(target, value);
                        case ::util::lock::memory_order_acquire:
                            return InterlockedAddAcquire64(target, value);
                        case ::util::lock::memory_order_release:
                            return InterlockedAddRelease64(target, value);
                        default:
                            return InterlockedAdd64(target, value);
                    }
                }

                static opr_t sub(volatile opr_t * target, opr_t value, ::util::lock::memory_order order) {
                    switch(order) {
                        case ::util::lock::memory_order_relaxed:
                            return InterlockedAddNoFence64(target, ~(value - 1);
                        case ::util::lock::memory_order_acquire:
                            return InterlockedAddAcquire64(target, ~(value - 1));
                        case ::util::lock::memory_order_release:
                            return InterlockedAddRelease64(target, ~(value - 1));
                        default:
                            return InterlockedAdd64(target, ~(value - 1));
                    }
                }

                static opr_t and(volatile opr_t * target, opr_t value, ::util::lock::memory_order order) {
                    switch(order) {
                        case ::util::lock::memory_order_relaxed:
                            return InterlockedAnd64NoFence(target, value);
                        case ::util::lock::memory_order_acquire:
                            return InterlockedAnd64Acquire(target, value);
                        case ::util::lock::memory_order_release:
                            return InterlockedAnd64Release(target, value);
                        default:
                            return InterlockedAnd64(target, value);
                    }
                }

                static opr_t or(volatile opr_t * target, opr_t value, ::util::lock::memory_order order) {
                    switch(order) {
                        case ::util::lock::memory_order_relaxed:
                            return InterlockedOr64NoFence(target, value);
                        case ::util::lock::memory_order_acquire:
                            return InterlockedOr64Acquire(target, value);
                        case ::util::lock::memory_order_release:
                            return InterlockedOr64Release(target, value);
                        default:
                            return InterlockedOr64(target, value);
                    }
                }

                static opr_t xor(volatile opr_t * target, opr_t value, ::util::lock::memory_order order) {
                    switch(order) {
                        case ::util::lock::memory_order_relaxed:
                            return InterlockedXor64NoFence(target, value);
                        case ::util::lock::memory_order_acquire:
                            return InterlockedXor64Acquire(target, value);
                        case ::util::lock::memory_order_release:
                            return InterlockedXor64Release(target, value);
                        default:
                            return InterlockedXor64(target, value);
                    }
                }
            };

            template<int INT_SIZE>
            struct atomic_msvc_oprs {
                typedef LONG opr_t;

                static opr_t exchange(volatile opr_t * target, opr_t value, ::util::lock::memory_order order) {
                    switch(order) {
                        case ::util::lock::memory_order_relaxed:
                            return InterlockedExchangeNoFence(target, value);
                        case ::util::lock::memory_order_acquire:
                            return InterlockedExchangeAcquire(target, value);
                        case ::util::lock::memory_order_release:
                            return InterlockedExchangeRelease(target, value);
                        default:
                            return InterlockedExchange(target, value);
                    }
                }

                static opr_t cas(volatile opr_t * target, opr_t value, opr_t expected, ::util::lock::memory_order order) {
                    switch(order) {
                        case ::util::lock::memory_order_relaxed:
                            return InterlockedCompareExchangeNoFence(target, value, expected);
                        case ::util::lock::memory_order_acquire:
                            return InterlockedCompareExchangeAcquire(target, value, expected);
                        case ::util::lock::memory_order_release:
                            return InterlockedCompareExchangeRelease(target, value, expected);
                        default:
                            return InterlockedCompareExchange(target, value, expected);
                    }
                }

                static opr_t inc(volatile opr_t * target, ::util::lock::memory_order order) {
                    switch(order) {
                        case ::util::lock::memory_order_relaxed:
                            return InterlockedIncrementNoFence(target);
                        case ::util::lock::memory_order_acquire:
                            return InterlockedIncrementAcquire(target);
                        case ::util::lock::memory_order_release:
                            return InterlockedIncrementRelease(target);
                        default:
                            return InterlockedIncrement(target);
                    }
                }

                static opr_t dec(volatile opr_t * target, ::util::lock::memory_order order) {
                    switch(order) {
                        case ::util::lock::memory_order_relaxed:
                            return InterlockedDecrementNoFence(target);
                        case ::util::lock::memory_order_acquire:
                            return InterlockedDecrementAcquire(target);
                        case ::util::lock::memory_order_release:
                            return InterlockedDecrementRelease(target);
                        default:
                            return InterlockedDecrement(target);
                    }
                }

                static opr_t add(volatile opr_t * target, opr_t value, ::util::lock::memory_order order) {
                    switch(order) {
                        case ::util::lock::memory_order_relaxed:
                            return InterlockedAddNoFence(target, value);
                        case ::util::lock::memory_order_acquire:
                            return InterlockedAddAcquire(target, value);
                        case ::util::lock::memory_order_release:
                            return InterlockedAddRelease(target, value);
                        default:
                            return InterlockedAdd(target, value);
                    }
                }

                static opr_t sub(volatile opr_t * target, opr_t value, ::util::lock::memory_order order) {
                    switch(order) {
                        case ::util::lock::memory_order_relaxed:
                            return InterlockedAddNoFence(target, ~(value - 1);
                        case ::util::lock::memory_order_acquire:
                            return InterlockedAddAcquire(target, ~(value - 1));
                        case ::util::lock::memory_order_release:
                            return InterlockedAddRelease(target, ~(value - 1));
                        default:
                            return InterlockedAdd(target, ~(value - 1));
                    }
                }

                static opr_t and(volatile opr_t * target, opr_t value, ::util::lock::memory_order order) {
                    switch(order) {
                        case ::util::lock::memory_order_relaxed:
                            return InterlockedAndNoFence(target, value);
                        case ::util::lock::memory_order_acquire:
                            return InterlockedAndAcquire(target, value);
                        case ::util::lock::memory_order_release:
                            return InterlockedAndRelease(target, value);
                        default:
                            return InterlockedAnd(target, value);
                    }
                }

                static opr_t or(volatile opr_t * target, opr_t value, ::util::lock::memory_order order) {
                    switch(order) {
                        case ::util::lock::memory_order_relaxed:
                            return InterlockedOrNoFence(target, value);
                        case ::util::lock::memory_order_acquire:
                            return InterlockedOrAcquire(target, value);
                        case ::util::lock::memory_order_release:
                            return InterlockedOrRelease(target, value);
                        default:
                            return InterlockedOr(target, value);
                    }
                }

                static opr_t xor(volatile opr_t * target, opr_t value, ::util::lock::memory_order order) {
                    switch(order) {
                        case ::util::lock::memory_order_relaxed:
                            return InterlockedXorNoFence(target, value);
                        case ::util::lock::memory_order_acquire:
                            return InterlockedXorAcquire(target, value);
                        case ::util::lock::memory_order_release:
                            return InterlockedXorRelease(target, value);
                        default:
                            return InterlockedXor(target, value);
                    }
                }
            };
        }
    }
}

#endif /* _UTIL_LOCK_ATOMIC_INT_TYPE_MSVC_IMPL_H_ */
