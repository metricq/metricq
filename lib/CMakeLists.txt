add_subdirectory(json)

# TODO Maybe revert this with CMP0077 some time in the future
set(FMT_INSTALL ON CACHE BOOL "If you can read this, something is wrong with CMake.")
add_subdirectory(fmt)

set_target_properties(fmt PROPERTIES
    POSITION_INDEPENDENT_CODE ${METRICQ_POSITION_INDEPENDENT_CODE}
)

find_package(OpenSSL REQUIRED)

set(AMQP_SRCS
amqpcpp/src/array.cpp
amqpcpp/src/channelimpl.cpp
amqpcpp/src/connectionimpl.cpp
amqpcpp/src/deferredcancel.cpp
amqpcpp/src/deferredconfirm.cpp
amqpcpp/src/deferredconsumer.cpp
amqpcpp/src/deferredextreceiver.cpp
amqpcpp/src/deferredget.cpp
amqpcpp/src/deferredpublisher.cpp
amqpcpp/src/deferredreceiver.cpp
amqpcpp/src/field.cpp
amqpcpp/src/flags.cpp
amqpcpp/src/receivedframe.cpp
amqpcpp/src/table.cpp
amqpcpp/src/linux_tcp/openssl.cpp
amqpcpp/src/linux_tcp/tcpconnection.cpp
amqpcpp/src/watchable.cpp
)

add_library(amqpcpp STATIC ${AMQP_SRCS})
target_include_directories(amqpcpp SYSTEM
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/amqpcpp/src
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/amqpcpp/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(amqpcpp
    PRIVATE
        pthread
        ${CMAKE_DL_LIBS}
    PUBLIC
        OpenSSL::SSL
)
target_compile_features(amqpcpp PUBLIC cxx_std_14)
set_target_properties(amqpcpp PROPERTIES
    POSITION_INDEPENDENT_CODE ${METRICQ_POSITION_INDEPENDENT_CODE}
)

add_library(amqpcpp::amqpcpp ALIAS amqpcpp)

install(TARGETS amqpcpp EXPORT amqpcppTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(EXPORT amqpcppTargets
    DESTINATION lib/cmake/amqpcpp
    NAMESPACE amqpcpp::
)

install(DIRECTORY amqpcpp/include/ DESTINATION include
        FILES_MATCHING PATTERN "*.h")

add_library(asio INTERFACE)
target_include_directories(asio
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/asio/asio/include>
        $<INSTALL_INTERFACE:include>
)
target_compile_definitions(asio INTERFACE ASIO_STANDALONE)

add_library(asio::asio ALIAS asio)

install(TARGETS asio EXPORT asioTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(EXPORT asioTargets
    DESTINATION lib/cmake/asio
    NAMESPACE asio::
)

install(DIRECTORY asio/asio/include/ DESTINATION include
FILES_MATCHING PATTERN "*.hpp")
