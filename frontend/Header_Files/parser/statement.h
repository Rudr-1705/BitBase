#pragma once
#include <string>
#include <cstdint>

enum class StatementType {
    INSERT,
    SELECT,
    DELETE,
    UPDATE,
    CREATE_TABLE,
    DROP_TABLE
};

struct Statement {
    StatementType type;
    uint32_t id = 0;
    std::string username;
    std::string email;
    int age = 0;
    bool is_active = false;
    uint32_t created_at = 0;
};
