#pragma once

#include <exception>
#include <functional>
#include <string>
#include <tuple>
#include <typeinfo>
#include <variant>

namespace inline_try {

struct StdExceptionWrapper {
  StdExceptionWrapper(const std::exception& e) : m_msg(e.what()), m_type(&typeid(e)) {}
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

template <typename T, typename... Es>
struct ValueOrException {
  ValueOrException(T&& t) : m_variant(std::in_place_index_t<0>{}, std::forward<T>(t)) {}
  template <size_t E_IDX, typename E>
    requires(sizeof...(Es) > 1)
  ValueOrException(std::in_place_index_t<E_IDX>, E&& e)
      : m_variant(std::in_place_index_t<1>{}, std::in_place_index_t<E_IDX>{}, std::forward<E>(e)) {}
  template <typename E>
    requires(sizeof...(Es) == 1)
  ValueOrException(std::in_place_index_t<0>, E&& e)
      : m_variant(std::in_place_index_t<1>{}, std::forward<E>(e)) {}

  bool has_value() const noexcept { return m_variant.index() == 0; }
  operator bool() const noexcept { return has_value(); }

  T& value() noexcept requires(!std::is_void_v<T>) {
    static_assert(!std::is_void_v<T>);
    if (m_variant.index() != 0) {
      abort();
    }
    return std::get<0>(m_variant);
  }
  const T& value() const noexcept requires(!std::is_void_v<T>) {
    if (m_variant.index() != 0) {
      abort();
    }
    return std::get<0>(m_variant);
  }
  T& operator*() noexcept requires(!std::is_void_v<T>) { return value(); }
  const T& operator*() const noexcept requires(!std::is_void_v<T>) { return value(); }

  using ExceptionVariant = std::variant<typename TransformException<Es>::type...>;
  using ExceptionType = std::conditional_t<
      sizeof...(Es) == 1,
      std::variant_alternative_t<0, ExceptionVariant>,
      ExceptionVariant>;

  const ExceptionType& exception() const noexcept {
    if (m_variant.index() != 1) {
      abort();
    }
    return std::get<1>(m_variant);
  }
  ExceptionType& exception() noexcept {
    if (m_variant.index() != 1) {
      abort();
    }
    return std::get<1>(m_variant);
  }

 private:
  void check_has_exception() const {
    if (has_value()) {
      abort();
    }
  }

  using VoidToMono = std::conditional_t<std::is_void_v<T>, std::monostate, T>;
  using ValueType = std::conditional_t<
      std::is_reference_v<VoidToMono>,
      std::reference_wrapper<std::remove_reference_t<VoidToMono>>,
      VoidToMono>;

  std::variant<ValueType, ExceptionType> m_variant;
};

template <typename... Es>
  requires(ARE_EXCEPTIONS_OK<Es...>)
struct InlineTry {
 private:
  static constexpr size_t NUM_EXCEPTIONS = sizeof...(Es);

  template <size_t E_IDX>
    requires(E_IDX < NUM_EXCEPTIONS)
  using NthException = std::tuple_element<E_IDX, std::tuple<Es...>>::type;

  static constexpr bool FINAL_CATCH_ALL =
      NUM_EXCEPTIONS > 0 && std::is_void_v<NthException<NUM_EXCEPTIONS - 1>>;

  template <typename Fn>
  using ResT = ValueOrException<decltype(std::declval<Fn>()()), Es...>;

  template <typename Fn, size_t E_IDX>
  static ResT<Fn> call_impl(Fn&& fn) {
    using CurrException = NthException<E_IDX>;
    try {
      if constexpr (E_IDX == 0) {
        return ResT<Fn>(std::forward<Fn>(fn)());
      } else {
        return call_impl<Fn, E_IDX - 1>(std::move(fn));
      }
    } catch (CurrException& e) {
      return ResT<Fn>(std::in_place_index_t<E_IDX>{}, std::move(e));
    }
  }

  template <typename Fn>
    requires(FINAL_CATCH_ALL)
  static ResT<Fn> call_impl_void(Fn&& fn) noexcept {
    constexpr size_t E_IDX = NUM_EXCEPTIONS - 1;
    try {
      if constexpr (E_IDX == 0) {
        return ResT<Fn>(std::forward<Fn>(fn)());
      } else {
        return call_impl<Fn, E_IDX - 1>(std::move(fn));
      }
    } catch (...) {
      return ResT<Fn>(std::in_place_index_t<E_IDX>{}, std::monostate{});
    }
  }

 public:
  template <typename Fn>
    requires(!FINAL_CATCH_ALL)
  static ResT<Fn> call(Fn&& fn) { return call_impl<Fn, NUM_EXCEPTIONS - 1>(std::forward<Fn>(fn)); }

  template <typename Fn>
  static ResT<Fn> call(Fn&& fn) noexcept requires(FINAL_CATCH_ALL) {
    return call_impl_void<Fn>(std::forward<Fn>(fn));
  }

  template <typename Fn>
  static auto wrap(Fn&& fn) {
    return [fn = std::forward<Fn>(fn)]<typename... Args>(Args && ... args) {
      return call([=]() -> decltype(fn(std::forward<Args>(args)...)) { return fn(args...); });
    };
  }
};

}  // namespace inline_try
