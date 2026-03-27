#include "utils/tokenizer.h"
#include <vector>
#include <string>
#include <cctype>

std::vector<std::string> tokenize(const std::string &input)
{
    std::vector<std::string> tokens;
    std::string current;
    bool in_string = false;

    for (size_t i = 0; i < input.size(); i++)
    {
        char c = input[i];

        if (c == '\'')
        {
            if (in_string)
            {
                tokens.push_back(current);
                current.clear();
                in_string = false;
            }
            else
            {
                in_string = true;
            }
            continue;
        }

        if (in_string)
        {
            current += c;
            continue;
        }

        if (std::isspace(c))
        {
            if (!current.empty())
            {
                tokens.push_back(current);
                current.clear();
            }
        }
        else if (c == '(' || c == ')' || c == ',' || c == ';')
        {
            if (!current.empty())
            {
                tokens.push_back(current);
                current.clear();
            }
            tokens.emplace_back(1, c);
        }
        else
        {
            current += c;
        }
    }

    if (!current.empty())
        tokens.push_back(current);

    return tokens;
}