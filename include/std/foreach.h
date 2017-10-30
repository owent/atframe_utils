/**
 * @file foreach.hpp
 * @brief foreach 定义
 *        支持VC++、GCC
 *        foreach分支
 *        1. 在编译器支持并启用了C++11标准是会使用C++11的 range-based-loop
 *        2. 老版本的VC在支持for each(var obj in arr)时会使用VC编译器的for each语法
 *        3. 如果项目使用了BOOST库，会使用BOOST_FOREACH
 *        4. 如果以上情况都不满足，则会使用自己实现的简单foreach [仅在 GCC 4.1.2 4.4.5 4.7.1 和 VC 11.0 RC 下测试过]
 *
 *
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author OWenT, owt5008137@live.com
 * @date 2012.06.27
 * @example
 *       //数组
 *       const int arr[] = {1, 7, 3, 9, 5, 6, 2, 8, 4};
 *       owent_foreach(const int& v, arr) {
 *           printf("%d\n", v);
 *       }
 *
 *       //std::vector
 *       std::vector<int> vec;
 *       //...
 *       owent_foreach(const int& v, vec) {
 *           printf("%d\n", v);
 *       }
 *
 *       //std::map
 *       std::map<int, int> mp;
 *       //...
 *       typedef std::pair<const int, int> map_pair; // 由于foreach是宏定义，所以类型里带逗号的话必须这么处理，否则编译器会认为这个逗号是参数分隔符
 *       owent_foreach(map_pair& pr, mp) {
 *           pr.second = 0;
 *       }
 *
 * @history
 *   2012.07.19 增加对有const申明的stl容器的支持
 *
 */
 
#ifndef owent_foreach
 
# pragma once
 
// ============================================================
// 公共基础库
// foreach，实现原理类似boost的foreach
// 自动采用编译器提供的foreach功能
// ============================================================
  
/**
 * foreach函数
 * 如果是G++且版本高于4.6且开启了c++0x或c++11, 或者是支持C++11的VC++
 * 则会启用C++11标准的range-based-for循环
 *
 * 如果是VC++且版本高于9.0 SP1
 * 则会启用VC++的 for each (object var in collection_to_loop)
 *
 * 如果启用了BOOST库 (增加宏定义 FOREACH_WITH_BOOST_HPP )
 * 则会使用BOOST_FOREACH
 *
 * 否则自定义foreach方法
 */
  
// VC11.0 SP1以上分支判断
#if defined(_MSC_VER) && (_MSC_VER > 1500 || (_MSC_VER == 1500 && defined (_HAS_TR1)))
    #if _MSC_VER >= 1700
        // 采用 VC 的range-based-for循环
        #define owent_foreach(VAR, COL) for(VAR : COL)
    #else
        // 采用 VC 的 for each (object var in collection_to_loop)
        #define owent_foreach(VAR, COL) for each (VAR in COL)
    #endif
#elif defined(__clang__) && __clang_major__ >= 3 && defined(__cplusplus) && __cplusplus >= 201103L
    // 采用 clang 的range-based-for循环
    #define owent_foreach(VAR, COL) for(VAR : COL)
#elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)) && (__cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__))
    // 采用G++ 的range-based-for循环
    #define owent_foreach(VAR, COL) for(VAR : COL)
#elif defined(OWENT_WITH_BOOST_HPP) && defined(OWENT_ENABLE_BOOST_FOREACH)
    #include <boost/foreach.hpp>
    #define owent_foreach(VAR, COL) BOOST_FOREACH(VAR, COL)
