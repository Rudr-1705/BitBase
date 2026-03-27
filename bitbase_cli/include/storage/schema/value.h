#pragma once

#include <variant>
#include <string>
#include <cstdint>

enum class DataType : uint8_t
{
    INT32,
    INT64,
    FLOAT,
    DOUBLE,
    BOOL,
    TEXT
};

using Value = std::variant<
    int32_t,
    int64_t,
    float,
    double,
    bool,
    std::string>;