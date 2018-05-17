#include <dataheap2/db.hpp>

#include "log.hpp"
#include "util.hpp"

namespace dataheap2
{
Db::Db(const std::string& token) : Sink(token)
{
}

void Db::setup_history_queue(const AMQP::QueueCallback& callback)
{
    assert(!data_server_address_.empty());
    assert(!history_queue_.empty());
    if (!data_connection_)
    {
        data_connection_ = std::make_unique<AMQP::TcpConnection>(
            &data_handler_, AMQP::Address(data_server_address_));
    }
    if (!data_channel_)
    {
        data_channel_ = std::make_unique<AMQP::TcpChannel>(data_connection_.get());
        data_channel_->onError(debug_error_cb("db data channel error"));
    }

    data_channel_->declareQueue(history_queue_).onSuccess(callback);
}

void Db::history_callback(const AMQP::Message& incoming_message)
{
    const auto& metric_name = incoming_message.routingkey();
    auto message_string = std::string(incoming_message.body(), incoming_message.bodySize());

    history_request_.Clear();
    history_request_.ParseFromString(message_string);

    auto response = history_callback(metric_name, history_request_);

    std::string reply_message = response.SerializeAsString();
    AMQP::Envelope envelope(reply_message.data(), reply_message.size());
    envelope.setCorrelationID(incoming_message.correlationID());
    envelope.setContentType("application/json");

    data_channel_->publish("", incoming_message.replyTo(), envelope);
}

HistoryResponse Db::history_callback(const std::string& id, const HistoryRequest& content)
{
    HistoryResponse response;
    response.set_metric(id);

    return response;
}

void Db::db_config_callback(const json& config)
{
}

void Db::setup_complete()
{
    rpc("db.register", [this](const auto& config) { config_callback(config); });
}

void Db::config_callback(const json& response)
{
    data_server_address_ = response["dataServerAddress"];
    data_queue_ = response["dataQueue"];
    history_queue_ = response["historyQueue"];

    db_config_callback(response["config"]);

    setup_data_queue([this](const std::string& name, int msgcount, int consumercount) {
        log::notice("setting up data queue, msgcount: {}, consumercount: {}", msgcount,
                    consumercount);
        // we do not tolerate other consumers
        assert(consumercount == 0);

        auto message_cb = [this](const AMQP::Message& message, uint64_t deliveryTag,
                                 bool redelivered) {
            (void)redelivered;

            data_callback(message);
            data_channel_->ack(deliveryTag);
        };

        data_channel_->consume(name)
            .onReceived(message_cb)
            .onSuccess(debug_success_cb("sink data queue consume success"))
            .onError(debug_error_cb("sink data queue consume error"))
            .onFinalize([]() { log::info("sink data queue consume finalize"); });
    });

    setup_history_queue([this](const std::string& name, int msgcount, int consumercount) {
        log::notice("setting up history queue, msgcount: {}, consumercount: {}", msgcount,
                    consumercount);
        // we do not tolerate other consumers
        assert(consumercount == 0);

        auto message_cb = [this](const AMQP::Message& message, uint64_t deliveryTag,
                                 bool redelivered) {
            (void)redelivered;

            history_callback(message);
            data_channel_->ack(deliveryTag);
        };

        data_channel_->consume(name)
            .onReceived(message_cb)
            .onSuccess(debug_success_cb("sink history queue consume success"))
            .onError(debug_error_cb("sink history queue consume error"))
            .onFinalize([]() { log::info("sink history queue consume finalize"); });
    });
}
} // namespace dataheap2
