#pragma once

#include <dataheap2/sink.hpp>

#include <nlohmann/json_fwd.hpp>

namespace dataheap2
{
using json = nlohmann::json;

class Db : public Sink
{
public:
    Db(const std::string& token);

protected:
    virtual json history_callback(const std::string& id, const json& content);
    virtual std::vector<TimeValue> history_callback(const std::string& id,
                                                    const std::string& from_timestamp,
                                                    const std::string& to_timestamp,
                                                    int interval_ms, int max_datapoints);

protected:
    void history_callback(const AMQP::Message&);
    void setup_history_queue(const AMQP::QueueCallback& callback);

protected:
    virtual void db_config_callback(const json& config);
    void config_callback(const json& response);
    void setup_complete() override;
    json get_metrics_callback(const json& response);

protected:
    std::string history_queue_;
    // Stored permanently to avoid expensive allocations
    DataChunk data_chunk_;
};
} // namespace dataheap2
