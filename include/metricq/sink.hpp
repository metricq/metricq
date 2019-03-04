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

#include <metricq/data_client.hpp>
#include <metricq/datachunk.pb.h>
#include <metricq/json.hpp>
#include <metricq/metadata.hpp>
#include <metricq/types.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace metricq
{

class Sink : public DataClient
{
public:
    explicit Sink(const std::string& token, bool add_uuid = false);
    virtual ~Sink() = 0;

protected:
    class DataCompletion
    {
        friend class Sink;

    private:
        DataCompletion(Sink& self, uint64_t delivery_tag) : self(self), delivery_tag(delivery_tag)
        {
        }

    public:
        void operator()();

    private:
        Sink& self;
        uint64_t delivery_tag;
    };

    /**
     * override this only in special cases where you manually handle acknowledgements
     * or do something after acks
     */
    virtual void on_data(const AMQP::Message& message, uint64_t delivery_tag, bool redelivered);

    /**
     * override this to handle chunks efficiently
     */
    virtual void on_data(const std::string& id, const DataChunk& chunk);

    /**
     * must call complete in this context after you are finished handling the data
     */
    virtual void on_data(const std::string& id, const DataChunk& chunk, DataCompletion complete);

    /**
     * override this to handle individual values with immediate auto acknowledgement
     */
    virtual void on_data(const std::string& id, TimeValue tv);

    void sink_config(const json& config);

    void subscribe(const std::vector<std::string>& metrics, int64_t expires = 0);

    void setup_data_queue();

private:
    // let's hope the child classes never need to deal with this and the generic callback is
    // sufficient
    void setup_data_queue(const AMQP::QueueCallback& callback);

protected:
    std::string data_queue_;
    // Stored permanently to avoid expensive allocations
    DataChunk data_chunk_;
    std::unordered_map<std::string, Metadata> metadata_;
};
} // namespace metricq
