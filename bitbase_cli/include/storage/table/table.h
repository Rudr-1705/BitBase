#pragma once

#include <vector>
#include <array>
#include "storage/pager/pager.h"
#include "storage/schema/schema.h"
// #include "storage/schema/row.h"
#include "storage/row_format/row_format.h"

class Table
{
public:
    Pager *pager;
    uint32_t num_rows;

    Table(const char *filename);
    ~Table();

    Schema schema;
    void set_schema(const Schema &s);

    void insert(const Row &row);
    std::vector<Row> get_all() const;
    bool delete_by_id(uint32_t id);
    bool update(const Row &row);

private:
    void persist_num_rows();
};