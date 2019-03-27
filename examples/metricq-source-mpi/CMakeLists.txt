cmake_minimum_required(VERSION 3.8)

project(metricq-source-mpi VERSION 1.0)

set(MPI_CXX_SKIP_MPICXX TRUE)
find_package(MPI REQUIRED)
# THANK YOU FOR NOTHING YOU STUPIDITY OF A FUCKED UP BUILD SYSTEM
# add_definitions(-DOMPI_SKIP_MPICXX)

add_executable(metricq-source-mpi src/main.cpp src/stress_test_source.cpp)
target_compile_features(metricq-source-mpi PRIVATE cxx_std_17)
target_compile_options(metricq-source-mpi PRIVATE -Wall -Wextra -pedantic)

target_link_libraries(metricq-source-mpi
    PRIVATE
        metricq::source
        Nitro::core
        metricq-logger-nitro
        MPI::MPI_CXX
)

install(TARGETS metricq-source-mpi RUNTIME DESTINATION bin)
