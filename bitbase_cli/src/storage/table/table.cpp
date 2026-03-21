#include "storage/table/table.h"

// INSERT
void Table::insert(const Row& row) {
    rows.push_back(row);
}

// SELECT *
const std::vector<Row>& Table::get_all() const {
    return rows;
}

// DELETE
bool Table::delete_by_id(uint32_t id) {
    for (auto it = rows.begin(); it != rows.end(); ++it) {
        if (it->id == id) {
            rows.erase(it);
            return true;
        }
    }
    return false;
}

// UPDATE
bool Table::update(const Row& row) {
    for (auto& r : rows) {
        if (r.id == row.id) {
            r = row;
            return true;
        }
    }
    return false;
}