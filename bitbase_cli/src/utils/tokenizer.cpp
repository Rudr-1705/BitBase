#include "utils/tokenizer.h"
#include <vector>
#include <string>
#include <cctype>

std::vector<std::string> tokenize(const std::string &input)
{
    std::vector<std::string> tokens;
    std::string current;

    for (char c : input)
    {
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
    {
        tokens.push_back(current);
    }

    return tokens;
}