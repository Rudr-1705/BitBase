#include "executor/executor.h"
#include <iostream>

Database &Executor::get_db()
{
	return db;
}

void Executor::execute(const Statement &statement)
{
	switch (statement.type)
	{
	case StatementType::INSERT:
	{
		Table *table = db.get_table(statement.table_name);

		if (!table)
		{
			std::cout << "Error: Table not found\n";
			break;
		}

		Row r;
		r.id = statement.id;
		r.username = statement.username;
		r.email = statement.email;
		r.age = statement.age;
		r.is_active = statement.is_active;

		table->insert(r);

		std::cout << "Executed INSERT\n";
		break;
	}

	case StatementType::SELECT:
	{
		Table *table = db.get_table(statement.table_name);

		if (!table)
		{
			std::cout << "Error: Table not found\n";
			break;
		}

		const auto rows = table->get_all();

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

	case StatementType::DELETE:
	{
		Table *table = db.get_table(statement.table_name);

		if (!table)
		{
			std::cout << "Error: Table not found\n";
			break;
		}

		bool deleted = table->delete_by_id(statement.id);

		if (!deleted)
		{
			std::cout << "Row not found\n";
		}
		else
		{
			std::cout << "Executed DELETE\n";
		}

		break;
	}

	case StatementType::UPDATE:
	{
		Table *table = db.get_table(statement.table_name);

		if (!table)
		{
			std::cout << "Error: Table not found\n";
			break;
		}

		Row r;
		r.id = statement.id;
		r.username = statement.username;
		r.email = statement.email;
		r.age = statement.age;
		r.is_active = statement.is_active;

		bool updated = table->update(r);

		if (!updated)
		{
			std::cout << "Row not found\n";
		}
		else
		{
			std::cout << "Executed UPDATE\n";
		}

		break;
	}

	case StatementType::CREATE_TABLE:
	{
		if (!db.create_table(statement.table_name))
		{
			std::cout << "Table already exists\n";
		}
		else
		{
			Table *table = db.get_table(statement.table_name);

			table->set_schema(statement.schema);

			std::cout << "Executed CREATE TABLE\n";
		}

		break;
	}

	case StatementType::DROP_TABLE:
	{
		if (!db.drop_table(statement.table_name))
		{
			std::cout << "Table not found\n";
		}
		else
		{
			std::cout << "Executed DROP TABLE\n";
		}

		break;
	}

	default:
		break;
	}
}