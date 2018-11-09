project(metricq-transformer-dummy)

cmake_minimum_required(VERSION 3.8)

include(cmake/DefaultBuildType.cmake)

set(metricq-transformer-dummy_VERSION 1.0)

set(SRCS src/main.cpp src/dummy_transformer.cpp src/log.cpp)

add_executable(metricq-transformer-dummy ${SRCS})
target_include_directories(metricq-transformer-dummy
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_TRANSFORMER_DIR}/include>
)
target_compile_features(metricq-transformer-dummy PRIVATE cxx_std_17)
target_compile_options(metricq-transformer-dummy PRIVATE -Wall -Wextra -pedantic)

target_link_libraries(metricq-transformer-dummy
    PRIVATE
        metricq::transformer
        Nitro::nitro-log
        Nitro::nitro-env
        Nitro::nitro-jiffy
)

install(TARGETS metricq-transformer-dummy RUNTIME DESTINATION bin)
