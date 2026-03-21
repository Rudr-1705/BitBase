cmake_minimum_required(VERSION 3.16)

project(BitBase)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add subprojects
add_subdirectory(bitbase_cli)

# Uncomment when backend is added
# add_subdirectory(backend)