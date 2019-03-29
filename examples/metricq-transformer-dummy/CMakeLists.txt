cmake_minimum_required(VERSION 3.8)

project(metricq-transformer-dummy VERSION 1.0)

add_executable(metricq-transformer-dummy src/main.cpp src/dummy_transformer.cpp)
target_compile_features(metricq-transformer-dummy PRIVATE cxx_std_17)
target_compile_options(metricq-transformer-dummy PRIVATE -Wall -Wextra -pedantic)

target_link_libraries(metricq-transformer-dummy
    PRIVATE
        metricq::transformer
        metricq::logger-nitro
        Nitro::core
)

install(TARGETS metricq-transformer-dummy RUNTIME DESTINATION bin)
