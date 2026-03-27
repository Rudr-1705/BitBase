#include "storage/table/table.h"
#include "storage/btree/node.h"
#include <cstring>

constexpr uint32_t METADATA_PAGE_NUM = 0;
constexpr uint32_t ROOT_PAGE_NUM = 1;
constexpr uint32_t NUM_ROWS_OFFSET = 0;

Table::Table(const char *filename)
{
    pager = new Pager(filename);

    void *meta = pager->get_page(METADATA_PAGE_NUM);

    if (pager->file_length == 0)
    {
        // fresh DB
        num_rows = 0;

        void *root = pager->get_page(ROOT_PAGE_NUM);
        initialize_leaf_node(root);
        *node_is_root(root) = 1;

        // store num_rows
        *(uint32_t *)((char *)meta + NUM_ROWS_OFFSET) = 0;
        pager->flush(METADATA_PAGE_NUM);
    }
    else
    {
        num_rows = *(uint32_t *)((char *)meta + NUM_ROWS_OFFSET);
    }
}

void Table::persist_num_rows()
{
    void *page = pager->get_page(METADATA_PAGE_NUM);

    *(uint32_t *)((char *)page + NUM_ROWS_OFFSET) = num_rows;

    pager->flush(METADATA_PAGE_NUM);
}

Table::~Table()
{
    delete pager;
}

void *row_slot(Table *table, uint32_t row_num)
{
    uint32_t page_num = row_num / (PAGE_SIZE / ROW_SIZE) + 2; // ✅ FIX (skip meta + root)

    void *page = table->pager->get_page(page_num);

    uint32_t row_offset = row_num % (PAGE_SIZE / ROW_SIZE);
    uint32_t byte_offset = row_offset * ROW_SIZE;

    return (char *)page + byte_offset;
}

void Table::insert(const Row &row)
{
    // 1. store row
    void *destination = row_slot(this, num_rows);
    serialize_row(row, (char *)destination);

    // 2. insert into B+ tree
    void *root = pager->get_page(ROOT_PAGE_NUM);

    leaf_node_insert(ROOT_PAGE_NUM, root, row.id, num_rows, pager);

    num_rows++;

    persist_num_rows();
}

std::vector<Row> Table::get_all() const
{
    std::vector<Row> result;

    for (uint32_t i = 0; i < num_rows; i++)
    {
        Row r;

        void *source = row_slot((Table *)this, i);
        deserialize_row((char *)source, r);

        result.push_back(r);
    }

    return result;
}

bool Table::delete_by_id(uint32_t id)
{
    for (uint32_t i = 0; i < num_rows; i++)
    {
        Row r;
        void *source = row_slot(this, i);
        deserialize_row((char *)source, r);

        if (r.id == id)
        {
            for (uint32_t j = i + 1; j < num_rows; j++)
            {
                void *dest = row_slot(this, j - 1);
                void *src = row_slot(this, j);
                memcpy(dest, src, ROW_SIZE);
            }

            num_rows--;
            persist_num_rows();
            return true;
        }
    }
    return false;
}

bool Table::update(const Row &row)
{
    for (uint32_t i = 0; i < num_rows; i++)
    {
        Row r;
        void *dest = row_slot(this, i);
        deserialize_row((char *)dest, r);

        if (r.id == row.id)
        {
            serialize_row(row, (char *)dest);
            return true;
        }
    }
    return false;
}