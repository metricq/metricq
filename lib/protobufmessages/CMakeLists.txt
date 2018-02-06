if(DEFINED ENV{PROTOBUF_LIB})
  set(PROTOBUF_LIBRARY "$ENV{PROTOBUF_LIB}/libprotobuf.so")
endif()

if(DEFINED ENV{PROTOBUF_INC})
  set(PROTOBUF_INCLUDE_DIR $ENV{PROTOBUF_INC})
endif()

find_package(Protobuf REQUIRED)
protobuf_generate_cpp(PROTO_SRC PROTO_HEADER include/datapoint.proto)
add_library(protobufmessages ${PROTO_HEADER} ${PROTO_SRC})
target_link_libraries(protobufmessages PUBLIC ${PROTOBUF_LIBRARY})

target_include_directories(protobufmessages
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
        ${PROTOBUF_INCLUDE_DIR}
)

add_library(dataheap2::protobufmessages ALIAS protobufmessages)

install(TARGETS protobufmessages EXPORT dataheap2Targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} DESTINATION include
        FILES_MATCHING PATTERN "*.h")
