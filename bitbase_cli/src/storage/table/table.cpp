#include "storage/table/table.h"
#include <cstring>
#include <iostream>

// ===================== CONSTANTS =====================

constexpr uint32_t METADATA_PAGE_NUM = 0;
constexpr uint32_t NUM_ROWS_OFFSET = 0;
constexpr uint32_t SCHEMA_SIZE_OFFSET = sizeof(uint32_t);

// ===================== CTOR =====================

Table::Table(const char *filename)
{
    pager = new Pager(filename);

    void *page = pager->get_page(METADATA_PAGE_NUM);

    if (pager->file_length == 0)
    {
        std::memset(page, 0, PAGE_SIZE);

        num_rows = 0;
        *(uint32_t *)((char *)page + NUM_ROWS_OFFSET) = 0;
        *(uint32_t *)((char *)page + SCHEMA_SIZE_OFFSET) = 0;

        pager->flush(METADATA_PAGE_NUM);
    }
    else
    {
        num_rows = *(uint32_t *)((char *)page + NUM_ROWS_OFFSET);

        uint32_t schema_size =
            *(uint32_t *)((char *)page + SCHEMA_SIZE_OFFSET);

        if (schema_size > 0)
        {
            const char *schema_data =
                (char *)page + sizeof(uint32_t) + sizeof(uint32_t);

            schema.deserialize(schema_data);
        }
    }

    // ===== INIT ROOT NODE =====
    void *root = pager->get_page(root_page);

    if (*node_type(root) != (uint8_t)NodeType::LEAF)
    {
        initialize_leaf_node(root);
        *node_is_root(root) = 1;
    }
}

// ===================== DTOR =====================

Table::~Table()
{
    persist_num_rows();
    delete pager;
}

// ===================== METADATA =====================

void Table::persist_num_rows()
{
    void *page = pager->get_page(METADATA_PAGE_NUM);

    *(uint32_t *)((char *)page + NUM_ROWS_OFFSET) = num_rows;

    pager->flush(METADATA_PAGE_NUM);
}

// ===================== HELPER =====================

uint32_t Table::get_row_start_page() const
{
    return 2;
}

// ===================== INSERT =====================

void Table::insert(const std::vector<std::string> &values)
{
    std::vector<char> row_bytes =
        serialize_dynamic_row(schema, values);

    uint32_t row_size = row_bytes.size();

    uint32_t page_num = get_row_start_page();

    while (true)
    {
        void *page = pager->get_page(page_num);

        uint32_t *used_ptr = (uint32_t *)page;

        if (*used_ptr == 0)
            *used_ptr = sizeof(uint32_t);

        uint32_t used = *used_ptr;

        if (used + sizeof(uint32_t) + row_size <= PAGE_SIZE)
        {
            char *ptr = (char *)page + used;

            uint32_t offset = used;

            std::memcpy(ptr, &row_size, sizeof(uint32_t));
            ptr += sizeof(uint32_t);

            std::memcpy(ptr, row_bytes.data(), row_size);

            *used_ptr += sizeof(uint32_t) + row_size;

            pager->flush(page_num);

            // ===== B+ TREE INSERT (FIXED) =====
            RowPointer rp{page_num, offset};

            uint32_t key = std::stoi(values[0]);

            SplitResult res = btree_insert(root_page, key, rp, pager); // ✅ FIX

            if (res.did_split)
            {
                uint32_t old_root = root_page;
                create_new_root(pager, old_root, res.new_page, res.key);
                root_page = pager->num_pages - 1;
            }

            break;
        }

        page_num++;
    }

    num_rows++;
    persist_num_rows();
}

// ===================== SELECT =====================
std::vector<std::vector<Value>> Table::get_all_dynamic() const
{
    std::vector<std::vector<Value>> result;

    uint32_t page_num = get_row_start_page();

    while (true)
    {
        void *page = pager->get_page(page_num);

        uint32_t used = *(uint32_t *)page;

        // 🚨 STOP CONDITION (VERY IMPORTANT)
        if (used == 0)
        {
            break;
        }

        if (used < sizeof(uint32_t))
        {
            break;
        }

        char *ptr = (char *)page + sizeof(uint32_t);
        char *end = (char *)page + used;

        while (ptr + sizeof(uint32_t) <= end)
        {
            int32_t row_size;
            memcpy(&row_size, ptr, sizeof(int32_t));

            // 🚨 skip deleted rows
            if (row_size < 0)
            {
                ptr += sizeof(int32_t) + (-row_size);
                continue;
            }

            // SAFETY
            if (row_size == 0 || row_size > PAGE_SIZE)
                break;

            if (ptr + sizeof(uint32_t) + row_size > end)
                break;

            ptr += sizeof(uint32_t);

            std::vector<Value> values;
            deserialize_dynamic_row(schema, ptr, values);

            result.push_back(values);

            ptr += row_size;
        }

        page_num++;
    }

    return result;
}

// ===================== DELETE =====================
bool Table::delete_by_id(uint32_t key)
{
    bool found;

    RowPointer rp = btree_find(root_page, key, pager, found);

    if (!found)
        return false;

    void *page = pager->get_page(rp.page_id);

    char *ptr = (char *)page + rp.offset;

    // 🚨 MARK AS DELETED USING NEGATIVE SIZE
    int32_t row_size;
    std::memcpy(&row_size, ptr, sizeof(int32_t));

    row_size = -row_size; // mark deleted

    std::memcpy(ptr, &row_size, sizeof(int32_t));

    return true;
}

// ===================== UPDATE =====================

bool Table::update()
{
    return false;
}

// ===================== SCHEMA =====================

void Table::set_schema(const Schema &s)
{
    schema = s;

    void *page = pager->get_page(0);

    std::vector<char> schema_bytes = schema.serialize();
    uint32_t size = schema_bytes.size();

    std::memcpy((char *)page + SCHEMA_SIZE_OFFSET, &size, sizeof(uint32_t));

    std::memcpy((char *)page + sizeof(uint32_t) + sizeof(uint32_t),
                schema_bytes.data(), size);

    pager->flush(0);
}

bool Table::find_by_id(uint32_t key, std::vector<Value> &result)
{
    bool found;

    RowPointer rp = btree_find(root_page, key, pager, found);

    if (!found)
        return false;

    void *page = pager->get_page(rp.page_id);

    char *ptr = (char *)page + rp.offset;

    uint32_t row_size;
    std::memcpy(&row_size, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);

    deserialize_dynamic_row(schema, ptr, result);

    return true;
}