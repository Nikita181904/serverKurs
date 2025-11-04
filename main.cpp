#include "server.h"
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        ServerInterface serverInterface;
        return serverInterface.run(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Fatal error in main: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred" << std::endl;
        return 1;
    }
}