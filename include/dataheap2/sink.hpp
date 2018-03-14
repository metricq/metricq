#pragma once

#include <dataheap2/connection.hpp>
#include <dataheap2/datachunk.pb.h>
#include <dataheap2/types.hpp>

#include <nlohmann/json_fwd.hpp>

#include <memory>
#include <string>

namespace dataheap2
{
using json = nlohmann::json;

class Sink : public Connection
{
public:
    explicit Sink(const std::string& token);

protected:
    virtual void sink_config_callback([[maybe_unused]] const json& config){};
    virtual void ready_callback() = 0;
    virtual void data_callback(const std::string& id, TimeValue tv);
    virtual void data_callback(const std::string& id, const DataChunk& chunk);

    void config_callback(const json& config) override;

    void close() override;

protected:
    void data_callback(const AMQP::Message&);

protected:
    std::unique_ptr<AMQP::TcpConnection> data_connection_;
    std::unique_ptr<AMQP::TcpChannel> data_channel_;
    std::string data_server_address_;
    std::string data_queue_;
    DataChunk datachunk_;
};
} // namespace dataheap2
