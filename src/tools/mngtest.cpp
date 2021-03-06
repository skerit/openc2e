#include "mngfile.h"
#include <iostream>

int main(int argc, char **argv) {
	if (argc != 2) return 1;

	try {
		MNGFile mng(argv[1]);
		std::cout << mng.dump();
	} catch (MNGFileException &e) {
		std::cout << e.what();
	}
}

