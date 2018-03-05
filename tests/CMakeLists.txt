find_package(benchmark QUIET)

add_executable(dataheap2.test_data_sizes test_data_sizes.cpp)
add_test(dataheap2.test_data_sizes dataheap2.test_data_sizes)
target_link_libraries(dataheap2.test_data_sizes PRIVATE dataheap2::core)

if (benchmark_FOUND)
    message(STATUS "Looking for google benchmark - found, will build benchmark tests.")

    add_executable(dataheap2.bench_data_chunk bench_data_chunk.cpp)
    add_test(dataheap2.bench_data_chunk dataheap2.bench_data_chunk)
    target_link_libraries(dataheap2.bench_data_chunk PRIVATE dataheap2::core benchmark::benchmark)
else()
    message(STATUS "Looking for google benchmark - not found, skipping benchmark tests.")
endif()


add_executable(asio_event_loop_test asio_event_loop_test.cpp)
target_link_libraries(asio_event_loop_test PRIVATE asio::asio amqpcpp::amqpcpp)
