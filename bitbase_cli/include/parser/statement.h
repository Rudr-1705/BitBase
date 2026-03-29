#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "storage/schema/schema.h"

// ===================== TYPES =====================

enum class StatementType
{
    INSERT,
    SELECT,
    DELETE,
    UPDATE,
    CREATE_TABLE,
    DROP_TABLE
};

// ===================== STATEMENT =====================

struct Statement
{
    StatementType type;

    // -------- common --------
    std::string table_name;

    // -------- CREATE TABLE --------
    Schema schema;

    // -------- INSERT --------
    std::vector<std::string> raw_values;

    // -------- WHERE (for SELECT / DELETE / UPDATE) --------
    bool has_where = false;
    std::string where_column; // e.g. "id"
    std::string where_value;  // keep string, convert later

    // -------- OPTIONAL: parsed numeric (fast path) --------
    uint32_t where_id = 0;

    // -------- FUTURE (optional, safe to keep) --------
    bool has_limit = false;
    uint32_t limit = 0;
};