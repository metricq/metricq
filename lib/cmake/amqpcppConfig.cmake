include(CMakeFindDependencyMacro)

find_dependency(OpenSSL)

include("${CMAKE_CURRENT_LIST_DIR}/amqpcppTargets.cmake")
