#include <gtest/gtest.h>
#include <postgres/Connection.h>
#include <postgres/Error.h>
#include <postgres/Transaction.h>

namespace postgres {

inline auto constexpr CREATE = "CREATE TEMP TABLE tx_test (val INT)";
inline auto constexpr INSERT = "INSERT INTO tx_test (val) VALUES (1)";
inline auto constexpr SELECT = "SELECT val FROM tx_test";

TEST(TransactionTest, Ok) {
    Connection conn{};
    auto _ = conn.exec(CREATE);
    auto _2 = conn.transact(INSERT, INSERT);
    ASSERT_EQ(2, conn.exec(SELECT).size());
}

TEST(TransactionTest, Bad) {
    Connection conn{};
    auto _ = conn.exec(CREATE);
    ASSERT_THROW(auto _ = conn.transact(INSERT, "BAD"), RuntimeError);
    ASSERT_EQ(0, conn.exec(SELECT).size());
}

TEST(TransactionTest, Commit) {
    Connection conn{};
    auto _ = conn.exec(CREATE);
    auto tx = conn.begin();
    auto _2 = conn.exec(INSERT);
    auto _3 = tx.commit();
    ASSERT_EQ(1, conn.exec(SELECT).size());
}

TEST(TransactionTest, Rollback) {
    Connection conn{};
    auto _ = conn.exec(CREATE);
    {
        auto tx = conn.begin();
        auto _ = conn.exec(INSERT);
    }
    ASSERT_EQ(0, conn.exec(SELECT).size());
}

TEST(TransactionTest, Misuse) {
    Connection conn{};
    auto       tx = conn.begin();
    auto _ = tx.commit();
    ASSERT_THROW(auto _ = tx.commit(), LogicError);
}

TEST(TransactionTest, Move) {
    Connection conn{};
    auto       tx  = conn.begin();
    auto       tx2 = std::move(tx);
    ASSERT_THROW(auto _ = tx.commit(), LogicError);
    auto _ = tx2.commit();
}

}  // namespace postgres
