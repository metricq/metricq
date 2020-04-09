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

#include <metricq/chrono.hpp>
#include <metricq/history_client.hpp>
#include <metricq/types.hpp>

#include "connection_handler.hpp"
#include "log.hpp"
#include "util.hpp"

namespace metricq
{
HistoryClient::HistoryClient(const std::string& token, bool add_uuid) : Connection(token, add_uuid)
{
    register_management_callback("discover", [starting_time = Clock::now()](const json&) -> json {
        auto current_time = Clock::now();
        auto uptime = (current_time - starting_time).count(); // current uptime in nanoseconds

        return { { "alive", true },
                 { "currentTime", Clock::format_iso(current_time) },
                 { "startingTime", Clock::format_iso(starting_time) },
                 { "uptime", uptime } };
    });
}

HistoryClient::~HistoryClient() = default;

void HistoryClient::setup_history_queue(const AMQP::QueueCallback& callback)
{
    assert(history_channel_);
    history_channel_->declareQueue(history_queue_, AMQP::passive).onSuccess(callback);
}

std::string HistoryClient::history_request(const std::string& id, TimePoint begin, TimePoint end,
                                           Duration interval)
{
    HistoryRequest request;
    request.set_start_time(begin.time_since_epoch().count());
    request.set_end_time(end.time_since_epoch().count());
    request.set_interval_max(interval.count());
    // TODO expose the request type to the user. For now it's fine.
    request.set_type(HistoryRequest::FLEX_TIMELINE);

    auto correlation_id = std::string("metricq-history-") + token() + "-" + uuid();

    std::string message = request.SerializeAsString();
    AMQP::Envelope envelope(message.data(), message.size());
    envelope.setCorrelationID(correlation_id);
    envelope.setContentType("application/json");
    envelope.setReplyTo(history_queue_);

    history_channel_->publish(history_exchange_, id, envelope);

    return correlation_id;
}

void HistoryClient::on_history_response(const AMQP::Message& incoming_message)
{
    auto message_string = std::string(incoming_message.body(), incoming_message.bodySize());

    history_response_.Clear();
    history_response_.ParseFromString(message_string);

    on_history_response(incoming_message.correlationID(), history_response_);
}

void HistoryClient::on_connected()
{
    rpc("history.register", [this](const auto& response) { config(response); });
}

void HistoryClient::config(const metricq::json& config)
{
    history_config(config);

    if (!history_exchange_.empty() && config["historyExchange"] != history_exchange_)
    {
        log::fatal("changing historyExchange on the fly is not currently supported");
        std::abort();
    }

    history_exchange_ = config["historyExchange"];
    history_queue_ = config["historyQueue"];

    on_history_config(config["config"]);

    setup_history_queue([this](const std::string& name, int message_count, int consumer_count) {
        log::notice("setting up history queue, messages {}, consumers {}", message_count,
                    consumer_count);

        auto message_cb = [this](const AMQP::Message& message, uint64_t deliveryTag,
                                 bool redelivered) {
            (void)redelivered;

            on_history_response(message);
            history_channel_->ack(deliveryTag);
        };

        history_channel_->consume(name)
            .onReceived(message_cb)
            .onSuccess(debug_success_cb("history queue consume success"))
            .onError(debug_error_cb("history queue consume error"))
            .onFinalize([]() { log::info("history queue consume finalize"); });
    });

    on_history_ready();
}

void HistoryClient::on_history_channel_ready()
{
}

void HistoryClient::history_config(const json& config)
{
    AMQP::Address new_data_server_address =
        derive_address(config["dataServerAddress"].get<std::string>());
    log::debug("start parsing history config");
    if (history_connection_)
    {
        log::debug("history connection already exists");
        if (new_data_server_address != data_server_address_)
        {
            log::fatal("changing dataServerAddress on the fly is not currently supported");
            std::abort();
        }
        // We should be fine, connection and channel is already setup and the same
        return;
    }

    data_server_address_ = new_data_server_address;

    log::debug("opening history connection to {}", *data_server_address_);
    if (data_server_address_->secure())
    {
        history_connection_ = std::make_unique<SSLConnectionHandler>(io_service);
    }
    else
    {
        history_connection_ = std::make_unique<ConnectionHandler>(io_service);
    }

    history_connection_->connect(*data_server_address_);
    history_channel_ = history_connection_->make_channel();
    history_channel_->onReady([this]() {
        log::debug("history_channel ready");
        this->on_history_channel_ready();
    });
    history_channel_->onError(debug_error_cb("history channel error"));
}

void HistoryClient::close()
{
    // Close data connection first, then close the management connection
    if (!history_connection_)
    {
        log::debug("closing HistoryClient, no history_connection up yet");
        Connection::close();
        return;
    }

    // don't let the data_connection::close() call the on_closed() of this class, the close of the
    // management connection shall call on_closed().
    history_connection_->close([this]() {
        log::info("closed history_connection");
        Connection::close();
    });
}

void HistoryClient::on_history_response(const std::string& id, const HistoryResponse& response)
{
    if (response.value_size() > 0)
    {
        on_history_response(id, *static_cast<const HistoryResponseValueView*>(&response));
    }
    else
    {
        on_history_response(id, *static_cast<const HistoryResponseAggregateView*>(&response));
    }
}

} // namespace metricq
