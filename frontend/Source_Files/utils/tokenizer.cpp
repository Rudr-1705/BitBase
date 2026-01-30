#include "tokenizer.h"
#include "string_utils.h"

#include <sstream>

std::vector<std::string> tokenize(const std::string& input) {
    std::vector<std::string> tokens;

    std::string trimmed = trim(input);

    std::stringstream ss(trimmed);
    std::string token;

    while (ss >> token) {
        tokens.push_back(token);
    }

    return tokens;
}
