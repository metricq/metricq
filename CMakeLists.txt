# TODO This file is part of dataheap2 (https://github.com/tud-zih-energy/dataheap2)
# TOD dataheap2 - A wrapper for the Open Trace Format 2 library
#
# Copyright (c) 2018, Technische Universität Dresden, Germany
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# * Neither the name of the copyright holder nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

project(dataheap2)

cmake_minimum_required(VERSION 3.8)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

find_package(libev REQUIRED)
add_subdirectory(lib)

set(dataheap2_VERSION 1.0)

set(CORE_SRCS src/core/main.cpp)
set(SOURCE_SRCS src/source/source.cpp)
set(SINK_SRCS src/sink/sink.cpp)
set(DB_SRCS src/db/db.cpp)

add_library(dataheap2-core ${CORE_SRCS})
target_include_directories(dataheap2-core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(dataheap2-core
    PUBLIC
        RabbitMQ::RabbitMQ
        libev::libev
)

target_compile_features(dataheap2-core PUBLIC cxx_std_17)

add_library(dataheap2-source ${SOURCE_SRCS})
target_link_libraries(dataheap2-source PUBLIC dataheap2-core)

add_library(dataheap2-sink ${SINK_SRCS})
target_link_libraries(dataheap2-sink PUBLIC dataheap2-core)

add_library(dataheap2-db ${DB_SRCS})
target_link_libraries(dataheap2-db PUBLIC dataheap2-core dataheap2-sink)

add_library(dataheap2-all INTERFACE)
target_link_libraries(dataheap2-all
    INTERFACE
        dataheap2::core
        dataheap2::source
        dataheap2::sink
        dateheap2::db
)

add_library(dataheap2::dataheap2 ALIAS dataheap2-all)
add_library(dataheap2::core ALIAS dataheap2-core)
add_library(dataheap2::source ALIAS dataheap2-source)
add_library(dataheap2::sink ALIAS dataheap2-sink)
add_library(dataheap2::db ALIAS dataheap2-db)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    target_compile_options(dataheap2-core
        INTERFACE
            -Wall -Wextra -pedantic
    )

    install(DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/include/dataheap2
        DESTINATION include
    )
    install(TARGETS dataheap2-core dataheap2-all dataheap2-source dataheap2-sink EXPORT dataheap2Targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
    )
    install(EXPORT dataheap2Targets
        FILE dataheap2Targets.cmake
        NAMESPACE dataheap2::
        DESTINATION lib/cmake/dataheap2
    )

    include(CMakePackageConfigHelpers)
    write_basic_package_version_File("dataheap2ConfigVersion.cmake"
        VERSION ${dataheap2_VERSION}
        COMPATIBILITY SameMajorVersion
    )
    install(FILES "dataheap2Config.cmake" "${CMAKE_CURRENT_BINARY_DIR}/dataheap2ConfigVersion.cmake"
        DESTINATION lib/cmake/dataheap2
    )

    # include(CTest)
    # add_subdirectory(tests)
else()
    set_target_properties(dataheap2-core PROPERTIES EXCLUDE_FROM_ALL TRUE)
    set_target_properties(dataheap2-source PROPERTIES EXCLUDE_FROM_ALL TRUE)
    set_target_properties(dataheap2-sink PROPERTIES EXCLUDE_FROM_ALL TRUE)
    set_target_properties(dataheap2-db PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()
