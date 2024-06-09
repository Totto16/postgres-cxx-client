/// ## Usage
///
/// This section shows how to actually write code using PgCC.
/// All the examples are built and run as part of a CI process and are guaranteed to work.

/// ### Get Started with a Connection
///
/// The following example gives you the basic idea of how to use the library.
/// Each feature is explained in detail in its corresponding section below.
/// ```cpp
#include <chrono>
#include <iostream>
#include <string>
#include <vector>
#include <postgres/Postgres.h>

using postgres::Command;
using postgres::Connection;
using postgres::Time;

struct MyTable {
    int                                   id;
    std::string                           info;
    std::chrono::system_clock::time_point create_time;

    POSTGRES_CXX_TABLE("my_table", id, info, create_time);
};

void getStarted() {
    // Connect to a database.
    Connection conn{};

    // Create my_table.
    conn.create<MyTable>();

    auto now = std::chrono::system_clock::now();

    // Populate the table with data.
    std::vector<MyTable> data{{1, "foo", now},
                              {2, "bar", now},
                              {3, "baz", now}};
    conn.insert(data.begin(), data.end());

    // Retrieve some data from the table.
    auto query = "SELECT info, create_time FROM my_table WHERE $1 < id";

    for (auto const& row : conn.exec(Command{query, 1})) {
        std::cout
            << row["create_time"].as<Time>().toString()
            << " "
            << row["info"].as<std::string>()
            << std::endl;
    }
}
/// ```

/// ### Get Started with a Connection Pool
///
/// Here is one more example to get you started.
/// At this time a connection pool usage is demonstrated.
/// ```cpp
#include <iostream>
#include <vector>
#include <postgres/Postgres.h>

using postgres::Client;
using postgres::Command;
using postgres::Connection;
using postgres::Result;

void getStartedPool() {
    // Create a connection pool.
    Client client{};

    std::vector<std::future<Result>> results{};
    results.reserve(10);

    // Send queries to separate threads.
    for (auto i = 0; i < 10; ++i) {
        results.push_back(client.query([i](Connection& conn) {
            return conn.exec(Command{"SELECT $1::INT", i});
        }));
    }

    // Wait for the results to be ready and handle them.
    for (auto& res : results) {
        std::cout << res.get()[0][0].as<int>() << std::endl;
    }
}
/// ```

/// ### What To Include
///
/// PgCC provides the all-in-one header file "postgres/Postgres.h",
/// the one with forward declarations - "postgres/Fwd.h",
/// and also every class that is a part of a public API has its own header which you can include.
/// It is strongly discouraged to declare any of the library types in your project code.
/// Include the "postgres/Fwd.h" instead when you need just a declaration,
/// say in a function signature.
/// If compilation time is not a paramount concern use the "postgres/Postgres.h" in implementation,
/// otherwise include only needed files from the "postgres" directory.
/// The examples in this document use the "postgres/Postgres.h" for brevity.

/// ### Configuring
///
/// You can find a comprehensive description of Postgres configuration options
/// in the official libpq documentation at https://www.postgresql.org/docs/11/libpq-connect.html.
/// Here we will focus on the interface PgCC provides
/// to make it more convenient configuring a database connection.
///
/// Postgres has default values for all of its configuration parameters.
/// For instance, a username defaults to the operating system name of the user running the app,
/// and a database name is the same as the username.
/// There are several ways to override the defaults:
/// - environment variables;
/// - connection string;
/// - URL;
/// - configuration builder.
///
/// Let’s consider each one in turn with the examples.
/// ```cpp
void config() {
    Connection conn{};
}
/// ```
/// Here we connect to a database using the default values and environment variables.
/// This is a good choice to pass sensitive information like passwords.
/// For example, the project is tested assuming that
/// PGUSER, PGPASSWORD and PGDATABASE variables are set.
///
/// Alternatively, we can use a connection string:
/// ```cpp
void configStr() {
    Connection conn{"user=cxx_client password=cxx_client dbname=cxx_client"};
}
/// ```
/// ...or URL:
/// ```cpp
void configUrl() {
    Connection conn{"postgresql://cxx_client:cxx_client@/cxx_client"};
}
/// ```
/// And the last approach is to exploit a configuration builder:
/// ```cpp
using postgres::Config;

