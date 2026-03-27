#pragma once
#include <string>
#include <storage/database/database.h>

enum class MetaCommandResult
{
	SUCCESS,
	EXIT,
	UNRECOGNIZED
};

class MetaCommandHandler
{
public:
	MetaCommandResult handle(const std::string &input, Database &db);
};