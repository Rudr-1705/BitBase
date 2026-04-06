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

		int pk_idx = table->schema.get_primary_index();

		std::vector<std::vector<Value>> rows;

		// ================= RANGE =================
		if (statement.is_range)
		{
			rows = table->range_query(statement.range_start, statement.range_end);
		}
		else if (statement.has_where)
		{
			std::string pk_name = (pk_idx != -1)
									  ? table->schema.columns[pk_idx].name
									  : "";

			bool has_pk = false;
			uint32_t key = 0;

			for (auto &c : statement.conditions)
			{
				if (c.column == pk_name && c.op == "=")
				{
					has_pk = true;
					key = std::stoul(c.value);
				}
			}

			if (has_pk)
			{
				rows = table->find_all_by_id(key);
			}
			else
			{
				rows = table->get_all_dynamic(); // ALWAYS for base scan
			}

			rows = table->filter_rows(rows, statement.conditions);
		}
		else
		{
			rows = table->get_all_dynamic(); // ALWAYS for base scan
		}

		// ================= ORDER BY =================
		if (statement.has_order)
		{
			rows = table->order_rows(rows, statement.order_column);
		}

		if (statement.has_limit && (int)rows.size() > statement.limit)
		{
			rows.resize(statement.limit);
		}

		// ================= PRINT =================
		if (rows.empty())
		{
			std::cout << "Row not found\n";
			break;
		}

		for (const auto &row : rows)
		{
			std::cout << "(";

			if (statement.select_all)
			{
				for (size_t i = 0; i < row.size(); i++)
				{
					std::visit([](auto &&val)
							   { std::cout << val; }, row[i]);

					if (i != row.size() - 1)
						std::cout << ", ";
				}
			}
			else
			{
				for (size_t i = 0; i < statement.select_columns.size(); i++)
				{
					int idx = table->schema.get_column_index(statement.select_columns[i]);

					if (idx == -1)
					{
						std::cout << "NULL";
					}
					else
					{
						std::visit([](auto &&val)
								   { std::cout << val; }, row[idx]);
					}

					if (i != statement.select_columns.size() - 1)
						std::cout << ", ";
				}
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