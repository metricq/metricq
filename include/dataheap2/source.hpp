#pragma once
#include <dataheap2/chrono.hpp>
#include <dataheap2/connection.hpp>
#include <dataheap2/datachunk.pb.h>
#include <dataheap2/source_metric.hpp>
#include <dataheap2/types.hpp>

#include <amqpcpp.h>

#include <nlohmann/json.hpp>

#include <memory>
#include <string>

namespace ev
{
class timer;
}

namespace dataheap2
{

class Source : public Connection
{
public:
    Source(const std::string& token);
    ~Source();

    void send(const std::string& id, TimeValue tv);
    void send(const std::string& id, const DataChunk& dc);

    SourceMetric& operator[](const std::string& id)
    {
        auto ret = metrics_.try_emplace(id, id, *this);
        return ret.first->second;
    }

protected:
    void setup_complete() override;
    void send_metrics_list();
    virtual void source_config_callback(const nlohmann::json& config) = 0;
    virtual void ready_callback() = 0;

private:
    void config_callback(const nlohmann::json& config);

private:
    AMQP::LibAsioHandler data_handler_;
    std::unique_ptr<AMQP::TcpConnection> data_connection_;
    std::unique_ptr<AMQP::TcpChannel> data_channel_;
    std::string data_exchange_;
    std::string data_server_address_;

    std::unordered_map<std::string, SourceMetric> metrics_;
};
} // namespace dataheap2
