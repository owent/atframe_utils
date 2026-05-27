// Copyright 2026 atframework
//
// Licenses under the MIT License
//
// @note These helpers make it easier to implement standard-compatible allocators.
// @example
//   ```cpp
//   template <class T, class BackendAllocator = ::std::allocator<T>>
//   struct custom_allocator
//       : public ::atfw::util::memory::allocator_adapter<custom_allocator, T, BackendAllocator> {
//     using base_type = ::atfw::util::memory::allocator_adapter<custom_allocator, T, BackendAllocator>;
//     using pointer = typename base_type::pointer;
//     using size_type = typename base_type::size_type;
//
//     using base_type::base_type;
//
//     pointer allocate(size_type n) { /* custom logic */ return base_type::allocate(n); }
//     void deallocate(pointer p, size_type n) { /* custom logic */ base_type::deallocate(p, n); }
//   };
//
//   using custom_allocator_traits = ::std::allocator_traits<custom_allocator<int>>;
//   ```
// @note Never specialize ::std::allocator_traits for user allocators. Standard library templates are not extension
// points; put customization on the allocator type itself and let the standard traits primary template detect it.

#pragma once

#include <config/atframe_utils_build_feature.h>

#include <config/compile_optimize.h>
#include <std/explicit_declare.h>

#include <memory>
#include <type_traits>
#include <utility>

// STL declare allocator's callbacks as constexpr since C++20, we keep the same behavior for compatibility
#if (!defined(__cplusplus) && !defined(_MSVC_LANG)) || \
    !((defined(__cplusplus) && __cplusplus >= 202002L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L))
#  define ATFRAMEWORK_UTILS_MEMORY_ALLOCATOR_CONSTEXPR
#else
#  define ATFRAMEWORK_UTILS_MEMORY_ALLOCATOR_CONSTEXPR constexpr
#endif

#if defined(_MSC_VER)
#  pragma warning(push)
#  if _MSC_VER >= 1922 && \
      ((defined(__cplusplus) && __cplusplus >= 201704L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201704L))
