#include "parser/parser.h"
#include "utils/tokenizer.h"
#include <iostream>

bool Parser::parse(const std::string &input, Statement &statement, std::string &error)
{
    statement.raw_values.clear();
    statement.schema.columns.clear();
    
    auto tokens = tokenize(input);

    if (tokens.empty())
    {
        error = "Empty input";
        return false;
    }

    // ---------------- INSERT ----------------
    if (tokens[0] == "insert")
    {
        // INSERT INTO users VALUES ( ... )

        if (tokens.size() < 6 || tokens[1] != "into" || tokens[3] != "values")
        {
            error = "Invalid INSERT syntax";
            return false;
        }

        statement.type = StatementType::INSERT;
        statement.table_name = tokens[2];

        if (tokens[4] != "(")
        {
            error = "Expected '(' after VALUES";
            return false;
        }

        int i = 5;

        while (i < tokens.size())
        {
            if (tokens[i] == ")")
                break;

            statement.raw_values.push_back(tokens[i]);

            i++;

            if (tokens[i] == ",")
                i++;
        }

        return true;
    }

    // ---------------- SELECT ----------------
    if (tokens[0] == "select")
    {
        // select * from users
        if (tokens.size() < 4 || tokens[2] != "from")
        {
            error = "Invalid SELECT syntax";
            return false;
        }

        statement.type = StatementType::SELECT;
        statement.table_name = tokens[3];

        return true;
    }

    // ---------------- DELETE ----------------
    if (tokens[0] == "delete")
    {
        // delete from users 1
        if (tokens.size() < 4 || tokens[1] != "from")
        {
            error = "Invalid DELETE syntax";
            return false;
        }

        statement.type = StatementType::DELETE;
        statement.table_name = tokens[2];
        statement.id = std::stoi(tokens[3]);

        return true;
    }

    // ---------------- UPDATE ----------------
    if (tokens[0] == "update")
    {
        // update users 1 alice mail 20 true
        if (tokens.size() < 7)
        {
            error = "Invalid UPDATE syntax";
            return false;
        }

        statement.type = StatementType::UPDATE;
        statement.table_name = tokens[1];

        statement.id = std::stoi(tokens[2]);
        statement.username = tokens[3];
        statement.email = tokens[4];
        statement.age = std::stoi(tokens[5]);
        statement.is_active = (tokens[6] == "true");

        return true;
    }

    // ---------------- CREATE TABLE ----------------
    if (tokens[0] == "create" && tokens[1] == "table")
    {
        statement.type = StatementType::CREATE_TABLE;
        statement.table_name = tokens[2];

        if (tokens[3] != "(")
        {
            error = "Expected '(' after table name";
            return false;
        }

        int i = 4;

        while (i < tokens.size())
        {
            if (tokens[i] == ")")
                break;

            std::string col_name = tokens[i++];
            std::string type_str = tokens[i++];

            DataType type;

            if (type_str == "INT")
                type = DataType::INT32;
            else if (type_str == "BIGINT")
                type = DataType::INT64;
            else if (type_str == "FLOAT")
                type = DataType::FLOAT;
            else if (type_str == "DOUBLE")
                type = DataType::DOUBLE;
            else if (type_str == "BOOL")
                type = DataType::BOOL;
            else if (type_str == "TEXT")
                type = DataType::TEXT;
            else
            {
                error = "Unknown data type: " + type_str;
                return false;
            }

            statement.schema.add_column(col_name, type);

            if (tokens[i] == ",")
                i++; // skip comma
        }

        return true;
    }

    // ---------------- DROP TABLE ----------------
    if (tokens[0] == "drop")
    {
        // drop table users
        if (tokens.size() < 3 || tokens[1] != "table")
        {
            error = "Invalid DROP syntax";
            return false;
        }

        statement.type = StatementType::DROP_TABLE;
        statement.table_name = tokens[2];

        return true;
    }

    error = "Unrecognized command";
    return false;
}