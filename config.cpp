#include "config.h"
#include <iostream>
#include <fstream>
#include <limits>
#include <cstdlib>

bool ServerConfig::validate() const {
    if (clientDbFile.empty()) {
        throw ConfigException("Client database file path cannot be empty");
    }
    
    if (logFile.empty()) {
        throw ConfigException("Log file path cannot be empty");
    }
    
    if (port < 1024) {
        throw ConfigException("Port must be in range 1024-65535");
    }
    
    // Проверка доступности файла базы данных
    std::ifstream testFile(clientDbFile);
    if (!testFile.is_open()) {
        // Если файл не существует, создаем его с базовым пользователем
        std::ofstream createFile(clientDbFile);
        if (createFile.is_open()) {
            createFile << "user:P@ssW0rd\n";
            createFile.close();
            std::cout << "Created default client database file: " << clientDbFile << std::endl;
        } else {
            throw FileException("Cannot access or create client database file: " + clientDbFile);
        }
    } else {
        testFile.close();
    }
    
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
        else if (arg == "-p" || arg == "--port") {
            if (i + 1 < argc) {
                setPort(argv[++i]);
            } else {
                throw ConfigException("Missing value for --port option");
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

void Config::setPort(const std::string& portStr) {
    try {
        long port_long = std::stol(portStr);
        if (port_long < 1024 || port_long > 65535) {
            throw ConfigException("Port must be in range 1024-65535");
        }
        config_.port = static_cast<uint16_t>(port_long);
    } catch (const std::invalid_argument&) {
        throw ConfigException("Invalid port number: " + portStr);
    } catch (const std::out_of_range&) {
        throw ConfigException("Port number out of range: " + portStr);
    }
}

void Config::showHelp() {
    std::cout << "Server for vector calculations\n"
              << "Technical requirements:\n"
              << "  - Default port: 33333\n"
              << "  - Default client database: /etc/vcalc.conf\n"
              << "  - Default log file: /var/log/vcalc.log\n"
              << "  - Client sends: vectors with float values\n"
              << "  - Single-threaded request processing\n"
              << "  - SHA-1 authentication with server-side salt\n"
              << "  - Binary data protocol\n\n"
              << "Usage: server [OPTIONS]\n\n"
              << "Options:\n"
              << "  -h, --help          Show this help message\n"
              << "  -c, --config FILE   Client database file (default: /etc/vcalc.conf)\n"
              << "  -l, --log FILE      Log file (default: /var/log/vcalc.log)\n"
              << "  -p, --port PORT     Server port (default: 33333, range: 1024-65535)\n\n"
              << "Client database format:\n"
              << "  Each line: username:password\n"
              << "  Example: user:P@ssW0rd\n\n"
              << "Examples:\n"
              << "  server -c /etc/my_vcalc.conf -l /var/log/my_vcalc.log -p 8080\n"
              << "  server --config /etc/vcalc.conf --port 44444\n"
              << "  server -p 12345  # Use custom port with other default settings\n";
}
