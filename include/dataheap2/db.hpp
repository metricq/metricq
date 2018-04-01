#pragma once

#include <dataheap2/sink.hpp>

namespace dataheap2
{
class Db : public Sink
{
public:
    Db(const std::string& token);

protected:
    virtual void db_config_callback(const json& config);
    void config_callback(const json& response);
    void setup_complete() override;
};
} // namespace dataheap2
