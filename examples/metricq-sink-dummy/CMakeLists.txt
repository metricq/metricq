cmake_minimum_required(VERSION 3.8)

project(metricq-sink-dummy VERSION 1.0)

add_executable(metricq-sink-dummy src/main.cpp src/dummy_sink.cpp)
target_compile_features(metricq-sink-dummy PRIVATE cxx_std_17)
target_compile_options(metricq-sink-dummy PRIVATE -Wall -Wextra -pedantic)

target_link_libraries(metricq-sink-dummy
    PRIVATE
        metricq::sink
        metricq::logger-nitro
        Nitro::broken-options
)

install(TARGETS metricq-sink-dummy RUNTIME DESTINATION bin)
