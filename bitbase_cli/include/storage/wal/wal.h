#pragma once

#include <vector>
#include <string>
#include <fstream>

class WALManager
{
private:
    std::ofstream log_file;
    std::string log_filename; // 🔥 IMPORTANT

public:
    WALManager(const std::string &filename);

    void log_insert(const std::string &table,
                    const std::vector<std::string> &values);

    void log_delete(const std::string &table,
                    const std::string &key);

    void log_update(const std::string &table,
                    const std::string &key,
                    const std::string &old_val,
                    const std::string &new_val);

    void recover(class Database &db);

    void flush();
};