﻿// Copyright 2023 ShenMian
// License(Apache-2.0)

#include "sokoban.hpp"

#include "crc32.hpp"
#include "level.hpp"

int main(int argc, char* argv[])
{
	try
	{
		Sokoban sokoban;
		sokoban.run(argc, argv);
	}
	catch(std::runtime_error& e)
	{
		std::cerr << "ERROR: " << e.what() << "\n";
		std::cerr << "Press enter to exit...\n";
		std::string line;
		std::getline(std::cin, line);
		return 1;
	}

	return 0;
}