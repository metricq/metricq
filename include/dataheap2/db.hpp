#pragma once

#include <dataheap2/sink.hpp>

#include <ev.h>


namespace dataheap2
{
class Db : public Sink
{
public:
    Db(const std::string& token, struct ev_loop* loop = EV_DEFAULT);
};
}
