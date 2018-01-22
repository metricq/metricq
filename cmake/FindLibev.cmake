find_path(libev_INCLUDE_DIRS
    NAMES ev.h
    HINTS ${libev_ROOT_DIR}/include
    ENV C_INCLUDE_PATH
    PATH_SUFFIXES libev
)
find_library(libev_LIBRARIES
    NAMES ev
    HINTS ${libev_ROOT_DIR}/lib
    ENV LD_LIBRARY_PATH
    ENV LIBRARY_PATH
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(libev DEFAULT_MSG libev_INCLUDE_DIRS libev_LIBRARIES)

if(libev_FOUND)
    mark_as_advanced(libev_INCLUDE_DIRS libev_LIBRARIES)
endif()

add_library(libev INTERFACE)
target_link_libraries(libev INTERFACE ${libev_LIBRARIES})
target_include_directories(libev INTERFACE ${libev_INCLUDE_DIRS})
add_library(libev::libev ALIAS libev)
