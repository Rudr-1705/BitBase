#pragma once
#include "parser/statement.h"
#include "storage/table/table.h"

class Executor
{
public:
	Table table;
	Executor(const char *filename);
	void execute(const Statement &statement);
};