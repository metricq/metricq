#include <dataheap2/db.hpp>

#include <ev.h>

namespace dataheap2
{
Db::Db(const std::string& token, struct ev_loop* loop) : Sink(token, loop)
{
}
}