void configBuilder() {
    Connection conn{Config::Builder{}.user("cxx_client")
                                     .password("cxx_client")
                                     .dbname("cxx_client")
                                     .build()};
}
/// ```
/// The `Config::Builder` provides setter methods for all parameters
/// available (and not deprecated) at the moment of writing.
/// Method names are in *snake_case* to exactly mirror corresponding parameter names.
/// There are quite a few options, e.g:
/// ```cpp
using namespace std::chrono_literals;
using postgres::SslMode;

void configBuilderExtra() {
    Connection conn{Config::Builder{}.application_name("APP")
                                     .keepalives(true)
                                     .keepalives_count(2)
                                     .keepalives_idle(3min)
                                     .sslmode(SslMode::DISABLE)
                                     .build()};
}
/// ```
/// We can also do the same thing with general purpose setters of the `Config::Builder`,
/// but their use is not recommended unless there is some brand new parameter
/// that has not yet been supported by the library.
/// Just in case:
/// ```cpp
void configBuilderManual() {
    Connection conn{Config::Builder{}.set("application_name", "APP")
                                     .enable("keepalives", true)
                                     .setNumber("keepalives_count", 2)
                                     .setInterval("keepalives_idle", 3min)
                                     .set("sslmode", "disable")
                                     .build()};
}
/// ```

/// ### Error Handling
///
/// One of PgCC goals was to eliminate some sorts of bugs by design and at compile time,
/// but of course runtime errors are unavoidable.
///
/// An exception classes hierarchy consists of a base class `postgres::Error`
/// and two classes derived from it: `postgres::LogicError` and `postgres::RuntimeError`.
/// The `Error` in turn is a child of `std::exception`.
/// The `LogicError` is triggered to indicate a bug in your code
/// such as trying to access a row that is out of bounds or misusing the library in some other way.
/// Invalid SQL-query and broken connection are examples of a runtime error.
///
/// Some errors might stem from a connection loss.
/// When the connection breaks, it can be easily reset without the need to reconfigure it anew,
/// but the state including prepared statements is gone.
/// ```cpp
void connectReset(Connection& conn) {
    if (!conn.isOk()) {
        conn.reset();
    }
}
/// ```

/// ### Statement Execution
///
/// Now that we've learned how to connect to a database let’s execute some SQL-statements:
/// ```cpp
void exec(Connection& conn) {
    auto const res = conn.exec("SELECT 1");
    // Handle the result...
}
/// ```
/// The `exec()` returns an object of type `Result`.
/// The result is completely detached from the connection -
/// it is safe to use it even after the connection has been closed.
/// The method throws an instance of the `RuntimeError` if statement execution fails.
///
/// It is often needed to parametrize a statement with values computed at runtime.
/// You have an option to embed those values directly into the statement text,
/// but it is a bad choice for several reasons:
/// - your app could get vulnerable to an SQL-injection attack;
/// - you have to deal with escaping;
/// - data is passed as text instead of binary format.
///
/// PgCC provides a better solution:
/// ```cpp
using postgres::Command;

void args(Connection& conn) {
    conn.exec(Command{"SELECT $1, $2", 42, "foo"});
}
/// ```
/// Under the hood argument types are passed to Postgres along with their values.
/// The `Command` automatically detects those types, but sometimes you have to be explicit.
/// In the example below if we hadn't specified the type of the argument
/// it would've been guessed to be plain text instead of JSON:
/// ```cpp
using postgres::bindOid;

