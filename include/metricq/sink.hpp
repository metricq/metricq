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
#include <metricq/datachunk.pb.h>
#include <metricq/types.hpp>

#include <memory>
#include <string>

namespace metricq
{

class Sink : public Connection
{
public:
    explicit Sink(const std::string& token, bool add_uuid=false);
    virtual ~Sink() = 0;

protected:
    virtual void data_callback(const std::string& id, TimeValue tv);
    virtual void data_callback(const std::string& id, const DataChunk& chunk);

    void close() override;

protected:
    void data_callback(const AMQP::Message&);
    void setup_data_queue(const AMQP::QueueCallback& callback);

protected:
    AMQP::LibAsioHandler data_handler_;
    std::unique_ptr<AMQP::TcpConnection> data_connection_;
    std::unique_ptr<AMQP::TcpChannel> data_channel_;
    std::string data_server_address_;
    std::string data_queue_;
    // Stored permanently to avoid expensive allocations
    DataChunk data_chunk_;
};
} // namespace metricq
