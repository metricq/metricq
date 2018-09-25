include(CMakeFindDependencyMacro)

find_dependency(fmt)
find_dependency(amqpcpp)
find_dependency(asio)

include("${CMAKE_CURRENT_LIST_DIR}/metricqTargets.cmake")
