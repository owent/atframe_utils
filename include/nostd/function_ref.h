// Copyright 2024 atframework
// Created by owent on 2024-05-20

#pragma once

#include <config/atframe_utils_build_feature.h>
#include <config/compile_optimize.h>

#include <assert.h>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#include "nostd/type_traits.h"

LIBATFRAME_UTILS_NAMESPACE_BEGIN
namespace nostd {

namespace details {
struct UTIL_SYMBOL_VISIBLE functional_ref_void_ptr {
  const void* obj;
  void (*fn)();
};

// Chooses the best type for passing T as an argument.
// Attempt to be close to SystemV AMD64 ABI. Objects with trivial copy ctor are
// passed by value.
template <class T, bool IsLValueReference = ::std::is_lvalue_reference<T>::value>
struct UTIL_SYMBOL_VISIBLE functional_ref_pass_by_value : ::std::false_type {};

template <class T>
struct UTIL_SYMBOL_VISIBLE functional_ref_pass_by_value<T, /*IsLValueReference=*/false> :
#if (defined(__cplusplus) && __cplusplus >= 201402L) || ((defined(_MSVC_LANG) && _MSVC_LANG >= 201402L))
    ::std::integral_constant<
        bool, ::std::is_trivially_copy_constructible<T>::value&& ::std::is_trivially_copy_assignable<
                  typename ::std::remove_cv<T>::type>::value&& ::std::is_trivially_destructible<T>::value &&
                  sizeof(T) <= 2 * sizeof(void*)>
#else
    ::std::integral_constant<bool, ::std::is_trivial<T>::value && sizeof(T) <= 2 * sizeof(void*)>
#endif
{
};

template <class T>
struct UTIL_SYMBOL_VISIBLE functional_ref_forward_type
    : public ::std::conditional<functional_ref_pass_by_value<T>::value, T, T&&> {};

template <class R, class... Args>
using functional_ref_invoker = R (*)(functional_ref_void_ptr, typename functional_ref_forward_type<Args>::type...);

template <class R>
struct UTIL_SYMBOL_VISIBLE functional_ref_invoker_action;

//
// InvokeObject and InvokeFunction provide static "Invoke" functions that can be
// used as Invokers for objects or functions respectively.
//
// static_cast<R> handles the case the return type is void.
template <typename Obj, typename R, typename... Args>
UTIL_SYMBOL_VISIBLE R functional_ref_invoke_object(functional_ref_void_ptr ptr,
                                                   typename functional_ref_forward_type<Args>::type... args) {
  auto o = static_cast<const Obj*>(ptr.obj);
  return static_cast<R>(::LIBATFRAME_UTILS_NAMESPACE_ID::nostd::invoke(*o, ::std::forward<Args>(args)...));
}

template <typename Fun, typename R, typename... Args>
UTIL_SYMBOL_VISIBLE R functional_ref_invoke_function(functional_ref_void_ptr ptr,
                                                     typename functional_ref_forward_type<Args>::type... args) {
  auto f = reinterpret_cast<Fun>(ptr.fn);
  return static_cast<R>(::LIBATFRAME_UTILS_NAMESPACE_ID::nostd::invoke(f, ::std::forward<Args>(args)...));
}

template <typename Sig>
UTIL_SYMBOL_VISIBLE void functional_ref_assert_non_null(const std::function<Sig>& f) {
  assert(f != nullptr);
  (void)f;
}

template <typename F>
UTIL_SYMBOL_VISIBLE void functional_ref_assert_non_null(const F&) {}

template <typename F, typename C>
UTIL_SYMBOL_VISIBLE void functional_ref_assert_non_null(F C::*f) {
  assert(f != nullptr);
  (void)f;
}

}  // namespace details

// function_ref
//
// Dummy class declaration to allow the partial specialization based on function
// types below.
template <class T>
class UTIL_SYMBOL_VISIBLE function_ref;

// function_ref
//
// An `nostd::function_ref` is a lightweight wrapper to any invocable object with
// a compatible signature. Generally, an `nostd::function_ref` should only be used
// as an argument type and should be preferred as an argument over a const
// reference to a `std::function`. `nostd::function_ref` itself does not allocate,
// although the wrapped invocable may.
//
// Example:
//
//   // The following function takes a function callback by const reference
//   bool Visitor(const std::function<void(my_proto&,
//                                         nostd::string_view)>& callback);
//
//   // Assuming that the function is not stored or otherwise copied, it can be
//   // replaced by an `nostd::function_ref`:
//   bool Visitor(nostd::function_ref<void(my_proto&, nostd::string_view)>
//                  callback);
//
// Note: the assignment operator within an `nostd::function_ref` is intentionally
// deleted to prevent misuse; because the `nostd::function_ref` does not own the
// underlying type, assignment likely indicates misuse.
template <class R, class... Args>
class UTIL_SYMBOL_VISIBLE function_ref<R(Args...)> {
 private:
  // Used to disable constructors for objects that are not compatible with the
  // signature of this function_ref.
  template <class F, class FR = invoke_result_t<F, Args&&...>>
  using enable_if_compatible =
      typename ::std::enable_if<std::is_void<R>::value || ::std::is_convertible<FR, R>::value, void>::type;

 public:
  function_ref(std::nullptr_t) = delete;  // NOLINTNEXTLINE(runtime/explicit)

  // To help prevent subtle lifetime bugs, function_ref is not assignable.
  // Typically, it should only be used as an argument type.
  function_ref& operator=(const function_ref& rhs) = delete;
  function_ref(const function_ref& rhs) = default;

  // Constructs a function_ref from any invocable type.
  template <class F, class = enable_if_compatible<const F&>>
  // NOLINTNEXTLINE(runtime/explicit)
  function_ref(const F& f UTIL_ATTRIBUTE_LIFETIME_BOUND)
      : invoker_(&details::functional_ref_invoke_object<F, R, Args...>) {
    details::functional_ref_assert_non_null(f);
    ptr_.obj = &f;
  }

  // Overload for function pointers. This eliminates a level of indirection that
  // would happen if the above overload was used (it lets us store the pointer
  // instead of a pointer to a pointer).
  //
  // This overload is also used for references to functions, since references to
  // functions can decay to function pointers implicitly.
  template <class F, class = enable_if_compatible<F*>, enable_if_t<is_function<F>::value, int> = 0>
  function_ref(F* f)  // NOLINT(runtime/explicit)
      : invoker_(&details::functional_ref_invoke_function<F*, R, Args...>) {
    assert(f != nullptr);
    ptr_.fn = reinterpret_cast<decltype(ptr_.fn)>(f);
  }

  // Call the underlying object.
  R operator()(Args... args) const { return invoker_(ptr_, std::forward<Args>(args)...); }

 private:
  details::functional_ref_void_ptr ptr_;
  details::functional_ref_invoker<R, Args...> invoker_;
};

// Allow const qualified function signatures. Since FunctionRef requires
// constness anyway we can just make this a no-op.
template <typename R, typename... Args>
class function_ref<R(Args...) const> : public function_ref<R(Args...)> {
 public:
  using function_ref<R(Args...)>::function_ref;
};

}  // namespace nostd
LIBATFRAME_UTILS_NAMESPACE_END
