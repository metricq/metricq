#include <protobufmessages/datachunk.pb.h>
#include <protobufmessages/datapoint.pb.h>

#include <dataheap2/types.hpp>

#include <iostream>

#include <cassert>

int main()
{
    for (int distance : { 1000, 1000000, 1000000000 })
    {
        std::cout << "For timestamp distance " << distance << "\n";
        for (int count = 1; count <= 1 << 20; count *= 8)
        {
            dataheap2::DataChunk data_chunk;
            for (int i = 0; i < count; i++)
            {
                if (i == 0)
                {
                    data_chunk.add_time_delta(1519832293179227888);
                }
                else
                {
                    data_chunk.add_time_delta(distance);
                }
                data_chunk.add_value(0.1 + i / 3.0);
            }
            assert(count = data_chunk.time_delta().size());
            assert(count = data_chunk.value().size());
            auto size = data_chunk.SerializeAsString().size();
            std::cout << "[" << count << "] elements " << size << "B, "
                      << static_cast<double>(size) / count << " B/elem\n";
        }
    }
}
