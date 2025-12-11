#pragma once

#include <string>

#include "./Types.h"
#include <postgres/Oid.h>

namespace postgres {

struct Enum : CustomType {
  pg_types::Text value;

  Enum(pg_types::Text &&value) : value{std::move(value)} {};
  Enum(const pg_types::Text &value) : value{std::move(value)} {};
};

} // namespace postgres

template <typename T>
concept IsPostgresCXXEnum = std::is_base_of_v<postgres::Enum, T>;

#define OID_STORAGE_NAME_ENUM(Name) _global_oid_storage_##Name##_Enum

#define POSTGRES_CXX_ENUM(CXXName, PqlName)                                    \
  static std::optional<Oid> OID_STORAGE_NAME_ENUM(CXXName) = std::nullopt;     \
  struct CXXName final : postgres::Enum {                                      \
    static constexpr const char *name = PqlName;                               \
    static constexpr PgType pg_type = PgType::Enum;                            \
    static Oid enum_oid() {                                                    \
      return get_oid(&OID_STORAGE_NAME_ENUM(CXXName));                         \
    };                                                                         \
    static void set_enum_oid(Oid oid) {                                        \
      set_oid(&OID_STORAGE_NAME_ENUM(CXXName), oid);                           \
    };                                                                         \
    CXXName(pg_types::Text &&value) : postgres::Enum{std::move(value)} {};     \
    CXXName(const pg_types::Text &value)                                       \
        : postgres::Enum{std::move(value)} {};                                 \
  }
