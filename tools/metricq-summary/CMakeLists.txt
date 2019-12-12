cmake_minimum_required(VERSION 3.8)

project(metricq-summary VERSION 1.0)

include(cmake/DefaultBuildType.cmake)

set(SRCS
    src/main.cpp
    src/summary.cpp
    src/run_command.cpp
    )

add_executable(metricq-summary ${SRCS})
target_include_directories(metricq-summary
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_compile_features(metricq-summary PRIVATE cxx_std_17)
target_compile_options(metricq-summary PRIVATE -Wall -Wextra -pedantic)

target_link_libraries(metricq-summary
    PUBLIC
        metricq::sink
        metricq-logger-nitro
        Nitro::broken-options
)

install(TARGETS metricq-summary RUNTIME DESTINATION bin)
