#include <gtest/gtest.h>
#include <postgres/Config.h>
#include <postgres/Connection.h>
#include <postgres/PreparedCommand.h>
#include <postgres/PrepareData.h>
#include <postgres/Receiver.h>
#include <postgres/Result.h>
#include <postgres/Enum.h>
#include <postgres/Visitable.h>
#include "Samples.h"


POSTGRES_CXX_ENUM(TestEnum, "test_enum");

static_assert(IsPostgresCXXEnum<TestEnum>, "TestEnum must be a postgres::Enum");

POSTGRES_CXX_ARRAY_OF_PG_TYPE(TestEnumArray, TestEnum);

static_assert(IsPostgresCXXArray<TestEnumArray>, "TestEnumArray must be a postgres::Array");

static_assert(!IsPostgresCXXArray<TestEnum>, "TestEnum must NOT be a postgres::Array");

struct PreparedCommandEnumTestTable {
    TestEnum e;
    TestEnumArray vec;
    int val;

    POSTGRES_CXX_TABLE("prepared_enum_cmd_test", e, vec, val);
};


POSTGRES_CXX_ENUM(TestEnum2, "test_enum2");

static_assert(IsPostgresCXXEnum<TestEnum2>, "TestEnum2 must be a postgres::Enum");

POSTGRES_CXX_ARRAY_OF_PG_TYPE(TestEnum2Array, TestEnum2);

static_assert(IsPostgresCXXArray<TestEnum2Array>, "TestEnum2Array must be a postgres::Array");

static_assert(!IsPostgresCXXArray<TestEnum2>, "TestEnum2 must NOT be a postgres::Array");


struct CommandEnumTestTable {
    TestEnum2 e;
    TestEnum2Array vec;

    POSTGRES_CXX_TABLE("enum_cmd_test", e, vec);
};

struct CommandStringVectorTestTable {
    std::string e;
    std::vector<std::string> vec;

    POSTGRES_CXX_TABLE("string_vector_cmd_test", e, vec);
};


