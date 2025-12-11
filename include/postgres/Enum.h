#pragma once

#include <string>

#include "./Types.h"

namespace postgres {

struct Enum {
  pg_types::Text value;
};

} // namespace postgres

#define POSTGRES_CXX_ENUM(CXXName, PqlName)                                    \
  struct CXXName : postgres::Enum {                                            \
    static constexpr const char *name = PqlName;                               \
    static constexpr PgType pg_type = PgType::Enum;                            \
  }
