// Copyright (c) 2018, ZIH,
// Technische Universitaet Dresden,
// Federal Republic of Germany
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of metricq nor the names of its contributors
//       may be used to endorse or promote products derived from this software
//       without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
