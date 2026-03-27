#include "parser/parser.h"
#include "utils/tokenizer.h"
#include <iostream>

bool Parser::parse(const std::string &input, Statement &statement, std::string &error)
{
    auto tokens = tokenize(input);

    if (tokens.empty())
    {
        error = "Empty input";
        return false;
    }

    // ---------------- INSERT ----------------
    if (tokens[0] == "insert")
    {
        // insert into users 1 alice mail 20 true
        if (tokens.size() < 8 || tokens[1] != "into")
        {
            error = "Invalid INSERT syntax";
            return false;
        }

        statement.type = StatementType::INSERT;
        statement.table_name = tokens[2];

        statement.id = std::stoi(tokens[3]);
        statement.username = tokens[4];
        statement.email = tokens[5];
        statement.age = std::stoi(tokens[6]);
        statement.is_active = (tokens[7] == "true");

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
    if (tokens[0] == "create")
    {
        // create table users
        if (tokens.size() < 3 || tokens[1] != "table")
        {
            error = "Invalid CREATE syntax";
            return false;
        }

        statement.type = StatementType::CREATE_TABLE;
        statement.table_name = tokens[2];

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