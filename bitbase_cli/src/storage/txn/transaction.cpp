#include "storage/txn/transaction.h"
#include "storage/database/database.h"
#include "storage/table/table.h"
#include <sstream>

void TransactionManager::record(const TxnEntry &entry)
{
    log.push_back(entry);
}

void TransactionManager::rollback(Database &db)
{
    for (auto it = log.rbegin(); it != log.rend(); ++it)
    {
        Table *table = db.get_table(it->table);
        if (!table)
            continue;

        std::istringstream ss(it->data);

        if (it->type == "INSERT")
        {
            // undo INSERT → delete
            uint32_t key;
            ss >> key;
            table->delete_by_id(key);
        }
        else if (it->type == "DELETE")
        {
            // undo DELETE → reinsert
            std::vector<std::string> vals;
            std::string val;

            while (ss >> val)
                vals.push_back(val);

            table->insert(vals);
        }
        else if (it->type == "UPDATE")
        {
            // undo UPDATE → restore old value
            uint32_t key;
            std::string col, old_val;

            ss >> key >> col >> old_val;

            table->update_by_id(key, col, old_val);
        }
    }

    log.clear();
}

void TransactionManager::clear()
{
    log.clear();
}