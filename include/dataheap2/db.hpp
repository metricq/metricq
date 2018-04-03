#pragma once

#include <dataheap2/history.pb.h>
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
    virtual HistoryResponse history_callback(const std::string& id, const HistoryRequest& content);

protected:
    void history_callback(const AMQP::Message&);
    void setup_history_queue(const AMQP::QueueCallback& callback);
    virtual void db_config_callback(const json& config);
    void config_callback(const json& response);
    void setup_complete() override;

protected:
    std::string history_queue_;
    // Stored permanently to avoid expensive allocations
    HistoryRequest history_request_;
};
} // namespace dataheap2