void argsOid(Connection& conn) {
    auto const json = R"({"foo": "bar"})";
    conn.exec(Command{"SELECT $1", bindOid(json, JSONOID)});
}
/// ```
/// If there are arguments possibly having NULL values, use pointers or `std::optional` type.
/// In the following example both `ptr` and `opt` will be treated as NULLs:
/// ```cpp
void argsNull(Connection& conn) {
    int* ptr = nullptr;
    std::optional<int> opt;
    conn.exec(Command{"SELECT $1, $2", ptr, opt});
}
/// ```
/// The `Command` stores all the arguments into its internal buffer.
/// But there are cases when it is desirable to avoid copying, e.g. for a large piece of text.
/// This can be achieved by passing pointer to underlying C-style string
/// or by using a `std::string_view`, but keep an eye on lifetimes.
/// The same is true for statements as well.
/// The both ways are shown below:
/// ```cpp
void argsLarge(Connection& conn) {
    std::string      text = "SOME VERY LONG TEXT...";
    std::string_view view = text;
    conn.exec(Command{"SELECT $1, $2", text.data(), view});
}
/// ```
/// That's how you can pass arguments stored in a container:
/// ```cpp
void argsRange(Connection& conn) {
    std::vector<int> args{1, 2, 3};
    conn.exec(Command{"SELECT $1, $2, $3", std::pair{args.begin(), args.end()}});
}
/// ```
/// Also there is an ability to add arguments afterwards:
/// ```cpp
void argsAfter(Connection& conn) {
    Command cmd{"SELECT $1, $2"};
    cmd << 42 << "foo";
    conn.exec(cmd);
}
/// ```
/// And a final note about timestamps.
/// The recommended way is to use a database type called `TIMESTAMP`,
/// which represents a number of microseconds since Postgres epoch in UTC.
/// Instances of `std::chrono::system_clock::time_point` are easily converted to that type
/// and are accepted by the `Command` as arguments.
/// Of course you can work with timestamps that include time zone information as well,
/// but PgCC won't help you here, just pass them as strings.
/// Well, almost won't help - actually you can preserve your local time zone:
/// ```cpp
using postgres::Time;

void argsTime(Connection& conn) {
    auto now = std::chrono::system_clock::now();
    conn.exec(Command{"SELECT $1", Time{now, true}});
}
/// ```

/// ### Prepared Statements
///
/// Using prepared statements is quite trivial.
/// To prepare a statement you have to specify its name, body and argument types if present.
/// Then you can use the name to actually execute the statement and bind argument values.
/// Consider an example:
/// ```cpp
using postgres::PreparedCommand;
using postgres::PrepareData;

void prepare(Connection& conn) {
    conn.exec(PrepareData{"my_select", "SELECT $1", {INT4OID}});
    conn.exec(PreparedCommand{"my_select", 123});
}
/// ```
/// Beware that the `Connection` is intentionally just a thin wrapper around native libpq handle
/// and doesn't keep any additional state.
/// Consequently, statements must be prepared again every time a connection's been reestablished.
/// Also using PgBouncer can lead to errors depending on its configuration:
/// if you're certain you've successfully prepared a statement and your code is correct,
/// but Postgres complains that the prepared statement doesn't exist,
/// then setting `pool_mode=session` in pgbouncer.ini is likely to solve the problem.

/// ### Multiple Statements in One
///
/// The `exec()` method described earlier allows to execute only one statement at a time,
/// which means that the following is a runtime error:
/// ```cpp
using postgres::Error;

void execMultiBad(Connection& conn) {
    try {
        conn.exec("SELECT 1; SELECT 2");
    } catch (Error const&) {
    }
}
/// ```
/// But what if we have a migration file with plenty of statements
/// which we want to apply all at once?
/// Or, generalizing the problem, just want to join several statements into one for some reason?
/// Here is the solution:
/// ```cpp
void execMultiOk(Connection& conn) {
    conn.execRaw("SELECT 1; SELECT 2");
}
/// ```
/// That's not an error anymore, but there are a couple of limitations.
/// The first one is that there is technically no way to pass arguments, only a statement.
/// Moreover, you are not allowed to obtain data.
/// Don't be confused by the example - it is quite silly and just for demonstration purposes,
/// normally there won't be any selects.
/// The second limitation is due to PgCC sends and receives arguments in a binary format,
/// but when multiple statements are passed there is no way to tell Postgres to enable binary mode.
/// Also when a select-statement is embedded somewhere between the other statements,
/// it is impossible to get the selected data
/// because only the result of the last statement is returned from a database.
/// Therefore, it was decided to completely disable data read and avoid aforementioned issues.

