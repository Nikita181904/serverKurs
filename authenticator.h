#ifndef AUTHENTICATOR_H
#define AUTHENTICATOR_H

#include <string>
#include <unordered_map>
#include <sys/socket.h> 
#include <unistd.h>      
#include "logger.h"
#include "error_handler.h"

class Authenticator {
private:
    std::unordered_map<std::string, std::string> users_;
    Logger& logger_;
    
public:
    Authenticator(Logger& logger);
    
    bool loadUsers(const std::string& filename);
    bool authenticateUser(int clientSocket, const std::string& login);
    bool userExists(const std::string& login) const;
    
private:
    std::string generateSalt();
    std::string calculateHash(const std::string& salt, const std::string& password);
    bool sendSalt(int clientSocket, const std::string& salt);
    bool receiveHash(int clientSocket, std::string& hash);
    bool sendResult(int clientSocket, bool success);
};

#endif // AUTHENTICATOR_H