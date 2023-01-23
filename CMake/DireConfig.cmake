include(CMakeFindDependencyMacro)

# Same syntax as find_package
find_dependency(RapidJSON REQUIRED)

# Any extra setup

# Add the targets file
include("${CMAKE_CURRENT_LIST_DIR}/DireTargets.cmake")