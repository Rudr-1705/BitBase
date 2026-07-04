#include "executor/executor.h"
#include <iostream>

Executor::Executor() : wal("wal.log")
{
	// wal.recover(db);
}

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

		// ===== UNIQUE CHECK =====
		for (int i = 0; i < (int)schema.columns.size(); i++)
		{
			if (schema.columns[i].is_unique &&
				table->exists_value_in_column(i, statement.raw_values[i]))
			{
				std::cout << "Error: Duplicate value for UNIQUE column\n";
				return;
			}
		}

		// ===== APPLY FIRST =====

		bool ok = table->insert(statement.raw_values);

		if (ok)
		{
			wal.log_insert(statement.table_name, statement.raw_values);
			wal.flush();

			std::cout << "Executed INSERT\n";
		}

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
					try
					{
						key = std::stoul(c.value);
						has_pk = true;
					}
					catch (...)
					{
					}
				}
			}

			if (has_pk)
				rows = table->find_all_by_id(key);
			else
				rows = table->get_all_dynamic();

			rows = table->filter_rows(rows, statement.conditions);
		}
		else
		{
			rows = table->get_all_dynamic();
		}

		if (statement.has_order)
			rows = table->order_rows(rows, statement.order_column);

		if (statement.has_limit && (int)rows.size() > statement.limit)
			rows.resize(statement.limit);

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
						std::cout << "NULL";
					else
						std::visit([](auto &&val)
								   { std::cout << val; }, row[idx]);

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

			wal.log_delete(statement.table_name, "ALL");
			wal.flush();

			std::cout << "Executed DELETE ALL\n";
			break;
		}

		int deleted = table->delete_where_full(statement.conditions);

		if (deleted == 0)
		{
			std::cout << "Row not found\n";
		}
		else
		{
			wal.log_delete(statement.table_name, "COND");
			wal.flush();

			std::cout << "Deleted " << deleted << " rows\n";
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

		int updated = table->update_where(
			statement.conditions,
			statement.update_column,
			statement.update_value);

		if (updated == 0)
		{
			std::cout << "Update failed\n";
		}
		else
		{
			wal.log_update(statement.table_name,
						   "COND",
						   "OLD",
						   statement.update_value);
			wal.flush();

			std::cout << "Updated " << updated << " rows\n";
		}

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