#include <amqpcpp.h>
#include <ev.h>
#include <iomanip>
#include <memory>
#include <string>
#include <time.h>

#include <json.hpp>

#include <protobufmessages/datapoint.pb.h>

#include "source/source.h"

namespace dataheap2 {
namespace source {

/**
 * new incomming data
 * @param dataSourceID  unique id for the data source
 * @param timestamp     synced and correct timestamp
 * @param value         the value for this timestamp
 */
void RabbitMqDatasource::newDoubleData(const std::string &dataSourceID,
                                       uint64_t timestamp,
                                       double value) noexcept {
  DataPoint datapoint;
  datapoint.set_timestamp(timestamp);
  datapoint.set_value(value);

  std::cout << "Sending new double data for exchange " << data_exchange
            << "and queue " << dataSourceID << std::endl;

  message_count += 1;
  if (start_time == 0) {
    start_time = time(NULL);
  }
  if (time(NULL) - step_time > 10) {
    auto step_diff = time(NULL) - step_time;
    auto start_diff = time(NULL) - start_time;
    auto message_rate = (float)message_count / (float)start_diff;
    auto step_message_rate =
        (float)(message_count - message_count_last_step) / (float)step_diff;
    std::cout << "Overall message rate is " << std::fixed
              << std::setprecision(2) << message_rate
              << " msg/s! Current step: " << step_message_rate << " msg/s"
              << std::endl;
    message_count_last_step = message_count;
    step_time = time(NULL);
  }

  data_channel->publish(data_exchange, // default direct exchange
                        dataSourceID,  // queue name
                        datapoint.SerializeAsString());
}

void RabbitMqDatasource::rpcResponseGetConfig(const nlohmann::json &config) {
  std::cout << "Start parsing config" << std::endl;

  const std::string &data_server_address = config["dataServerAddress"];
  data_exchange = config["dataExchange"];

  data_connection = std::make_unique<AMQP::TcpConnection>(
      &handler, AMQP::Address(data_server_address));
  data_channel = std::make_unique<AMQP::TcpChannel>(data_connection.get());
  data_channel->onError([](const char *message) {
    // report error
    std::cout << "data channel error: " << message << std::endl;
  });

  if (config.find("sourceConfig") != config.end()) {
    loadSourceConfig(config["sourceConfig"]);
  } else {
    loadSourceConfig(nlohmann::json());
  }
}
} // namespace source
} // namespace dataheap2
