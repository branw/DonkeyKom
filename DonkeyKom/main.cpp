#include <Windows.h>
#include <iostream>
#include <stdexcept>
#include "dk/dk.hpp"
#include "uac.hpp"

int main(int argc, char *argv[]) {
	try {
		// Check process elevation and elevate as necessary using token manipulation
		if (can_be_elevated()) {
			std::cout << "Process is not fully elevated; attempting elevation" << std::endl;
			elevate(GetCommandLineW());
			return EXIT_SUCCESS;
		}

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
