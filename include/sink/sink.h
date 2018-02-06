#pragma once

#include <core/core.h>
#include <protobufmessages/datapoint.pb.h>

namespace dataheap2 {
namespace sink {

class RabbitMqDatasink : public dataheap2::core::RabbitMqCore {
public:
  explicit RabbitMqDatasink(struct ev_loop *loop);

protected:
  virtual void loadSinkConfig(const nlohmann::json &config) = 0;
  virtual void handleIncomingDatapoint(const std::string &,
                                       const DataPoint &) = 0;

private:
  std::unique_ptr<AMQP::TcpConnection> data_connection;

  std::unique_ptr<AMQP::TcpChannel> data_channel;

  std::string data_queue;

  uint64_t message_count = 0, message_count_last_step = 0;
  time_t start_time = 0, step_time = 0;

  void rpcResponseGetConfig(const nlohmann::json &config) override;
  void handleAmqpData(const AMQP::Message &);
};

} // namespace sink
} // namespace dataheap2
