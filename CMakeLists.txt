project(dataheap2)

cmake_minimum_required(VERSION 3.8)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

# Intialize git submodules if not done already
file(GLOB SUBMODULE_FILES "lib/json/*")
list(LENGTH SUBMODULE_FILES COUNT_JSON)
file(GLOB SUBMODULE_FILES "lib/amqpcpp/*")
list(LENGTH SUBMODULE_FILES COUNT_AMQPCPP)
if(${COUNT_JSON} EQUAL 0 OR ${COUNT_AMQPCPP} EQUAL 0)
    message(STATUS "Initializing git submodule")
    execute_process(COMMAND "git" "submodule" "init" WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
    execute_process(COMMAND "git" "submodule" "update" WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endif()

find_package(Ev REQUIRED)
set(JSON_BuildTests OFF)
add_subdirectory(lib)

set(dataheap2_VERSION 1.0)

set(CORE_SRCS src/core/core.cpp)
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
        dataheap2::amqpcpp
        ev::ev
        nlohmann_json
        dataheap2::protobufmessages
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
