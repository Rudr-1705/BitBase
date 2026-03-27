#pragma once
#include <string>
#include <cstdint>
#include "storage/schema/schema.h"

enum class StatementType
{
    INSERT,
    SELECT,
    DELETE,
    UPDATE,
    CREATE_TABLE,
    DROP_TABLE
};

struct Statement
{
    StatementType type;

    std::string table_name;
    Schema schema;

    std::vector<std::string> raw_values;

    uint32_t id = 0;
    std::string username;
    std::string email;
    int age = 0;
    bool is_active = false;
    uint32_t created_at = 0;
};
