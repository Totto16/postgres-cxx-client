#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>
#include <libpq-fe.h>
#include <postgres/Command.h>
#include <postgres/Result.h>
#include <postgres/Row.h>
#include <postgres/Statement.h>
#include <postgres/Transaction.h>

namespace postgres {

class Config;
class Consumer;
class PreparedCommand;
class Receiver;
struct PrepareData;

class Connection {
public:
    [[nodiscard]] static PGPing ping();
    [[nodiscard]] static PGPing ping(Config const& cfg);
    [[nodiscard]] static PGPing ping(std::string const& uri);
    

    [[nodiscard]] explicit Connection();
    [[nodiscard]] explicit Connection(Config const& cfg);
    [[nodiscard]] explicit Connection(std::string const& uri);
    Connection(Connection const& other) = delete;
    Connection& operator=(Connection const& other) = delete;
    Connection(Connection&& other) noexcept;
    Connection& operator=(Connection&& other) noexcept;
    ~Connection() noexcept;

    template <typename T>
    [[nodiscard]] Status create() {
        return exec(Statement<T>::create());
    }

    template <typename T>
    [[nodiscard]] Status drop() {
        return exec(Statement<T>::drop());
    }

    template <typename T>
    [[nodiscard]] Status insert(T const& val) {
        return exec(Command{Statement<T>::insert(), val});
    }

    template <typename Iter>
    [[nodiscard]] Status insert(Iter const it, Iter const end) {
        return exec(Command{RangeStatement::insert(it, end), std::make_pair(it, end)});
    }

    template <typename T>
    [[nodiscard]] Status update(T const& val) {
        return exec(Command{Statement<T>::update(), val});
    }

    template <typename T>
    [[nodiscard]] Result select(std::vector<T>& out) {
        auto res = exec(Statement<T>::select());
        if (!res.isOk()) {
            return res;
        }

        out.reserve(out.size() + res.size());
        for (auto row : res) {
            out.emplace_back();
            row >> out.back();
        }
        return res;
    }

    template <typename... Ts>
    [[nodiscard]] std::enable_if_t<(1 < sizeof... (Ts)), Result> transact(Ts&& ... args) {
        auto tx  = begin();
        auto res = exec(std::forward<Ts>(args)...);
        auto _ign = tx.commit();
        return res;
    }

    [[nodiscard]] Result exec(PrepareData const& prep);
    [[nodiscard]] Result exec(Command const& cmd);
    [[nodiscard]] Result exec(PreparedCommand const& cmd);
    [[nodiscard]] Status execRaw(std::string_view stmt);

    [[nodiscard]] Receiver send(PrepareData const& prep);
    [[nodiscard]] Receiver send(Command const& cmd);
    [[nodiscard]] Receiver send(PreparedCommand const& cmd);
    [[nodiscard]] Consumer sendRaw(std::string_view stmt);

    [[nodiscard]] Receiver iter(Command const& cmd);
    [[nodiscard]] Receiver iter(PreparedCommand const& cmd);

    [[nodiscard]] Transaction begin();

    [[nodiscard]] bool reset();
    [[nodiscard]] bool isOk();
    [[nodiscard]] std::string message();

    [[nodiscard]] std::string esc(std::string const& in);
    [[nodiscard]] std::string escId(std::string const& in);

    [[nodiscard]] PGconn* native() const;

private:
    explicit Connection(PGconn* handle);

    template <typename T, typename... Ts>
    std::enable_if_t<(0 < sizeof... (Ts)), Result> exec(T&& arg, Ts&& ... args) {
        auto res = exec(std::forward<T>(arg));
        if (!res.isOk()) {
            return res;
        }

        return exec(std::forward<Ts>(args)...);
    };

    template <typename F>
    std::string doEsc(std::string const& in, F f);

    std::shared_ptr<PGconn> handle_;
};

}  // namespace postgres
