#include "meta.h"
#include <iostream>

MetaCommandResult MetaCommandHandler::handle(const std::string & input) {
	if (input == ".exit")
	{
		return MetaCommandResult::EXIT;
	}
	else if (input == ".tables")
	{
		std::cout << "Table: users\n";
		return MetaCommandResult::SUCCESS;
	}
	else if (input == ".help")
	{
		std::cout << "Available commands:\n";
		std::cout << "  .exit     Exit the program\n";
		std::cout << "  .tables   List tables\n";
		std::cout << "  .help     Show help\n";

		return MetaCommandResult::SUCCESS;
	}
	else
	{
		return MetaCommandResult::UNRECOGNIZED;
	}
}