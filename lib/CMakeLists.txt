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
        ${CMAKE_CURRENT_SOURCE_DIR}/amqpcpp/include
)

add_library(RabbitMQ::RabbitMQ ALIAS amqpcpp)
