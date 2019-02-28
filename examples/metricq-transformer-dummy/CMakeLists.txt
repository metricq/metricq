cmake_minimum_required(VERSION 3.8)

project(metricq-transformer-dummy VERSION 1.0)

include(cmake/DefaultBuildType.cmake)

add_executable(metricq-transformer-dummy src/main.cpp src/dummy_transformer.cpp)
target_include_directories(metricq-transformer-dummy
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_TRANSFORMER_DIR}/include>
)
target_compile_features(metricq-transformer-dummy PRIVATE cxx_std_17)
target_compile_options(metricq-transformer-dummy PRIVATE -Wall -Wextra -pedantic)

target_link_libraries(metricq-transformer-dummy
    PRIVATE
        metricq::transformer
        Nitro::core
        metricq-logger-nitro
)

install(TARGETS metricq-transformer-dummy RUNTIME DESTINATION bin)
