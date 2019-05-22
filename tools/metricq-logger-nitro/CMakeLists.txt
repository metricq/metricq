cmake_minimum_required(VERSION 3.8)

project(metricq-logger-nitro VERSION 1.0)

include(cmake/DefaultBuildType.cmake)

add_library(metricq-logger-nitro src/log.cpp)
target_include_directories(metricq-logger-nitro
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_compile_features(metricq-logger-nitro PRIVATE cxx_std_17)
target_compile_options(metricq-logger-nitro PRIVATE -Wall -Wextra -pedantic)
set_target_properties(metricq-logger-nitro PROPERTIES
    POSITION_INDEPENDENT_CODE ${METRICQ_POSITION_INDEPENDENT_CODE}
)

target_link_libraries(metricq-logger-nitro
    PRIVATE
        metricq::core
    PUBLIC
        Nitro::log
        Nitro::env
        date_interface
        tz
)

install(TARGETS metricq-logger-nitro
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
