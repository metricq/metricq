find_path(Ev_INCLUDE_DIRS
    NAMES ev.h
    HINTS ${Ev_ROOT_DIR}/include
    ENV C_INCLUDE_PATH
    PATH_SUFFIXES libev
)
find_library(Ev_LIBRARIES
    NAMES ev
    HINTS ${Ev_ROOT_DIR}/lib
    ENV LD_LIBRARY_PATH
    ENV LIBRARY_PATH
)
mark_as_advanced(Ev_INCLUDE_DIRS Ev_LIBRARIES)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Ev DEFAULT_MSG Ev_INCLUDE_DIRS Ev_LIBRARIES)

if(Ev_FOUND AND NOT TARGET ev::ev)
    add_library(ev::ev UNKNOWN IMPORTED)
    set_target_properties(ev::ev PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
        IMPORTED_LOCATION "${Ev_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${Ev_INCLUDE_DIRS}"
    )
endif()
