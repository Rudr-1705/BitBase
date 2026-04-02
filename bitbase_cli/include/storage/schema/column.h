#pragma once

#include <string>
#include "value.h"

struct Column
{
    std::string name;
    DataType type;
    bool is_primary = false;
    bool is_unique = false;
};