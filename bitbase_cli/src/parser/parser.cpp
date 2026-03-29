#include "parser/parser.h"
#include "utils/tokenizer.h"
#include <iostream>

bool Parser::parse(const std::string &input, Statement &statement, std::string &error)
{
    statement.raw_values.clear();
    statement.schema.columns.clear();
    statement.has_where = false;

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

            std::string val = tokens[i];

            // remove quotes if present
            if (val.front() == '\'' && val.back() == '\'')
            {
                val = val.substr(1, val.size() - 2);
            }

            statement.raw_values.push_back(val);

            i++;

            if (i < tokens.size() && tokens[i] == ",")
                i++;
        }

        return true;
    }

    // ---------------- SELECT ----------------
    if (tokens[0] == "select")
    {
        // select * from users [where id = X]

        if (tokens.size() < 4 || tokens[2] != "from")
        {
            error = "Invalid SELECT syntax";
            return false;
        }

        statement.type = StatementType::SELECT;
        statement.table_name = tokens[3];

        // WHERE support
        if (tokens.size() >= 8 && tokens[4] == "where")
        {
            statement.has_where = true;
            statement.where_column = tokens[5];
            statement.where_value = tokens[7];

            if (statement.where_column == "id")
            {
                statement.where_id = std::stoi(statement.where_value);
            }
        }

        return true;
    }

    // ---------------- DELETE ----------------
    if (tokens[0] == "delete")
    {
        // delete from users where id = X

        if (tokens.size() < 7 || tokens[1] != "from" || tokens[3] != "where")
        {
            error = "Invalid DELETE syntax";
            return false;
        }

        statement.type = StatementType::DELETE;
        statement.table_name = tokens[2];

        statement.has_where = true;
        statement.where_column = tokens[4];
        statement.where_value = tokens[6];

        if (statement.where_column == "id")
        {
            statement.where_id = std::stoi(statement.where_value);
        }

        return true;
    }

    // ---------------- UPDATE ----------------
    if (tokens[0] == "update")
    {
        // not implemented for dynamic schema yet
        statement.type = StatementType::UPDATE;
        statement.table_name = tokens[1];
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

            if (i < tokens.size() && tokens[i] == ",")
                i++;
        }

        return true;
    }

    // ---------------- DROP TABLE ----------------
    if (tokens[0] == "drop")
    {
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