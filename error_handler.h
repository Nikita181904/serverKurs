#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <string>
#include <stdexcept>
#include <iostream>
#include <cstdlib>

class ServerException : public std::runtime_error {
public:
    ServerException(const std::string& message) : std::runtime_error(message) {}
};

class ConfigException : public ServerException {
public:
    ConfigException(const std::string& message) : ServerException(message) {}
};

class NetworkException : public ServerException {
public:
    NetworkException(const std::string& message) : ServerException(message) {}
};

class AuthException : public ServerException {
public:
    AuthException(const std::string& message) : ServerException(message) {}
};

class FileException : public ServerException {
public:
    FileException(const std::string& message) : ServerException(message) {}
};

class ErrorHandler {
public:
    static void handleCritical(const std::string& message) {
        std::cerr << "CRITICAL: " << message << std::endl;
        std::exit(1);
    }
    
    static void handleError(const std::string& message) {
        std::cerr << "ERROR: " << message << std::endl;
    }
    
    static void handleWarning(const std::string& message) {
        std::cerr << "WARNING: " << message << std::endl;
    }
};

#endif // ERROR_HANDLER_H