#else
    #include <cstddef>
    // 功能受限的foreach函数
    namespace foreach_detail{
        ///////////////////////////////////////////////////////////////////////////////
        // 自动类型
        ///////////////////////////////////////////////////////////////////////////////
        struct auto_any_base
        {
            // 用于宏定义中的初始化判断
            operator bool() const
            {
                return false;
            }
        };
 
        template<typename T>
        struct auto_any : auto_any_base
        {
            explicit auto_any(T const &t)
              : item(t)
            {
            }
 
            // 声明为永久可变
            mutable T item;
        };
 
         // 设置flag为false
        inline bool set_false(bool &b)
        {
            b = false;
            return false;
        }


        typedef auto_any_base const& auto_any_t;
        #define OWENT_FOREACH_ANY_TYPE(x) auto_any<x>
        #define OWENT_FOREACH_ARRAY_DECL(x, y, z) x (&y)[z]
        #define OWENT_FOREACH_ALLOC_DECL(x, y) x& y
        #if defined(_MSC_VER) && _MSC_VER <= 1500
        // 某些版本的VC,由于语句块变量作用域的问题(比如VC 6)
        // 需要使用__LINE__来创建唯一标识符（参自BOOST_FOREACH）
            #define OWENT_FOREACH_CAT(x, y) x ## y
            #define OWENT_FOREACH_ID(x) OWENT_FOREACH_CAT(x, __LINE__)
        #else
            #define OWENT_FOREACH_ID(x) x
        #endif
 
        template<typename _Ty> inline
        _Ty& auto_any_cast(auto_any_t a)
        {
            return static_cast<auto_any<_Ty> const &>(a).item;
        }
 
        /////////////////////////////////////////////////////////////////////////////
        // 获取开始位置
        /////////////////////////////////////////////////////////////////////////////
     
        // 模板类迭代器
        template<typename _Ty> inline
        OWENT_FOREACH_ANY_TYPE(typename _Ty::const_iterator) begin(OWENT_FOREACH_ALLOC_DECL(const _Ty, c))
        {   // 获取容器起始迭代器
            return OWENT_FOREACH_ANY_TYPE(typename _Ty::const_iterator)(c.begin());
        }
 
        template<typename _Ty> inline
        OWENT_FOREACH_ANY_TYPE(typename _Ty::iterator) begin(OWENT_FOREACH_ALLOC_DECL(_Ty, c))
        {   // 获取容器起始迭代器
            return OWENT_FOREACH_ANY_TYPE(typename _Ty::iterator)(c.begin());
        }
 
        // 数组指针
        template<typename _Ty, std::size_t _Size> inline
        OWENT_FOREACH_ANY_TYPE(const _Ty*) begin(OWENT_FOREACH_ARRAY_DECL(const _Ty, arr, _Size))
        {   
            return OWENT_FOREACH_ANY_TYPE(const _Ty*)(arr);
        }
 
        template<typename _Ty, std::size_t _Size> inline
        OWENT_FOREACH_ANY_TYPE(_Ty*) begin(OWENT_FOREACH_ARRAY_DECL(_Ty, arr, _Size))
        {   
            return OWENT_FOREACH_ANY_TYPE(_Ty*)(arr);
        }
 
        /////////////////////////////////////////////////////////////////////////////
        // 移动游标
        /////////////////////////////////////////////////////////////////////////////
     
        // 模板类迭代器
        template<typename _Ty> inline
        void next(OWENT_FOREACH_ALLOC_DECL(const _Ty, _Array), auto_any_t cur)
        {
            ++ auto_any_cast<typename _Ty::const_iterator>(cur);
        }
 
        template<typename _Ty> inline
        void next(OWENT_FOREACH_ALLOC_DECL(_Ty, _Array), auto_any_t cur)
        {
            ++ auto_any_cast<typename _Ty::iterator>(cur);
        }
 
        // 数组指针
        template<typename _Ty, std::size_t _Size> inline
        void next(OWENT_FOREACH_ARRAY_DECL(const _Ty, arr, _Size), auto_any_t cur)
        {
            ++ auto_any_cast<const _Ty*>(cur);
        }
 
        template<typename _Ty, std::size_t _Size> inline
        void next(OWENT_FOREACH_ARRAY_DECL(_Ty, arr, _Size), auto_any_t cur)
        {
            ++ auto_any_cast<_Ty*>(cur);
        }
     
     
        /////////////////////////////////////////////////////////////////////////////
        // 判断是否结束
        /////////////////////////////////////////////////////////////////////////////
 
        // 模板类迭代器
        template<typename _Ty> inline
        bool end(OWENT_FOREACH_ALLOC_DECL(const _Ty, _Array), auto_any_t cur)
        {
            return auto_any_cast<typename _Ty::const_iterator>(cur) == _Array.end();
        }
 
        template<typename _Ty> inline
        bool end(OWENT_FOREACH_ALLOC_DECL(_Ty, _Array), auto_any_t cur)
        {
            return auto_any_cast<typename _Ty::iterator>(cur) == _Array.end();
        }
 
        // 数组指针
        template<typename _Ty, std::size_t _Size> inline
        bool end(OWENT_FOREACH_ARRAY_DECL(const _Ty, arr, _Size), auto_any_t cur)
        {
            return static_cast<std::size_t>(auto_any_cast<const _Ty*>(cur) - arr) >= _Size;
        }
 
        template<typename _Ty, std::size_t _Size> inline
        bool end(OWENT_FOREACH_ARRAY_DECL(_Ty, arr, _Size), auto_any_t cur)
        {
            return static_cast<std::size_t>(auto_any_cast<_Ty*>(cur) - arr) >= _Size;
        }
 
        /////////////////////////////////////////////////////////////////////////////
        // 数据转换
        /////////////////////////////////////////////////////////////////////////////
     
        // 模板类迭代器内容
        template<typename _Ty> inline
        typename _Ty::const_iterator& deref(OWENT_FOREACH_ALLOC_DECL(const _Ty, arr), auto_any_t cur)
        {
            return auto_any_cast<typename _Ty::const_iterator>(cur);
        }
 
        template<typename _Ty> inline
        typename _Ty::iterator& deref(OWENT_FOREACH_ALLOC_DECL(_Ty, arr), auto_any_t cur)
        {
            return auto_any_cast<typename _Ty::iterator>(cur);
        }
 
        // 数组元素
        template<typename _Ty, std::size_t _Size> inline
        const _Ty*& deref(OWENT_FOREACH_ARRAY_DECL(const _Ty, arr, _Size), auto_any_t cur)
        {
            return auto_any_cast<const _Ty*>(cur);
        }
 
        template<typename _Ty, std::size_t _Size> inline
        _Ty*& deref(OWENT_FOREACH_ARRAY_DECL(_Ty, arr, _Size), auto_any_t cur)
        {
            return auto_any_cast<_Ty*>(cur);
        }
    }
 
    #define OWENT_FOREACH(VAR, COL)                                                                                         \
        if (foreach_detail::auto_any_t OWENT_FOREACH_ID(_owent_foreach_cur) = foreach_detail::begin(COL)){ } else           \
        for (bool OWENT_FOREACH_ID(_owent_foreach_flag) = true;                                                             \
            OWENT_FOREACH_ID(_owent_foreach_flag) && !foreach_detail::end(COL, OWENT_FOREACH_ID(_owent_foreach_cur));       \
            OWENT_FOREACH_ID(_owent_foreach_flag)? foreach_detail::next(COL, OWENT_FOREACH_ID(_owent_foreach_cur)): (void)0)\
                                                                                                                            \
            if (foreach_detail::set_false(OWENT_FOREACH_ID(_owent_foreach_flag))){ } else                                   \
            for (VAR = *foreach_detail::deref(COL, OWENT_FOREACH_ID(_owent_foreach_cur));                                   \
                !OWENT_FOREACH_ID(_owent_foreach_flag);                                                                     \
                OWENT_FOREACH_ID(_owent_foreach_flag) = true)
 
    #define owent_foreach(VAR, COL) OWENT_FOREACH(VAR, COL)
#endif
 
#endif
