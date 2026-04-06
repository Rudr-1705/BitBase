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

    std::string table_name;
    Schema schema;

    std::vector<std::string> raw_values;

    // ===== NEW =====
    struct Condition
    {
        std::string column;
        std::string op;
        std::string value;
    };

    std::vector<Condition> conditions;

    // ===== existing =====
    bool has_where = false;
    uint32_t where_id = 0;

    bool is_range = false;
    uint32_t range_start = 0;
    uint32_t range_end = 0;

    std::string where_column;
    std::string where_value;

    std::string update_column;
    std::string update_value;

    bool has_order = false;
    std::string order_column;

    bool has_limit = false;
    int limit = 0;
};