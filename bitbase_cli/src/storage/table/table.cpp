#include "storage/table/table.h"

// INSERT
void Table::insert(const Row &row)
{
    std::array<char, ROW_SIZE> buffer;

    serialize_row(row, buffer.data());

    rows.push_back(buffer);
}

// SELECT *
std::vector<Row> Table::get_all() const
{
    std::vector<Row> result;
    for (const auto &buffer : rows)
    {
        Row dest;
        deserialize_row(buffer.data(), dest);
        result.push_back(dest);
    }

    return result;
}

// DELETE
bool Table::delete_by_id(uint32_t id)
{
    for (auto it = rows.begin(); it != rows.end(); ++it)
    {
        Row r;
        deserialize_row(it->data(), r);

        if (r.id == id)
        {
            rows.erase(it);
            return true;
        }
    }
    return false;
}

// UPDATE
bool Table::update(const Row &row)
{
    for (auto &buffer : rows)
    {
        Row exisiting;
        deserialize_row(buffer.data(), exisiting);

        if (exisiting.id == row.id)
        {
            serialize_row(row, buffer.data());
            return true;
        }
    }
    return false;
}