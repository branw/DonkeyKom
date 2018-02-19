#include <Windows.h>
#include <iostream>

#include "dk/dk.hpp"

int main(int argc, char *argv[])
{
	std::cout << "DonkeyKom kernel manipulation toolkit v0.1" << std::endl;

	try
	{
		std::cout << "Initializing DKOM" << std::endl;

		system("pause");

		std::cout << "Uninitializing DKOM" << std::endl;
	}
	catch (std::exception &ex)
	{
		std::cerr << "[ERROR] " << ex.what() << std::endl;
	}

	system("pause");
	return EXIT_SUCCESS;
}
