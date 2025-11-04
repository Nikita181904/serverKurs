#ifndef NETWORK_H
#define NETWORK_H

#include <cstdint>
#include <vector>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include "logger.h"
#include "error_handler.h"

class NetworkManager {
private:
    Logger& logger_;
    int serverSocket_;
    bool initialized_;
    
public:
    NetworkManager(Logger& logger);
    ~NetworkManager();
    
    bool initialize(uint16_t port);
    void shutdown();
    int acceptClient(std::string& clientIP);
    void closeClient(int clientSocket);
    
    // Методы для работы с клиентом
    bool receiveLogin(int clientSocket, std::string& login);
    bool receiveVectors(int clientSocket, std::vector<std::vector<float>>& vectors);
    bool sendResults(int clientSocket, const std::vector<float>& results);
    
private:
    bool createSocket();
    bool bindSocket(uint16_t port);
    bool startListening();
    bool setSocketOptions();
    
    // Вспомогательные методы
    bool receiveData(int clientSocket, void* buffer, size_t size);
    bool sendData(int clientSocket, const void* data, size_t size);
    bool receiveVector(int clientSocket, std::vector<float>& vector);
};

#endif // NETWORK_H