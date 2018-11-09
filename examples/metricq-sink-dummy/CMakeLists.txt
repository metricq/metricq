cmake_minimum_required(VERSION 3.8)

project(metricq-sink-dummy)

include(cmake/DefaultBuildType.cmake)

set(metricq-sink-dummy_VERSION 1.0)

set(SRCS src/main.cpp src/dummy_sink.cpp src/log.cpp)

add_executable(metricq-sink-dummy ${SRCS})
target_include_directories(metricq-sink-dummy
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_compile_features(metricq-sink-dummy PRIVATE cxx_std_17)
target_compile_options(metricq-sink-dummy PRIVATE -Wall -Wextra -pedantic)

target_link_libraries(metricq-sink-dummy
    PRIVATE
        metricq::sink
        Nitro::nitro-log
        Nitro::nitro-env
        Nitro::nitro-jiffy
)

install(TARGETS metricq-sink-dummy RUNTIME DESTINATION bin)
