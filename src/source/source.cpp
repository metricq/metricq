/** TODO This file is part of dataheap2 (https://github.com/tud-zih-energy/dataheap2)
* TOD dataheap2 - A wrapper for the Open Trace Format 2 library
*
* Copyright (c) 2018, Technische Universität Dresden, Germany
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice, this
*   list of conditions and the following disclaimer.
*
* * Redistributions in binary form must reproduce the above copyright notice,
*   this list of conditions and the following disclaimer in the documentation
*   and/or other materials provided with the distribution.
*
* * Neither the name of the copyright holder nor the names of its
*   contributors may be used to endorse or promote products derived from
*   this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**/

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
  // DataPoint datapoint;
  // datapoint.set_timestamp(timestamp);
  // datapoint.set_value(value);

  // std::cout << "Sending new double data for exchange " << data_exchange << "
  // and queue " << dataSourceID << std::endl;
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

  // data_channel->publish(data_exchange, // default direct exchange
  //                       dataSourceID,  // queue name
  //                       datapoint.SerializeAsString());
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
