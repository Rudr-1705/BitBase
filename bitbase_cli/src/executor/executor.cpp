#include "executor/executor.h"
#include <iostream>

Database &Executor::get_db()
{
	return db;
}

void fill_row_from_schema(const Schema &schema,
						  const std::vector<std::string> &values,
						  Row &r)
{
	for (size_t i = 0; i < schema.columns.size(); i++)
	{
		const auto &col = schema.columns[i];
		const std::string &val = values[i];

		if (col.name == "id")
		{
			r.id = std::stoi(val);
		}
		else if (col.name == "username")
		{
			r.username = val;
		}
		else if (col.name == "email")
		{
			r.email = val;
		}
		else if (col.name == "age")
		{
			r.age = std::stoi(val);
		}
		else if (col.name == "is_active")
		{
			r.is_active = (val == "true");
		}
	}
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

		const Schema &schema = table->schema;

		if (statement.raw_values.size() != schema.columns.size())
		{
			std::cout << "Column count mismatch\n";
			break;
		}

		Row r;

		try
		{
			fill_row_from_schema(schema, statement.raw_values, r);
		}
		catch (...)
		{
			std::cout << "Type conversion error\n";
			break;
		}

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