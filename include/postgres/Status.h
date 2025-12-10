#pragma once

#include <memory>
#include <libpq-fe.h>

namespace postgres {

class Consumer;

class Status {
public:
    Status(Status const& other) = delete;
    Status& operator=(Status const& other) = delete;
    Status(Status&& other) noexcept;
    Status& operator=(Status&& other) noexcept;
    ~Status() noexcept;

    [[nodiscard]] bool isOk() const;
    [[nodiscard]] bool isDone() const;
    [[nodiscard]] bool isEmpty() const;

    [[nodiscard]] int size() const;
    [[nodiscard]] int effect() const;
    [[nodiscard]] const char* message() const;
    [[nodiscard]] const char* describe() const;
    [[nodiscard]] ExecStatusType type() const;

    [[nodiscard]] PGresult* native() const;

protected:
    friend class Connection;
    friend class Consumer;

    explicit Status(PGresult* handle);
    explicit Status(PGresult* handle, Consumer*);

    void check() const;

private:
    std::unique_ptr<PGresult, void (*)(PGresult*)> handle_;
};

}  // namespace postgres
