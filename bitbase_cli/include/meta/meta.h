#pragma once
#include <string>

enum class MetaCommandResult {
	SUCCESS,
	EXIT,
	UNRECOGNIZED
};

class MetaCommandHandler {
public:
	MetaCommandResult handle(const std::string& input);
};