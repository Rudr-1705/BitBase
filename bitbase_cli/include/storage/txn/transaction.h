#pragma once

#include <vector>
#include <string>

struct TxnEntry
{
    std::string type; // INSERT / DELETE / UPDATE
    std::string table;
    std::string data;
};

class TransactionManager
{
private:
    std::vector<TxnEntry> log;

public:
    void record(const TxnEntry &entry);
    void rollback(class Database &db);
    void clear();
};