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

		// ===== PRIMARY KEY CHECK =====
		int pk_idx = schema.get_primary_index();

		if (pk_idx != -1)
		{
			uint32_t key;

			try
			{
				key = std::stoul(statement.raw_values[pk_idx]);
			}
			catch (...)
			{
				std::cout << "Error: Invalid primary key value\n";
				break;
			}

			if (table->exists_by_id(key))
			{
				std::cout << "Error: Duplicate primary key\n";
				break;
			}
		}

		// ===== UNIQUE CHECK =====
		for (int i = 0; i < (int)schema.columns.size(); i++)
		{
			if (schema.columns[i].is_unique)
			{
				if (table->exists_value_in_column(i, statement.raw_values[i]))
				{
					std::cout << "Error: Duplicate value for UNIQUE column\n";
					goto insert_end; // clean early exit
				}
			}
		}

		table->insert(statement.raw_values);

		std::cout << "Executed INSERT\n";

	insert_end:
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

		// ================= RANGE =================
		if (statement.is_range)
		{
			auto rows = table->range_query(statement.range_start, statement.range_end);

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
			break;
		}

		// ================= WHERE =================
		if (statement.has_where)
		{
			std::vector<std::vector<Value>> rows;

			int pk_idx = table->schema.get_primary_index();
			std::string pk_name = (pk_idx != -1)
									  ? table->schema.columns[pk_idx].name
									  : "";

			bool has_pk = false;
			uint32_t key = 0;

			for (auto &c : statement.conditions)
			{
				if (c.first == pk_name)
				{
					has_pk = true;
					key = std::stoul(c.second);
				}
			}

			if (has_pk)
				rows = table->find_all_by_id(key);
			else
				rows = table->scan_all_index();

			rows = table->filter_rows(rows, statement.conditions);

			if (rows.empty())
			{
				std::cout << "Row not found\n";
			}
			else
			{
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

		// ================= FULL SCAN =================
		auto rows = table->scan_all_index();

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
			table->delete_all();
			std::cout << "Executed DELETE ALL\n";
			break;
		}

		int pk_idx = table->schema.get_primary_index();
		std::string pk_name = (pk_idx != -1)
								  ? table->schema.columns[pk_idx].name
								  : "";

		if (statement.where_column != pk_name)
		{
			std::cout << "Error: DELETE only supported on primary key\n";
			break;
		}

		uint32_t key = std::stoul(statement.where_value);

		bool deleted = table->delete_by_id(key);

		if (!deleted)
			std::cout << "Row not found\n";
		else
			std::cout << "Executed DELETE\n";

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

		if (!statement.has_where)
		{
			table->update_all(statement.update_column,
							  statement.update_value);

			std::cout << "Executed UPDATE ALL\n";
			break;
		}

		int pk_idx = table->schema.get_primary_index();
		std::string pk_name = (pk_idx != -1)
								  ? table->schema.columns[pk_idx].name
								  : "";

		if (statement.where_column != pk_name)
		{
			std::cout << "Error: UPDATE only supported on primary key\n";
			break;
		}

		uint32_t key = std::stoul(statement.where_value);

		bool updated = table->update_by_id(
			key,
			statement.update_column,
			statement.update_value);

		if (!updated)
			std::cout << "Update failed\n";
		else
			std::cout << "Executed UPDATE\n";

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