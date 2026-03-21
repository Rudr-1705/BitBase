cmake_minimum_required(VERSION 3.16)

project(bitbase_cli)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include headers
include_directories(include)

# Gather all source files
file(GLOB_RECURSE SOURCES
    src/*.cpp
)

# Create executable
add_executable(bitbase_cli ${SOURCES})