#include <dataheap2/db.hpp>

#include "log.hpp"
#include "util.hpp"

namespace dataheap2
{
Db::Db(const std::string& token) : Sink(token)
{
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

    db_config_callback(response["config"]);

    setup_data_queue([this](const std::string& name, int msgcount, int consumercount) {
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
                        .onSuccess(debug_success_cb("sink data channel consume success"))
                        .onError(debug_error_cb("sink data channel consume error"));
    });
}
} // namespace dataheap2