/// ### Transactions
///
/// Each statement is executed as a separate transaction.
/// Multiple statements separated with semicolons as described in the previous section
/// are a single transaction as well.
/// There are two more approaches to treat multiple statements as a transaction.
/// Let’s start with the simplest one:
/// ```cpp
void transact(Connection& conn) {
    conn.transact("SELECT 1",
                  Command{"SELECT $1", 2},
                  PreparedCommand{"my_select", 3},
                  PrepareData{"my_select2", "SELECT $1"});
}
/// ```
/// The `transact()` accepts anything the `exec()` does:
/// strings, `Command`*s*, `PreparedCommand`*s* and `PrepareData` in any combination.
/// Either all of them succeed or none have any effect.
/// Again the example is a bit ridiculous, but imagine statements to be more meaningful,
/// for instance, inserting data to two different tables when one insert without the other
/// would leave a system in inconsistent state.
///
/// The second way gives more fine-grained control over transaction execution:
/// ```cpp
void transactManual(Connection& conn) {
    auto tx = conn.begin();
    conn.exec("SELECT 1");
    conn.exec("SELECT 2");
    tx.commit();
}
/// ```
/// This way allows to put some logic between statement execution
/// and build more complex and flexible transactions.
/// When a transaction handle goes out of scope it rollbacks the transaction
/// unless it has been explicitly committed already.

/// ### Reading the Result
///
/// Now it's time to talk about queries and how to access their results.
/// As mentioned above the `exec()` method returns an object of type `Result`.
/// Iterating over it will produce a `Row` instance on each iteration.
/// The `Row` in turn consists of a number of `Field`*s* accessible by their index or name.
/// Finally, you can read the value of the `Field` into a new variable or an existing one.
/// That is probably expressed a bit less verbosely in code:
/// ```cpp
void result(Connection& conn) {
    for (auto const& row : conn.exec("SELECT 'foo' AS foo, 'bar' AS bar")) {
        std::cout
            << row["foo"].as<std::string>()
            << " "
            << row["bar"].as<std::string>()
            << std::endl;
    }
}
/// ```
/// Now let’s store the same values into variables:
/// ```cpp
void resultVars(Connection& conn) {
    std::string foo, bar;

    auto const res = conn.exec("SELECT 'foo' AS foo, 'bar' AS bar");
    if (res.isEmpty()) {
        // Normally this case should be covered as well...
    }

    res[0]["foo"] >> foo;
    res[0]["bar"] >> bar;

    std::cout << foo << " " << bar << std::endl;
}
/// ```
/// In the last example it was evident that the result could not be empty.
/// In practice you should usually make a check before trying to access the data
/// or you will end up with a chance of going out of bounds.
/// Iterating eliminates this risk and therefore is safer and more preferable.
///
/// Similarly to the `Command` NULLs are represented with pointers or an `std::optional`:
/// ```cpp
void resultNull(Connection& conn) {
    auto const res = conn.exec("SELECT NULL::TEXT");
    auto const fld = res[0][0];

    // Bad idea.
    std::string s;
    try {
        fld >> s;
    } catch (Error const&) {
    }

    // Ok.
    auto opt = fld.as<std::optional<std::string>>();

    // Also ok.
    auto ptr = &s;
    fld >> ptr;
}
/// ```
/// You can cast the field to arithmetic type, but the rules are quite strict.
/// In particular, the following is prohibited:
/// - loss of precision (casting from floating point value to integral one and vice versa);
/// - narrowing (casting larger type to smaller);
/// - underflow (reading negative values into variables of unsigned types).
///
/// Let’s look how those three cases may appear in code:
/// ```cpp
void resultBadCast(Connection& conn) {
    auto const res = conn.exec("SELECT -1::BIGINT");
    auto const fld = res[0][0];

    try {
        // Loss of precision.
        fld.as<double>();

        // Narrowing.
        fld.as<int32_t>();

        // Underflow.
        fld.as<uint64_t>();
    } catch (Error const&) {
    }
}
/// ```
/// Also the library is able to read timestamps without time zones:
/// ```cpp
void resultTime(Connection& conn) {
    auto const res = conn.exec("SELECT '2017-08-25T13:03:35'::TIMESTAMP");
    auto const fld = res[0][0];

    // C++11 way.
    fld.as<std::chrono::system_clock::time_point>();

    // Getting time_t.
    fld.as<Time>().toUnix();
}
/// ```
/// Timestamps **with** time zone have to be converted to `TEXT` and then read into `std::string`:
/// ```cpp
void resultTimeZone(Connection& conn) {
    auto const res = conn.exec("SELECT now()::TEXT");
    auto const fld = res[0][0];

    // Prints something like '2019-03-21 12:58:13.256812+03'.
    std::cout << fld.as<std::string>() << std::endl;
}
/// ```
/// And a small caveat about `extract(EPOCH FROM ...`-like statements.
/// Working with such a statement be aware that it yields the result of type `DOUBLE PRECISION`:
/// ```cpp
void resultExtractEpoch(Connection& conn) {
    auto const res = conn.exec("SELECT extract(EPOCH FROM now())");
    auto const fld = res[0][0];

    // Nope!
    try {
        fld.as<std::chrono::system_clock::time_point>();
    } catch (Error const&) {
    }

    // Ok.
    std::cout << fld.as<double>() << std::endl;
}
/// ```
/// Finally you can read absolutely anything into `std::string`.
/// This doesn't perform any checks and just gives you a raw content of the field.
/// There is also an option to avoid copying data with help of a `std::string_view`,
/// but make sure the result is staying alive long enough.
/// ```cpp
void resultData(Connection& conn) {
    auto const res = conn.exec("SELECT 'DATA'");
    auto const fld = res[0][0];

    // Copying...
    std::cout << fld.as<std::string>() << std::endl;

    // ...and non-copying variants.
    std::cout << fld.as<std::string_view>() << std::endl;
}
/// ```

