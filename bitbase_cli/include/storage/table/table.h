#pragma once

#include <vector>
#include <string>

#include "storage/pager/pager.h"
#include "storage/schema/schema.h"
#include "storage/schema/value.h"
#include "storage/row_format/dynamic_row_format.h"
#include "storage/btree/node.h"

class Table
{
public:
    Pager *pager;
    uint32_t num_rows;

    Schema schema;

    uint32_t root_page = 1; // B+ tree root

    Table(const char *filename);
    ~Table();

    void set_schema(const Schema &s);

    void insert(const std::vector<std::string> &values);
    std::vector<std::vector<Value>> get_all_dynamic() const;

    bool find_by_id(uint32_t key, std::vector<Value> &result);
    bool delete_by_id(uint32_t id);
    bool update();
    std::vector<std::vector<Value>> range_query(uint32_t start, uint32_t end);
    std::vector<std::vector<Value>> scan_all_index();
    bool update_by_id(uint32_t key,
                      const std::string &column,
                      const std::string &value);

private:
    void persist_num_rows();
    uint32_t get_row_start_page() const;
};