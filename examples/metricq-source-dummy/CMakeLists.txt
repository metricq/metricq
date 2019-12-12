cmake_minimum_required(VERSION 3.8)

project(metricq-source-dummy VERSION 1.0)

add_executable(metricq-source-dummy src/main.cpp src/dummy_source.cpp)
target_compile_features(metricq-source-dummy PRIVATE cxx_std_17)
target_compile_options(metricq-source-dummy PRIVATE -Wall -Wextra -pedantic)

target_link_libraries(metricq-source-dummy
    PRIVATE
        metricq::source
        metricq::logger-nitro
        Nitro::broken-options
)

install(TARGETS metricq-source-dummy RUNTIME DESTINATION bin)
