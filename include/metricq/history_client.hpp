// Copyright (c) 2019, ZIH,
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

#include <metricq/chrono.hpp>
#include <metricq/data_client.hpp>
#include <metricq/history.pb.h>
#include <metricq/json_fwd.hpp>

#include <string>

namespace metricq
{
class BaseConnectionHandler;
class HistoryResponseValueView;
class HistoryResponseAggregateView;

class HistoryClient : public Connection
{
public:
    HistoryClient(const std::string& token, bool add_uuid = true);

    // We have to do this because of the ConnectionHandler forward declaration
    ~HistoryClient();

    // shall NOT be used before on_history_ready() was called
    std::string history_request(const std::string& id, TimePoint begin, TimePoint end,
                                Duration interval);

protected:
    virtual void on_history_response(const std::string& id, const HistoryResponse& response);
    virtual void on_history_response(const std::string& id,
                                     const HistoryResponseValueView& response) = 0;
    virtual void on_history_response(const std::string& id,
                                     const HistoryResponseAggregateView& response) = 0;
    virtual void on_history_config(const json& config) = 0;
    virtual void on_history_ready() = 0;

private:
    void on_history_response(const AMQP::Message&);

protected:
    void setup_history_queue(const AMQP::QueueCallback& callback);
    void config(const json& config);
    void on_connected() override;

protected:
    std::string history_exchange_;
    std::string history_queue_;

    // Stored permanently to avoid expensive allocations
    HistoryResponse history_response_;

protected:
    virtual void on_history_channel_ready();
    void history_config(const json& config);
    void close() override;

private:
    std::optional<AMQP::Address> data_server_address_;
    std::unique_ptr<BaseConnectionHandler> history_connection_;

protected:
    std::unique_ptr<AMQP::Channel> history_channel_;
};
} // namespace metricq
