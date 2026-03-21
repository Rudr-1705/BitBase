#pragma once

#include <vector>
#include "storage/table/row.h"

class Table {
public:
    // Insert a new row
    void insert(const Row& row);

    // Get all rows (for SELECT *)
    const std::vector<Row>& get_all() const;

    // Delete row by id (returns true if deleted)
    bool delete_by_id(uint32_t id);

    // Update row by id (returns true if updated)
    bool update(const Row& row);

private:
    std::vector<Row> rows;
};