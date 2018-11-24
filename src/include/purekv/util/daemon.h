#pragma once

#include <atomic>
#include <exception>
#include <thread>
#include <type_traits>

namespace zero_switch {

template <class Tick>
class Daemon {
    static_assert(std::is_nothrow_invocable_v<Tick>);

public:
    explicit Daemon(Tick tick)
        : tick_(tick),
          bg_thread_([this]{ run_(); })
    {}

    Daemon(const Daemon&) = delete;
    Daemon& operator=(const Daemon&) = delete;

    ~Daemon() noexcept
    {
        stop();
    }

    void stop() noexcept
    {
        if (running_) {
            running_ = false;
            bg_thread_.join();
        }
    }

    bool is_running() const noexcept
    {
        return running_;
    }

private:
    void run_() noexcept
    {
        while (running_) {
            tick_();
        }
    }

private:
    Tick tick_;
    std::atomic_bool running_ { true };
    std::thread bg_thread_;
};

}