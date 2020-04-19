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

#include "log.hpp"
#include "util.hpp"

#include <metricq/db.hpp>
#include <metricq/json.hpp>

#include <asio/dispatch.hpp>

namespace metricq
{
Db::Db(const std::string& token) : Sink(token)
{
    register_management_callback("config", [this](const json& config) {
        on_db_config(config, ConfigCompletion(*this, false));
        // Unfortunately we must send the response now because of how the management callback stuff
        // is working. Technically, a threaded DB could wait for the event of another thread here.
        // Hopefully ... coroutines soon
        // Subscription is handled by the completion
        return json::object();
    });
}

void Db::setup_history_queue(const AMQP::QueueCallback& callback)
{
    assert(data_channel_);
    data_channel_->declareQueue(history_queue_, AMQP::passive).onSuccess(callback);
}

json Db::on_db_config(const metricq::json&)
{
    log::fatal("unhandled DataChunk, implementation error.");
    std::abort();
}

void Db::on_db_config(const metricq::json& config, metricq::Db::ConfigCompletion complete)
{
    complete(on_db_config(config));
}

void Db::ConfigCompletion::operator()(json subscribe_metrics)
{
    auto run = [& self = this->self, initial = this->initial,
                subscribe_metrics = std::move(subscribe_metrics)]() {
        if (initial)
        {
            self.setup_data_queue();
            self.setup_history_queue();
        }
        // We don't respond to the config RPC here, that's done int he RPC handler already
        // No async there...
        self.db_subscribe(subscribe_metrics);
    };
    asio::dispatch(self.io_service, run);
}

void Db::on_history(const AMQP::Message& incoming_message)
{
    const auto& metric_name = incoming_message.routingkey();
    auto message_string = std::string(incoming_message.body(), incoming_message.bodySize());

    history_request_.Clear();
    history_request_.ParseFromString(message_string);

    on_history(
        metric_name, history_request_,
        HistoryCompletion(*this, incoming_message.correlationID(), incoming_message.replyTo()));
}

HistoryResponse Db::on_history(const std::string&, const metricq::HistoryRequest&)
{
    log::fatal("unhandled Db::on_history, implementation error.");
    std::abort();
}

void Db::on_history(const std::string& id, const metricq::HistoryRequest& content,
                    metricq::Db::HistoryCompletion complete)
{
    complete(on_history(id, content));
}

void Db::HistoryCompletion::operator()(const metricq::HistoryResponse& response)
{
    std::string reply_message = response.SerializeAsString();
    auto processing_duration = Clock::now() - begin_processing_;
    auto run = [processing_duration, begin_handling = this->begin_handling_,
                reply_message = std::move(reply_message),
                correlation_id = std::move(correlation_id), reply_to = std::move(reply_to),
                &self = this->self]() {
        auto handling_duration = Clock::now() - begin_handling;
        AMQP::Table headers;
        AMQP::Envelope envelope(reply_message.data(), reply_message.size());
        envelope.setCorrelationID(correlation_id);
        envelope.setContentType("application/json");
        envelope.setAppID(self.token());
        headers["x-request-duration"] = std::to_string(
            std::chrono::duration_cast<std::chrono::duration<double>>(handling_duration).count());
        headers["x-processing-duration"] = std::to_string(
            std::chrono::duration_cast<std::chrono::duration<double>>(processing_duration).count());
        envelope.setHeaders(headers);
        self.data_channel_->publish("", reply_to, envelope);
    };
    asio::dispatch(self.io_service, run);
}

void Db::on_connected()
{
    rpc("db.register", [this](const auto& response) { on_register_response(response); });
}

void Db::on_register_response(const json& response)
{
    log::debug("start parsing config");

    sink_config(response);

    history_queue_ = response["historyQueue"];

    on_db_config(response["config"], ConfigCompletion(*this, true));
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

void Db::db_subscribe(const json& metrics)
{
    // TODO reduce redundancy with Sink::subscribe
    rpc("db.subscribe",
        [this](const json& response) {
            if (this->data_queue_.empty() || this->history_queue_.empty())
            {
                // Data queue should really be filled already by the db.register
                throw std::runtime_error(
                    "data_queue or history_queue empty upon db.subscribe return");
            }
            if (this->data_queue_ != response.at("dataQueue") ||
                this->history_queue_ != response.at("historyQueue"))
            {
                throw std::runtime_error(
                    "inconsistent dataQueue or historyQueue from db.subscribe");
            }
        },
        { { "metrics", metrics }, { "metadata", false } });
}
} // namespace metricq
