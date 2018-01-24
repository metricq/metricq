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
amqpcpp/src/tcpconnection.cpp
amqpcpp/src/watchable.cpp
)

add_library(amqpcpp ${AMQP_SRCS})
target_include_directories(amqpcpp
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/amqpcpp/src
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/amqpcpp/include>
        $<INSTALL_INTERFACE:include/amqpcpp>
)

add_library(dataheap2::amqpcpp ALIAS amqpcpp)

install(TARGETS amqpcpp EXPORT dataheap2Targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY amqpcpp/include/ DESTINATION include/amqpcpp
        FILES_MATCHING PATTERN "*.h")
