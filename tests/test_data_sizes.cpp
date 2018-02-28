#include <protobufmessages/datachunk.pb.h>
#include <protobufmessages/datapoint.pb.h>

#include <dataheap2/types.hpp>

#include <iostream>

int main()
{
    {
        dataheap2::DataChunk data_chunk;
        data_chunk.set_timestamp_offset(1519832293179227888);
        data_chunk.set_value(1.0 / 3);

        std::cout << "Single value " << data_chunk.SerializeAsString().size() << " B\n";
    }
    for (int distance : { 1000, 1000000, 1000000000 })
    {
        std::cout << "For timestamp distance " << distance << "\n";
        for (int count = 1; count <= 1 << 20; count *= 8)
        {
            dataheap2::DataChunk data_chunk;
            data_chunk.set_timestamp_offset(1519832293179227888);
            for (int i = 0; i < count; i++)
            {
                auto data_point = data_chunk.add_data();
                if (i > 0)
                {
                    data_point->set_timestamp(distance);
                }
                else
                {
                    data_point->set_timestamp(0);
                }
                data_point->set_value(i / 3.0);
            }
            auto size = data_chunk.SerializeAsString().size();
            std::cout << "[" << count << "] elements " << size << "B, "
                      << static_cast<double>(size) / count << " B/elem\n";
        }
    }
}
