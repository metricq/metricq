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
#this is needed to get the generated protobuf files in a sane location : (
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

set(CORE_SRCS
    src/chrono.cpp
    src/connection.cpp
    src/connection_handler.cpp
    src/log.cpp
    src/data_client.cpp
    src/metadata.cpp
)

set(SOURCE_SRCS src/source.cpp)
set(SINK_SRCS src/sink.cpp src/drain.cpp src/subscriber.cpp src/simple.cpp)
set(TRANSFORMER_SRCS src/transformer.cpp)
set(DB_SRCS src/db.cpp)
set(HISTORY_SRCS src/history_client.cpp)

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
        fmt::fmt-header-only
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
        fmt::fmt-header-only
)
set_target_properties(metricq-source PROPERTIES
    POSITION_INDEPENDENT_CODE ${METRICQ_POSITION_INDEPENDENT_CODE}
)

add_library(metricq-sink ${SINK_SRCS})
target_link_libraries(metricq-sink
    PUBLIC
        metricq-core
    PRIVATE
        fmt::fmt-header-only
)
set_target_properties(metricq-sink PROPERTIES
    POSITION_INDEPENDENT_CODE ${METRICQ_POSITION_INDEPENDENT_CODE}
)

add_library(metricq-history ${HISTORY_SRCS})
target_link_libraries(metricq-history
    PUBLIC
        metricq-core
    PRIVATE
        fmt::fmt-header-only
)
set_target_properties(metricq-history PROPERTIES
    POSITION_INDEPENDENT_CODE ${METRICQ_POSITION_INDEPENDENT_CODE}
)

add_library(metricq-transformer ${TRANSFORMER_SRCS})
target_link_libraries(metricq-transformer
    PUBLIC
        metricq-core
        metricq-sink
    PRIVATE
        fmt::fmt-header-only
)
set_target_properties(metricq-transformer PROPERTIES
        POSITION_INDEPENDENT_CODE ${METRICQ_POSITION_INDEPENDENT_CODE}
)

add_library(metricq-db ${DB_SRCS})
target_link_libraries(metricq-db
    PUBLIC
        metricq-core
        metricq-sink
    PRIVATE
        fmt::fmt-header-only
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
        metricq::transformer
        metricq::db
        metricq::history
)

add_library(metricq::metricq ALIAS metricq-all)
add_library(metricq::core ALIAS metricq-core)
add_library(metricq::source ALIAS metricq-source)
add_library(metricq::sink ALIAS metricq-sink)
add_library(metricq::transformer ALIAS metricq-transformer)
add_library(metricq::db ALIAS metricq-db)
add_library(metricq::history ALIAS metricq-history)
add_library(metricq::messages ALIAS metricq-messages)


target_compile_options(metricq-core
    PRIVATE
        -Wall -Wextra -pedantic -Wsign-compare
)

if(TARGET Nitro::log)
    add_subdirectory(examples/metricq-logger-nitro)
    add_library(metricq::logger-nitro ALIAS metricq-logger-nitro)
    set_target_properties(metricq-logger-nitro PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    include(GNUInstallDirs)

    add_subdirectory(lib/nitro)
    add_subdirectory(examples)
    add_subdirectory(tools/metricq-summary)

    target_compile_options(metricq-core
        PUBLIC
            -Wall -Wextra -pedantic -Wsign-compare -Werror
    )

    install(DIRECTORY
        ${CMAKE_CURRENT_BINARY_DIR}/include/metricq
        ${CMAKE_CURRENT_SOURCE_DIR}/include/metricq
        DESTINATION include
    )

    set_target_properties(metricq-core PROPERTIES EXPORT_NAME core)
    set_target_properties(metricq-all PROPERTIES EXPORT_NAME all)
    set_target_properties(metricq-source PROPERTIES EXPORT_NAME source)
    set_target_properties(metricq-sink PROPERTIES EXPORT_NAME sink)
    set_target_properties(metricq-transformer PROPERTIES EXPORT_NAME transformer)
    set_target_properties(metricq-db PROPERTIES EXPORT_NAME db)
    set_target_properties(metricq-messages PROPERTIES EXPORT_NAME messages)

    install(
        TARGETS
            metricq-core
            metricq-all
            metricq-source
            metricq-sink
            metricq-transformer
            metricq-db
            metricq-history
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

    # Setup tests
    include(CTest)
    add_subdirectory(tests)

    # build docker image
    find_program(DOCKER_CMD docker DOC "docker executable used to build the docker image")
    if(DOCKER_CMD)
        message(STATUS "Docker found. Build metricq images with 'make docker'.")

        add_custom_target(docker-python
            COMMAND docker build -t metricq-python -f ${CMAKE_SOURCE_DIR}/Dockerfile-python ${CMAKE_SOURCE_DIR}
            COMMENT "Building metricq-python docker image"
        )

        add_custom_target(docker-cxx
            COMMAND docker build -t metricq-cxx -f ${CMAKE_SOURCE_DIR}/Dockerfile-cxx ${CMAKE_SOURCE_DIR}
            COMMENT "Building metricq-cxx docker image"
        )


        add_custom_target(docker DEPENDS docker-python docker-cxx)
    else()
        message(STATUS "Docker not found. SKIPPING.")
    endif()
else()
    set_target_properties(metricq-core PROPERTIES EXCLUDE_FROM_ALL TRUE)
    set_target_properties(metricq-source PROPERTIES EXCLUDE_FROM_ALL TRUE)
    set_target_properties(metricq-sink PROPERTIES EXCLUDE_FROM_ALL TRUE)
    set_target_properties(metricq-transformer PROPERTIES EXCLUDE_FROM_ALL TRUE)
    set_target_properties(metricq-db PROPERTIES EXCLUDE_FROM_ALL TRUE)
    set_target_properties(metricq-history PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()
