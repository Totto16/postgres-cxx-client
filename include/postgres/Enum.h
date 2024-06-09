#pragma once

#include <string>

namespace postgres {

struct Enum {
  std::string value;
};

} // namespace postgres

#define POSTGRES_CXX_ENUM(CXXName, PqlName)                                    \
  struct CXXName : postgres::Enum {                                            \
    static constexpr const char *name = PqlName;                               \
  }
