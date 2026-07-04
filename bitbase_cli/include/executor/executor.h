#pragma once

#include "parser/statement.h"
#include "storage/database/database.h"
#include "storage/wal/wal.h"
#include "storage/txn/transaction.h"

class Executor
{
private:
	Database db;
	WALManager wal;
	TransactionManager txn;

public:
	Executor(); // constructor (important for recovery)

	void execute(const Statement &statement);
	Database &get_db();
};