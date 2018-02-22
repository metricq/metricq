#pragma once

#include <dataheap2/connection.hpp>
#include <dataheap2/types.hpp>

#include <protobufmessages/datachunk.pb.h>
#include <protobufmessages/datapoint.pb.h>

#include <nlohmann/json_fwd.hpp>

#include <ev.h>

#include <memory>
#include <string>

namespace dataheap2
{
using json = nlohmann::json;

class Sink : public Connection
{
public:
    explicit Sink(const std::string& token, struct ev_loop* loop = EV_DEFAULT);

protected:
    virtual void sink_config_callback(const json&){};
    virtual void ready_callback() = 0;
    virtual void data_callback(const std::string&, TimeValue tv) = 0;
    virtual void data_callback(const std::string&, const DataPoint&);
    virtual void data_callback(const std::string&, const DataChunk&);

    void config_callback(const json& config) override;

private:
    void data_callback(const AMQP::Message&);

private:
    std::unique_ptr<AMQP::TcpConnection> data_connection_;
    std::unique_ptr<AMQP::TcpChannel> data_channel_;
    std::string data_server_address_;
    std::string data_queue_;
};
}
