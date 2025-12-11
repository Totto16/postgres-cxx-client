#pragma once

#include <string>

#include "./Types.h"

namespace postgres {

template <typename T> struct Array {
  std::vector<T> values;

  using UnderlyingType = T;

  Array(std::initializer_list<T> init) : values{init} {}
};

} // namespace postgres

template <template <class...> class Template, typename T>
struct is_derived_from_template {
private:
  // Overload chosen if U* can convert to Template<Args...>* —
  // meaning U derives from Template<Args...>
  template <typename... Args>
  static std::true_type test(const Template<Args...> *);

  // Fallback
  static std::false_type test(...);

public:
  static constexpr bool value = decltype(test(std::declval<T *>()))::value;
};

template <template <class...> class Template, typename T>
inline constexpr bool is_derived_from_template_v =
    is_derived_from_template<Template, T>::value;

template <typename T>
inline constexpr bool IsPostgresCXXArray =
    is_derived_from_template<postgres::Array, T>::value;

#define PG_ARRAY_TYPE_STR(Name) "_array_type_" Name "_generated_"

#define PG_ARRAY_TYPE_INTERNAL_TEXT "_text"

#define POSTGRES_CXX_ARRAY_OF_PG_TYPE(CXXName, Pql)                            \
  POSTGRES_CXX_ARRAY(CXXName, Pql::name, Pql)



#define POSTGRES_CXX_ARRAY(CXXName, PqlName, UnderlyingType)                   \
  struct CXXName : postgres::Array<UnderlyingType> {                           \
    static constexpr std::string get_name() {return std::string{} + PG_ARRAY_TYPE_STR( + PqlName + ); }      \
    static constexpr const char *underlying_name = PqlName;                    \
    static constexpr PgType pg_type = PgType::Array;                           \
  }
