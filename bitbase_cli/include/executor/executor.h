#pragma once
#include "parser/statement.h"

class Executor {
public:
	void execute(const Statement& statement);
};