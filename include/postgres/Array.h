#pragma once

#include <string>

#include "./Types.h"
#include <postgres/Oid.h>

namespace postgres {

template <typename T> struct Array : CustomType {
  std::vector<T> values;

  using UnderlyingType = T;

  Array() = default;

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

#define PG_ARRAY_TYPE_INTERNAL_TEXT "_text"

#define OID_STORAGE_NAME_ARRAY(Name) _global_oid_storage_##Name##_Array

#define POSTGRES_CXX_ARRAY(CXXName, PqlName, UnderlyingType)                   \
  static std::optional<Oid> OID_STORAGE_NAME_ARRAY(CXXName) = std::nullopt;    \
  struct CXXName final : postgres::Array<UnderlyingType> {                     \
    static constexpr const char *name = "_" PqlName;                           \
    static constexpr const char *underlying_name = PqlName;                    \
    static constexpr PgType pg_type = PgType::Array;                           \
                                                                               \
    CXXName(std::initializer_list<UnderlyingType> init)                        \
        : postgres::Array<UnderlyingType>{init} {};                            \
    static Oid array_oid() {                                                   \
      return get_oid(&OID_STORAGE_NAME_ARRAY(CXXName));                        \
    };                                                                         \
    static void set_array_oid(Oid oid) {                                       \
      set_oid(&OID_STORAGE_NAME_ARRAY(CXXName), oid);                          \
    };                                                                         \
    static void unset_array_oid() {                                             \
      unset_oid(&OID_STORAGE_NAME_ARRAY(CXXName));                              \
    };                                                                         \
  }
