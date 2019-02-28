cmake_minimum_required(VERSION 3.8)

project(metricq-history-dummy VERSION 1.0)

include(cmake/DefaultBuildType.cmake)

add_executable(metricq-history-dummy src/main.cpp src/dummy_history.cpp)
target_compile_features(metricq-history-dummy PRIVATE cxx_std_17)
target_compile_options(metricq-history-dummy PRIVATE -Wall -Wextra -pedantic)

target_link_libraries(metricq-history-dummy
    PRIVATE
        metricq::history
        Nitro::core
        metricq-logger-nitro
)

install(TARGETS metricq-history-dummy RUNTIME DESTINATION bin)
