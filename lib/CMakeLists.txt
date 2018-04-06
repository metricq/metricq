add_subdirectory(json)

set(FMT_INSTALL ON)
add_subdirectory(fmt)

set_target_properties(fmt PROPERTIES
    POSITION_INDEPENDENT_CODE ${DATAHEAP2_POSITION_INDEPENDENT_CODE}
)

set(AMQP_SRCS
amqpcpp/src/array.cpp
amqpcpp/src/channelimpl.cpp
amqpcpp/src/connectionimpl.cpp
amqpcpp/src/deferredcancel.cpp
amqpcpp/src/deferredconsumer.cpp
amqpcpp/src/deferredconsumerbase.cpp
amqpcpp/src/deferredget.cpp
amqpcpp/src/field.cpp
amqpcpp/src/flags.cpp
amqpcpp/src/receivedframe.cpp
amqpcpp/src/table.cpp
amqpcpp/src/linux_tcp/tcpconnection.cpp
amqpcpp/src/watchable.cpp
)

add_library(amqpcpp ${AMQP_SRCS})
target_include_directories(amqpcpp SYSTEM
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/amqpcpp/src
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/amqpcpp/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/amqpcpp-asio/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(amqpcpp PRIVATE pthread)
target_compile_features(amqpcpp PUBLIC cxx_std_14)
set_target_properties(amqpcpp PROPERTIES
    POSITION_INDEPENDENT_CODE ${DATAHEAP2_POSITION_INDEPENDENT_CODE}
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

install(DIRECTORY amqpcpp/include/ amqpcpp-asio/include/ DESTINATION include
        FILES_MATCHING PATTERN "*.h")

add_library(asio INTERFACE)
target_include_directories(asio
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/asio/asio/include>
        $<INSTALL_INTERFACE:include>
)

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
