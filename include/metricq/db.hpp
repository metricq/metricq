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

#include <metricq/history.pb.h>
#include <metricq/json_fwd.hpp>
#include <metricq/sink.hpp>

#include <functional>

namespace metricq
{

/**
 * This is the base class for "database agents" in MetricQ
 *
 * any subclass must implement the following virtual methods:
 * (at least) one of the two on_db_config
 * (at least) one of the two on_history
 * (at least) one of the four on_data (from Sink)
 *
 * All requirements from DataClient apply
 */
class Db : public Sink
{
public:
    Db(const std::string& token);

protected:
    class ConfigCompletion
    {
        friend class Db;

    private:
        ConfigCompletion(Db& self) : self(self)
        {
        }

    public:
        ConfigCompletion(const ConfigCompletion&) = delete;
        ConfigCompletion(ConfigCompletion&&) = default;
        ConfigCompletion& operator=(const ConfigCompletion&) = delete;
        ConfigCompletion& operator=(ConfigCompletion&&) = delete;

        void operator()();

    private:
        Db& self;
    };

    class HistoryCompletion
    {
        friend class Db;

    private:
        HistoryCompletion(Db& self, std::string correlation_id, std::string reply_to)
        : self(self), correlation_id(std::move(correlation_id)), reply_to(std::move(reply_to)),
          begin_handling_(Clock::now()), begin_processing_(begin_handling_)
        {
        }

        /* Timeline:
         * begin_handling (ctor)
         * begin_processing() -- if not called, defaults to begin_handling
         * end_processing (local in operator())
         * end_handling (local in operator()'s handler)
         */
    public:
        HistoryCompletion(const HistoryCompletion&) = delete;
        HistoryCompletion(HistoryCompletion&&) = default;
        HistoryCompletion& operator=(const HistoryCompletion&) = delete;
        HistoryCompletion& operator=(HistoryCompletion&&) = delete;

        void begin_processing()
        {
            begin_processing_ = Clock::now();
        }

        void operator()(const HistoryResponse& response);

    private:
        Db& self;
        std::string correlation_id;
        std::string reply_to;
        TimePoint begin_handling_;
        TimePoint begin_processing_;
    };

protected:
    /**
     * The completion handler should be called eventually.
     * The completion handler can be called from any context,
     * it will dispatch to the right io_context internally.
     * The completion handler must be cleaned up before the Db object dies.
     * This can be enforced e.g. by owning all threads in the subclass of Db.
     */
    virtual void on_db_config(const json& config, ConfigCompletion complete);
    virtual void on_db_config(const json& config);

    virtual void on_history(const std::string& id, const HistoryRequest& content,
                            HistoryCompletion complete);
    virtual HistoryResponse on_history(const std::string& id, const HistoryRequest& content);

    virtual void on_db_ready() = 0;

private:
    void on_history(const AMQP::Message&);

    void setup_history_queue();
    void setup_history_queue(const AMQP::QueueCallback& callback);

protected:
    void config(const json& config);
    void on_connected() override;

protected:
    std::string history_queue_;
    // Stored permanently to avoid expensive allocations
    HistoryRequest history_request_;
};
} // namespace metricq
