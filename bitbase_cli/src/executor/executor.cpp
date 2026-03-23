#include "executor/executor.h"
#include "storage/table/table.h"
#include <iostream>

Executor::Executor(const char *filename)
	: table(filename)
{
}

void Executor::execute(const Statement &statement)
{
	switch (statement.type)
	{
	case (StatementType::INSERT):
	{
		Row r;
		r.id = statement.id;
		r.username = statement.username;
		r.email = statement.email;
		r.age = statement.age;
		r.is_active = statement.is_active;
		table.insert(r);
		std::cout << "Executed.\n";
		break;
	}

	case (StatementType::SELECT):
	{
		const auto &rows = table.get_all();

		for (const auto &r : rows)
		{
			std::cout << "("
					  << r.id << ", "
					  << r.username << ", "
					  << r.email << ", "
					  << r.age << ", "
					  << (r.is_active ? "true" : "false")
					  << ")\n";
		}

		break;
	}

	case (StatementType::DELETE):
	{
		bool deleted = table.delete_by_id(statement.id);
		std::cout << "Executed.\n";
		break;
	}

	case (StatementType::UPDATE):
	{
		Row r;
		r.id = statement.id;
		r.username = statement.username;
		r.email = statement.email;
		r.age = statement.age;
		r.is_active = statement.is_active;
		table.update(r);
		std::cout << "Executed.\n";
		break;
	}

	case (StatementType::CREATE_TABLE):
	{
		std::cout << "Executed CREATE TABLE\n";
		break;
	}

	case (StatementType::DROP_TABLE):
	{
		std::cout << "Executed DROP TABLE\n";
		break;
	}

	default:
		break;
	}
}