#include <Windows.h>
#include <iostream>
#include <stdexcept>
#include "dk/dk.hpp"

int main(int argc, char *argv[]) {
	try {
		std::cout << "Initializing DKOM" << std::endl;

		dk::donkey_kom dkom;

		system("pause");

		std::cout << "Uninitializing DKOM" << std::endl;
	}
	catch (std::exception ex) {
		std::cerr << "[ERROR] " << ex.what() << std::endl;
	}

	system("pause");
	return EXIT_SUCCESS;
}
