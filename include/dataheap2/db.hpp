#pragma once

#include <dataheap2/sink.hpp>

namespace dataheap2
{
class Db : public Sink
{
public:
    Db(const std::string& token);
};
} // namespace dataheap2
