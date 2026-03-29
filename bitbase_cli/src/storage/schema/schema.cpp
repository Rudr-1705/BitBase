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

std::vector<char> Schema::serialize() const
{
    std::vector<char> buffer;

    uint32_t count = columns.size();
    buffer.insert(buffer.end(), (char *)&count, (char *)&count + sizeof(count));

    for (const auto &col : columns)
    {
        uint32_t name_len = col.name.size();
        buffer.insert(buffer.end(), (char *)&name_len, (char *)&name_len + sizeof(name_len));
        buffer.insert(buffer.end(), col.name.begin(), col.name.end());

        uint8_t type = (uint8_t)col.type;
        buffer.push_back(type);
    }

    return buffer;
}

void Schema::deserialize(const char *data)
{
    columns.clear();

    const char *ptr = data;

    uint32_t count;
    memcpy(&count, ptr, sizeof(count));
    ptr += sizeof(count);

    for (uint32_t i = 0; i < count; i++)
    {
        uint32_t len;
        memcpy(&len, ptr, sizeof(len));
        ptr += sizeof(len);

        std::string name(ptr, len);
        ptr += len;

        uint8_t type = *ptr;
        ptr++;

        columns.push_back({name, (DataType)type});
    }
}