namespace postgres {

TEST(ConnectionTest, Ping) {
    ASSERT_EQ(PQPING_OK, Connection::ping());
    ASSERT_EQ(PQPING_OK, Connection::ping(CONNECT_STR));
    ASSERT_EQ(PQPING_OK, Connection::ping(CONNECT_URI));

    ASSERT_EQ(PQPING_NO_RESPONSE, Connection::ping(Config::Builder{}.port(2345).build()));
    ASSERT_EQ(PQPING_NO_RESPONSE, Connection::ping("port=2345"));
    ASSERT_EQ(PQPING_NO_RESPONSE, Connection::ping("postgresql://:2345"));

    ASSERT_EQ(PQPING_NO_ATTEMPT, Connection::ping(Config::Builder{}.set("k", "v").build()));
    ASSERT_EQ(PQPING_NO_ATTEMPT, Connection::ping("k=v"));
    ASSERT_EQ(PQPING_NO_ATTEMPT, Connection::ping("postgresql://?k=v"));
}

TEST(ConnectionTest, Connect) {
    Connection conn{};
    ASSERT_TRUE(conn.isOk());
    ASSERT_TRUE(conn.message().empty());
    ASSERT_TRUE(conn.reset());
}

TEST(ConnectionTest, ConnectStr) {
    Connection conn{CONNECT_STR};
    ASSERT_TRUE(conn.isOk());
    ASSERT_TRUE(conn.message().empty());
    ASSERT_TRUE(conn.reset());
}

TEST(ConnectionTest, ConnectUri) {
    Connection conn{CONNECT_URI};
    ASSERT_TRUE(conn.isOk());
    ASSERT_TRUE(conn.message().empty());
    ASSERT_TRUE(conn.reset());
}

TEST(ConnectionTest, ConnectBad) {
    ASSERT_THROW(auto _ = Connection{Config::Builder{}.port(2345).build()}, RuntimeError);
    ASSERT_THROW(auto _ = Connection{"port=2345"}, RuntimeError);
    ASSERT_THROW(auto _ = Connection{"postgresql://:2345"}, RuntimeError);
}

TEST(ConnectionTest, Exec) {
    Connection conn{};
    ASSERT_TRUE(conn.exec("SELECT 1").isOk());
    ASSERT_THROW(auto _ign = conn.exec("SELECT 1; SELECT 2"), RuntimeError);
    ASSERT_THROW(auto _ign = conn.exec("BAD"), RuntimeError);
}

TEST(ConnectionTest, ExecRaw) {
    Connection conn{};
    ASSERT_TRUE(conn.execRaw("SELECT 1").isOk());
    ASSERT_TRUE(conn.execRaw("SELECT 1; SELECT 2").isOk());
    ASSERT_THROW(auto _ = conn.execRaw("BAD"), RuntimeError);
}

TEST(ConnectionTest, Prepare) {
    Connection conn{};
    ASSERT_TRUE(conn.exec(PrepareData{"select1", "SELECT 1",{}}).isOk());
    ASSERT_TRUE(conn.exec(PreparedCommand{"select1"}).isOk());
    ASSERT_THROW(auto _ = conn.exec(PrepareData{"bad", "BAD",{}}), RuntimeError);
    ASSERT_THROW(auto _ = conn.exec(PreparedCommand{"bad"}), RuntimeError);
}

TEST(ConnectionTest, PrepareArgs) {
    Connection conn{};
    ASSERT_TRUE(conn.exec(PrepareData{"select1", "SELECT $1", {INT4OID}}).isOk());
    ASSERT_TRUE(conn.exec(PreparedCommand{"select1", 1}).isOk());
    ASSERT_TRUE(conn.exec(PrepareData{"bad", "SELECT $1",{}}).isOk());
    ASSERT_THROW(auto _ = conn.exec(PreparedCommand{"bad", 2}), RuntimeError);
}

TEST(ConnectionTest, PrepareArgsEnumInsert) {
    Connection conn{};

    ASSERT_TRUE(conn.exec(Command{std::string{"DROP TABLE IF EXISTS "} + Statement<PreparedCommandEnumTestTable>::table()}).isOk());
    ASSERT_TRUE(conn.exec(Command{std::string{"DROP TYPE IF EXISTS "} + TestEnum::name}).isOk());
    ASSERT_TRUE(conn.exec(Command{std::string{"CREATE TYPE "} + TestEnum::name + R"( AS ENUM (
	'test1',
	'test2'
    ))"}).isOk());
    ASSERT_TRUE(conn.exec(Command{Statement<PreparedCommandEnumTestTable>::create()}).isOk());

    ASSERT_TRUE(conn.exec(PrepareData{"enum_insert_1", PreparedStatement<PreparedCommandEnumTestTable>::insert(), PreparedStatement<PreparedCommandEnumTestTable>::types()}).isOk());

    PreparedCommandEnumTestTable tbl{TestEnum{"test1"},TestEnumArray{TestEnum{"test1"},TestEnum{"test2"}},21};
    ASSERT_TRUE(conn.exec(PreparedCommand{"enum_insert_1", tbl}).isOk());

    PreparedCommandEnumTestTable tbl2{TestEnum{"unknown_value"},{},2};
    ASSERT_THROW(auto _ = conn.exec(PreparedCommand{"enum_insert_1", tbl2}), RuntimeError);

    PreparedCommandEnumTestTable tbl3{TestEnum{"test1"},{},13};
    ASSERT_TRUE(conn.exec(PreparedCommand{"enum_insert_1", tbl3}).isOk());

    ASSERT_TRUE(conn.exec(Command{"DROP TABLE IF EXISTS " + Statement<PreparedCommandEnumTestTable>::table()}).isOk());
    ASSERT_TRUE(conn.exec(Command{std::string{"DROP TYPE IF EXISTS "} + TestEnum::name}).isOk());
}

TEST(ConnectionTest, EnumInsertNormal) {
    Connection conn{};

    ASSERT_TRUE(conn.exec(Command{"DROP TABLE IF EXISTS " + Statement<CommandEnumTestTable>::table()}).isOk());
    ASSERT_TRUE(conn.exec(Command{std::string{"DROP TYPE IF EXISTS "} + TestEnum2::name}).isOk());
    ASSERT_TRUE(conn.exec(Command{std::string{"CREATE TYPE "} + TestEnum2::name + R"( AS ENUM (
	'test1',
	'test2'
    ))"}).isOk());
    ASSERT_TRUE(conn.exec(Command{Statement<CommandEnumTestTable>::create()}).isOk());

    CommandEnumTestTable tbl{TestEnum2{"test1"},{TestEnum2{"test1"},TestEnum2{"test2"}}};
    ASSERT_TRUE(conn.exec(Command{Statement<CommandEnumTestTable>::insert(), tbl}).isOk());

    CommandEnumTestTable tbl2{TestEnum2{"unknown_value"},{}};
    ASSERT_THROW(auto _ = conn.exec(Command{Statement<CommandEnumTestTable>::insert(), tbl2}), RuntimeError);

    CommandEnumTestTable tbl3{TestEnum2{"test1"},{}};
    ASSERT_TRUE(conn.exec(Command{Statement<CommandEnumTestTable>::insert(), tbl3}).isOk());

    ASSERT_TRUE(conn.exec(Command{"DROP TABLE IF EXISTS " + Statement<CommandEnumTestTable>::table()}).isOk());
    ASSERT_TRUE(conn.exec(Command{std::string{"DROP TYPE IF EXISTS "} + TestEnum2::name}).isOk());
}


TEST(ConnectionTest, StringVectorInsert) {
    Connection conn{};

    ASSERT_TRUE(conn.exec(Command{"DROP TABLE IF EXISTS " + Statement<CommandStringVectorTestTable>::table()}).isOk());

    ASSERT_TRUE(conn.exec(Command{Statement<CommandStringVectorTestTable>::create()}).isOk());

    CommandStringVectorTestTable tbl{"test1",{"test1","test2"}};
    ASSERT_TRUE(conn.exec(Command{Statement<CommandStringVectorTestTable>::insert(), tbl}).isOk());

    CommandStringVectorTestTable tbl2{"test1",{}};
    ASSERT_TRUE(conn.exec(Command{Statement<CommandStringVectorTestTable>::insert(), tbl2}).isOk());

    ASSERT_TRUE(conn.exec(Command{"DROP TABLE IF EXISTS " + Statement<CommandStringVectorTestTable>::table()}).isOk());
}


TEST(ConnectionTest, ExecAsync) {
    Connection conn{};
    ASSERT_TRUE(conn.send("SELECT 1").receive().isOk());
    ASSERT_THROW(auto _ =  conn.send("SELECT 1; SELECT 2").receive(), RuntimeError);
    ASSERT_THROW(auto _ =  conn.send("BAD").receive(), RuntimeError);
}

TEST(ConnectionTest, ExecRawAsync) {
    Connection conn{};
    ASSERT_TRUE(conn.sendRaw("SELECT 1").consume().isOk());
    ASSERT_TRUE(conn.sendRaw("SELECT 1; SELECT 2").consume().isOk());
    ASSERT_THROW(auto _ =  conn.sendRaw("BAD").consume(), RuntimeError);
}

TEST(ConnectionTest, PrepareAsync) {
    Connection conn{};
    ASSERT_TRUE(conn.send(PrepareData{"select1", "SELECT 1",{}}).receive().isOk());
    ASSERT_TRUE(conn.send(PreparedCommand{"select1"}).receive().isOk());
    ASSERT_THROW(auto _ =  conn.send(PrepareData{"bad", "BAD",{}}).receive(), RuntimeError);
    ASSERT_THROW(auto _ =  conn.send(PreparedCommand{"bad"}).receive(), RuntimeError);
}

TEST(ConnectionTest, RowByRow) {
    Connection conn{};
    ASSERT_TRUE(conn.iter("SELECT 1").receive().isOk());
    ASSERT_THROW(auto _ = conn.iter("SELECT 1; SELECT 2").receive(), RuntimeError);
    ASSERT_THROW(auto _ = conn.iter("BAD").receive(), RuntimeError);
}

TEST(ConnectionTest, PrepareRowByRow) {
    Connection conn{};
    ASSERT_TRUE(conn.exec(PrepareData{"select1", "SELECT 1",{}}).isOk());
    ASSERT_TRUE(conn.iter(PreparedCommand{"select1"}).receive().isOk());
    ASSERT_THROW(auto _ = conn.exec(PrepareData{"bad", "BAD",{}}), RuntimeError);
    ASSERT_THROW(auto _ = conn.iter(PreparedCommand{"bad"}).receive(), RuntimeError);
}

TEST(ConnectionTest, Esc) {
    Connection conn{};
    ASSERT_EQ("'E''SCAPE_ME'", conn.esc("E'SCAPE_ME"));
    ASSERT_EQ("\"e'scapeMe\"", conn.escId("e'scapeMe"));
}

}  // namespace postgres
