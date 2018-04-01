#include <dataheap2/drain.hpp>

#include "log.hpp"
#include "util.hpp"

namespace dataheap2
{
Drain::~Drain()
{
}

void Drain::setup_complete()
{
    assert(!metrics_.empty());
    rpc("unsubscribe", [this](const auto& response) { unsubscribe_complete(response); },
        { { "dataQueue", data_queue_ }, { "metrics", metrics_ } });
}

void Drain::unsubscribe_complete(const json& response)
{
    assert(!data_queue_.empty());
    data_server_address_ = response["dataServerAddress"];
    setup_data_queue([this](const std::string& name, int msgcount, int consumercount) {
        log::notice("setting up drain queue, msgcount: {}, consumercount: {}", msgcount,
                    consumercount);
        // we do not tolerate other consumers
        assert(consumercount == 0);

        auto message_cb = [this](const AMQP::Message& message, uint64_t deliveryTag,
                                 bool redelivered) {
            (void)redelivered;
            if (message.typeName() == "end")
            {
                data_channel_->ack(deliveryTag);
                end();
                return;
            }
            data_callback(message);
            data_channel_->ack(deliveryTag);
        };

        data_channel_->consume(name)
            .onReceived(message_cb)
            .onSuccess(debug_success_cb("sink data channel consume success"))
            .onError(debug_error_cb("sink data channel consume error"));
    });
}

void Drain::end()
{
    log::debug("received end message");
    // to avoid any stupidity, close our data connection now
    // it will be closed once more, so what
    data_connection_->close();
    rpc("release", [this](const auto&) { close(); }, { { "dataQueue", data_queue_ } });
}
}
