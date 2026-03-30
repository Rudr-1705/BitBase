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

// 🔥 FIXED VALUE → STRING
std::string value_to_string(const Value &v)
{
    return std::visit([](auto &&val) -> std::string
                      {
        using T = std::decay_t<decltype(val)>;

        if constexpr (std::is_same_v<T, std::string>)
            return val;
        else if constexpr (std::is_same_v<T, bool>)
            return val ? "true" : "false";
        else
            return std::to_string(val); }, v);
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

            std::memcpy(ptr, &row_size, sizeof(int32_t));
            ptr += sizeof(int32_t);

            std::memcpy(ptr, row_bytes.data(), row_size);

            *used_ptr += sizeof(int32_t) + row_size;

            pager->flush(page_num);

            // ===== B+ TREE INSERT =====
            RowPointer rp{page_num, offset};

            uint32_t key = std::stoi(values[0]);

            SplitResult res = btree_insert(root_page, key, rp, pager);

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

        if (used <= sizeof(uint32_t))
            break;

        char *ptr = (char *)page + sizeof(uint32_t);
        char *end = (char *)page + used;

        while (ptr + sizeof(int32_t) <= end)
        {
            int32_t row_size;
            std::memcpy(&row_size, ptr, sizeof(int32_t));

            if (row_size == 0 || std::abs(row_size) > PAGE_SIZE)
                break;

            if (row_size < 0)
            {
                ptr += sizeof(int32_t) + (-row_size);
                continue;
            }

            if (ptr + sizeof(int32_t) + row_size > end)
                break;

            ptr += sizeof(int32_t);

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

    if (!btree_delete(root_page, key, pager))
        return false;

    void *page = pager->get_page(rp.page_id);
    char *ptr = (char *)page + rp.offset;

    int32_t row_size;
    std::memcpy(&row_size, ptr, sizeof(int32_t));

    row_size = -row_size;
    std::memcpy(ptr, &row_size, sizeof(int32_t));

    return true;
}

// ===================== FIND =====================

bool Table::find_by_id(uint32_t key, std::vector<Value> &result)
{
    bool found;

    RowPointer rp = btree_find(root_page, key, pager, found);

    if (!found)
        return false;

    void *page = pager->get_page(rp.page_id);
    char *ptr = (char *)page + rp.offset;

    int32_t row_size;
    std::memcpy(&row_size, ptr, sizeof(int32_t));

    if (row_size < 0)
        return false;

    ptr += sizeof(int32_t);

    deserialize_dynamic_row(schema, ptr, result);

    return true;
}

// ===================== RANGE =====================

std::vector<std::vector<Value>> Table::range_query(uint32_t start, uint32_t end)
{
    std::vector<std::vector<Value>> result;

    uint32_t leaf_page = btree_find_leaf(root_page, start, pager);

    while (leaf_page != 0)
    {
        void *node = pager->get_page(leaf_page);

        uint32_t n = *leaf_node_num_cells(node);

        for (uint32_t i = 0; i < n; i++)
        {
            uint32_t key = *leaf_node_key(node, i);

            if (key < start)
                continue;

            if (key > end)
                return result;

            RowPointer rp = *leaf_node_value(node, i);

            void *page = pager->get_page(rp.page_id);
            char *ptr = (char *)page + rp.offset;

            int32_t row_size;
            std::memcpy(&row_size, ptr, sizeof(int32_t));

            if (row_size < 0)
                continue;

            ptr += sizeof(int32_t);

            std::vector<Value> values;
            deserialize_dynamic_row(schema, ptr, values);

            result.push_back(values);
        }

        leaf_page = *leaf_node_next_leaf(node);
    }

    return result;
}

// ===================== INDEX SCAN =====================

std::vector<std::vector<Value>> Table::scan_all_index()
{
    std::vector<std::vector<Value>> result;

    uint32_t leaf = find_leftmost_leaf(pager, root_page);

    while (leaf != 0)
    {
        void *node = pager->get_page(leaf);

        uint32_t n = *leaf_node_num_cells(node);

        for (uint32_t i = 0; i < n; i++)
        {
            RowPointer rp = *leaf_node_value(node, i);

            void *page = pager->get_page(rp.page_id);
            char *ptr = (char *)page + rp.offset;

            int32_t row_size;
            std::memcpy(&row_size, ptr, sizeof(int32_t));

            if (row_size < 0)
                continue;

            ptr += sizeof(int32_t);

            std::vector<Value> values;
            deserialize_dynamic_row(schema, ptr, values);

            result.push_back(values);
        }

        leaf = *leaf_node_next_leaf(node);
    }

    return result;
}

// ===================== UPDATE =====================

bool Table::update_by_id(uint32_t key,
                         const std::string &column,
                         const std::string &value)
{
    bool found;
    RowPointer rp = btree_find(root_page, key, pager, found);

    if (!found)
        return false;

    void *page = pager->get_page(rp.page_id);
    char *ptr = (char *)page + rp.offset;

    int32_t row_size;
    std::memcpy(&row_size, ptr, sizeof(int32_t));

    if (row_size < 0)
        return false;

    uint32_t old_size = row_size;

    ptr += sizeof(int32_t);

    std::vector<Value> values;
    deserialize_dynamic_row(schema, ptr, values);

    int col_idx = -1;
    for (size_t i = 0; i < schema.columns.size(); i++)
    {
        if (schema.columns[i].name == column)
        {
            col_idx = i;
            break;
        }
    }

    if (col_idx == -1)
        return false;

    DataType type = schema.columns[col_idx].type;

    if (type == DataType::INT32)
        values[col_idx] = std::stoi(value);
    else if (type == DataType::INT64)
        values[col_idx] = std::stoll(value);
    else if (type == DataType::FLOAT)
        values[col_idx] = std::stof(value);
    else if (type == DataType::DOUBLE)
        values[col_idx] = std::stod(value);
    else if (type == DataType::BOOL)
        values[col_idx] = (value == "true");
    else if (type == DataType::TEXT)
        values[col_idx] = value;

    std::vector<std::string> str_values;
    for (auto &v : values)
        str_values.push_back(value_to_string(v));

    std::vector<char> new_bytes =
        serialize_dynamic_row(schema, str_values);

    uint32_t new_size = new_bytes.size();

    if (new_size <= old_size)
    {
        std::memcpy((char *)page + rp.offset + sizeof(int32_t),
                    new_bytes.data(), new_size);
    }
    else
    {
        int32_t neg = -old_size;
        std::memcpy((char *)page + rp.offset, &neg, sizeof(int32_t));

        insert(str_values);
    }

    return true;
}

// ===================== SCHEMA =====================

void Table::set_schema(const Schema &s)
{
    schema = s;

    void *page = pager->get_page(METADATA_PAGE_NUM);

    std::vector<char> schema_bytes = schema.serialize();
    uint32_t size = schema_bytes.size();

    // write schema size
    std::memcpy((char *)page + SCHEMA_SIZE_OFFSET,
                &size,
                sizeof(uint32_t));

    // write schema data
    std::memcpy((char *)page + sizeof(uint32_t) + sizeof(uint32_t),
                schema_bytes.data(),
                size);

    pager->flush(METADATA_PAGE_NUM);
}