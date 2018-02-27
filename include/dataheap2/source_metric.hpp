#pragma once

#include <dataheap2/types.hpp>

#include <protobufmessages/datachunk.pb.h>

#include <string>

namespace dataheap2
{
class Source;

class SourceMetric
{
public:
    SourceMetric(const std::string& id, Source& source) : id_(id), source_(source)
    {
    }

    void send(TimeValue tv);

    const std::string& id() const
    {
        return id_;
    }

    void enable_chunking(size_t n)
    {
        chunk_size_ = n;
    }
    void flush();

private:
    std::string id_;
    Source& source_;
    // It wasn't me, it's protobuf
    int chunk_size_ = 0;
    DataChunk chunk_;
};
} // namespace dataheap2
