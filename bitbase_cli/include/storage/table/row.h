#pragma once

#include <string>
#include <cstdint>

struct Row {
    uint32_t id;
    std::string username;
    std::string email;
    int age;
    bool is_active;
};