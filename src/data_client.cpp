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

#include <metricq/chrono.hpp>
#include <metricq/data_client.hpp>

#include "log.hpp"
#include "util.hpp"

namespace metricq
{
DataClient::DataClient(const std::string& token, bool add_uuid) : Connection(token, add_uuid)
{
    TimePoint starting_time = Clock::now();
    register_management_callback("discover", [token, starting_time](const json&) {
        json response;
        response["alive"] = true;
        response["currentTime"] = Clock::format_iso(Clock::now());
        response["startingTime"] = Clock::format_iso(starting_time);
        return response;
    });
}

void DataClient::data_config(const metricq::json& config)
{
    AMQP::Address new_data_server_address =
        add_credentials(config["dataServerAddress"].get<std::string>());
    log::debug("start parsing data config");
    if (data_connection_)
    {
        log::debug("data connection already exists");
        if (new_data_server_address != data_server_address_)
        {
            log::fatal("changing dataServerAddress on the fly is not currently supported");
            std::abort();
        }
        // We should be fine, connection and channel is already setup and the same
        return;
    }

    data_server_address_ = new_data_server_address;

    log::debug("opening data connection to {}", *data_server_address_);
    data_connection_ = std::make_unique<ConnectionHandler>(io_service, *data_server_address_);
    data_channel_ = data_connection_->make_channel();
    data_channel_->onReady([this]() {
        log::debug("data_channel ready");
        this->on_data_channel_ready();
    });
    data_channel_->onError(debug_error_cb("data channel error"));
}

void DataClient::close()
{
    Connection::close();
    if (!data_connection_)
    {
        log::debug("closing DataClient, no data_connection up yet");
        return;
    }
    auto alive = data_connection_->close();
    log::info("closed data_connection: {}", alive);
}

void DataClient::on_data_channel_ready()
{
}
} // namespace metricq
