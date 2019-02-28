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
#include <metricq/db.hpp>

#include "log.hpp"
#include "util.hpp"

namespace metricq
{
Db::Db(const std::string& token) : Sink(token)
{
}

void Db::setup_history_queue(const AMQP::QueueCallback& callback)
{
    assert(data_channel_);
    data_channel_->declareQueue(history_queue_, AMQP::passive).onSuccess(callback);
}

void Db::on_history(const AMQP::Message& incoming_message)
{
    const auto& metric_name = incoming_message.routingkey();
    auto message_string = std::string(incoming_message.body(), incoming_message.bodySize());

    history_request_.Clear();
    history_request_.ParseFromString(message_string);

    auto correlation_id = incoming_message.correlationID();
    auto reply_to = incoming_message.replyTo();
    // believe it or not, auto doesn't work here
    // no I don't understand why there is
    // no known conversion for argument 3 from
    // ‘metricq::Db::on_history(const AMQP::Message&)
    // ::<lambda(const metricq::HistoryResponse&)>’
    // to ‘std::function<void(const metricq::HistoryResponse&)>&’
    std::function<void(const metricq::HistoryResponse&)> respond =
        [this, correlation_id, reply_to](const HistoryResponse& response) {
            std::string reply_message = response.SerializeAsString();
            AMQP::Envelope envelope(reply_message.data(), reply_message.size());
            envelope.setCorrelationID(correlation_id);
            envelope.setContentType("application/json");

            data_channel_->publish("", reply_to, envelope);
        };
    on_history(metric_name, history_request_, respond);
}

void Db::on_connected()
{
    rpc("db.register", [this](const auto& response) { config(response); });
}

void Db::config(const json& config)
{
    log::debug("start parsing config");

    sink_config(config);

    history_queue_ = config["historyQueue"];

    on_db_config(config["config"]);
}

void Db::setup_history_queue()
{
    setup_history_queue([this](const std::string& name, int message_count, int consumer_count) {
        log::notice("setting up history queue, messages {}, consumers {}", message_count,
                    consumer_count);

        // we do not tolerate other consumers
        if (consumer_count != 0)
        {
            log::fatal("unexpected consumer count {} - are we not alone in the queue?",
                       consumer_count);
        }

        auto message_cb = [this](const AMQP::Message& message, uint64_t deliveryTag,
                                 bool redelivered) {
            (void)redelivered;

            on_history(message);
            data_channel_->ack(deliveryTag);
        };

        data_channel_->consume(name)
            .onReceived(message_cb)
            .onSuccess(debug_success_cb("sink history queue consume success"))
            .onError(debug_error_cb("sink history queue consume error"))
            .onFinalize([]() { log::info("sink history queue consume finalize"); });
    });

    on_db_ready();
}
} // namespace metricq
