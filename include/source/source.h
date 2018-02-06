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

#pragma once

#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include <ev.h>
#include <time.h>

#include <iomanip>
#include <memory>
#include <string>

#include <json.hpp>

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

protected:
  virtual void loadSourceConfig(const nlohmann::json &config) = 0;

private:
  std::unique_ptr<AMQP::TcpConnection> data_connection;

  std::unique_ptr<AMQP::TcpChannel> data_channel;

  std::string data_exchange;

  uint64_t message_count = 0, message_count_last_step = 0;
  time_t start_time = 0, step_time = 0;

  void rpcResponseGetConfig(const nlohmann::json &config) override;
};

} // namespace source
} // namespace dataheap2