#    pragma warning(disable : 4996)
#  endif
#elif defined(__GNUC__) && !defined(__clang__) && !defined(__apple_build_version__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wdeprecated"
#elif defined(__clang__) || defined(__apple_build_version__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wdeprecated"
#endif

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace memory {
template <class BackendAllocator>
class __util_memory_allocator_adapter_is_always_equal {
  template <class AllocatorOther>
  static typename AllocatorOther::is_always_equal _S_test(int);

  template <class>
  static typename ::std::is_empty<BackendAllocator>::type _S_test(...);

 public:
  using type = decltype(_S_test<BackendAllocator>(0));
};

template <class LeftBackendAllocator, class RightBackendAllocator>
class __util_memory_allocator_adapter_backend_equal {
  template <class LeftAllocator, class RightAllocator>
  static auto _S_check(int)
      -> decltype(::std::declval<const LeftAllocator&>() == ::std::declval<const RightAllocator&>(),
                  ::std::true_type());

  template <class, class>
  static ::std::false_type _S_check(...);

  using has_equal = decltype(_S_check<LeftBackendAllocator, RightBackendAllocator>(0));

  static bool _S_equal(const LeftBackendAllocator& left, const RightBackendAllocator& right,
                       ::std::true_type) noexcept(noexcept(left == right)) {
    return left == right;
  }

  static bool _S_equal(const LeftBackendAllocator&, const RightBackendAllocator&, ::std::false_type) noexcept {
    return __util_memory_allocator_adapter_is_always_equal<LeftBackendAllocator>::type::value &&
           __util_memory_allocator_adapter_is_always_equal<RightBackendAllocator>::type::value;
  }

 public:
  static bool equal(const LeftBackendAllocator& left,
                    const RightBackendAllocator& right) noexcept(noexcept(_S_equal(left, right, has_equal()))) {
    return _S_equal(left, right, has_equal());
  }
};

// A CRTP-style adapter for building allocators from an existing backend allocator.
// Users inherit from allocator_adapter<user_allocator, T, BackendAllocator> and only override the operations that need
// custom behavior. The inherited nested types, rebind, construction, destruction, and backend forwarding operations are
// visible to ::std::allocator_traits<user_allocator<T, BackendAllocator>> without specializing std::allocator_traits.
template <template <class, class> class Allocator, class T, class BackendAllocator = ::std::allocator<T>>
class ATFW_UTIL_SYMBOL_VISIBLE allocator_adapter {
 public:
  using allocator_type = Allocator<T, BackendAllocator>;
  using backend_allocator_type = BackendAllocator;
  using backend_allocator_traits = ::std::allocator_traits<backend_allocator_type>;
  using value_type = T;
  using pointer = typename backend_allocator_traits::pointer;
  using const_pointer = typename backend_allocator_traits::const_pointer;
  using void_pointer = typename backend_allocator_traits::void_pointer;
  using const_void_pointer = typename backend_allocator_traits::const_void_pointer;
  using difference_type = typename backend_allocator_traits::difference_type;
  using size_type = typename backend_allocator_traits::size_type;
  using reference = value_type&;
  using const_reference = const value_type&;
  using propagate_on_container_copy_assignment =
      typename backend_allocator_traits::propagate_on_container_copy_assignment;
  using propagate_on_container_move_assignment =
      typename backend_allocator_traits::propagate_on_container_move_assignment;
  using propagate_on_container_swap = typename backend_allocator_traits::propagate_on_container_swap;
  using is_always_equal = typename __util_memory_allocator_adapter_is_always_equal<backend_allocator_type>::type;

  template <class U>
  struct rebind {
    using __rebind_backend_type_other = typename backend_allocator_traits::template rebind_alloc<U>;
    using other = Allocator<U, __rebind_backend_type_other>;
  };

  inline ATFRAMEWORK_UTILS_MEMORY_ALLOCATOR_CONSTEXPR allocator_adapter() noexcept(
      ::std::is_nothrow_default_constructible<backend_allocator_type>::value)
      : backend_allocator_() {}

  inline explicit ATFRAMEWORK_UTILS_MEMORY_ALLOCATOR_CONSTEXPR
  allocator_adapter(const backend_allocator_type& backend) noexcept(
      ::std::is_nothrow_copy_constructible<backend_allocator_type>::value)
      : backend_allocator_(backend) {}

  inline ATFRAMEWORK_UTILS_MEMORY_ALLOCATOR_CONSTEXPR allocator_adapter(const allocator_adapter&) noexcept(
      ::std::is_nothrow_copy_constructible<backend_allocator_type>::value) = default;

  inline ATFRAMEWORK_UTILS_MEMORY_ALLOCATOR_CONSTEXPR allocator_adapter(allocator_adapter&&) noexcept(
      ::std::is_nothrow_move_constructible<backend_allocator_type>::value) = default;

  inline ATFRAMEWORK_UTILS_MEMORY_ALLOCATOR_CONSTEXPR allocator_adapter& operator=(const allocator_adapter&) noexcept(
      ::std::is_nothrow_copy_assignable<backend_allocator_type>::value) = default;

  inline ATFRAMEWORK_UTILS_MEMORY_ALLOCATOR_CONSTEXPR allocator_adapter& operator=(allocator_adapter&&) noexcept(
      ::std::is_nothrow_move_assignable<backend_allocator_type>::value) = default;

  template <class U, class UBackendAllocator>
  inline ATFRAMEWORK_UTILS_MEMORY_ALLOCATOR_CONSTEXPR
  allocator_adapter(const allocator_adapter<Allocator, U, UBackendAllocator>& other) noexcept(
      ::std::is_nothrow_constructible<backend_allocator_type, const UBackendAllocator&>::value)
      : backend_allocator_(other.backend_allocator()) {}

  template <class U, class UBackendAllocator>
  inline ATFRAMEWORK_UTILS_MEMORY_ALLOCATOR_CONSTEXPR
  allocator_adapter(allocator_adapter<Allocator, U, UBackendAllocator>&& other) noexcept(
      ::std::is_nothrow_constructible<backend_allocator_type, UBackendAllocator&&>::value)
      : backend_allocator_(::std::move(other.backend_allocator())) {}

  inline ATFRAMEWORK_UTILS_MEMORY_ALLOCATOR_CONSTEXPR backend_allocator_type& backend_allocator() noexcept {
    return backend_allocator_;
  }

  inline ATFRAMEWORK_UTILS_MEMORY_ALLOCATOR_CONSTEXPR const backend_allocator_type& backend_allocator() const noexcept {
    return backend_allocator_;
  }

 private:
  template <class AllocatorOther>
  inline static ATFRAMEWORK_UTILS_MEMORY_ALLOCATOR_CONSTEXPR auto _S_allocate(AllocatorOther& allocator, size_type n,
                                                                              const_void_pointer hint, int)
      -> decltype(allocator.allocate(n, hint)) {
    return allocator.allocate(n, hint);
  }

  template <class AllocatorOther>
  inline static ATFRAMEWORK_UTILS_MEMORY_ALLOCATOR_CONSTEXPR pointer _S_allocate(AllocatorOther& allocator, size_type n,
                                                                                 const_void_pointer, ...) {
    return ::std::allocator_traits<AllocatorOther>::allocate(allocator, n);
  }

 public:
  ATFW_EXPLICIT_NODISCARD_ATTR
  inline ATFRAMEWORK_UTILS_MEMORY_ALLOCATOR_CONSTEXPR pointer allocate(size_type n) {
    return backend_allocator_traits::allocate(backend_allocator_, n);
  }

  ATFW_EXPLICIT_NODISCARD_ATTR
  inline ATFRAMEWORK_UTILS_MEMORY_ALLOCATOR_CONSTEXPR pointer allocate(size_type n, const_void_pointer hint) {
    return _S_allocate(backend_allocator_, n, hint, 0);
  }

#if ((defined(__cplusplus) && __cplusplus >= 202302L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 202302L)) && \
    defined(__cpp_lib_allocate_at_least) && __cpp_lib_allocate_at_least >= 202302L
  ATFW_EXPLICIT_NODISCARD_ATTR
  inline UTIL_CONFIG_CONSTEXPR ::std::allocation_result<pointer, size_type> allocate_at_least(size_type n) {
    return backend_allocator_traits::allocate_at_least(backend_allocator_, n);
  }
#endif

  inline ATFRAMEWORK_UTILS_MEMORY_ALLOCATOR_CONSTEXPR void deallocate(pointer p, size_type n) {
    backend_allocator_traits::deallocate(backend_allocator_, p, n);
  }

  template <class U, class... Args>
  inline UTIL_CONFIG_CONSTEXPR void construct(U* p, Args&&... args) noexcept(
      noexcept(backend_allocator_traits::construct(backend_allocator_, p, ::std::forward<Args>(args)...))) {
    backend_allocator_traits::construct(backend_allocator_, p, ::std::forward<Args>(args)...);
  }

  template <class U>
  inline UTIL_CONFIG_CONSTEXPR void destroy(U* p) noexcept(
      noexcept(backend_allocator_traits::destroy(backend_allocator_, p))) {
    backend_allocator_traits::destroy(backend_allocator_, p);
  }

  inline ATFRAMEWORK_UTILS_MEMORY_ALLOCATOR_CONSTEXPR size_type max_size() const
      noexcept(noexcept(backend_allocator_traits::max_size(backend_allocator_))) {
    return backend_allocator_traits::max_size(backend_allocator_);
  }

  inline ATFRAMEWORK_UTILS_MEMORY_ALLOCATOR_CONSTEXPR allocator_type select_on_container_copy_construction() const {
    return static_cast<const allocator_type&>(*this);
  }

 private:
  backend_allocator_type backend_allocator_;
};

template <template <class, class> class Allocator, class T, class BackendAllocator, class U, class UBackendAllocator>
ATFW_UTIL_SYMBOL_VISIBLE inline bool operator==(
    const allocator_adapter<Allocator, T, BackendAllocator>& left,
    const allocator_adapter<Allocator, U, UBackendAllocator>&
        right) noexcept(noexcept(__util_memory_allocator_adapter_backend_equal<BackendAllocator, UBackendAllocator>::
                                     equal(left.backend_allocator(), right.backend_allocator()))) {
  return __util_memory_allocator_adapter_backend_equal<BackendAllocator, UBackendAllocator>::equal(
      left.backend_allocator(), right.backend_allocator());
}

template <template <class, class> class Allocator, class T, class BackendAllocator, class U, class UBackendAllocator>
ATFW_UTIL_SYMBOL_VISIBLE inline bool operator!=(
    const allocator_adapter<Allocator, T, BackendAllocator>& left,
    const allocator_adapter<Allocator, U, UBackendAllocator>& right) noexcept(noexcept(!(left == right))) {
  return !(left == right);
}

}  // namespace memory
ATFRAMEWORK_UTILS_NAMESPACE_END

#if defined(_MSC_VER)
#  pragma warning(pop)
#elif defined(__GNUC__) && !defined(__clang__) && !defined(__apple_build_version__)
#  pragma GCC diagnostic pop
#elif defined(__clang__) || defined(__apple_build_version__)
#  pragma clang diagnostic pop
#endif
