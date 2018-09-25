project(metricq-source-dummy)

cmake_minimum_required(VERSION 3.8)

include(cmake/DefaultBuildType.cmake)

set(metricq-source-dummy_VERSION 1.0)

set(SRCS src/main.cpp src/dummy_source.cpp src/log.cpp)

add_executable(metricq-source-dummy ${SRCS})
target_include_directories(metricq-source-dummy
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_compile_features(metricq-source-dummy PRIVATE cxx_std_17)
target_compile_options(metricq-source-dummy PRIVATE -Wall -Wextra -pedantic)

target_link_libraries(metricq-source-dummy
    PRIVATE
        metricq::source
        Nitro::nitro-log
        Nitro::nitro-env
        Nitro::nitro-jiffy
)

install(TARGETS metricq-source-dummy RUNTIME DESTINATION bin)
