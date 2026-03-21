#pragma once
#include "parser/statement.h"
#include "storage/table/table.h"

class Executor {
	Table table;
public:
	void execute(const Statement& statement);
};