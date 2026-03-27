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
add_executable(bitbase_cli
    src/main.cpp
    src/repl/repl.cpp
    src/parser/parser.cpp
    src/executor/executor.cpp
    src/meta/meta.cpp
    src/utils/tokenizer.cpp
    src/utils/string_utils.cpp

    src/storage/table/table.cpp
    src/storage/row_format/row_format.cpp
    src/storage/pager/pager.cpp
    src/storage/btree/node.cpp
    src/storage/database/database.cpp
)