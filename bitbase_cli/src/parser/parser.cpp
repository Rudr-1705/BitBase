#include "parser/parser.h"
#include "utils/tokenizer.h"
#include <iostream>

bool Parser::parse(const std::string &input, Statement &statement, std::string &error)
{
    statement.raw_values.clear();
    statement.schema.columns.clear();
    statement.has_where = false;
    statement.conditions.clear();

    auto tokens = tokenize(input);

    if (tokens.empty())
    {
        error = "Empty input";
        return false;
    }

    // ---------------- INSERT ----------------
    if (tokens[0] == "insert")
    {
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

            if (val.front() == '\'' && val.back() == '\'')
                val = val.substr(1, val.size() - 2);

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
        if (tokens.size() < 4 || tokens[2] != "from")
        {
            error = "Invalid SELECT syntax";
            return false;
        }

        statement.type = StatementType::SELECT;
        statement.table_name = tokens[3];

        statement.has_where = false;
        statement.is_range = false;
        statement.conditions.clear();
        statement.has_order = false;

        // ---------------- WHERE ----------------
        int i = 4;

        if (i < tokens.size() && tokens[i] == "where")
        {
            i++;

            // RANGE OPTIMIZATION
            if (tokens.size() >= i + 7 &&
                tokens[i] == "id" && tokens[i + 1] == ">=" &&
                tokens[i + 3] == "and" &&
                tokens[i + 4] == "id" && tokens[i + 5] == "<=")
            {
                statement.is_range = true;
                statement.range_start = std::stoi(tokens[i + 2]);
                statement.range_end = std::stoi(tokens[i + 6]);
                i += 7;
            }
            else
            {
                while (i < tokens.size())
                {
                    if (i + 2 >= tokens.size())
                        break;

                    Statement::Condition cond;

                    cond.column = tokens[i++];
                    cond.op = tokens[i++];
                    cond.value = tokens[i++];

                    if (cond.value.front() == '\'' && cond.value.back() == '\'')
                        cond.value = cond.value.substr(1, cond.value.size() - 2);

                    statement.conditions.push_back(cond);

                    if (cond.column == "id" && cond.op == "=")
                    {
                        try
                        {
                            statement.where_id = std::stoul(cond.value);
                        }
                        catch (...)
                        {
                        }
                    }

                    if (i < tokens.size() && tokens[i] == "and")
                        i++;
                    else
                        break;
                }

                statement.has_where = !statement.conditions.empty();
            }
        }

        // ---------------- ORDER BY ----------------
        for (; i < tokens.size(); i++)
        {
            if (tokens[i] == "order" && i + 2 < tokens.size() && tokens[i + 1] == "by")
            {
                statement.has_order = true;
                statement.order_column = tokens[i + 2];
                break;
            }
        }

        for (int j = 0; j < tokens.size(); j++)
        {
            if (tokens[j] == "limit" && j + 1 < tokens.size())
            {
                statement.has_limit = true;
                statement.limit = std::stoi(tokens[j + 1]);
                break;
            }
        }

        return true;
    }

    // ---------------- DELETE ----------------
    if (tokens[0] == "delete")
    {
        if (tokens.size() < 3 || tokens[1] != "from")
        {
            error = "Invalid DELETE syntax";
            return false;
        }

        statement.type = StatementType::DELETE;
        statement.table_name = tokens[2];

        if (tokens.size() >= 7 && tokens[3] == "where")
        {
            statement.has_where = true;
            statement.where_column = tokens[4];
            statement.where_value = tokens[6];
            statement.where_id = std::stoi(statement.where_value);
        }
        else
        {
            statement.has_where = false;
        }

        return true;
    }

    // ---------------- UPDATE ----------------
    if (tokens[0] == "update")
    {
        if (tokens.size() < 6 || tokens[2] != "set")
        {
            error = "Invalid UPDATE syntax";
            return false;
        }

        statement.type = StatementType::UPDATE;
        statement.table_name = tokens[1];

        statement.update_column = tokens[3];
        statement.update_value = tokens[5];

        if (statement.update_value.front() == '\'' && statement.update_value.back() == '\'')
        {
            statement.update_value =
                statement.update_value.substr(1, statement.update_value.size() - 2);
        }

        if (tokens.size() >= 10 && tokens[6] == "where")
        {
            statement.has_where = true;
            statement.where_column = tokens[7];
            statement.where_value = tokens[9];
            statement.where_id = std::stoi(statement.where_value);
        }
        else
        {
            statement.has_where = false;
        }

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
        bool pk_found = false; // ✅ per-query, not static

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

            bool is_pk = false;
            bool is_unique = false;

            if (i + 1 < tokens.size() &&
                tokens[i] == "primary" &&
                tokens[i + 1] == "key")
            {
                if (pk_found)
                {
                    error = "Multiple primary keys not allowed";
                    return false;
                }

                is_pk = true;
                pk_found = true;
                i += 2;
            }
            else if (tokens[i] == "unique")
            {
                is_unique = true;
                i++;
            }

            Column col;
            col.name = col_name;
            col.type = type;
            col.is_primary = is_pk;
            col.is_unique = is_unique;

            statement.schema.columns.push_back(col);

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