#include <Windows.h>
#include <iostream>

#include "donkey_kom.hpp"


int main() {
	try {
		dk::initialize();
	}
	catch (std::exception ex) {
		std::cerr << ex.what() << std::endl;
	}

	system("pause");
	return EXIT_SUCCESS;
}
