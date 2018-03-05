#pragma once

#include <asio/basic_waitable_timer.hpp>
#include <asio/io_service.hpp>

#include <functional>
#include <system_error>

namespace dataheap2
{

class Timer
{
public:
    enum class TimerResult
    {
        repeat,
        cancel
    };

    using Callback = std::function<TimerResult(std::error_code)>;

    Timer(asio::io_service& io_service, Callback callback = Callback())
    : timer_(io_service), callback_(callback)
    {
    }

    void start(std::chrono::microseconds interval)
    {
        interval_ = interval;
        timer_.expires_from_now(interval);
        timer_.async_wait([this](auto error) { this->timer_callback(error); });
    }

    void start(Callback callback, std::chrono::microseconds interval)
    {
        callback_ = callback;
        start(interval);
    }

private:
    void timer_callback(std::error_code err)
    {
        auto res = callback_(err);

        if (res == TimerResult::repeat)
        {
            timer_.expires_at(timer_.expires_at() + interval_);
            timer_.async_wait([this](auto error) { this->timer_callback(error); });
        }
    }

private:
    asio::basic_waitable_timer<std::chrono::system_clock> timer_;
    Callback callback_;
    std::chrono::microseconds interval_;
};

} // namespace dataheap2
