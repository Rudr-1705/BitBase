#include "executor.h"
#include <iostream>

void Executor::execute(const Statement& statement) {
	switch (statement.type)
	{
		case(StatementType::INSERT) :
		{
			std::cout << "Executed INSERT\n";
			break;
		}

		case(StatementType::SELECT) :
		{
			std::cout << "Executed SELECT\n";
			break;
		}

		case(StatementType::DELETE) :
		{
			std::cout << "Executed DELETE\n";
			break;
		}

		case(StatementType::UPDATE) :
		{
			std::cout << "Executed UPDATE\n";
			break;
		}

		case(StatementType::CREATE_TABLE) :
		{
			std::cout << "Executed CREATE TABLE\n";
			break;
		}

		case(StatementType::DROP_TABLE) :
		{
			std::cout << "Executed DROP TABLE\n";
			break;
		}

		default:
			break;
	}
}