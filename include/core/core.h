#pragma once

#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include <ev.h>
#include <iomanip>
#include <memory>
#include <string>
#include <time.h>

#include <json.hpp>

#define RABBITMQ_RPC_QUEUE "rpcQueue"

namespace dataheap2 {
namespace core {

class RabbitMqCore {
public:
  explicit RabbitMqCore(struct ev_loop *loop);

  void connectToServer(const std::string &server_address);

  void rpcGetConfig();

protected:
  // handler for libev (so we don't have to implement AMQP::TcpHandler!)
  AMQP::LibEvHandler handler;

  virtual void rpcResponseGetConfig(const nlohmann::json &config) = 0;

private:
  std::unique_ptr<AMQP::TcpConnection> rpc_connection;

  std::unique_ptr<AMQP::TcpChannel> rpc_channel;

  std::string callback_queue;

  std::unique_ptr<AMQP::TcpConnection> data_connection;

  std::unique_ptr<AMQP::TcpChannel> data_channel;

  std::string data_exchange;

  uint64_t message_count = 0, message_count_last_step = 0;
  time_t start_time = 0, step_time = 0;

  void sendRpcMessage(const std::string &message);

  void handleRpcRespone(const AMQP::Message &message);
};

} // namespace core
} // namespace dataheap2
