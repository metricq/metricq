#pragma once
#include <dataheap2/connection.hpp>
#include <dataheap2/types.hpp>

#include <protobufmessages/datachunk.pb.h>

#include <amqpcpp.h>
#include <amqpcpp/libev.h>

#include <nlohmann/json.hpp>

#include <ev.h>

#include <memory>
#include <string>

namespace dataheap2
{

class Source;

class SourceMetric
{
public:
    SourceMetric(const std::string& id, Source& source) : id_(id), source_(source)
    {
    }

    void operator<<(TimeValue tv);

    const std::string& id() const
    {
        return id_;
    }

    void enable_chunking(size_t n)
    {
        chunk_size_ = n;
    }
    void flush();

private:
    std::string id_;
    Source& source_;
    // It wasn't me, it's protobuf
    int chunk_size_ = 0;
    DataChunk chunk_;
};

class Source : public Connection
{
public:
    Source(const std::string& token, struct ev_loop* loop = EV_DEFAULT);

    void send(const std::string& id, TimeValue tv);
    void send(const std::string& id, const DataChunk& dc);

    SourceMetric& operator[](const std::string& id)
    {
        auto ret = metrics_.try_emplace(id, id, *this);
        return ret.first->second;
    }

protected:
    virtual void source_config_callback(const nlohmann::json& config) = 0;
    virtual void ready_callback() = 0;

private:
    void config_callback(const nlohmann::json& config) override;

private:
    std::unique_ptr<AMQP::TcpConnection> data_connection_;
    std::unique_ptr<AMQP::TcpChannel> data_channel_;
    std::string data_exchange_;
    std::string data_server_address_;

    std::unordered_map<std::string, SourceMetric> metrics_;
};
}