/// ### Escaping
///
/// Thanks to the `Command`, it should be extremely rare when you have to deal with escaping.
/// But just in case there are a couple of methods performing that kind of task.
/// ```cpp
void escape(Connection& conn) {
    // Literals.
    std::cout << conn.esc("E'SCAPE_ME") << std::endl;

    // Identifiers.
    std::cout << conn.escId("escape me") << std::endl;
}
/// ```

/// ### Asynchronous Interface
///
/// Statement execution methods considered so far are synchronous,
/// meaning that the calling thread is blocked until a database gives back the result.
/// Now let’s look at an asynchronous family of methods
/// allowing you to split the execution process into sending and receiving phases.
/// Don't confuse it with multithreaded mode of a connection pool which is covered later.
/// That's what it looks like:
/// ```cpp
void send(Connection& conn) {
    // Sending doesn't block.
    auto receiver = conn.send("SELECT 123::INT");

    while (receiver.isBusy()) {
        // Do some other job here...
    }

    // But receiving does block unless the result is ready.
    auto const res = receiver.receive();
    std::cout << res[0][0].as<int>() << std::endl;

    // No more results.
    std::cout << receiver.receive().isDone() << std::endl;
}
/// ```
/// Calling the `send()` method returns an instance of type `Receiver`
/// which allows to obtain the result later.
/// It is a RAII-type which performs some cleanup in its destructor
/// to leave the connection in a valid state ready for reuse.
/// As a consequence, the destructor can block for a short period of time
/// until all the results are taken,
/// but it normally shouldn't be an issue assuming the proper use of PgCC.
///
/// You can't have multiple active sends simultaneously.
/// Either receive the results until `isDone()` gives true
/// or let the receiver go out of scope.
/// ```cpp
void sendTWice(Connection& conn) {
    auto rec1 = conn.send("SELECT 1");

    // Error!
    try {
        auto rec2 = conn.send("SELECT 2");
    } catch (Error const&) {
    }
}
/// ```
/// There are also asynchronous counterparts for prepared and raw statements.
/// There is nothing special about them so we won't waste our time on examples.
/// What's more interesting is a so-called "single-row mode",
/// the primary goal of which is to receive large datasets.
/// You may think of it as establishing a stream of rows.
/// As always there is a tradeoff - the single-row mode works a bit slower.
/// Let’s look at an example:
/// ```cpp
void sendRowByRow(Connection& conn) {
    // Imagine this query to end up with billions of rows.
    auto const query = "SELECT 1::INT"
                       " UNION ALL SELECT 2::INT"
                       " UNION ALL SELECT 3::INT";

    // Receive the result one row at a time.
    for (auto const& res : conn.iter(query)) {
        if (res.isEmpty()) {
            continue;
        }

        std::cout << res[0][0].as<int>() << std::endl;
    }
}
/// ```
/// Notice that the result is checked for emptiness inside the loop body -
/// this is because of how libpq works, and you always have to do the same thing.

