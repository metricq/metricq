find_package(benchmark QUIET)

if (benchmark_FOUND)
    message(STATUS "Looking for google benchmark - found, will build benchmark tests.")

    add_executable(dataheap2.bench_data_chunk bench_data_chunk.cpp)
    add_test(dataheap2.bench_data_chunk dataheap2.bench_data_chunk)
    target_link_libraries(dataheap2.bench_data_chunk PRIVATE dataheap2::core benchmark::benchmark)

    add_executable(dataheap2.bench_data_point bench_data_point.cpp)
    add_test(dataheap2.bench_data_point dataheap2.bench_data_point)
    target_link_libraries(dataheap2.bench_data_point PRIVATE dataheap2::core benchmark::benchmark)
else()
    message(STATUS "Looking for google benchmark - not found, skipping benchmark tests.")
endif()
