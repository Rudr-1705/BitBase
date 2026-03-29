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
	// ===================== INSERT =====================
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

		// ✅ Directly pass values (no Row, no mapping)
		table->insert(statement.raw_values);

		std::cout << "Executed INSERT\n";
		break;
	}

	// ===================== SELECT =====================
	case StatementType::SELECT:
	{
		Table *table = db.get_table(statement.table_name);

		if (!table)
		{
			std::cout << "Error: Table not found\n";
			break;
		}

		if (statement.has_where)
		{
			std::vector<Value> row;

			if (table->find_by_id(statement.where_id, row))
			{
				std::cout << "(";
				for (size_t i = 0; i < row.size(); i++)
				{
					std::visit([](auto &&val)
							   { std::cout << val; }, row[i]);

					if (i != row.size() - 1)
						std::cout << ", ";
				}
				std::cout << ")\n";
			}
			else
			{
				std::cout << "Row not found\n";
			}
		}
		else
		{
			auto rows = table->get_all_dynamic();

			for (const auto &row : rows)
			{
				std::cout << "(";
				for (size_t i = 0; i < row.size(); i++)
				{
					std::visit([](auto &&val)
							   { std::cout << val; }, row[i]);

					if (i != row.size() - 1)
						std::cout << ", ";
				}
				std::cout << ")\n";
			}
		}

		break;
	}

	// ===================== DELETE =====================
	case StatementType::DELETE:
	{
		Table *table = db.get_table(statement.table_name);

		if (!table)
		{
			std::cout << "Error: Table not found\n";
			break;
		}

		if (!statement.has_where)
		{
			std::cout << "DELETE requires WHERE id = ...\n";
			break;
		}

		bool deleted = table->delete_by_id(statement.where_id);

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

	// ===================== UPDATE =====================
	case StatementType::UPDATE:
	{
		Table *table = db.get_table(statement.table_name);

		if (!table)
		{
			std::cout << "Error: Table not found\n";
			break;
		}

		// ⚠️ Not implemented for dynamic rows yet
		std::cout << "UPDATE not supported yet\n";
		break;
	}

	// ===================== CREATE =====================
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

	// ===================== DROP =====================
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