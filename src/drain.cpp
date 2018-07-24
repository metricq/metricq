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
