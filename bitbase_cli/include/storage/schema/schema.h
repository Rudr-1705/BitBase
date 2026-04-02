#pragma once

#include <vector>
#include <string>
#include "column.h"

class Schema
{
public:
    std::vector<Column> columns;

    void add_column(const std::string &name, DataType type);

    int get_column_index(const std::string &name) const;

    size_t size() const { return columns.size(); }

    std::vector<char> serialize() const;
    void deserialize(const char *data);

    int get_primary_index() const;
};