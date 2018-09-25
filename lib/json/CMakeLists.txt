if(NOT TARGET json::json)
    include(GNUInstallDirs)

    add_library(json INTERFACE)
    target_include_directories(json INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )
    add_library(json::json ALIAS json)

    install(TARGETS json EXPORT jsonTargets
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(EXPORT jsonTargets
            FILE jsonTargets.cmake
            NAMESPACE json::
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/json
    )

    install(FILES "jsonConfig.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/json
    )
endif()
