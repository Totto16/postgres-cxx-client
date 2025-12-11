#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

enum class PgType {
  Enum,
  Table,
  Array,
};

namespace pg_types {

// primitive types

using Bool = bool;

// integers types

// signed
using SmallInt = std::int16_t;

using Int = std::int32_t;

using BigInt = std::int64_t;

// unsigned
using SmallSerial = std::uint16_t;

using Serial = std::uint32_t;

using BigSerial = std::uint64_t;

// extensions

// signed
using Int2 = SmallInt;

using Int4 = Int;

using Int8 = BigInt;

// unsigned
using Serial2 = SmallSerial;

using Serial4 = Serial;

using Serial8 = BigSerial;

// string / char / text types

using Text = std::string;

// special types

// TODO: support postgres::Time and TimestampWithTimeZone / TimestampZ

using Timestamp = std::chrono::system_clock::time_point;

// templated types

template <typename T> using Nullable = std::optional<T>;

template <typename T> using Array = std::vector<T>;

// floating point numbers, as decided by oru internal visitors, we use those for
// the respecting c++ types
using Real = float;

using DoublePrecision = double;

// TODO: make an "opaque" serial type, that doesn't allow initialization by the
// user, and only can be get by the database, support serial everywhere, also in
// inserts

//  serial types

// TODO

// builtin vector types

using TextArray = std::vector<std::string>;

} // namespace pg_types
