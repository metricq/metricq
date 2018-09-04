find_package(benchmark QUIET)

add_executable(metricq.test_data_sizes test_data_sizes.cpp)
add_test(metricq.test_data_sizes metricq.test_data_sizes)
target_link_libraries(metricq.test_data_sizes PRIVATE metricq::core)

if (benchmark_FOUND)
    message(STATUS "Looking for google benchmark - found, will build benchmark tests.")

    add_executable(metricq.bench_data_chunk bench_data_chunk.cpp)
    add_test(metricq.bench_data_chunk metricq.bench_data_chunk)
    target_link_libraries(metricq.bench_data_chunk PRIVATE metricq::core benchmark::benchmark)
else()
    message(STATUS "Looking for google benchmark - not found, skipping benchmark tests.")
endif()


add_executable(asio_event_loop_test asio_event_loop_test.cpp)
target_link_libraries(asio_event_loop_test PRIVATE asio::asio amqpcpp::amqpcpp)
