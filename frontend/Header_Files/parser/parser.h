#pragma once
#include <string>
#include "statement.h"

class Parser {
public:
	bool parse(const std::string& input, Statement& statement, std::string& error_message);
};