#include <iostream>
#include <string>

#include "repl.h"
#include "meta.h"
#include "parser.h"
#include "executor.h"

void Repl::start() {
	MetaCommandHandler meta_handler;
	Parser parser;
	Executor executor;

	std::string input;

	while (true) {
		std::cout << "Bitbase> ";

		if (!std::getline(std::cin, input)) {
			std::cout << "\n";
			break;
		}

		if (input.empty()) {
			continue;
		}

		if (input[0] == '.')
		{
			MetaCommandResult result = meta_handler.handle(input);

			if (result == MetaCommandResult::EXIT) {
				break;
			}

			continue;
		}

		Statement statement;
		std::string error_message;

		bool success = parser.parse(input, statement, error_message);

		if (!success) {
			std::cout << "Error : " << error_message << "\n";
			continue;
		}

		executor.execute(statement);
	}
}