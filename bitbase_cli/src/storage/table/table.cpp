#include "storage/table/table.h"
#include <cstring>

Table::Table(const char *filename)
{
    pager = new Pager(filename);
    num_rows = 0;
}

Table::~Table()
{
    delete pager;
}

void* row_slot(Table* table, uint32_t row_num)
{
    uint32_t page_num = row_num / (PAGE_SIZE / ROW_SIZE);

    void* page = table->pager->get_page(page_num);
    
    uint32_t row_offset = row_num % (PAGE_SIZE / ROW_SIZE);
    uint32_t byte_offset = row_offset * ROW_SIZE;

    return (char*)page + byte_offset;               
}

void Table::insert(const Row& row)
{
    void* destination = row_slot(this, num_rows);

    serialize_row(row, (char*)destination);

    num_rows++;
}

std::vector<Row> Table::get_all() const
{
    std::vector<Row> result;

    for (uint32_t i = 0; i < num_rows; i++)
    {
        Row r;

        void* source = row_slot((Table*)this, i);
        deserialize_row((char*)source, r);

        result.push_back(r);
    }

    return result;
}

bool Table::delete_by_id(uint32_t id)
{
    for (uint32_t i = 0; i < num_rows; i++)
    {
        Row r;
        void* source = row_slot(this, i);
        deserialize_row((char*)source, r);

        if (r.id == id)
        {
            // shift rows left
            for (uint32_t j = i + 1; j < num_rows; j++)
            {
                void* dest = row_slot(this, j - 1);
                void* src = row_slot(this, j);
                memcpy(dest, src, ROW_SIZE);
            }

            num_rows--;
            return true;
        }
    }
    return false;
}

bool Table::update(const Row& row)
{
    for (uint32_t i = 0; i < num_rows; i++)
    {
        Row r;
        void* dest = row_slot(this, i);
        deserialize_row((char*)dest, r);

        if (r.id == row.id)
        {
            serialize_row(row, (char*)dest);
            return true;
        }
    }
    return false;
}