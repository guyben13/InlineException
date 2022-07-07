#pragma once

#include <exception>
#include <string>
#include <tuple>
#include <typeinfo>
#include <variant>

namespace inline_try {

struct StdExceptionWrapper {
  StdExceptionWrapper(const std::exception& e)
      : m_msg(e.what()), m_type(&typeid(e)) {}
  const char* what() const noexcept { return m_msg.c_str(); }
  const std::type_info& type() const noexcept { return *m_type; }

 private:
  std::string m_msg;
  const std::type_info* m_type;
};

template <typename E>
struct TransformException {
  using type = E;
};

template <>
struct TransformException<void> {
  using type = std::monostate;
};

template <>
struct TransformException<std::monostate> {};

template <>
struct TransformException<std::exception> {
  using type = StdExceptionWrapper;
};

template <typename... Es>
constexpr bool ARE_EXCEPTIONS_OK = true;

template <typename... Es>
constexpr bool ARE_EXCEPTIONS_OK<std::monostate, Es...> = false;

template <typename E, typename... Es>
constexpr bool ARE_EXCEPTIONS_OK<void, E, Es...> = false;

template <typename E, typename... Es>
constexpr bool ARE_EXCEPTIONS_OK<E, Es...> = ARE_EXCEPTIONS_OK<Es...>;

template <typename... Es>
requires(ARE_EXCEPTIONS_OK<Es...>)  //
    struct InlineTry {
 private:
  static constexpr size_t NUM_EXCEPTIONS = sizeof...(Es);

  template <size_t E_IDX>
  requires(E_IDX < NUM_EXCEPTIONS)  //
      using NthException = std::tuple_element<E_IDX, std::tuple<Es...>>::type;

  static constexpr bool FINAL_CATCH_ALL =
      NUM_EXCEPTIONS > 0 && std::is_void_v<NthException<NUM_EXCEPTIONS - 1>>;

  template <typename T>
  using VoidToMono = std::conditional_t<std::is_void_v<T>, std::monostate, T>;

  template <typename T>
  using RefWrapIfNeeded = std::conditional_t<
      std::is_reference_v<T>,
      std::reference_wrapper<std::remove_reference_t<T>>,
      T>;
  template <typename Fn>
  using ResT = std::variant<
      RefWrapIfNeeded<VoidToMono<decltype(std::declval<Fn>()())>>,
      typename TransformException<Es>::type...>;

  template <typename Fn, size_t E_IDX>
  static ResT<Fn> wrap_impl(Fn&& fn) {
    using CurrException = NthException<E_IDX>;
    static_assert(
        !std::is_void_v<CurrException>,
        "void (catch-all) can only be the last exception");
    try {
      if constexpr (E_IDX == 0) {
        return ResT<Fn>(std::in_place_index_t<0>{}, std::forward<Fn>(fn)());
      } else {
        return wrap_impl<Fn, E_IDX - 1>(std::move(fn));
      }
    } catch (CurrException& e) {
      return ResT<Fn>(std::in_place_index_t<E_IDX + 1>{}, std::move(e));
    }
  }

  template <typename Fn, size_t E_IDX>
  static ResT<Fn> wrap_impl_void(Fn&& fn) noexcept {
    using CurrException = NthException<E_IDX>;
    static_assert(!std::is_same_v<CurrException, std::monostate>, "XXX");
    try {
      if constexpr (E_IDX == 0) {
        return ResT<Fn>(std::in_place_index_t<0>{}, std::forward<Fn>(fn)());
      } else {
        return wrap_impl<Fn, E_IDX - 1>(std::move(fn));
      }
    } catch (...) {
      return ResT<Fn>(std::in_place_index_t<E_IDX + 1>{}, std::monostate{});
    }
  }

 public:
  template <typename Fn>
  static ResT<Fn> wrap(Fn&& fn) requires(!FINAL_CATCH_ALL) {
    return wrap_impl<Fn, NUM_EXCEPTIONS - 1>(std::forward<Fn>(fn));
  }

  template <typename Fn>
  static ResT<Fn> wrap(Fn&& fn) noexcept requires(FINAL_CATCH_ALL) {
    return wrap_impl_void<Fn, NUM_EXCEPTIONS - 1>(std::forward<Fn>(fn));
  }
};

}  // namespace inline_try
