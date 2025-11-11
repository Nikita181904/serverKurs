#include "authenticator.h"
#include <fstream>
#include <cstring>
#include <algorithm>

#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <cryptopp/osrng.h>
#include <cryptopp/filters.h>

using namespace CryptoPP;

Authenticator::Authenticator(Logger& logger) : logger_(logger) {
    // Фиксированный пользователь больше не добавляется автоматически
    // Все пользователи загружаются из файла
}

bool Authenticator::loadUsers(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        logger_.error("Cannot open user database file: " + filename);
        return false; // Теперь возвращаем false, так как файл обязателен
    }
    
    std::string line;
    int userCount = 0;
    
    while (std::getline(file, line)) {
        // Пропускаем пустые строки и комментарии
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        size_t pos = line.find(':');
        if (pos != std::string::npos) {
            std::string login = line.substr(0, pos);
            std::string password = line.substr(pos + 1);
            
            // Убираем пробелы в начале и конце
            login.erase(0, login.find_first_not_of(" \t"));
            login.erase(login.find_last_not_of(" \t") + 1);
            password.erase(0, password.find_first_not_of(" \t"));
            password.erase(password.find_last_not_of(" \t") + 1);
            
            if (!login.empty() && !password.empty()) {
                users_[login] = password;
                userCount++;
                logger_.debug("Loaded user: " + login);
            } else {
                logger_.warning("Invalid user entry in database: " + line);
            }
        } else {
            logger_.warning("Malformed line in user database: " + line);
        }
    }
    
    file.close();
    
    if (userCount == 0) {
        logger_.error("No valid users found in database: " + filename);
        return false;
    }
    
    logger_.info("Loaded " + std::to_string(userCount) + " users from database: " + filename);
    return true;
}

bool Authenticator::userExists(const std::string& login) const {
    return users_.find(login) != users_.end();
}

bool Authenticator::authenticateUser(int clientSocket, const std::string& login) {
    logger_.info("Authentication attempt for user: " + login);
    
    // Проверка существования пользователя
    if (!userExists(login)) {
        logger_.warning("User not found: " + login);
        return sendResult(clientSocket, false);
    }
    
    // Генерация и отправка соли
    std::string salt = generateSalt();
    if (!sendSalt(clientSocket, salt)) {
        logger_.error("Failed to send salt to client");
        return false;
    }
    
    // Получение хеша от клиента
    std::string clientHash;
    if (!receiveHash(clientSocket, clientHash)) {
        logger_.error("Failed to receive hash from client");
        return false;
    }
    
    // Проверка хеша
    auto it = users_.find(login);
    std::string expectedHash = calculateHash(salt, it->second);
    
    bool authenticated = (clientHash == expectedHash);
    
    if (authenticated) {
        logger_.info("User authenticated successfully: " + login);
    } else {
        logger_.warning("Authentication failed for user: " + login);
        logger_.debug("Expected hash: " + expectedHash);
        logger_.debug("Received hash: " + clientHash);
    }
    
    return sendResult(clientSocket, authenticated);
}

std::string Authenticator::generateSalt() {
    AutoSeededRandomPool prng;
    byte salt[8]; // 64 бита согласно ТЗ
    prng.GenerateBlock(salt, sizeof(salt));
    
    std::string saltHex;
    ArraySource(salt, sizeof(salt), true,
        new HexEncoder(
            new StringSink(saltHex)
        )
    );
    
    while (saltHex.length() < 16) {
        saltHex = "0" + saltHex;
    }
    
    return saltHex.substr(0, 16);
}

std::string Authenticator::calculateHash(const std::string& salt, const std::string& password) {
    SHA1 sha1; // SHA-1 согласно ТЗ
    std::string data = salt + password;
    std::string hash;
    
    StringSource(data, true,
        new HashFilter(sha1,
            new HexEncoder(
                new StringSink(hash)
            )
        )
    );
   
    for (char& c : hash) {
        c = std::toupper(c);
    }
    
    return hash;
}

bool Authenticator::sendSalt(int clientSocket, const std::string& salt) {
    ssize_t bytesSent = send(clientSocket, salt.c_str(), salt.length(), 0);
    if (bytesSent != static_cast<ssize_t>(salt.length())) {
        return false;
    }
    logger_.debug("Sent salt to client: " + salt);
    return true;
}

bool Authenticator::receiveHash(int clientSocket, std::string& hash) {
    char buffer[41] = {0}; // SHA-1 hash is 40 hex characters + null terminator
    ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytesReceived <= 0) {
        return false;
    }
    
    hash.assign(buffer, bytesReceived);
    logger_.debug("Received hash from client: " + hash);
    return true;
}

bool Authenticator::sendResult(int clientSocket, bool success) {
    std::string response = success ? "OK" : "ERR";
    ssize_t bytesSent = send(clientSocket, response.c_str(), response.length(), 0);
    
    if (bytesSent == static_cast<ssize_t>(response.length())) {
        logger_.debug("Sent authentication result: " + response);
        return true;
    }
    return false;
}