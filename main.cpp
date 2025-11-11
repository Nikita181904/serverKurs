#include "server.h"
#include <iostream>

int main(int argc, char* argv[]) {
    std::cout << "Запуск сервера для векторных вычислений..." << std::endl;
    
    try {
        ServerInterface serverInterface;
        return serverInterface.run(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Фатальная ошибка в main: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Неизвестная фатальная ошибка" << std::endl;
        return 1;
    }
}