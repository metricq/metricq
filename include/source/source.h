#pragma once

#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include <ev.h>
#include <time.h>

#include <iomanip>
#include <memory>
#include <string>

#include <json.hpp>

#include <protobufmessages/datachunk.pb.h>

#include <core/core.h>

namespace dataheap2 {
namespace source {

class RabbitMqDatasource : public dataheap2::core::RabbitMqCore {
public:
  explicit RabbitMqDatasource(struct ev_loop *loop);

  /**
   * new incomming data
   * @param dataSourceID  unique id for the data source
   * @param timestamp     synced and correct timestamp
   * @param value         the value for this timestamp
   */
  void newDoubleData(const std::string &dataSourceID, uint64_t timestamp,
                     double value) noexcept;

  void newChunkData();
  void addChunkDoubleData(uint64_t timestamp,
                     double value);
  void flushChunkData(const std::string &dataSourceID);

protected:
  virtual void loadSourceConfig(const nlohmann::json &config) = 0;

private:
  std::unique_ptr<AMQP::TcpConnection> data_connection;

  std::unique_ptr<AMQP::TcpChannel> data_channel;

  std::string data_exchange;
  std::unique_ptr<DataChunk> chunk_data;

  uint64_t message_count = 0, message_count_last_step = 0;
  time_t start_time = 0, step_time = 0;

  void rpcResponseGetConfig(const nlohmann::json &config) override;
};

} // namespace source
} // namespace dataheap2
