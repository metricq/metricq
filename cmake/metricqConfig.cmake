include(CMakeFindDependencyMacro)

find_dependency(amqpcpp)
find_dependency(asio)
find_dependency(fmt)
find_dependency(json)

include("${CMAKE_CURRENT_LIST_DIR}/metricqTargets.cmake")
