#include <vector>
#include <optional>
#include <gtest/gtest.h>
#include <postgres/internal/Bytes.h>
#include <postgres/PreparedCommand.h>
#include <postgres/Statement.h>
#include <postgres/Visitable.h>
#include <postgres/PrepareData.h>
#include "Samples.h"

namespace postgres {

struct PreparedCommandTestTable {
    std::string s;
    int32_t     n = 0;
    double      f = 0.0;
    std::optional<int> opt = std::nullopt;
    std::vector<std::string> vec ={};

    POSTGRES_CXX_TABLE("prepared_cmd_test", s, n, f, opt, vec);
};

TEST(PrepareDataTest, Oid) {
    PrepareData const          data{"prepared_command", postgres::Statement<PreparedCommandTestTable>::insert(), postgres::PreparedStatement<PreparedCommandTestTable>::types()};
    ASSERT_EQ(5, data.types.size());

    ASSERT_EQ(Oid{TEXTOID}, data.types[0]);

    ASSERT_EQ(Oid{INT4OID}, data.types[1]);

    ASSERT_EQ(Oid{FLOAT8OID}, data.types[2]);

    ASSERT_EQ(Oid{INT4OID}, data.types[3]);

    ASSERT_EQ(Oid{TEXTARRAYOID}, data.types[4]);
}

}
