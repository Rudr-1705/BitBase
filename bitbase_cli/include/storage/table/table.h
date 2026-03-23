#pragma once

#include <vector>
#include <array>
#include "storage/row_format/row_format.h"
#include "storage/pager/pager.h"

class Table
{
public:
    Pager *pager;
    uint32_t num_rows;

    Table(const char *filename);
    ~Table();

    void insert(const Row &row);
    std::vector<Row> get_all() const;
    bool delete_by_id(uint32_t id);
    bool update(const Row &row);
};