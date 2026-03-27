#include "storage/schema/schema.h"

void Schema::add_column(const std::string &name, DataType type)
{
    columns.push_back({name, type});
}

int Schema::get_column_index(const std::string &name) const
{
    for (size_t i = 0; i < columns.size(); i++)
    {
        if (columns[i].name == name)
            return i;
    }
    return -1;
}