#pragma once

#include "parser/statement.h"
#include "storage/database/database.h"

class Executor
{
private:
	Database db;

public:
	Executor() = default;

	void execute(const Statement &statement);
};