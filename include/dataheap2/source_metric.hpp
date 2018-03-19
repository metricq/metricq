#pragma once

#include <dataheap2/datachunk.pb.h>
#include <dataheap2/types.hpp>

#include <algorithm>
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

    /**
     * @param n size of the chunk for automatic flushing
     * set to 0 to do only manual flushes - use at your own risk!
     * set to 1 to flush on every new value
     */
    void set_chunksize(size_t n)
    {
        chunk_size_ = n;
    }
    void flush();

private:
    std::string id_;
    Source& source_;

    int chunk_size_ = 1;
    int64_t previous_timestamp_ = 0;
    DataChunk chunk_;
};
} // namespace dataheap2
