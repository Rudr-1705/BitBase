#include "storage/table/table.h"
#include <cstring>
#include <iostream>
#include <algorithm>

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

        if (used + sizeof(int32_t) + row_size <= PAGE_SIZE)
        {
            char *ptr = (char *)page + used;

            uint32_t offset = used;

            // write size
            std::memcpy(ptr, &row_size, sizeof(int32_t));
            ptr += sizeof(int32_t);

            // write data
            std::memcpy(ptr, row_bytes.data(), row_size);

            *used_ptr += sizeof(int32_t) + row_size;

            pager->flush(page_num);

            // ================= INDEX INSERT =================
            int pk_idx = schema.get_primary_index();

            if (pk_idx != -1)
            {
                uint32_t key;

                try
                {
                    key = std::stoul(values[pk_idx]);
                }
                catch (...)
                {
                    std::cout << "Error: Invalid primary key value\n";
                    return;
                }

                RowPointer rp{page_num, offset};

                SplitResult res = btree_insert(root_page, key, rp, pager);

                if (res.did_split)
                {
                    uint32_t old_root = root_page;
                    create_new_root(pager, old_root, res.new_page, res.key);
                    root_page = pager->num_pages - 1;
                }
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
    auto rows = find_all_by_id(key);

    if (rows.empty())
        return false;

    // delete ALL matching keys from B+ tree
    while (true)
    {
        bool removed = btree_delete(root_page, key, pager);
        if (!removed)
            break;
    }

    // mark storage rows deleted
    uint32_t leaf = btree_find_leaf(root_page, key, pager);

    while (leaf != 0)
    {
        void *node = pager->get_page(leaf);
        uint32_t n = *leaf_node_num_cells(node);

        for (uint32_t i = 0; i < n; i++)
        {
            uint32_t k = *leaf_node_key(node, i);

            if (k < key)
                continue;
            if (k > key)
                return true;

            RowPointer rp = *leaf_node_value(node, i);

            void *page = pager->get_page(rp.page_id);
            char *ptr = (char *)page + rp.offset;

            int32_t row_size;
            std::memcpy(&row_size, ptr, sizeof(int32_t));

            if (row_size > 0)
            {
                row_size = -row_size;
                std::memcpy(ptr, &row_size, sizeof(int32_t));
            }
        }

        leaf = *leaf_node_next_leaf(node);
    }

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

    // ================= FIND COLUMN =================
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

    // ================= 🔥 PRIMARY KEY CHECK =================
    int pk_idx = schema.get_primary_index();

    if (pk_idx != -1 && (int)col_idx == pk_idx)
    {
        uint32_t new_key;

        try
        {
            new_key = std::stoul(value);
        }
        catch (...)
        {
            std::cout << "Invalid primary key value\n";
            return false;
        }

        // only check if key is actually changing
        if (new_key != key && exists_by_id(new_key))
        {
            std::cout << "Error: Duplicate primary key\n";
            return false;
        }
    }

    // ================= UNIQUE CHECK =================
    if (schema.columns[col_idx].is_unique)
    {
        // if value is changing
        if (value_to_string(values[col_idx]) != value)
        {
            if (exists_value_in_column(col_idx, value))
            {
                std::cout << "Error: Duplicate value for UNIQUE column\n";
                return false;
            }
        }
    }

    // ================= TYPE CONVERSION =================
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

    // ================= SERIALIZE =================
    std::vector<std::string> str_values;
    for (auto &v : values)
        str_values.push_back(value_to_string(v));

    std::vector<char> new_bytes =
        serialize_dynamic_row(schema, str_values);

    uint32_t new_size = new_bytes.size();

    // ================= WRITE =================
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

std::vector<std::vector<Value>> Table::find_all_by_id(uint32_t key)
{
    std::vector<std::vector<Value>> result;

    uint32_t leaf = btree_find_leaf(root_page, key, pager);

    while (leaf != 0)
    {
        void *node = pager->get_page(leaf);
        uint32_t n = *leaf_node_num_cells(node);

        for (uint32_t i = 0; i < n; i++)
        {
            uint32_t k = *leaf_node_key(node, i);

            if (k < key)
                continue;

            if (k > key)
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

        leaf = *leaf_node_next_leaf(node);
    }

    return result;
}

void Table::delete_all()
{
    int pk_idx = schema.get_primary_index();

    if (pk_idx == -1)
        return;

    auto rows = scan_all_index();

    for (auto &row : rows)
    {
        uint32_t key = std::stoul(value_to_string(row[pk_idx]));
        delete_by_id(key);
    }
}

void Table::update_all(const std::string &column,
                       const std::string &value)
{
    int pk_idx = schema.get_primary_index();

    if (pk_idx == -1)
        return;

    auto rows = scan_all_index();

    for (auto &row : rows)
    {
        uint32_t key = std::stoul(value_to_string(row[pk_idx]));
        update_by_id(key, column, value);
    }
}

std::vector<std::vector<Value>> Table::filter_rows(
    const std::vector<std::vector<Value>> &rows,
    const std::vector<Statement::Condition> &conds)
{
    std::vector<std::vector<Value>> result;

    for (const auto &row : rows)
    {
        bool ok = true;

        for (const auto &cond : conds)
        {
            int idx = schema.get_column_index(cond.column);
            if (idx == -1)
            {
                ok = false;
                break;
            }

            std::string val = value_to_string(row[idx]);
            std::string target = cond.value;

            DataType type = schema.columns[idx].type;

            try
            {
                // ===== NUMERIC TYPES =====
                if (type == DataType::INT32 || type == DataType::INT64)
                {
                    long long v = std::stoll(val);
                    long long t = std::stoll(target);

                    if (cond.op == "=" && v != t)
                        ok = false;
                    else if (cond.op == "!=" && v == t)
                        ok = false;
                    else if (cond.op == ">" && !(v > t))
                        ok = false;
                    else if (cond.op == "<" && !(v < t))
                        ok = false;
                }
                else if (type == DataType::FLOAT || type == DataType::DOUBLE)
                {
                    double v = std::stod(val);
                    double t = std::stod(target);

                    if (cond.op == "=" && v != t)
                        ok = false;
                    else if (cond.op == "!=" && v == t)
                        ok = false;
                    else if (cond.op == ">" && !(v > t))
                        ok = false;
                    else if (cond.op == "<" && !(v < t))
                        ok = false;
                }
                else
                {
                    // ===== STRING / BOOL =====
                    if (cond.op == "=" && val != target)
                        ok = false;
                    else if (cond.op == "!=" && val == target)
                        ok = false;
                    else if (cond.op == ">" && !(val > target))
                        ok = false;
                    else if (cond.op == "<" && !(val < target))
                        ok = false;
                }
            }
            catch (...)
            {
                // conversion failed → treat as mismatch
                ok = false;
            }

            if (!ok)
                break;
        }

        if (ok)
            result.push_back(row);
    }

    return result;
}

bool Table::exists_by_id(uint32_t key)
{
    bool found;
    RowPointer rp = btree_find(root_page, key, pager, found);

    if (!found)
        return false;

    void *page = pager->get_page(rp.page_id);
    char *ptr = (char *)page + rp.offset;

    int32_t row_size;
    std::memcpy(&row_size, ptr, sizeof(int32_t));

    return row_size > 0; // not deleted
}

bool Table::exists_value_in_column(int col_idx, const std::string &value)
{
    auto rows = scan_all_index();

    for (auto &row : rows)
    {
        if (value_to_string(row[col_idx]) == value)
            return true;
    }

    return false;
}

std::vector<std::vector<Value>> Table::order_rows(
    std::vector<std::vector<Value>> rows,
    const std::string &column)
{
    int idx = schema.get_column_index(column);
    if (idx == -1)
        return rows;

    DataType type = schema.columns[idx].type;

    std::sort(rows.begin(), rows.end(),
              [&](const std::vector<Value> &a, const std::vector<Value> &b)
              {
                  std::string va = value_to_string(a[idx]);
                  std::string vb = value_to_string(b[idx]);

                  try
                  {
                      if (type == DataType::INT32 || type == DataType::INT64)
                          return std::stoll(va) < std::stoll(vb);

                      if (type == DataType::FLOAT || type == DataType::DOUBLE)
                          return std::stod(va) < std::stod(vb);
                  }
                  catch (...)
                  {
                  }

                  return va < vb;
              });

    return rows;
}