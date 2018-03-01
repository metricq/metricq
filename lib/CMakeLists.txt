add_subdirectory(json)

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
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(amqpcpp PRIVATE pthread)
target_compile_features(amqpcpp PUBLIC cxx_std_14)

add_library(dataheap2::amqpcpp ALIAS amqpcpp)

install(TARGETS amqpcpp EXPORT dataheap2Targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY amqpcpp/include/ DESTINATION include
        FILES_MATCHING PATTERN "*.h")