/// ### Generating Statements
///
/// Since PgCC was not intended to be a fully-fledged ORM,
/// it is capable of producing just the most basic statements for you.
/// It is possible to create and drop tables,
/// perform inserts, selects and updates having no extra clauses.
///
/// This feature may come in handy when testing or prototyping,
/// but real-world applications often require more sophisticated SQL-statements,
/// and you can find some helpers in the library to construct them.
/// Remember the very first code example?
/// We've created a table called "my_table" there
/// to demonstrate basic statement generation facilities.
///
/// Now let’s use that table to show how we can perform an upsert.
/// The so-called "upsert" is a special kind of Postgres statement
/// consisting of an insert which on conflict turns into an update.
/// The starting example left our table in the following state:
///
/// id | info |        create_time
/// ---|------|---------------------------
/// 1  | foo  | 2019-03-21 13:01:25.729536
/// 2  | bar  | 2019-03-21 13:01:25.729536
/// 3  | baz  | 2019-03-21 13:01:25.729536
///
/// Lets update it:
/// ```cpp
using postgres::Statement;
using postgres::RangeStatement;

void myTableUpdate(Connection& conn) {
    // Needed for the example to work.
    conn.exec("ALTER TABLE my_table ADD PRIMARY KEY (id)");

    auto const now = std::chrono::system_clock::now();

    // 2 and 3 collide with existing ids.
    std::vector<MyTable> data{{2, "spam", now},
                              {3, "ham",  now},
                              {4, "eggs", now}};

    auto const range = std::pair{data.begin(), data.end()};

    // Generate an upsert statement.
    auto const upsert = "INSERT INTO "
                        + Statement<MyTable>::table()
                        + " ("
                        + Statement<MyTable>::fields()
                        + ") VALUES "
                        + RangeStatement::placeholders(range.first, range.second)
                        + " ON CONFLICT (id) DO UPDATE SET info = EXCLUDED.info";

    conn.exec(Command{upsert, range});
}
/// ```
/// We've just changed the content to the following:
///
/// id | info |        create_time
/// ---|------|----------------------------
/// 1  | foo  | 2019-03-21 13:01:25.729536
/// 2  | spam | 2019-03-21 13:46:04.580402
/// 3  | ham  | 2019-03-21 13:46:04.580402
/// 4  | eggs | 2019-03-21 13:46:04.693358
///
/// Recall the definition of MyTable:
/// ```cpp
/// struct MyTable {
///     int                                   id;
///     std::string                           info;
///     std::chrono::system_clock::time_point create_time;
///
///     POSTGRES_CXX_TABLE("my_table", id, info, create_time);
/// };
/// ```
/// It is the `POSTGRES_CXX_TABLE` macro that does the magic.
/// Once we've added it to a type definition it becomes possible
/// to visit all the data members along with their names using generated methods.
/// Those methods are `visitPostgresDefinition()` and `visitPostgresFields()`,
/// and you can use them to produce SQL-statements for your custom data types.
/// Here is a code skeleton to start with:
/// ```cpp
struct Generator {
    // Called by visitPostgresDefinition.
    template <typename T>
    void accept(char const* column_name) {
        std::cout << column_name << std::endl;
    }

    // Called by visitPostgresFields.
    template <typename T>
    void accept(char const* column_name, T const&) {
        std::cout << column_name << std::endl;
    }
};

