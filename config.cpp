#include "config.h"
#include <iostream>
#include <fstream>
#include <limits>

bool ServerConfig::validate() const {
    if (clientDbFile.empty()) {
        throw ConfigException("Client database file path cannot be empty");
    }
    
    if (logFile.empty()) {
        throw ConfigException("Log file path cannot be empty");
    }
    
    if (port != 33333) {
        throw ConfigException("Port must be 33333 according to client requirements");
    }
    
    // Проверка доступности файла базы данных
    std::ifstream testFile(clientDbFile);
    if (!testFile.is_open()) {
        throw FileException("Cannot access client database file: " + clientDbFile);
    }
    testFile.close();
    
    return true;
}

bool Config::parseCommandLine(int argc, char* argv[]) {
    if (argc == 1) {
        showHelp();
        return false;
    }
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            showHelp();
            return false;
        }
        else if (arg == "-c" || arg == "--config") {
            if (i + 1 < argc) {
                setClientDbFile(argv[++i]);
            } else {
                throw ConfigException("Missing value for --config option");
            }
        }
        else if (arg == "-l" || arg == "--log") {
            if (i + 1 < argc) {
                setLogFile(argv[++i]);
            } else {
                throw ConfigException("Missing value for --log option");
            }
        }
        else {
            throw ConfigException("Unknown option: " + arg);
        }
    }
    
    config_.validate();
    return true;
}

void Config::setClientDbFile(const std::string& filename) {
    config_.clientDbFile = filename;
}

void Config::setLogFile(const std::string& filename) {
    config_.logFile = filename;
}

void Config::showHelp() {
    std::cout << "Server for vector calculations\n"
              << "Technical requirements:\n"
              << "  - Port: 33333 (fixed)\n"
              << "  - Client login: 'user', password: 'P@ssW0rd'\n"
              << "  - Client sends: 4 vectors with 4 values each\n"
              << "  - Single-threaded request processing\n"
              << "  - SHA-1 authentication with server-side salt\n"
              << "  - Binary data protocol\n\n"
              << "Usage: server [OPTIONS]\n\n"
              << "Options:\n"
              << "  -h, --help          Show this help message\n"
              << "  -c, --config FILE   Client database file (default: /etc/vcalc.conf)\n"
              << "  -l, --log FILE      Log file (default: /var/log/vcalc.log)\n\n"
              << "Examples:\n"
              << "  server -c users.db -l server.log\n"
              << "  server --config /etc/vcalc.conf\n";
}