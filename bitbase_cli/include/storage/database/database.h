#pragma once

#include <unordered_map>
#include <string>
#include "storage/table/table.h"

class Database
{
public:
    std::unordered_map<std::string, Table *> tables;

    Database();
    ~Database();

    bool create_table(const std::string &name);
    bool drop_table(const std::string &name);
    Table *get_table(const std::string &name);

    void load_catalog();
    void persist_catalog();
};