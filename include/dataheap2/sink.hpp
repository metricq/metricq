#pragma once

#include <dataheap2/connection.hpp>
#include <dataheap2/datachunk.pb.h>
#include <dataheap2/types.hpp>

#include <memory>
#include <string>

namespace dataheap2
{

class Sink : public Connection
{
public:
    explicit Sink(const std::string& token, bool add_uuid=false);

protected:
    virtual void data_callback(const std::string& id, TimeValue tv);
    virtual void data_callback(const std::string& id, const DataChunk& chunk);

    void close() override;

protected:
    void data_callback(const AMQP::Message&);
    void setup_data_queue(const AMQP::QueueCallback& callback);

protected:
    std::unique_ptr<AMQP::TcpConnection> data_connection_;
    std::unique_ptr<AMQP::TcpChannel> data_channel_;
    std::string data_server_address_;
    std::string data_queue_;
    // Stored permanently to avoid expensive allocations
    DataChunk data_chunk_;
};
} // namespace dataheap2
