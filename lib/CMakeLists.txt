add_subdirectory(json)

include(GNUInstallDirs)

# BEGIN fmt

# TODO Maybe revert this with CMP0077 some time in the future
set(FMT_INSTALL ON CACHE BOOL "If you can read this, something is wrong with CMake.")
add_subdirectory(fmt)

set_target_properties(fmt PROPERTIES
    POSITION_INDEPENDENT_CODE ${METRICQ_POSITION_INDEPENDENT_CODE}
)

# END fmt
# BEGIN amqpcpp

find_package(OpenSSL REQUIRED)

set(AMQP_SRCS
amqpcpp/src/array.cpp
amqpcpp/src/channel.cpp
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
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT amqpcppTargets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/amqpcpp
    NAMESPACE amqpcpp::
)

install(FILES "cmake/amqpcppConfig.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/amqpcpp
)

install(DIRECTORY amqpcpp/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

# END amqpcpp
# BEGIN asio

add_library(asio INTERFACE)
target_include_directories(asio
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/asio/asio/include>
        $<INSTALL_INTERFACE:include>
)
target_compile_definitions(asio INTERFACE ASIO_STANDALONE)

add_library(asio::asio ALIAS asio)

install(TARGETS asio EXPORT asioTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT asioTargets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/asio
    NAMESPACE asio::
)

install(FILES "cmake/asioConfig.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/asio
)

install(DIRECTORY asio/asio/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.ipp"
)

# END asio
