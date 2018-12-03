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

#include <metricq/datachunk.pb.h>
#include <metricq/types.hpp>

#include <algorithm>
#include <string>

namespace metricq
{

template <class Writer>
class Metric
{
public:
    Metric(const std::string& id, Writer& writer) : id_(id), writer_(writer)
    {
    }

    void send(TimeValue tv);

    const std::string& id() const
    {
        return id_;
    }

    /**
     * @param n size of the chunk for automatic flushing
     * set to 0 to do only manual flushes - use at your own risk!
     * set to 1 to flush on every new value
     */
    void chunk_size(size_t n)
    {
        chunk_size_ = n;
    }

    void chunk_size() const
    {
        return chunk_size_;
    }

    void flush();

private:
    std::string id_;
    Writer& writer_;

    int chunk_size_ = 1;
    int64_t previous_timestamp_ = 0;
    DataChunk chunk_;
};

template <class Writer>
inline void Metric<Writer>::flush()
{
    writer_.send(id_, chunk_);
    chunk_.clear_time_delta();
    chunk_.clear_value();
    previous_timestamp_ = 0;
}

template <class Writer>
inline void Metric<Writer>::send(TimeValue tv)
{
    chunk_.add_time_delta(tv.time.time_since_epoch().count() - previous_timestamp_);
    previous_timestamp_ = tv.time.time_since_epoch().count();
    chunk_.add_value(tv.value);

    assert(chunk_.time_delta_size() == chunk_.value_size());
    if (chunk_size_ && chunk_.time_delta_size() == chunk_size_)
    {
        flush();
    }
}

} // namespace metricq
