#include "parser.h"
#include "tokenizer.h"
#include <limits>
#include <stdexcept>
#include <cctype>

bool parse_uint32(const std::string& token,
    uint32_t& out,
    std::string& error_message)
{
    if (token.empty()) {
        error_message = "Invalid id: empty";
        return false;
    }

    for (char c : token) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            error_message = "Invalid id: must contain only digits";
            return false;
        }
    }

    try {
        unsigned long value = std::stoul(token);

        if (value > std::numeric_limits<uint32_t>::max()) {
            error_message = "Invalid id: out of uint32_t range";
            return false;
        }

        out = static_cast<uint32_t>(value);
        return true;
    }
    catch (const std::out_of_range&) {
        error_message = "Invalid id: out of uint32_t range";
        return false;
    }
}

bool parse_int(const std::string& token,
    int& out,
    std::string& error_message)
{
    if (token.empty()) {
        error_message = "Invalid age: empty";
        return false;
    }

    for (size_t i = 0; i < token.size(); ++i) {
        if (i == 0 && token[i] == '-') continue;
        if (!std::isdigit(static_cast<unsigned char>(token[i]))) {
            error_message = "Invalid age: must be an integer";
            return false;
        }
    }

    try {
        long value = std::stol(token);

        if (value < 0 || value > 150) {
            error_message = "Invalid age: out of valid range";
            return false;
        }

        out = static_cast<int>(value);
        return true;
    }
    catch (...) {
        error_message = "Invalid age: must be an integer";
        return false;
    }
}

bool parse_bool(const std::string& token,
    bool& out,
    std::string& error_message)
{
    if (token == "true" || token == "1") {
        out = true;
        return true;
    }

    if (token == "false" || token == "0") {
        out = false;
        return true;
    }

    error_message = "Invalid is_active: expected true/false or 1/0";
    return false;
}

bool Parser::parse(const std::string& input,
    Statement& statement,
    std::string& error_message)
{
    std::vector<std::string> tokens = tokenize(input);

    if (tokens.empty()) {
        error_message = "Please enter a command";
        return false;
    }

    const std::string& op = tokens[0];

    if (op == "insert") {
        if (tokens.size() != 6) {
            error_message = "INSERT expects 5 arguments: id username email age is_active";
            return false;
        }

        if (!parse_uint32(tokens[1], statement.id, error_message)) return false;

        statement.username = tokens[2];
        statement.email = tokens[3];

        if (statement.username.size() > 32) {
            error_message = "Username exceeds 32 characters";
            return false;
        }

        if (statement.email.size() > 255) {
            error_message = "Email exceeds 255 characters";
            return false;
        }

        if (!parse_int(tokens[4], statement.age, error_message)) return false;
        if (!parse_bool(tokens[5], statement.is_active, error_message)) return false;

        statement.type = StatementType::INSERT;
        return true;
    }
    else if (op == "select") {
        if (tokens.size() != 1) {
            error_message = "SELECT expects 0 arguments";
            return false;
        }

        statement.type = StatementType::SELECT;
        return true;
    }
    else if (op == "delete") {
        if (tokens.size() != 2) {
            error_message = "DELETE expects 1 argument: id";
            return false;
        }

        if (!parse_uint32(tokens[1], statement.id, error_message)) return false;

        statement.type = StatementType::DELETE;
        return true;
    }
    else if (op == "update") {
        if (tokens.size() != 6) {
            error_message = "UPDATE expects 5 arguments: id username email age is_active";
            return false;
        }

        if (!parse_uint32(tokens[1], statement.id, error_message)) return false;

        statement.username = tokens[2];
        statement.email = tokens[3];

        if (statement.username.size() > 32) {
            error_message = "Username exceeds 32 characters";
            return false;
        }

        if (statement.email.size() > 255) {
            error_message = "Email exceeds 255 characters";
            return false;
        }

        if (!parse_int(tokens[4], statement.age, error_message)) return false;
        if (!parse_bool(tokens[5], statement.is_active, error_message)) return false;

        statement.type = StatementType::UPDATE;
        return true;
    }
    else if (op == "create") {
        if (tokens.size() != 2 || tokens[1] != "table") {
            error_message = "CREATE TABLE expects no additional arguments";
            return false;
        }

        statement.type = StatementType::CREATE_TABLE;
        return true;
    }
    else if (op == "drop") {
        if (tokens.size() != 2 || tokens[1] != "table") {
            error_message = "DROP TABLE expects no additional arguments";
            return false;
        }

        statement.type = StatementType::DROP_TABLE;
        return true;
    }
    else {
        error_message = "Please enter a valid command";
        return false;
    }
}
