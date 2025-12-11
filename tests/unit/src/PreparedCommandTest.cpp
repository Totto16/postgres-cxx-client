#include "Samples.h"
#include <gtest/gtest.h>
#include <optional>
#include <postgres/Enum.h>
#include <postgres/PrepareData.h>
#include <postgres/PreparedCommand.h>
#include <postgres/Statement.h>
#include <postgres/Visitable.h>
#include <postgres/internal/Bytes.h>
#include <vector>

namespace postgres {

struct PreparedCommandTestTable {
  std::string s;
  int32_t n = 0;
  double f = 0.0;
  std::optional<int> opt = std::nullopt;
  std::vector<std::string> vec = {};

  POSTGRES_CXX_TABLE("prepared_cmd_test", s, n, f, opt, vec);
};

POSTGRES_CXX_ENUM(TestEnum2, "test_enum2");

static_assert(IsPostgresCXXEnum<TestEnum2>,
              "TestEnum2 must be a postgres::Enum");

POSTGRES_CXX_ARRAY(TestEnum2Array, "test_enum2", TestEnum2);

static_assert(IsPostgresCXXArray<TestEnum2Array>,
              "TestEnum2Array must be a postgres::Array");

static_assert(!IsPostgresCXXArray<TestEnum2>,
              "TestEnum2 must NOT be a postgres::Array");

struct PreparedCommandEnumTestTable {
  TestEnum2 e;
  TestEnum2Array vec;

  POSTGRES_CXX_TABLE("prepared_enum_cmd_test", e, vec);
};

TEST(PrepareDataTest, Oid) {
  PrepareData const data{
      "prepared_command",
      postgres::Statement<PreparedCommandTestTable>::insert(),
      postgres::PreparedStatement<PreparedCommandTestTable>::types()};
  ASSERT_EQ(5, data.types.size());

  ASSERT_EQ(Oid{TEXTOID}, data.types[0]);

  ASSERT_EQ(Oid{INT4OID}, data.types[1]);

  ASSERT_EQ(Oid{FLOAT8OID}, data.types[2]);

  ASSERT_EQ(Oid{INT4OID}, data.types[3]);

  ASSERT_EQ(Oid{TEXTARRAYOID}, data.types[4]);
}

TEST(PrepareDataTest, EnumOidUnknown) {
  PrepareData const data{
      "prepared_command",
      postgres::Statement<PreparedCommandTestTable>::insert(),
      postgres::PreparedStatement<PreparedCommandEnumTestTable>::types()};
  ASSERT_EQ(2, data.types.size());

  ASSERT_EQ(Oid{UNKNOWNOID}, data.types[0]);

  ASSERT_EQ(Oid{UNKNOWNOID}, data.types[1]);
}

TEST(PrepareDataTest, EnumOidValid) {

  Oid enum_oid = 13001;
  decltype(PreparedCommandEnumTestTable::e)::set_enum_oid(enum_oid);

  Oid array_oid = 13002;
  decltype(PreparedCommandEnumTestTable::vec)::set_array_oid(array_oid);

  PrepareData const data{
      "prepared_command",
      postgres::Statement<PreparedCommandTestTable>::insert(),
      postgres::PreparedStatement<PreparedCommandEnumTestTable>::types()};
  ASSERT_EQ(2, data.types.size());

  ASSERT_EQ(enum_oid, data.types[0]);

  ASSERT_EQ(array_oid, data.types[1]);
}

} // namespace postgres
