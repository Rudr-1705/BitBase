#include "storage/wal/wal.h"
#include "storage/database/database.h"
#include "storage/table/table.h"

#include <sstream>
#include <iostream>

WALManager::WALManager(const std::string &filename)
    : log_filename(filename)
{
    log_file.open(log_filename, std::ios::app);
}

// ================= INSERT =================
void WALManager::log_insert(const std::string &table,
                            const std::vector<std::string> &values)
{
    log_file << "INSERT " << table;

    for (const auto &v : values)
    {
        log_file << " \"" << v << "\"";
    }

    log_file << "\n";
}

// ================= DELETE =================
void WALManager::log_delete(const std::string &table,
                            const std::string &key)
{
    log_file << "DELETE " << table << " " << key << "\n";
}

// ================= UPDATE =================
void WALManager::log_update(const std::string &table,
                            const std::string &key,
                            const std::string &old_val,
                            const std::string &new_val)
{
    log_file << "UPDATE " << table << " "
             << key << " "
             << old_val << " -> " << new_val << "\n";
}

// ================= RECOVERY =================
void WALManager::recover(Database &db)
{
    std::ifstream in(log_filename);
    if (!in.is_open())
        return;

    std::vector<std::string> lines;
    std::string line;

    // ===== READ WAL =====
    while (std::getline(in, line))
        lines.push_back(line);

    in.close();

    // 🔥 CRITICAL FIX STARTS HERE

    // CLOSE APPEND HANDLE FIRST
    log_file.close();

    // CLEAR WAL FILE
    std::ofstream clear(log_filename, std::ios::trunc);
    clear.close();

    // REOPEN FOR FUTURE LOGGING
    log_file.open(log_filename, std::ios::app);

    // 🔥 CRITICAL FIX ENDS HERE

    // ===== APPLY WAL =====
    for (auto &line : lines)
    {
        std::istringstream ss(line);

        std::string type, table;
        ss >> type >> table;

        Table *tbl = db.get_table(table);
        if (!tbl)
            continue;

        if (type == "INSERT")
        {
            std::vector<std::string> vals;
            std::string val;

            while (ss >> val)
            {
                if (val.size() >= 2 && val.front() == '"' && val.back() == '"')
                    val = val.substr(1, val.size() - 2);
                else
                    val = "";

                vals.push_back(val);
            }

            if (vals.size() != tbl->schema.columns.size())
                continue;

            // SAFE: insert already prevents duplicates
            tbl->insert(vals);
        }

        else if (type == "DELETE")
        {
            std::string key;
            ss >> key;

            if (key == "ALL")
                tbl->delete_all();
        }

        else if (type == "UPDATE")
        {
            continue;
        }
    }
}

// ================= FLUSH =================
void WALManager::flush()
{
    log_file.flush();
}