void myTableVisit(Connection& conn) {
    Generator gen{};
    MyTable::visitPostgresDefinition(gen);

    MyTable data{1, "foo"};
    data.visitPostgresFields(gen);
}
/// ```
/// You might be wondering, how did the library figure out the field types
/// when we asked it to create a table for the definition of `MyTable`?
/// The rules are summarized here:
///
/// PostgreSQL type  | C++ types (with possible example for 64-bit machine)
/// -----------------|------------------------------------------------------
/// BOOL             | bool
/// REAL             | Floating point numbers fitting in 4 bytes (float).
/// DOUBLE PRECISION | Floating point numbers fitting in 8 bytes (double).
/// SMALLINT         | Signed integral numbers fitting in 2 bytes (short).
/// INT              | Signed integral numbers fitting in 4 bytes (int).
/// BIGINT           | Signed integral numbers fitting in 8 bytes (long).
/// SMALLSERIAL      | Unsigned integral numbers fitting in 2 bytes (unsigned short).
/// SERIAL           | Unsigned integral numbers fitting in 4 bytes (unsigned int).
/// BIGSERIAL        | Unsigned integral numbers fitting in 8 bytes (unsigned long).
/// TEXT             | std::string
/// TIMESTAMP        | std::chrono::system_clock::time_point
///
/// Be careful working with unsigned integers since the SQL standard doesn't support them
/// and neither do Postgres.
/// Moreover, it is considered to be a good practice in C++ to use signed numbers for arithmetic
/// and unsigned ones for bitmasks.
/// The design decision for table generation was to utilize unsigned integers
/// to create auto-incremented fields, which are useful for producing unique identifiers.

/// ### Connection Pool
///
/// Now that you know how to use a connection let’s move on to a higher-level feature.
/// Connection pool was designed to execute multiple statements concurrently.
/// You've seen it in the "Get started with connection pool" section.
/// Here we'll explore all the details and nuances.
/// To start using the pool create an instance of a `Client` class.
/// An interface is rather compact: you can invoke either `exec()` or `query()`,
/// passing any callable object accepting a reference to a connection as its parameter.
/// Which one to use depends on the connection method which is going to do the job.
/// ```cpp
void pool() {
    Client cl{};

    // The result is of type std::future<Result>.
    auto res = cl.query([](Connection& conn) {
        return conn.exec("SELECT 1");
    });

    std::cout << res.get().size() << std::endl;
}
/// ```
/// The `Client` implements single-producer-multiple-consumers pattern
/// and is not thread-safe by itself: protect it with a mutex for concurrent access.
/// The interface is quite straightforward to use,
/// however, a lot of flexibility is hidden in a connection pool's configuration,
/// so let’s discover it.
///
/// First of all, any available connection option can be passed
/// to a client to let it know how to establish a connection.
/// We've covered how to configure a connection in the corresponding section.
/// The only difference is that a config or URL must be wrapped in a `Context`
/// to be passed to a client.
/// ```cpp
using postgres::Context;

void poolConfig() {
    auto cfg = Config::Builder{}.user("cxx_client")
                                .password("cxx_client")
                                .dbname("cxx_client")
                                .build();

    Client cl{Context::Builder{}.config(std::move(cfg)).build()};
}
/// ```
/// The same technique is used for prepared statements:
/// ```cpp
void poolPrepare() {
    Client cl{Context::Builder{}.prepare({"my_select", "SELECT 1"}).build()};
}
/// ```
/// And finally there are parameters affecting the behaviour of a connection pool:
/// ```cpp
using postgres::ShutdownPolicy;

void poolBehaviour() {
    Client cl{Context::Builder{}.idleTimeout(1min)
                                .maxConcurrency(2)
                                .maxQueueSize(30)
                                .shutdownPolicy(ShutdownPolicy::DROP)
                                .build()};
}
/// ```
/// Idle timeout causes a thread to stop and close its connection to a database
/// after specified duration of inactivity.
/// Its primary purpose is to reduce the number of allocated resources back to the usual level
/// after a load spike has gone.
/// This feature is disabled by default.
///
/// Maximum concurrency specifies the number of threads/connections
/// and defaults to hardware concurrency.
/// Also the internal queue size can be limited.
/// Exceeding the limit results in an exception in a thread calling the client methods.
/// By default the queue is allowed to grow until application runs out of memory and crashes.
///
/// Shutdown policy regulates how to handle the queue on shutdown.
/// Default policy is to stop gracefully: all requests waiting in the queue will be executed.
/// You can alternatively choose to drop the queue,
/// but active requests are not canceled and can take some time to complete anyway.
/// And the last one policy is to abort, resulting in an undefined behaviour.
