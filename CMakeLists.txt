cmake_minimum_required(VERSION 3.8)

project(metricq)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

include(cmake/DefaultBuildType.cmake)
include(cmake/GitSubmoduleUpdate.cmake)
git_submodule_update()

find_package(Protobuf 3.0 REQUIRED)

set(metricq_VERSION 1.0)

if(NOT METRICQ_POSITION_INDEPENDENT_CODE)
    set(METRICQ_POSITION_INDEPENDENT_CODE OFF)
endif()
option(METRICQ_POSITION_INDEPENDENT_CODE "Whether to build MetricQ libraries with position
independent code" ${METRICQ_POSITION_INDEPENDENT_CODE})

add_subdirectory(lib)

protobuf_generate_cpp(PROTO_SRC PROTO_HEADER src/datachunk.proto src/history.proto)
add_library(metricq-messages ${PROTO_SRC})
# this is needed to get the generated protobuf files in a sane location :(
add_custom_command(TARGET metricq-messages PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/include/metricq
    COMMAND ${CMAKE_COMMAND} -E copy ${PROTO_HEADER} ${CMAKE_CURRENT_BINARY_DIR}/include/metricq
    COMMENT "Copy generated protobuf headers..."
)
target_include_directories(metricq-messages
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
)
target_link_libraries(metricq-messages PUBLIC protobuf::libprotobuf)
set_target_properties(metricq-messages PROPERTIES
    POSITION_INDEPENDENT_CODE ${METRICQ_POSITION_INDEPENDENT_CODE}
)
target_compile_features(metricq-messages PRIVATE cxx_std_14)

set(CORE_SRCS src/chrono.cpp src/connection.cpp src/log.cpp)
set(SOURCE_SRCS src/source.cpp)
set(SINK_SRCS src/sink.cpp src/drain.cpp src/subscriber.cpp src/simple.cpp)
set(DB_SRCS src/db.cpp)

add_library(metricq-core ${CORE_SRCS})
target_include_directories(metricq-core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(metricq-core
    PUBLIC
        asio::asio
        amqpcpp::amqpcpp
        json::json
        metricq::messages
    PRIVATE
        fmt::fmt
)

set_target_properties(metricq-core PROPERTIES
    POSITION_INDEPENDENT_CODE ${METRICQ_POSITION_INDEPENDENT_CODE}
)

target_compile_features(metricq-core PUBLIC cxx_std_17)

add_library(metricq-source ${SOURCE_SRCS})
target_link_libraries(metricq-source
    PUBLIC
        metricq-core
    PRIVATE
        fmt::fmt
)
set_target_properties(metricq-source PROPERTIES
    POSITION_INDEPENDENT_CODE ${METRICQ_POSITION_INDEPENDENT_CODE}
)

add_library(metricq-sink ${SINK_SRCS})
target_link_libraries(metricq-sink
    PUBLIC
        metricq-core
    PRIVATE
        fmt::fmt
)
set_target_properties(metricq-sink PROPERTIES
    POSITION_INDEPENDENT_CODE ${METRICQ_POSITION_INDEPENDENT_CODE}
)

add_library(metricq-db ${DB_SRCS})
target_link_libraries(metricq-db
    PUBLIC
        metricq-core
        metricq-sink
    PRIVATE
        fmt::fmt
)
set_target_properties(metricq-db PROPERTIES
    POSITION_INDEPENDENT_CODE ${METRICQ_POSITION_INDEPENDENT_CODE}
)

add_library(metricq-all INTERFACE)
target_link_libraries(metricq-all
    INTERFACE
        metricq::core
        metricq::source
        metricq::sink
        metricq::db
)

add_library(metricq::metricq ALIAS metricq-all)
add_library(metricq::core ALIAS metricq-core)
add_library(metricq::source ALIAS metricq-source)
add_library(metricq::sink ALIAS metricq-sink)
add_library(metricq::db ALIAS metricq-db)
add_library(metricq::messages ALIAS metricq-messages)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    include(GNUInstallDirs)

    add_subdirectory(lib/nitro)
    add_subdirectory(examples/metricq-source-dummy)

    target_compile_options(metricq-core
        INTERFACE
            -Wall -Wextra -pedantic
    )

    install(DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/include/metricq
        DESTINATION include
    )
    install(
        TARGETS
            metricq-core
            metricq-all
            metricq-source
            metricq-sink
            metricq-db
            metricq-messages
        EXPORT metricqTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    set(METRICQ_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/metricq CACHE STRING
        "Installation directory for MetricQ cmake files, relative to ${CMAKE_INSTALL_PREFIX}."
    )

    install(EXPORT metricqTargets
        FILE metricqTargets.cmake
        NAMESPACE metricq::
        DESTINATION ${METRICQ_CMAKE_DIR}
    )

    include(CMakePackageConfigHelpers)
    write_basic_package_version_File("metricqConfigVersion.cmake"
        VERSION ${metricq_VERSION}
        COMPATIBILITY SameMajorVersion
    )
    install(
        FILES
            "cmake/metricqConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/metricqConfigVersion.cmake"
        DESTINATION ${METRICQ_CMAKE_DIR}
    )

    include(CTest)
    add_subdirectory(tests)
else()
    set_target_properties(metricq-core PROPERTIES EXCLUDE_FROM_ALL TRUE)
    set_target_properties(metricq-source PROPERTIES EXCLUDE_FROM_ALL TRUE)
    set_target_properties(metricq-sink PROPERTIES EXCLUDE_FROM_ALL TRUE)
    set_target_properties(metricq-db PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()
