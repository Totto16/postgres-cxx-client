#include <postgres/internal/Worker.h>

#include <utility>
#include <postgres/internal/Channel.h>
#include <postgres/Connection.h>
#include <postgres/Context.h>

namespace postgres::internal {

Worker::Worker(std::shared_ptr<Context const> ctx, std::shared_ptr<Channel> chan)
    : ctx_{std::move(ctx)}, chan_{std::move(chan)} {
}

Worker::~Worker() noexcept {
    if (thread_.joinable()) {
        switch (ctx_->shutdownPolicy()) {
            case ShutdownPolicy::GRACEFUL:
            case ShutdownPolicy::DROP: {
                thread_.join();
                break;
            }
            case ShutdownPolicy::ABORT: {
                thread_.detach();
                break;
            }
        }
    }
}

void Worker::run() {
    if (thread_.joinable()) {
        thread_.join();
    }
    thread_ = std::thread([this, conn = ctx_->connect()]() mutable {
        while (true) {
            chan_->receive(slot_);
            if (!slot_.job) {
                break;
            }

            slot_.job(conn);
            if (!conn.isOk() && !conn.reset()) {
                break;
            }
        }
        chan_->recycle(*this);
    });
}

}  // namespace postgres::internal
