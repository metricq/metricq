#include <dataheap2/drain.hpp>

#include "util.hpp"

namespace dataheap2
{
void Drain::unsubscribe_complete(const json& response)
{
    assert(!data_queue_.empty());
    data_server_address_ = response["dataServerAddress"];
    setup_data_queue([this](const std::string& name, int msgcount, int consumercount) {
        std::cerr << "setting up drain queue. msgcount: " << msgcount
                  << ", consumercount: " << consumercount << std::endl;
        // we do not tolerate other consumers
        assert(consumercount == 0);

        auto message_cb = [this](const AMQP::Message& message, uint64_t deliveryTag,
                                 bool redelivered) {
            (void)redelivered;
            if (message.typeName() == "end")
            {
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
    std::cerr << "received end message\n";
    // don't ack the end message in case something fishy happens
    rpc("release", [this](const auto&) { close(); }, { { "dataQueue", data_queue_ } });
}

void Drain::setup_complete()
{
    assert(!metrics_.empty());
    rpc("unsubscribe", [this](const auto& response) { unsubscribe_complete(response); },
        { { "dataQueue", data_queue_ }, { "metrics", metrics_ } });
}
}
