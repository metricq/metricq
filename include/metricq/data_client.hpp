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
#pragma once

#include <metricq/connection.hpp>

#include <amqpcpp.h>

namespace metricq
{

class DataClient : public Connection
{
public:
    DataClient(const std::string& token, bool add_uuid = false);

protected:
    virtual void on_data_channel_ready();
    void data_config(const json& config);
    void close() override;

    template <typename PB>
    void data_publish(const std::string& routing_key, const PB& protobuf)
    {
        if (data_exchange_.empty())
        {
            throw std::logic_error("cannot publish on DataClient with empty data_exchange_");
        }
        std::string payload = protobuf.SerializeAsString();
        AMQP::Envelope envelope(payload.data(), payload.size());
        envelope.setPersistent(data_persistent_);
        data_channel_->publish(data_exchange_, routing_key, envelope);
    }

private:
    AMQP::LibAsioHandler data_handler_;
    std::optional<AMQP::Address> data_server_address_;
    std::unique_ptr<AMQP::TcpConnection> data_connection_;

protected:
    // Used by Transformer and Source, but not Sink in general
    std::string data_exchange_;
    bool data_persistent_ = true;

protected:
    std::unique_ptr<AMQP::TcpChannel> data_channel_;
};
} // namespace metricq
