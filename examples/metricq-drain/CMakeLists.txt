cmake_minimum_required(VERSION 3.8)

project(metricq-drain VERSION 1.0)

include(cmake/DefaultBuildType.cmake)

add_executable(metricq-drain src/main.cpp)
target_compile_features(metricq-drain PRIVATE cxx_std_17)
target_compile_options(metricq-drain PRIVATE -Wall -Wextra -pedantic)

target_link_libraries(metricq-drain
    PRIVATE
        metricq::sink
        Nitro::core
        metricq-logger-nitro
)

install(TARGETS metricq-drain RUNTIME DESTINATION bin)
