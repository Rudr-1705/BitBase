#pragma once

#include <vector>
#include <string>

#include "storage/pager/pager.h"
#include "storage/schema/schema.h"
#include "storage/schema/value.h"
#include "storage/row_format/dynamic_row_format.h"
#include "storage/btree/node.h"
#include "parser/statement.h"

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
    std::vector<std::vector<Value>> find_all_by_id(uint32_t key);
    bool delete_by_id(uint32_t id);
    std::vector<std::vector<Value>> range_query(uint32_t start, uint32_t end);
    std::vector<std::vector<Value>> scan_all_index();
    bool update_by_id(uint32_t key,
                      const std::string &column,
                      const std::string &value);
    void delete_all();
    void update_all(const std::string &column, const std::string &value);

    std::vector<std::vector<Value>> filter_rows(
        const std::vector<std::vector<Value>> &rows,
        const std::vector<Statement::Condition> &conds);

    bool exists_by_id(uint32_t key);
    bool exists_value_in_column(int col_idx, const std::string &value);

    std::vector<std::vector<Value>> order_rows(
        std::vector<std::vector<Value>> rows,
        const std::string &column);

private:
    void persist_num_rows();
    uint32_t get_row_start_page() const;
};