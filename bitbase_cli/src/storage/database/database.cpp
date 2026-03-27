#include "storage/database/database.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <cstdio>

static const std::string DATA_DIR = "data/";
static const std::string CATALOG_FILE = DATA_DIR + "bitbase.meta";

Database::Database()
{
    std::filesystem::create_directory(DATA_DIR);
    load_catalog();
}

Database::~Database()
{
    for (auto &p : tables)
    {
        delete p.second;
    }
}

bool Database::create_table(const std::string &name)
{
    if (tables.count(name))
        return false;

    std::string filename = DATA_DIR + name + ".db";
    tables[name] = new Table(filename.c_str());

    persist_catalog();
    return true;
}

bool Database::drop_table(const std::string &name)
{
    if (!tables.count(name))
        return false;

    // 1. Delete table object (important: closes pager/file)
    delete tables[name];
    tables.erase(name);

    // 2. Delete file from disk
    std::string filename = DATA_DIR + name + ".db";
    if (std::remove(filename.c_str()) != 0)
    {
        std::cerr << "Warning: Failed to delete file " << filename << "\n";
    }

    // 3. Update catalog
    persist_catalog();

    return true;
}
Table *Database::get_table(const std::string &name)
{
    if (!tables.count(name))
        return nullptr;
    return tables[name];
}

void Database::load_catalog()
{
    std::ifstream file(CATALOG_FILE);

    if (!file.is_open())
        return;

    std::string name;
    while (file >> name)
    {
        std::string filename = DATA_DIR + name + ".db";
        tables[name] = new Table(filename.c_str());
    }
}

void Database::persist_catalog()
{
    std::ofstream file(CATALOG_FILE);

    for (auto &p : tables)
    {
        file << p.first << "\n";
    }
}